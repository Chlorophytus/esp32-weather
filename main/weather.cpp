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
#include "include/weather_gps.hpp"
#include "include/weather_logging.hpp"
#include "include/weather_trace.hpp"
#include <chrono>
#include <cstdlib>
#include <format>
#include <thread>
using namespace weather;

static constexpr const char *TAG = "weather";

extern "C" void app_main() {
  logging::group &loggers = logging::group::get_instance();
  try {
    loggers.listeners.emplace_back(new logging::esplog_listener());

    loggers.log(TAG, logging::severity::information,
                "Weather meters initialized successfully.");

    gps::service &gps = gps::service::get_instance();

    while (true) {
      weather::gps::sentence *sentence = gps.try_get_sentence();
      if (sentence != nullptr) {
        if (sentence->get_type() == gps::sentence_t::gga) {
          auto gga = dynamic_cast<gps::sentence_gga *>(sentence);
          loggers.log(TAG, logging::severity::information,
                      "Received time: ", std::format("{:02}", gga->hours),
                      ":", std::format("{:02}", gga->minutes), ":",
                      std::format("{:02}", gga->seconds),
                      ", fix type: ", gps::get_quality_name(gga->quality));
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } catch (const std::exception &e) {
    trace::print_nested_exception(e);
    loggers.log(TAG, logging::severity::error, "Restarting in 5 seconds");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    abort();
  }
}