/*
 * Copyright (c) 2023-2026 Roland Metivier <metivier.roland@chlorophyt.us>
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
#include "esp_event.h"
#include "freertos/idf_additions.h"
#include "iic_mux.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "weather_task_gps_time.h"
#include "weather_task_net.h"
#include <inttypes.h>
#include <stdio.h>

static const char *TAG = "main";

#define TASK_STACK_SIZE 2048

void app_main(void) {
  ESP_LOGI(TAG, "Initialize Nonvolatile Storage...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "Creating default event loop...");
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_LOGI(TAG, "Creating GPS Time task...");
  weather_task_gps_time_t gps_time;
  weather_task_gps_time_config(&gps_time);

  weather_task_net_t net;
  ESP_LOGI(TAG, "Initializing wireless system...");
  wireless_init(&(net.wifi));
  ESP_LOGI(TAG, "Joining wireless network...");
  wireless_start(net.wifi);

  ESP_LOGI(TAG, "Initializing I2C multiplexing system...");
  iic_mux_init(&(net.i2c));
  ESP_LOGI(TAG, "Probing I2C multiplexing system...");
  iic_mux_start(net.i2c);

  ESP_LOGI(TAG, "Dispatching GPS time task...");
  xTaskCreate(weather_task_gps_time_task, "gps_time_task", 5000, &gps_time, 10,
              NULL);

  // ESP_LOGI(TAG, "Dispatching I2C task...");
  // xTaskCreate(weather_task_i2c_task, "i2c_task",10000, &i2c, 20, NULL);

  ESP_LOGI(TAG, "Dispatching wireless task...");
  xTaskCreate(weather_task_net_task, "net_task", 10000, &net, 10, NULL);

  // Sensors are monitored in the main section
  while(1) {
    iic_mux_refresh(net.i2c);
    vTaskDelay(CONFIG_WEATHER_POLL_SENSORS_INTERVAL / portTICK_PERIOD_MS);
  }
}
