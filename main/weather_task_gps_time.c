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
#include "weather_task_gps_time.h"

static const char *TAG = "task_gps_time";

void weather_task_gps_time_config(weather_task_gps_time_t *pointer) {
  gps_time_fill(&(pointer->data));
  pointer->uart_config = (uart_config_t){.baud_rate = 9600,
                          .data_bits = UART_DATA_8_BITS,
                          .parity = UART_PARITY_DISABLE,
                          .stop_bits = UART_STOP_BITS_1,
                          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                          .source_clk = UART_SCLK_DEFAULT};

  int interrupt_alloc_flags = 0;

  ESP_ERROR_CHECK(uart_driver_install(WEATHER_TASK_GPS_TIME_UART,
                                      sizeof(pointer->uart_buffer), 0, 0, NULL,
                                      interrupt_alloc_flags));
  ESP_ERROR_CHECK(
      uart_param_config(WEATHER_TASK_GPS_TIME_UART, &(pointer->uart_config)));
  ESP_ERROR_CHECK(uart_set_pin(WEATHER_TASK_GPS_TIME_UART, 17, 16,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void weather_task_gps_time_task(void *user_data) {
  weather_task_gps_time_t *pointer = user_data;

  char *data_current = pointer->uart_buffer;
  while (1) {
    int len = uart_read_bytes(WEATHER_TASK_GPS_TIME_UART, data_current, 1,
                              10 / portTICK_PERIOD_MS);
    if (len == 1) {
      if (*data_current == '\n') {
        *data_current = '\0';
        data_current = pointer->uart_buffer;
        gps_time_nmea_read(&(pointer->data), pointer->uart_buffer);
      } else {
        data_current++;
      }
    }
  }
  // return gracefully in case something happens
  vTaskDelete(NULL);
}
