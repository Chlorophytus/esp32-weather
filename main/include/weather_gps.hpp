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
#if !defined(WEATHER_HEADER_GPS)
#define WEATHER_HEADER_GPS

#include "weather.hpp"
#include "driver/uart.h"
#include <string_view>
#include <string>

namespace weather {
/// @brief Gets GPS time fix
namespace gps {
/// @brief A GPS NMEA sentence type
enum class sentence_t : U8 {
  /// @brief A sentence that was not handled
  unknown = 0,
  /// @brief GPS fixed data
  gga = 1,
  /// @brief Geographic position
  gll = 2,
  /// @brief Active satellites
  gsa = 3,
  /// @brief Satellites in view
  gsv = 4,
  /// @brief Position and time
  rmc = 5,
  /// @brief Course over ground
  vtg = 6,
};

/// @brief Indicates how good of a fix we got
enum class quality_indicator_t : U8 {
  /// @brief Unsupported fix type
  unknown = 255,
  /// @brief Invalid fix
  invalid = 0,
  /// @brief Valid GPS fix
  gps_fix = 1,
  /// @brief Valid Differential GPS fix
  differential_gps_fix = 2
};
/// @brief Get the human-readable name of a quality
/// @param quality The machine-readable quality
/// @return The human-readable quality
const char *get_quality_name(quality_indicator_t quality);

/// @brief A NMEA sentence structure
struct sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const;

  /// @brief Tries to parse a NMEA sentence
  /// @param raw The NMEA sentence to parse
  /// @return A sentence structure pointer
  static sentence *parse(const std::string_view &raw);
  virtual ~sentence() = default;

  bool is_checksum_valid = false;
};
struct sentence_gga : sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const override;

  // Precision is 1/10000 of a degree, negative if west/south
  // S32 latitude, longitude;

  // Precision is 1/10 of a meter
  // S32 altitude, wgs84_height;

  // U16 reference_station_id;
  U8 hours, minutes, seconds, centiseconds; //, centiseconds, satellites_used;
  quality_indicator_t quality = quality_indicator_t::unknown;
};
struct sentence_gll : sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const override;
};
struct sentence_gsa : sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const override;
};
struct sentence_gsv : sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const override;
};
struct sentence_rmc : sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const override;

  U8 hours, minutes, seconds, years, months, days;
};
struct sentence_vtg : sentence {
  /// @brief Get the type of this sentence
  /// @return The type of this sentence
  virtual sentence_t get_type() const override;
};

/// @brief The singleton GPS service
class service {
  service();

  service(const service &) = delete;
  service &operator=(const service &) = delete;
  service(service &&) = delete;
  service &operator=(service &&) = delete;

  constexpr static auto UART_CHUNK_READ = 512;
  uart_config_t _uart_config;
  char _uart_ch;
  std::string _uart_buffer{};
  sentence *_current_sentence = nullptr;

public:
  /// @brief Gets the service singleton
  /// @return A reference to the service singleton
  static service &get_instance();

  /// @brief Checks the UART for a GPS sentence, or returns nullptr
  /// @return A pointer to a GPS sentence structure
  sentence *try_get_sentence();
};
} // namespace gps
} // namespace weather

#endif