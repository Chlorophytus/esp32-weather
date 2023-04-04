/*
 * Copyright (c) 2023 Roland Metivier <metivier.roland@chlorophyt.us>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "wireless.h"

static const char *TAG = "wireless";

static void wireless_event_handler(void *user_data, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  wireless_t *pointer = user_data;

  if(event_base == WIFI_EVENT) {
    if(event_id == WIFI_EVENT_STA_START) {
      ESP_LOGI(TAG, "Connecting to Wi-Fi network");
      esp_wifi_connect();
    } else if(event_id == WIFI_EVENT_STA_DISCONNECTED) {
      pointer->status &= ~WIRELESS_STATUS_CONNECTED;

      if(pointer->tries_left > 0) {
        ESP_LOGW(TAG, "Can't connect, will try %" PRIu32 " more times", pointer->tries_left);
        esp_wifi_connect();
        (pointer->tries_left)--;
      } else {
        ESP_LOGW(TAG, "Ran out of tries, bailing out");
        xEventGroupSetBits(pointer->events, WIRELESS_FAIL_BIT);
      }
    }
  } else if (event_base == IP_EVENT) {
    if(event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = event_data;
      ESP_LOGI(TAG, "Obtained IP address: " IPSTR, IP2STR(&event->ip_info.ip));
      pointer->status |= WIRELESS_STATUS_CONNECTED;
      pointer->tries_left = CONFIG_WIRELESS_RETRIES;
      xEventGroupSetBits(pointer->events, WIRELESS_CONNECTED_BIT);
    }
  }
}
// ============================================================================
void wireless_init(wireless_t **pointer) {
  *pointer = malloc(sizeof(wireless_t));
  wireless_fill(*pointer);
}

void wireless_fill(wireless_t *pointer) {
  pointer->status = WIRELESS_STATUS_NONE;

#ifdef CONFIG_WIRELESS_SECURITY_OPEN
  wifi_auth_mode_t auth = WIFI_AUTH_OPEN;
#endif
#ifdef CONFIG_WIRELESS_SECURITY_WEP
  wifi_auth_mode_t auth = WIFI_AUTH_WEP;
#endif
#ifdef CONFIG_WIRELESS_SECURITY_WPA_PSK
  wifi_auth_mode_t auth = WIFI_AUTH_WPA_PSK;
#endif
#ifdef CONFIG_WIRELESS_SECURITY_WPA2_PSK
  wifi_auth_mode_t auth = WIFI_AUTH_WPA2_PSK;
#endif
#ifdef CONFIG_WIRELESS_SECURITY_WPA_WPA2_PSK
  wifi_auth_mode_t auth = WIFI_AUTH_WPA_WPA2_PSK;
#endif

  pointer->config = (wifi_config_t){
    .sta = {
      .ssid = CONFIG_WIRELESS_JOIN_SSID,
      .password = CONFIG_WIRELESS_JOIN_PASSPHRASE,
      .threshold.authmode = auth,
    },
  };

  pointer->tries_left = CONFIG_WIRELESS_RETRIES;
}

void wireless_start(wireless_t *pointer) {
  if((pointer->status & WIRELESS_STATUS_STARTED) == 0) {
    pointer->events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;


    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wireless_event_handler,
                                                        pointer,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wireless_event_handler,
                                                        pointer,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &(pointer->config)));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wireless component started");
    pointer->status |= WIRELESS_STATUS_STARTED;
  } else {
    ESP_LOGE(TAG, "Wireless component was already started!");
  }
}

void wireless_free(wireless_t *pointer) { free(pointer); }

// Event handler

