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
#include "driver/uart.h"
#include "include/weather_logging.hpp"
#include "portmacro.h"
#include <format>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

using namespace weather;

static constexpr const char *TAG = "weather_gps";

gps::service::service()
    : _uart_config{.baud_rate = 9600,
                   .data_bits = UART_DATA_8_BITS,
                   .parity = UART_PARITY_DISABLE,
                   .stop_bits = UART_STOP_BITS_1,
                   .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                   .rx_flow_ctrl_thresh = 0,
                   .source_clk = UART_SCLK_DEFAULT,
                   .flags = {
                       .allow_pd = 0,
                       .backup_before_sleep = 0,
                   }} {
  ESP_ERROR_CHECK(
      uart_driver_install(UART_NUM_2, UART_CHUNK_READ, 0, 0, nullptr, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &_uart_config));
  ESP_ERROR_CHECK(
      uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "Service initialized");
}

gps::service &gps::service::get_instance() {
  static gps::service instance;
  return instance;
}

gps::sentence_t gps::sentence::get_type() const {
  return gps::sentence_t::unknown;
}
gps::sentence_t gps::sentence_gga::get_type() const {
  return gps::sentence_t::gga;
}
gps::sentence_t gps::sentence_gll::get_type() const {
  return gps::sentence_t::gll;
}
gps::sentence_t gps::sentence_gsa::get_type() const {
  return gps::sentence_t::gsa;
}
gps::sentence_t gps::sentence_gsv::get_type() const {
  return gps::sentence_t::gsv;
}
gps::sentence_t gps::sentence_rmc::get_type() const {
  return gps::sentence_t::rmc;
}
gps::sentence_t gps::sentence_vtg::get_type() const {
  return gps::sentence_t::vtg;
}

U8 char_to_nybble(char ch) {
  switch (ch) {
  case '0': {
    return 0;
  }
  case '1': {
    return 1;
  }
  case '2': {
    return 2;
  }
  case '3': {
    return 3;
  }
  case '4': {
    return 4;
  }
  case '5': {
    return 5;
  }
  case '6': {
    return 6;
  }
  case '7': {
    return 7;
  }
  case '8': {
    return 8;
  }
  case '9': {
    return 9;
  }
  case 'A': {
    return 10;
  }
  case 'B': {
    return 11;
  }
  case 'C': {
    return 12;
  }
  case 'D': {
    return 13;
  }
  case 'E': {
    return 14;
  }
  case 'F': {
    return 15;
  }
  default: {
    return -1;
  }
  }
}

bool handle_checksum(const std::string_view &raw) {
  U8 check = 0x00;

  const auto check_at = raw.find('*');
  auto check_iter = check_at;
  for (auto i = 0; i < 2; i++) {
    check_iter++;
    check <<= 4;
    const auto nybble = char_to_nybble(raw[check_iter]);
    if (nybble > 15) {
      throw std::runtime_error{
          "illegal char to nybble conversion in NMEA checksum"};
    } else {
      check |= nybble;
    }
  }

  U8 my_check = 0x00;
  for (size_t i = 1; i < check_at; i++) {
    my_check ^= raw[i];
  }
  const bool is_valid = my_check == check;

  logging::group::get_instance().log(
      TAG, logging::severity::information, "with checksum ",
      std::format("{:02x}", check), ", valid?: ", is_valid);

  return is_valid;
}
gps::sentence *handle_gga(const std::string_view &raw) {
  logging::group::get_instance().log(TAG, logging::severity::information,
                                     "GGA: ", raw);
  gps::sentence_gga *pointer = new gps::sentence_gga;
  pointer->is_checksum_valid = handle_checksum(raw);

  auto cursor = 0;
  for (auto fsm = 0; fsm < 15; fsm++) {
    switch (fsm) {
    case 0: {
      break;
    }
    case 1: {
      // HOURS
      U8 nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 2) {
        throw std::runtime_error{"bad GGA time conversion (tens of hours)"};
      }
      pointer->hours = nybble * 10;
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 9) {
        throw std::runtime_error{"bad GGA time conversion (ones of hours)"};
      }
      pointer->hours += nybble;
      // MINUTES
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 6) {
        throw std::runtime_error{"bad GGA time conversion (tens of minutes)"};
      }
      pointer->minutes = nybble * 10;
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 9) {
        throw std::runtime_error{"bad GGA time conversion (ones of minutes)"};
      }
      pointer->minutes += nybble;
      // SECONDS
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 6) {
        throw std::runtime_error{"bad GGA time conversion (tens of seconds)"};
      }
      pointer->seconds = nybble * 10;
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 9) {
        throw std::runtime_error{"bad GGA time conversion (ones of seconds)"};
      }
      pointer->seconds += nybble;
      // SEPARATOR
      if (raw[cursor] != '.') {
        throw std::runtime_error{"bad GGA time conversion (separator)"};
      }
      cursor++;
      // CENTISECONDS
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 9) {
        throw std::runtime_error{
            "bad GGA time conversion (tens of centiseconds)"};
      }
      pointer->centiseconds = nybble * 10;
      nybble = char_to_nybble(raw[cursor++]);
      if (nybble > 9) {
        throw std::runtime_error{
            "bad GGA time conversion (ones of centiseconds)"};
      }
      pointer->centiseconds += nybble;
      break;
    }
    case 2: {
      break;
    }
    case 3: {
      break;
    }
    case 4: {
      break;
    }
    case 5: {
      break;
    }
    case 6: {
      U8 nybble = char_to_nybble(raw[cursor++]);
      pointer->quality =
          static_cast<gps::quality_indicator_t>(nybble > 2 ? 255 : nybble);
      break;
    }
    case 7: {
      break;
    }
    case 8: {
      break;
    }
    case 9: {
      break;
    }
    case 10: {
      break;
    }
    case 11: {
      break;
    }
    case 12: {
      break;
    }
    case 13: {
      break;
    }
    case 14: {
      break;
    }
    default: {
      throw std::runtime_error{
          "parser state machine malfunction with GGA statement"};
    }
    }
    cursor = raw.find(',', cursor) + 1;
  }

  return pointer;
}
gps::sentence *handle_gll(const std::string_view &raw) { return nullptr; }
gps::sentence *handle_gsa(const std::string_view &raw) { return nullptr; }
gps::sentence *handle_gsv(const std::string_view &raw) { return nullptr; }
gps::sentence *handle_rmc(const std::string_view &raw) { return nullptr; }
gps::sentence *handle_vtg(const std::string_view &raw) { return nullptr; }
const char *gps::get_quality_name(quality_indicator_t quality) {
  switch (quality) {
  case quality_indicator_t::invalid: {
    return "invalid";
  }
  case quality_indicator_t::gps_fix: {
    return "GPS";
  }
  case quality_indicator_t::differential_gps_fix: {
    return "differential GPS";
  }
  default: {
    break;
  }
  }
  return "N/A";
}

gps::sentence *gps::sentence::parse(const std::string_view &raw) {
  const static std::unordered_map<
      std::string_view,
      std::function<gps::sentence *(const std::string_view &)>>
      handlers{{"GGA", handle_gga}, {"GLL", handle_gll}, {"GSA", handle_gsa},
               {"GSV", handle_gsv}, {"RMC", handle_rmc}, {"VTG", handle_vtg}};

  if (handlers.contains(raw.substr(3, 3))) {
    logging::group::get_instance().log(TAG, logging::severity::debug,
                                       "Got sentence type ", raw.substr(1, 5));
    return handlers.at(raw.substr(3, 3))(raw);
  } else {
    logging::group::get_instance().log(TAG, logging::severity::debug,
                                       "Got unknown sentence type ",
                                       raw.substr(1, 5));
    return nullptr;
  }
}

gps::sentence *gps::service::try_get_sentence() {
  if (_current_sentence != nullptr) {
    delete _current_sentence;
  }
  while (true) {
    int len =
        uart_read_bytes(UART_NUM_2, &_uart_ch, 1, 10 / portTICK_PERIOD_MS);
    if (len > 0) {
      if (_uart_ch == '$') {
        _uart_buffer.clear();
      }
      _uart_buffer.push_back(_uart_ch);
      if (_uart_ch == '\n') {
        break;
      }
    }
  }
  _current_sentence = sentence::parse(_uart_buffer);
  return _current_sentence;
}