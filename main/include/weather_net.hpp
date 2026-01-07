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
#if !defined(WEATHER_HEADER_NET)
#define WEATHER_HEADER_NET

#include "esp_event.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "weather.hpp"
#include <ctime>

namespace weather {
/// @brief Handles MQTT and connecting to Wi-Fi
namespace net {

/// @brief The singleton MQTT service
class service {
  service();

  service(const service &) = delete;
  service &operator=(const service &) = delete;
  service(service &&) = delete;
  service &operator=(service &&) = delete;

  esp_mqtt_client_handle_t _mqtt;
  esp_mqtt_client_config_t _mqtt_config;

  wifi_config_t _wifi_config;
  wifi_init_config_t _wifi_init_config;
  esp_event_handler_instance_t _any_id, _got_ip;
  char _json_allocation[2048];
  time_t _last_send = 0;

public:
  /// @brief Wi-Fi status bitmask
  U32 status;

  /// @brief Wi-Fi is connected
  constexpr const static decltype(status) STATUS_CONNECTED = 1 << 0;
  /// @brief MQTT is connected
  constexpr const static decltype(status) STATUS_MQTT_ONLINE = 1 << 1;
  /// @brief MQTT is connected
  constexpr const static decltype(status) STATUS_MQTT_SUBSCRIBED = 1 << 2;
  /// @brief Nothing is connected
  constexpr const static decltype(status) STATUS_NONE = 0;

  /// @brief Gets the service singleton
  /// @return A reference to the service singleton
  static service &get_instance();

  /// @brief Sends off an update of the pressure and temperature
  void refresh();
};
} // namespace net
} // namespace weather

#endif