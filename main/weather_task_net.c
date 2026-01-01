/*
 * Copyright (c) 2023-2025 Roland Metivier <metivier.roland@chlorophyt.us>
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
#include "weather_task_net.h"

static const char *TAG = "task_net";
volatile uint32_t mqtt_connected = 0;

static void weather_task_net_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  esp_mqtt_event_id_t idx = event_id;

  switch(idx) {
    case MQTT_EVENT_CONNECTED: {
      esp_mqtt_client_subscribe(client, "weather/status", 0);
      mqtt_connected = 1;
      break;
    }
    case MQTT_EVENT_DISCONNECTED: {
      mqtt_connected = 0;
      break;
    }
    default: {
      break;
    }
  }
}
// ============================================================================
void weather_task_net_task(void *user_data) {
  weather_task_net_t *pointer = user_data;

  EventBits_t bits = xEventGroupWaitBits(pointer->wifi->events,
        WIRELESS_CONNECTED_BIT | WIRELESS_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);
  if((bits & WIRELESS_CONNECTED_BIT) == WIRELESS_CONNECTED_BIT) {
    const esp_mqtt_client_config_t mqtt_config = {
      .broker.address.uri = CONFIG_WEATHER_MQTT_BROKER,
    };
    ESP_LOGI(TAG, "Trying broker '%s'", CONFIG_WEATHER_MQTT_BROKER);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, weather_task_net_event_handler, NULL);
    esp_mqtt_client_start(client);

    while(mqtt_connected != 1) {
      ESP_LOGI(TAG, "Waiting for MQTT");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "MQTT connected!");

    while(1) {
      struct timeval time;
      gettimeofday(&time, NULL);

      cJSON *root = cJSON_CreateObject();
      cJSON_AddNumberToObject(root, "utc", time.tv_sec);
      char *str = cJSON_PrintUnformatted(root);
      esp_mqtt_client_publish(client, "weather/status", str, strlen(str), 0, 0);
      cJSON_Delete(root);
      vTaskDelay(CONFIG_WEATHER_MQTT_INTERVAL / portTICK_PERIOD_MS);
    }
  } else if ((bits & WIRELESS_FAIL_BIT) == WIRELESS_FAIL_BIT) {
    ESP_LOGW(TAG, "Wireless connection failed.");
  }
  // return gracefully in case something happens
  vTaskDelete(NULL);
}

