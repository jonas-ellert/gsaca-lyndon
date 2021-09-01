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

#include <chrono>
#include <sstream>
#include <iomanip>

namespace gsaca_lyndon {

struct timer {
private:
  decltype(std::chrono::high_resolution_clock::now()) begin_;
  decltype(std::chrono::high_resolution_clock::now()) end_;

public:
  static std::string dblstr(double const &dbl) {
    std::stringstream str;
    str << std::setprecision(2) << std::fixed << dbl;
    return str.str();
  }

  void begin() {
    begin_ = std::chrono::high_resolution_clock::now();
  }

  void end() {
    end_ = std::chrono::high_resolution_clock::now();
  }

  uint64_t millis() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end_ - begin_)
        .count();
  }

  double seconds() const {
    return millis() / 1000.0;
  }

  double mibs(uint64_t bytes) const {
    return bytes / seconds() / 1024.0 / 1024.0;
  }

  std::string throughput_string(uint64_t bytes) const {
    std::stringstream ss;
    ss << "[" << dblstr(mibs(bytes)) << "mib/s vs " <<
       dblstr(seconds()) << "s]";
    return ss.str();
  }
};

}
