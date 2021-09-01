//  Copyright (c) 2019 Jonas Ellert
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.

#pragma once

#include <algorithm>
#include <chrono>
#include <malloc_count.h>
#include <sstream>

struct time_measure {
  decltype(std::chrono::high_resolution_clock::now()) begin_;
  decltype(std::chrono::high_resolution_clock::now()) end_;

  void begin() {
    begin_ = std::chrono::high_resolution_clock::now();
  }

  void end() {
    end_ = std::chrono::high_resolution_clock::now();
  }

  uint64_t millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_ - begin_)
        .count();
  }
};

template<typename runner_type>
static inline std::pair <uint64_t, uint64_t>
get_time_mem(runner_type const &runner) {
  time_measure time_measurement;

  malloc_count_reset_peak();
  uint64_t const mem_pre = malloc_count_current();
  time_measurement.begin();
  runner();
  time_measurement.end();
  uint64_t const time = time_measurement.millis();
  uint64_t const mem_peak = malloc_count_peak();
  uint64_t const memory = mem_peak - mem_pre;
  return {time, memory};
}