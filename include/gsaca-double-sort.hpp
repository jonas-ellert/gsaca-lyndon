#pragma once

#include "common/extract.hpp"
#include "common/timer.hpp"
#include "common/util.hpp"
#include "common/logging.hpp"
#include "sequential/phase_1.hpp"
#include "sequential/phase_2.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace gsaca_lyndon {

namespace double_sort_internal {

template<typename buffer_type, typename F,
    typename index_type, typename value_type>
auto sort_by_prefix(value_type const *const text, index_type *const sa,
                    get_count_type <index_type, buffer_type> const n,
                    uint8_t const prefix) {
  using count_type = get_count_type<index_type, buffer_type>;
  using p1_stack_type = phase_1_stack_type<buffer_type>;
  using p1_group_type = typename p1_stack_type::value_type;

  p1_stack_type result;
  if (sizeof(value_type) == 1) {
      if (prefix == 1) {
        count_type histogram[256] = {};
        for (count_type i = 0; i < n; ++i) {
          ++histogram[text[i]];
        }
        count_type *const borders = histogram;
        count_type left_border = 2;
        for (count_type b = 1; b < 256; ++b) {
          count_type gsize = histogram[b];
          borders[b] = left_border;
          if (gsize > 0) {
            result.emplace_back(p1_group_type{left_border, gsize, 1, true, false});
          }
          left_border += gsize;
        }
        borders[0] = 0;
        for (count_type i = 0; i < n; ++i) {
          sa[borders[text[i]]++] = i;
        }
      } else {
        count_type const buckets = 1ULL << (prefix << 3);
        std::vector<count_type> histogram(buckets);
        count_type const stop = n - prefix - 1;

        for (count_type i = 1; i < stop; ++i) {
          ++histogram[extract(text, i, prefix)];
        }
        for (count_type i = stop; i < n - 1; ++i) {
          ++histogram[safe_extract(text, i, prefix)];
        }

        count_type *const borders = histogram.data();
        count_type left_border = 2;
        for (count_type b = buckets >> 8; b < buckets; ++b) {
          count_type gsize = histogram[b];
          borders[b] = left_border;
          if (gsize > 0) {
            result.emplace_back(p1_group_type{left_border, gsize, 1, true, false});
          }
          left_border += gsize;
        }

    	for (count_type i = 1; i < stop; ++i) {
      	    sa[borders[extract(text, i, prefix)]++] = F::conditional_add_flag(
          	text[i - 1] < text[i], i);
    	}
    	for (count_type i = stop; i < n - 1; ++i) {
            sa[borders[safe_extract(text, i, prefix)]++] = F::conditional_add_flag(
          	text[i - 1] < text[i], i);
    	}
      }
  }
  else {
      // fill sa with values
      for (count_type i = 0; i < n; ++i) {
          sa[i] = i;
      }

      // sort sa by first character
      auto comp = [&](auto a, auto b) {
           auto extracted1 = safe_extract(text, a, prefix);
           auto extracted2 = safe_extract(text, b, prefix);
           return (extracted1 < extracted2) || ((extracted1 == extracted2) && a < b);
      };
      ips4o::sort(&(sa[0]), &(sa[n]), comp);

      // determine gsizes
      count_type left_border = 2;
      count_type gsize = 1;
      for (count_type i = 2; i < n-1; ++i) {
          if (safe_extract(text, sa[i], prefix) == safe_extract(text, sa[i+1], prefix)) {
              ++gsize;
          }
          else {
              result.emplace_back(p1_group_type{left_border, gsize, 1, true, false});
              left_border = i+1;
              gsize = 1;
          }
      }
      result.emplace_back(p1_group_type{left_border, gsize, 1, true, false});
  }
  sa[0] = n - 1;
  sa[1] = 0;
  return result;
}

}

template<typename p1_sorter = MSD, typename p2_sorter = MSD,
    typename buffer_type = auto_buffer_type,
    bool use_flags = true,
    typename index_type, // auto deduce
    typename value_type, // auto deduce
    typename used_buffer_type = get_buffer_type <buffer_type, index_type>>
static void gsaca_ds(value_type const *const text, index_type *const sa,
                     size_t const n, size_t const initial_sort_prefix_len = 1) {
  static_assert(std::is_unsigned<value_type>::value);
  static_assert(std::is_unsigned<index_type>::value);
  static_assert(std::is_unsigned<used_buffer_type>::value);
  static_assert(check_buffer_type<buffer_type, index_type, used_buffer_type>);
  //static_assert(sizeof(value_type) == 1);

  using F = flag_type<use_flags>;

  timer time1;
  timer time2;
  time1.begin();
  time2.begin();
  LOG_VERBOSE << "\n\nStart SACA..." << std::endl;

  auto p1_input_groups =
      double_sort_internal::sort_by_prefix<used_buffer_type, F>(
          text, sa, n, initial_sort_prefix_len);
  used_buffer_type *const isa =
      (used_buffer_type *) malloc(n * sizeof(used_buffer_type));

  time2.end();
  LOG_VERBOSE << "Prepared phase 1: " << time2.throughput_string(n)
              << std::endl;
  LOG_STATS << "initial_buckets" << time2.millis();

  time2.begin();
  auto p2_input_groups = phase_1_by_sorting<p1_sorter, F>(sa, isa,
                                                          p1_input_groups);
  time2.end();
  time1.end();
  LOG_VERBOSE << "Phase 1 (excl. prepare):  " << time2.throughput_string(n)
              << "\n" << "Phase 1 (incl. prepare):  "
              << time1.throughput_string(n) << std::endl;
  LOG_STATS << "phase1" << time2.millis();


  time1.begin();
  phase_2_by_sorting<p2_sorter, F>(sa, isa, n, p2_input_groups.data(),
                                   p2_input_groups.size());
  free(isa);
  time1.end();

  LOG_VERBOSE << "Phase 2 (incl. free isa): " << time1.throughput_string(n)
              << std::endl;
  LOG_STATS << "phase2" << time1.millis();
}

template<typename p1_sorter = MSD, typename p2_sorter = MSD,
    typename buffer_type = auto_buffer_type,
    typename index_type, // auto deduce
    typename value_type>
static void gsaca_ds1(value_type const *const text, index_type *const sa,
                      size_t const n) {
  gsaca_ds<p1_sorter, p2_sorter, buffer_type>(text, sa, n, 1);
}

template<typename p1_sorter = MSD, typename p2_sorter = MSD,
    typename buffer_type = auto_buffer_type,
    typename index_type, // auto deduce
    typename value_type>
static void gsaca_ds2(value_type const *const text, index_type *const sa,
                      size_t const n) {
  gsaca_ds<p1_sorter, p2_sorter, buffer_type>(text, sa, n, 2);
}

template<typename p1_sorter = MSD, typename p2_sorter = MSD,
    typename buffer_type = auto_buffer_type,
    typename index_type, // auto deduce
    typename value_type>
static void gsaca_ds3(value_type const *const text, index_type *const sa,
                      size_t const n) {
  gsaca_ds<p1_sorter, p2_sorter, buffer_type>(text, sa, n, 3);
}

}
