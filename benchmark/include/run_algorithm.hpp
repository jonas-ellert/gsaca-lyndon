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

#include <iostream>
#include <time_measure.hpp>

template<typename index_type, bool disable_cout = false, typename runner_type>
void run_generic(const std::string algo,
                 const std::string info,
                 const uint64_t n,
                 const uint64_t runs,
                 runner_type &runner) {
  if (runs > 0) {

    struct {
      std::streambuf *oldCoutStreamBuf = std::cout.rdbuf();
      std::ostringstream silence;

      inline void mute() {
        std::cout.rdbuf(silence.rdbuf());
      }

      inline void unmute() {
        std::cout.rdbuf(oldCoutStreamBuf);
      }
    } manage_cout;


    std::cout << "RESULT algo=" << algo << " " << info << " runs=" << runs
              << " n=" << n << std::flush;

    if constexpr (disable_cout)
      manage_cout.mute();

    using time_mem_type = std::pair<uint64_t, uint64_t>;
    using stats_type = std::pair<time_mem_type, std::string>;

    std::vector<stats_type> stats;

    for (size_t i = 0; i < runs; ++i) {
      std::vector<index_type> sa_vec(n);
      auto tm = get_time_mem([&]() { runner(sa_vec.data()); });
      auto l = gsaca_lyndon::clog.get_and_clear_log();
      stats.emplace_back(tm, l);
    }

    if constexpr (disable_cout)
      manage_cout.unmute();

    std::sort(stats.begin(), stats.end(),
              [&](stats_type const &a, stats_type const &b) {
                  return a.first < b.first;
              });

    uint64_t time = stats[runs >> 1].first.first;
    uint64_t mem = stats[runs >> 1].first.second;

    if constexpr (disable_cout)
      manage_cout.unmute();

    auto additional_bpn = (8.0 * mem) / n;
    auto mibs = (n / 1024.0 / 1024.0) / (time / 1000.0);

    std::cout << " " << stats[runs >> 1].second
              << " median_time=" << time
              << " mibs=" << mibs
              << " additional_memory=" << mem
              << " additional_bpn=" << additional_bpn << std::endl;
  }
}
