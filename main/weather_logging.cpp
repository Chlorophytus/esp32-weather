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
#include "include/weather_logging.hpp"
#include "esp_log.h"

using namespace weather;

logging::group &logging::group::get_instance() {
  static logging::group instance;
  return instance;
}

void logging::esplog_listener::_log(const char *tag, std::string &s,
                                        const logging::severity severity) {
                                          const std::string string = s;
  switch (severity) {
  case severity::error:
    ESP_LOGE(tag, "%s", s.c_str());
    break;
  case severity::warning:
    ESP_LOGW(tag, "%s", s.c_str());
    break;
  case severity::information:
    ESP_LOGI(tag, "%s", s.c_str());
    break;
  case severity::debug:
    ESP_LOGD(tag, "%s", s.c_str());
    break;
  case severity::verbose:
    ESP_LOGV(tag, "%s", s.c_str());
    break;
  }
}