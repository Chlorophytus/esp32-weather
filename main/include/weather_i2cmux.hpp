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
#if !defined(WEATHER_HEADER_I2CMUX)
#define WEATHER_HEADER_I2CMUX

#include "driver/i2c_master.h"
#include "weather.hpp"

namespace weather {
/// @brief Handles I2C based sensors (Bosch BME280, etc)
namespace i2cmux {

/// @brief The singleton I2C multiplexer service
class service {
  service();

  service(const service &) = delete;
  service &operator=(const service &) = delete;
  service(service &&) = delete;
  service &operator=(service &&) = delete;

  
  i2c_master_bus_config_t _i2c_config;
  i2c_master_bus_handle_t _i2c_handle;
  i2c_device_config_t _bme280_config;
  i2c_master_dev_handle_t _bme280_handle;
  U16 _dig_T1, _dig_P1;
  S16 _dig_T2, _dig_T3, _dig_P2, _dig_P3, _dig_P4, _dig_P5, _dig_P6, _dig_P7, _dig_P8, _dig_P9;

  U32 _pressure;
  S32 _temperature;
public:
  /// @brief Gets the service singleton
  /// @return A reference to the service singleton
  static service &get_instance();

  void refresh();
};
} // namespace i2cmux
} // namespace weather

#endif