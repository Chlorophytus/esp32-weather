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
#include "include/weather_i2cmux.hpp"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "include/weather_logging.hpp"
#include <cstring>

using namespace weather;

static constexpr const char *TAG = "weather_i2cmux";

// GCC C++ 14.2 doesn't support bit_cast that well
S16 to_s16(U16 from) {
  S16 to;
  std::memcpy(&to, &from, sizeof(U16));
  return to;
}
S32 to_s32(U32 from) {
  S32 to;
  std::memcpy(&to, &from, sizeof(U32));
  return to;
}

i2cmux::service::service()
    : _i2c_config{.i2c_port = -1,
                  .sda_io_num = GPIO_NUM_21,
                  .scl_io_num = GPIO_NUM_22,
                  .clk_source = I2C_CLK_SRC_DEFAULT,
                  .glitch_ignore_cnt = 7,
                  .intr_priority = 0,
                  .trans_queue_depth = 0,
                  .flags = {.enable_internal_pullup = 1, .allow_pd = 0}},
      _bme280_config{.dev_addr_length = I2C_ADDR_BIT_LEN_7,
                     .device_address = 0x77,
                     .scl_speed_hz = 100000,
                     .scl_wait_us = 0,
                     .flags = {.disable_ack_check = 0}} {
  // Configure the master
  ESP_ERROR_CHECK(i2c_new_master_bus(&_i2c_config, &_i2c_handle));

  // Attach Bosch BME280
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "Initializing sensor: BME280");
  ESP_ERROR_CHECK(
      i2c_master_bus_add_device(_i2c_handle, &_bme280_config, &_bme280_handle));
  // Set IIR filter off
  ESP_ERROR_CHECK(
      i2c_master_transmit(_bme280_handle, (U8[2]){0xF5, 0b00000000}, 2, 500));
  // Get trimming parameters for temperature and pressure
  U8 dig_T[6]{0};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(_bme280_handle, (U8[1]){0x88}, 1,
                                              dig_T, 6, 1000));
  U8 dig_P[18]{0};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(_bme280_handle, (U8[1]){0x8E}, 1,
                                              dig_P, 18, 1000));
  // Set trimming parameter storage
  _dig_T1 = ((static_cast<U16>(dig_T[1]) << 8) | dig_T[0]);
  _dig_T2 = to_s16((static_cast<U16>(dig_T[3]) << 8) | dig_T[2]);
  _dig_T3 = to_s16((static_cast<U16>(dig_T[5]) << 8) | dig_T[4]);

  _dig_P1 = ((static_cast<U16>(dig_P[1]) << 8) | dig_P[0]);
  _dig_P2 = to_s16((static_cast<U16>(dig_P[3]) << 8) | dig_P[2]);
  _dig_P3 = to_s16((static_cast<U16>(dig_P[5]) << 8) | dig_P[4]);
  _dig_P4 = to_s16((static_cast<U16>(dig_P[7]) << 8) | dig_P[6]);
  _dig_P5 = to_s16((static_cast<U16>(dig_P[9]) << 8) | dig_P[8]);
  _dig_P6 = to_s16((static_cast<U16>(dig_P[11]) << 8) | dig_P[10]);
  _dig_P7 = to_s16((static_cast<U16>(dig_P[13]) << 8) | dig_P[12]);
  _dig_P8 = to_s16((static_cast<U16>(dig_P[15]) << 8) | dig_P[14]);
  _dig_P9 = to_s16((static_cast<U16>(dig_P[17]) << 8) | dig_P[16]);

  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "BME280 temperature trimming parameters:");
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_T1: ", _dig_T1);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_T2: ", _dig_T2);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_T3: ", _dig_T3);

  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "BME280 pressure trimming parameters:");
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P1: ", _dig_P1);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P2: ", _dig_P2);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P3: ", _dig_P3);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P4: ", _dig_P4);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P5: ", _dig_P5);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P6: ", _dig_P6);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P7: ", _dig_P7);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P8: ", _dig_P8);
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "dig_P9: ", _dig_P9);

  // Done
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "Service initialized");
}

i2cmux::service &i2cmux::service::get_instance() {
  static i2cmux::service instance;
  return instance;
}

void i2cmux::service::refresh() {
  ESP_ERROR_CHECK(
      i2c_master_transmit(_bme280_handle, (U8[2]){0xF4, 0b00100101}, 2, 500));
  U8 T_bytes[3]{0};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(_bme280_handle, (U8[1]){0xFA}, 1,
                                              T_bytes, 3, 1000));
  U8 P_bytes[3]{0};
  ESP_ERROR_CHECK(i2c_master_transmit_receive(_bme280_handle, (U8[1]){0xF7}, 1,
                                              P_bytes, 3, 1000));

  // Unsigned raw temperature
  U32 T_u = T_bytes[0];
  T_u <<= 8;
  T_u |= T_bytes[1];
  T_u <<= 8;
  T_u |= T_bytes[2];
  T_u >>= 4;

  // Raw temperature
  S32 T = static_cast<S32>(T_u);

  // Raw pressure
  U32 P = P_bytes[0];
  P <<= 8;
  P |= P_bytes[1];
  P <<= 8;
  P |= P_bytes[2];
  P >>= 4;

  // get temperature
  // https://github.com/boschsensortec/BME280_SensorAPI/blob/c90d419492e26dd95586598a794e65eb2760753a/bme280.c#L1247
  S32 t_var1, t_var2, t_fine;
  t_var1 = (T / 8) - (static_cast<S32>(_dig_T1) * 2);
  t_var1 = (t_var1 * static_cast<S32>(_dig_T2)) / 2048;
  t_var2 = (T / 16) - static_cast<S32>(_dig_T1);
  t_var2 = (((t_var2 * t_var2) / 4096) * static_cast<S32>(_dig_T3)) / 16384;
  t_fine = t_var1 + t_var2;
  _temperature = (t_fine * 5 + 128) / 256;

  // get pressure
  // https://github.com/boschsensortec/BME280_SensorAPI/blob/c90d419492e26dd95586598a794e65eb2760753a/bme280.c#L1159
  F64 p_var1, p_var2, p_var3, pressure;
  p_var1 = (static_cast<F64>(t_fine) / 2.0) - 64000.0;
  p_var2 = p_var1 * p_var1 * static_cast<F64>(_dig_P6) / 32768.0;
  p_var2 = p_var2 + p_var1 * static_cast<F64>(_dig_P5) * 2.0;
  p_var2 = (p_var2 / 4.0) + (static_cast<F64>(_dig_P4) * 65536.0);
  p_var3 = static_cast<F64>(_dig_P3) * p_var1 * p_var1 / 524288.0;
  p_var1 = (p_var3 + static_cast<F64>(_dig_P2) * p_var1) / 524288.0;
  p_var1 = (1.0 + p_var1 / 32768.0) * static_cast<F64>(_dig_P1);

  if(p_var1 > 0.0) {
    pressure = 1048576.0 - static_cast<F64>(P);
    pressure = (pressure - (p_var2 / 4096.0)) * 6250.0 / p_var1;
    p_var1 = static_cast<F64>(_dig_P9) * pressure * pressure / 2147483648.0; 
    p_var2 = pressure * static_cast<F64>(_dig_P8) / 32768.0;
    pressure = pressure + (p_var1 + p_var2 + static_cast<F64>(_dig_P7)) / 16.0;
    _pressure = pressure;
  }
#if 0
    // https://github.com/boschsensortec/BME280_SensorAPI/blob/c90d419492e26dd95586598a794e65eb2760753a/bme280.c#L1281
  S64 p_var1, p_var2, p_var3, p_var4;
  p_var1 = static_cast<S64>(t_fine) - 128000;
  p_var2 = p_var1 * p_var1 * static_cast<S64>(_dig_P6);
  p_var2 = p_var2 + ((p_var1 * static_cast<S64>(_dig_P5)) * 131072);
  p_var2 = p_var2 + (static_cast<S64>(_dig_P4) / 34359738368);
  p_var1 = ((p_var1 * p_var1 * static_cast<S64>(_dig_P3)) / 256) +
           ((p_var1 * static_cast<S64>(_dig_P2) / 4096));
  p_var3 = 140737488355328;
  p_var1 = (p_var3 + p_var1) * static_cast<S64>(_dig_P1) / 8589934592;

  if (p_var1 != 0) {
    p_var4 = 1048576 - P;
    p_var4 =
        (((p_var4 * static_cast<S64>(2147483648)) - p_var2) * 3125) / p_var1;
    p_var1 = (static_cast<S64>(_dig_P9) * (p_var4 / 8192) * (p_var4 / 8192)) /
             33554432;
    p_var2 = (static_cast<S64>(_dig_P8) * p_var4) / 524288;
    p_var4 =
        ((p_var4 + p_var1 + p_var2) / 256) + (static_cast<S64>(_dig_P7) * 16);
    _pressure = static_cast<U32>(((p_var4 / 2) * 100) / 128);
  }
#endif

  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "Got temperature of ", _temperature,
                                     " and pressure ", _pressure);
}