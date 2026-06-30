// Copyright (c) 2023-2026 Roland Metivier <metivier.roland@chlorophyt.us>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#include "include/weather_net.hpp"
#include "esp_err.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "include/weather_gps.hpp"
#include "include/weather_i2cmux.hpp"
#include "include/weather_logging.hpp"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include <ArduinoJson.h>
#include <cstring>
#include <sstream>
#include <strings.h>

using namespace weather;

static constexpr const char *TAG = "weather_net";

void event_handler(void *user_data, esp_event_base_t event_base, S32 event_id,
                   void *event_data) {
  net::service *service = reinterpret_cast<net::service *>(user_data);
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START: {
      logging::group::get_instance().log(TAG, logging::severity::information,
                                         "Connecting to Wi-Fi network");
      esp_wifi_connect();
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
      service->status &= ~net::service::STATUS_CONNECTED;

      logging::group::get_instance().log(
          TAG, logging::severity::warning,
          "Reconnecting to Wi-Fi network due to disconnect");
      esp_wifi_connect();
      break;
    }
    }
  } else if (event_base == IP_EVENT) {
    service->status |= net::service::STATUS_CONNECTED;
  } else {
    switch (event_id) {
    case MQTT_EVENT_CONNECTED: {
      service->status |= net::service::STATUS_MQTT_ONLINE;
      break;
    }
    case MQTT_EVENT_DISCONNECTED: {
      service->status &= ~(net::service::STATUS_MQTT_ONLINE |
                           net::service::STATUS_MQTT_SUBSCRIBED);
      break;
    }
    default: {
      break;
    }
    }
  }
}

net::service::service() : status(net::service::STATUS_NONE) {
  bzero(&_wifi_config, sizeof(_wifi_config));
  bzero(&_mqtt_config, sizeof(_mqtt_config));

  std::memcpy(_wifi_config.sta.ssid, CONFIG_WEATHER_JOIN_SSID,
              strlen(CONFIG_WEATHER_JOIN_SSID));
#ifdef CONFIG_WEATHER_WIRELESS_SECURITY_OPEN
  _wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
#endif
#ifdef CONFIG_WEATHER_WIRELESS_SECURITY_WEP
  std::memcpy(_wifi_config.sta.password, CONFIG_WEATHER_JOIN_PASSPHRASE,
              strlen(CONFIG_WEATHER_JOIN_PASSPHRASE));
  _wifi_config.sta.threshold.authmode = WIFI_AUTH_WEP;
#endif
#ifdef CONFIG_WEATHER_WIRELESS_SECURITY_WPA_PSK
  std::memcpy(_wifi_config.sta.password, CONFIG_WEATHER_JOIN_PASSPHRASE,
              strlen(CONFIG_WEATHER_JOIN_PASSPHRASE));
  _wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
#endif
#ifdef CONFIG_WEATHER_WIRELESS_SECURITY_WPA2_PSK
  std::memcpy(_wifi_config.sta.password, CONFIG_WEATHER_JOIN_PASSPHRASE,
              strlen(CONFIG_WEATHER_JOIN_PASSPHRASE));
  _wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
#endif
#ifdef CONFIG_WEATHER_WIRELESS_SECURITY_WPA_WPA2_PSK
  std::memcpy(_wifi_config.sta.password, CONFIG_WEATHER_JOIN_PASSPHRASE,
              strlen(CONFIG_WEATHER_JOIN_PASSPHRASE));
  _wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
#endif

  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_sta();

  _wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&_wifi_init_config));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, &_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, this, &_got_ip));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  _mqtt_config.broker.address.uri = CONFIG_WEATHER_MQTT_BROKER;
  _mqtt = esp_mqtt_client_init(&_mqtt_config);
  ESP_ERROR_CHECK(esp_mqtt_client_start(_mqtt));

  // Done
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "Service initialized");
}

net::service &net::service::get_instance() {
  static net::service instance;
  return instance;
}

void net::service::refresh() {
  if ((status & (net::service::STATUS_CONNECTED |
                 net::service::STATUS_MQTT_ONLINE)) != 0) {
    if ((status & net::service::STATUS_MQTT_SUBSCRIBED) == 0) {
      esp_mqtt_client_subscribe(_mqtt, "weather/status", 0);
      status |= net::service::STATUS_MQTT_SUBSCRIBED;
    }
    if (gps::service::get_instance().has_fix() &&
        time(nullptr) >= (_last_send + 15)) {
      _last_send = time(nullptr);

      JsonDocument root;
      const auto &plog = i2cmux::service::get_instance().get_pressure_log();
      const auto &tlog = i2cmux::service::get_instance().get_temperature_log();

      root["unix_time"] = _last_send;
      root["data"]["pressure"] =
          i2cmux::service::get_instance().get_pressure();
      root["data"]["temperature"] =
          i2cmux::service::get_instance().get_temperature();

      for (auto i = 0; i < plog.size(); i++) {
        root["previous"]["pressure"][i] = plog[i];
      }
      for (auto i = 0; i < tlog.size(); i++) {
        root["previous"]["temperature"][i] = tlog[i];
      }

      std::stringstream json;
      serializeJson(root, json);

      const std::string &jstr = json.str();

      esp_mqtt_client_publish(_mqtt, "weather/status", jstr.c_str(),
                              jstr.size(), 0, 0);
    }
  }
}