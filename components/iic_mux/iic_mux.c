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
#include "iic_mux.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>

static const char *TAG = "iic_mux";

void iic_mux_init(iic_mux_t **pointer) {
  *pointer = malloc(sizeof(iic_mux_t));
  iic_mux_fill(*pointer);
}

void iic_mux_fill(iic_mux_t *pointer) {
  pointer->bme280_pressure = 0;
  pointer->bme280_temperature = 0;
}

void iic_mux_start(iic_mux_t *pointer) {
  ESP_LOGI(TAG, "Will connect to I2C devices now");
  i2c_master_bus_config_t i2c_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = -1,
      .scl_io_num = GPIO_NUM_22,
      .sda_io_num = GPIO_NUM_21,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_config, &pointer->bus_handle));

#ifdef CONFIG_I2CMUX_BME280
  ESP_LOGI(TAG, "BME280 is enabled");
  i2c_device_config_t bme280_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = 0x77,
      .scl_speed_hz = 400000,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(pointer->bus_handle, &bme280_config,
                                            &pointer->bme280_handle));

  ESP_LOGI(TAG, "Handling BME280 settings...");
  const uint8_t write_iir[2] = {0xf5, 0b000100000};
  ESP_ERROR_CHECK(
      i2c_master_transmit(pointer->bme280_handle, write_iir, 2, 500));

  ESP_LOGI(TAG, "Reading BME280 temperature calibration data...");
  const uint8_t read_dig_T[1] = {0x88};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(
      pointer->bme280_handle, read_dig_T, 1, pointer->bme280_buf_dig_T,
      I2CMUX_DIG_TEMPERATURE_SIZE * 2, 1000));
  pointer->bme280_temperature_coeff_1 = pointer->bme280_buf_dig_T[1];
  pointer->bme280_temperature_coeff_1 <<= 8;
  pointer->bme280_temperature_coeff_1 |= pointer->bme280_buf_dig_T[0];
  ESP_LOGI(TAG, "| dig_T1 |  %5hu |", pointer->bme280_temperature_coeff_1);
  for (uint32_t i = 1; i < I2CMUX_DIG_TEMPERATURE_SIZE; i++) {
    uint16_t calibration = pointer->bme280_buf_dig_T[i * 2 + 1];
    calibration <<= 8;
    calibration |= pointer->bme280_buf_dig_P[i * 2 + 0];
    pointer->bme280_temperature_coeffs[i] = (int16_t)(calibration);
    ESP_LOGI(TAG, "| dig_T%01u | %6hd |", i + 1,
             pointer->bme280_temperature_coeffs[i - 1]);
  }

  ESP_LOGI(TAG, "Reading BME280 pressure calibration data...");
  const uint8_t read_dig_P[1] = {0x8E};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(
      pointer->bme280_handle, read_dig_P, 1, pointer->bme280_buf_dig_P,
      I2CMUX_DIG_PRESSURE_SIZE * 2, 1000));
  pointer->bme280_pressure_coeff_1 = pointer->bme280_buf_dig_P[1];
  pointer->bme280_pressure_coeff_1 <<= 8;
  pointer->bme280_pressure_coeff_1 |= pointer->bme280_buf_dig_P[0];
  ESP_LOGI(TAG, "| dig_P1 |  %5hu |", pointer->bme280_pressure_coeff_1);
  for (uint32_t i = 1; i < I2CMUX_DIG_PRESSURE_SIZE; i++) {
    uint16_t calibration = pointer->bme280_buf_dig_P[i * 2 + 1];
    calibration <<= 8;
    calibration |= pointer->bme280_buf_dig_P[i * 2 + 0];
    pointer->bme280_pressure_coeffs[i] = (int16_t)(calibration);
    ESP_LOGI(TAG, "| dig_P%01u | %6hd |", i + 1,
             pointer->bme280_pressure_coeffs[i - 1]);
  }
#else
  ESP_LOGI(TAG, "BME280 is disabled");
#endif
}

void iic_mux_refresh(iic_mux_t *pointer) {
#ifdef CONFIG_I2CMUX_BME280
  ESP_LOGI(TAG, "Handling BME280 now...");
  const uint8_t write_oversampling[2] = {0xf4, 0b10110101};
  ESP_ERROR_CHECK(
      i2c_master_transmit(pointer->bme280_handle, write_oversampling, 2, 500));

  const uint8_t read_temperature[1] = {0xFA};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(
      pointer->bme280_handle, read_temperature, 1,
      pointer->bme280_buf_temperature, I2CMUX_BUF_TEMPERATURE_SIZE, 250));
  const uint8_t read_pressure[1] = {0xF7};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(
      pointer->bme280_handle, read_pressure, 1, pointer->bme280_buf_pressure,
      I2CMUX_BUF_PRESSURE_SIZE, 250));
  uint32_t adc_Tu = 0;
  adc_Tu |= pointer->bme280_buf_temperature[0];
  adc_Tu <<= 8;
  adc_Tu |= pointer->bme280_buf_temperature[1];
  adc_Tu <<= 8;
  adc_Tu |= pointer->bme280_buf_temperature[2];
  adc_Tu >>= 4;
  int32_t adc_T = (int32_t)(adc_Tu);
  uint32_t adc_P = 0;
  adc_P |= pointer->bme280_buf_pressure[0];
  adc_P <<= 8;
  adc_P |= pointer->bme280_buf_pressure[1];
  adc_P <<= 8;
  adc_P |= pointer->bme280_buf_pressure[2];
  adc_P >>= 4;

  // store pressure and temperature compensated per Bosch BME280 datasheet
  // section 4.2.3
  // temperature is needed first
  int32_t t_var1, t_var2, t_fine;
  const uint16_t dig_T1 = pointer->bme280_temperature_coeff_1;
  const int16_t dig_T2 = pointer->bme280_temperature_coeffs[0];
  const int16_t dig_T3 = pointer->bme280_temperature_coeffs[1];
  // https://github.com/boschsensortec/BME280_SensorAPI/blob/c90d419492e26dd95586598a794e65eb2760753a/bme280.c#L1247
  t_var1 = ((adc_T / 8) - ((int32_t)dig_T1 * 2));
  t_var1 = (t_var1 * ((int32_t)dig_T2)) / 2048;
  t_var2 = ((adc_T / 16) - ((int32_t)dig_T1));
  t_var2 = (((t_var2 * t_var2) / 4096) * ((int32_t)dig_T3)) / 16384;
  t_fine = t_var1 + t_var2;
  pointer->bme280_temperature = (t_fine * 5 + 128) / 256;

  // get pressure next
  int64_t p_var1, p_var2, p_var3, p_var4;
  const uint16_t dig_P1 = pointer->bme280_pressure_coeff_1;
  const int16_t dig_P2 = pointer->bme280_pressure_coeffs[0];
  const int16_t dig_P3 = pointer->bme280_pressure_coeffs[1];
  const int16_t dig_P4 = pointer->bme280_pressure_coeffs[2];
  const int16_t dig_P5 = pointer->bme280_pressure_coeffs[3];
  const int16_t dig_P6 = pointer->bme280_pressure_coeffs[4];
  const int16_t dig_P7 = pointer->bme280_pressure_coeffs[5];
  const int16_t dig_P8 = pointer->bme280_pressure_coeffs[6];
  const int16_t dig_P9 = pointer->bme280_pressure_coeffs[7];
  // https://github.com/boschsensortec/BME280_SensorAPI/blob/c90d419492e26dd95586598a794e65eb2760753a/bme280.c#L1281
  p_var1 = ((int64_t)t_fine) - 128000;
  p_var2 = p_var1 * p_var1 * (int64_t)dig_P6;
  p_var2 = p_var2 + ((p_var1 * (int64_t)dig_P5) * 131072);
  p_var2 = p_var2 + (((int64_t)dig_P4) * 34359738368);
  p_var1 = ((p_var1 * p_var1 * (int64_t)dig_P3) / 256) +
           ((p_var1 * ((int64_t)dig_P2) * 4096));
  p_var3 = ((int64_t)1) * 140737488355328;
  p_var1 = (p_var3 + p_var1) * ((int64_t)dig_P1) / 8589934592;
  if (p_var1 != 0) {
    p_var4 = 1048576 - adc_P;
    p_var4 = (((p_var4 * (int64_t)2147483648) - p_var2) * 3125) / p_var1;
    p_var1 = (((int64_t)dig_P9) * (p_var4 / 8192) * (p_var4 / 8192)) / 33554432;
    p_var2 = (((int64_t)dig_P8) * p_var4) / 524288;
    p_var4 = ((p_var4 + p_var1 + p_var2) / 256) + (((int64_t)dig_P7) * 16);
    pointer->bme280_pressure = (uint32_t)(((p_var4 / 2) * 100) / 128);
  } else {
    ESP_LOGI(TAG, "Read an invalid pressure from BME280");
  }
#endif
}

void iic_mux_free(iic_mux_t *pointer) {
#ifdef CONFIG_I2CMUX_BME280
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(pointer->bme280_handle));
#endif
  ESP_ERROR_CHECK(i2c_del_master_bus(pointer->bus_handle));
  free(pointer);
}