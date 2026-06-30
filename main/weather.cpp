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
#include "include/weather_i2cmux.hpp"
#include "include/weather_logging.hpp"
#include "include/weather_net.hpp"
#include "include/weather_trace.hpp"
#include "nvs_flash.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <sys/select.h>
#include <thread>
using namespace weather;

static constexpr const char *TAG = "weather";

extern "C" void app_main() {
  logging::group &loggers = logging::group::get_instance();
  try {
    loggers.listeners.emplace_back(new logging::esplog_listener());
    loggers.log(TAG, logging::severity::information,
                "Initializing Nonvolatile Storage");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    loggers.log(TAG, logging::severity::information,
                "Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    loggers.log(TAG, logging::severity::information,
                "Weather meters initialized successfully.");

    std::thread{[] {
      i2cmux::service &i2c = i2cmux::service::get_instance();

      while (true) {
        i2c.refresh();

        std::this_thread::sleep_for(std::chrono::milliseconds(15000));
      }
    }}.detach();

    gps::service &gps = gps::service::get_instance();
    net::service &net = net::service::get_instance();
    time_t last_time_set = 0;
    while (true) {
      weather::gps::sentence *sentence = gps.try_get_sentence();
      if (sentence != nullptr) {
        switch (sentence->get_type()) {
        case gps::sentence_t::rmc: {
          auto rmc = static_cast<gps::sentence_rmc *>(sentence);
          if (gps.has_fix() && rmc->is_checksum_valid &&
              (last_time_set == 0 || time(nullptr) >= (last_time_set + 30))) {
            logging::group::get_instance().log(
                TAG, logging::severity::information,
                "Setting time of day to UTC: ",
                std::format("{:04}/{:02}/{:02} {:02}:{:02}:{:02}",
                            static_cast<U16>(rmc->years) + 2000, rmc->months,
                            rmc->days, rmc->hours, rmc->minutes, rmc->seconds));

            struct tm time;
            time.tm_year = rmc->years + 100;
            time.tm_mon = rmc->months - 1;
            time.tm_mday = rmc->days;

            time.tm_hour = rmc->hours;
            time.tm_min = rmc->minutes;
            time.tm_sec = rmc->seconds;

            last_time_set = mktime(&time);
            const struct timeval tval = {.tv_sec = last_time_set, .tv_usec = 0};
            settimeofday(&tval, nullptr);
          }
          break;
        }
        default: {
          break;
        }
        }
      }
      net.refresh();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } catch (const std::exception &e) {
    trace::print_nested_exception(e);
    loggers.log(TAG, logging::severity::error, "Restarting in 5 seconds");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    abort();
  }
}