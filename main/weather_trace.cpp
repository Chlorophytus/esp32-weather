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
#include "include/weather_trace.hpp"
#include "include/weather_logging.hpp"
#include <cstdlib>
#include <cxxabi.h>
#include <typeinfo>

using namespace weather;

static constexpr const char *TAG = "weather_trace";

void trace::print_nested_exception(const std::exception &e, U32 level) {
  int status;
  char *name;

  // demangle name with C++ ABI helper, need to free it afterward
  // https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_demangling.html
  name = abi::__cxa_demangle(typeid(e).name(), nullptr, nullptr, &status);
  logging::group::get_instance().log(TAG, logging::severity::error, "[", level, "] ",
                                     name, ": ", e.what());
  std::free(name);

  try {
    std::rethrow_if_nested(e);
  } catch (const std::exception &nested) {
    print_nested_exception(nested, level + 1);
  }
}