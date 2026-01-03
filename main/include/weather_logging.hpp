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
#if !defined(WEATHER_HEADER_LOGGING)
#define WEATHER_HEADER_LOGGING

#include "weather.hpp"
#include <filesystem>
#include <format>
#include <string>
#include <vector>

namespace weather {
/// @brief Logs messages to I/O devices
namespace logging {
/// @brief The severity of an event, in ESP-IDF logger verbosity
enum class severity : U8 {
  /// @brief Critical error that requires intervention
  error = 0,

  /// @brief Might cause an issue but handled
  warning = 1,

  /// @brief Genreal information
  information = 2,

  /// @brief Detailed diagnostic messages
  debug = 3,

  /// @brief Highly detailed and frequent debugging messages
  verbose = 4
};

/// @brief An Abstract Base Class for a listener logger
class base_listener {
  std::string _make_string(void *head) { return std::format("{:016}", head); }
  std::string _make_string(std::filesystem::path head) { return head.string(); }
  std::string _make_string(std::string head) { return head; }
  std::string _make_string(const char *head) { return head; }
  std::string _make_string(bool head) { return head ? "true" : "false"; }
  std::string _make_string(U8 head) { return std::to_string(head); }
  std::string _make_string(U16 head) { return std::to_string(head); }
  std::string _make_string(U32 head) { return std::to_string(head); }
  std::string _make_string(U64 head) { return std::to_string(head); }
  std::string _make_string(S8 head) { return std::to_string(head); }
  std::string _make_string(S16 head) { return std::to_string(head); }
  std::string _make_string(S32 head) { return std::to_string(head); }
  std::string _make_string(S64 head) { return std::to_string(head); }
  std::string _make_string(F32 head) { return std::to_string(head); }
  std::string _make_string(F64 head) { return std::to_string(head); }

  virtual void _log(const char *, std::string &, const severity) = 0;

public:
  /// @brief Log to this individual logger
  /// @tparam ...Ts Template parameter pack for `params`
  /// @param tag A namespace to apply, useful for the ESP-IDF logger
  /// @param severity Log priority/severity
  /// @param ...params Parameters to log
  template <class... Ts>
  void log(const char *tag, const severity severity, Ts... params) {
    std::string s{};
    s += (... + _make_string(params));
    _log(tag, s, severity);
  }

  /// @brief Cleans up the listener
  virtual ~base_listener() {};
};

/// @brief ESP-IDF logger I/O listener
class esplog_listener : public base_listener {
  void _log(const char *, std::string &, const severity) override;
};

/// @brief The singleton group of loggers
class group {
  group() = default;

  group(const group &) = delete;
  group &operator=(const group &) = delete;
  group(group &&) = delete;
  group &operator=(group &&) = delete;

public:
  /// @brief All loggers in this singleton
  std::vector<std::unique_ptr<base_listener>> listeners{};

  /// @brief Gets the logging singleton
  /// @return A reference to the logging singleton
  static group &get_instance();

  /// @brief Logs a message to all available loggers
  /// @tparam ...Ts Template parameter pack for `params`
  /// @param tag A namespace to apply, useful for the ESP-IDF logger
  /// @param severity Log priority/severity
  /// @param ...params Parameters to log
  template <class... Ts>
  void log(const char *tag, const severity severity, Ts... params) {
    for (auto &logger : listeners) {
      logger->log(tag, severity, params...);
    }
  }
};
} // namespace logging
} // namespace weather

#endif