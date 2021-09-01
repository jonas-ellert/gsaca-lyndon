#pragma once

#include "common/logging.hpp"
#include "common/timer.hpp"
#include "common/phase_types.hpp"
#include <ips4o/ips4o.hpp>

#include <sorting/radix32.hpp>

namespace gsaca_lyndon {

template<typename sorter, typename F = flag_type<false>,
    typename index_type, typename buffer_type, // flag_type
    bool measure_all = false,
    bool measure_sorting = measure_all,
    bool measure_keyfetch = measure_all,
    bool measure_subgrouping = measure_all,
    bool measure_writing = measure_all>
inline void
phase_2_by_sorting(index_type *const sa, buffer_type *const isa, size_t const n,
                   phase_2_group_type<buffer_type> const *const groups,
                   size_t const number_of_groups) {
  LOG_VERBOSE << "Phase 2 call: " << number_of_groups << " groups" << std::endl;

  using count_type = get_count_type<index_type, buffer_type>;
  using key_value_pair = radix_key_val_pair<buffer_type>;

  count_type max_group_size = 0;
  for (count_type g = 0; g < number_of_groups; ++g) {
    max_group_size = std::max(max_group_size, (count_type) groups[g].size);
  }

  constexpr count_type sg_count_threshold = 256ULL * 1024; // 1MiB buffer
  void *memory = malloc(
      sg_count_threshold * sizeof(count_type) +
      ((max_group_size + 1) << 1) * sizeof(key_value_pair));

  count_type *const subgroup_border_buffer = (count_type *) memory;
  key_value_pair *grouped_indices = (key_value_pair *) (subgroup_border_buffer +
                                                        sg_count_threshold);
  key_value_pair *grouped_indices_buffer = grouped_indices + max_group_size + 1;

  buffer_type *const subgroup_size =
      (buffer_type *) &(grouped_indices_buffer[0]);
  buffer_type *const subgroup_id =
      (buffer_type *) &(grouped_indices_buffer[(max_group_size >> 1) + 1]);

  timer tSort, tFetch, tSg, tWrite;
  uint64_t millisSort = 0;
  uint64_t millisFetch = 0;
  uint64_t millisSg = 0;
  uint64_t millisWrite = 0;
  uint64_t sort_cnt = 0;
  uint64_t sort_n_sum = 0;
  uint64_t sort_hists[66] = {};

  count_type left_border = 2;
  for (count_type g = 2; g < number_of_groups; ++g) {
    count_type const gsize = groups[g].size;
    if (gsize == 1) {
      sa[left_border] = F::remove_flag(sa[left_border]);
      isa[sa[left_border]] = left_border;
      ++left_border;
    } else {
      if constexpr(measure_subgrouping) tSg.begin();

      count_type const lyn = groups[g].lyndon;
      index_type *const sa_interval = &(sa[left_border]);

      for (count_type i = 0; i < gsize + 1; ++i) {
        subgroup_size[i] = 0;
      }

      subgroup_id[gsize - 1] = 0;
      subgroup_size[0] = 1;
      for (count_type i = gsize - 1; i > 0; --i) {
        subgroup_id[i - 1] = (F::remove_flag(sa_interval[i]) ==
                              (F::remove_flag(sa_interval[i - 1]) + lyn))
                             ? (subgroup_id[i] + 1)
                             : ((buffer_type) 0);
        ++subgroup_size[subgroup_id[i - 1]];
      }

      count_type sg_count = 0;
      while (subgroup_size[sg_count] > 0) ++sg_count;

      count_type *const subgroup_border =
          (gsaca_likely(sg_count < sg_count_threshold))
          ? (subgroup_border_buffer)
          : ((count_type *) malloc(sg_count * sizeof(count_type)));


      count_type local_left_border = 0;
      for (count_type i = 0; i < sg_count; ++i) {
        subgroup_border[i] = local_left_border;
        local_left_border += subgroup_size[i];
      }

      for (count_type i = 0; i < gsize; ++i) {
        count_type &border = subgroup_border[subgroup_id[i]];
        grouped_indices[border++].value = sa_interval[i];
      }

      if constexpr(measure_subgrouping) tSg.end();
      if constexpr(measure_subgrouping) millisSg += tSg.millis();

      count_type previous_border = 0;
      for (count_type j = 0; j < sg_count; ++j) {
        count_type const stop = subgroup_border[j];

        if constexpr(measure_keyfetch) tFetch.begin();
        // retrieve lexicographical rank of inducers
        for (count_type i = previous_border; i < stop; ++i) {
          grouped_indices[i].key = isa[
              F::remove_flag(grouped_indices[i].value) + lyn];
        }
        if constexpr(measure_keyfetch) tFetch.end();
        if constexpr(measure_keyfetch) millisFetch += tFetch.millis();

        if constexpr(measure_sorting) {
          tSort.begin();
          ++sort_cnt;
          sort_n_sum += stop - previous_border;
          sort_hists[std::min(65, (int) (stop - previous_border))]++;
        }

        if (gsaca_likely(stop - previous_border < 33)) {
          radix_internal::insertion<true>(
              &(grouped_indices[previous_border]), stop - previous_border);
        } else {
          // increasing sort, no need for stable sort
          sorter::template sort<true, false>(
              &(grouped_indices[previous_border]),
              grouped_indices_buffer,
              stop - previous_border, n - 1);
        }


        if constexpr(measure_sorting) tSort.end();
        if constexpr(measure_sorting) millisSort += tSort.millis();

        if constexpr(measure_writing) tWrite.begin();
        for (count_type i = previous_border; i < stop; ++i) {
          sa_interval[i] = grouped_indices[i].value;
        }
        for (count_type i = previous_border; i < stop; ++i) {
          if (!F::is_flagged(sa_interval[i])) {
            isa[sa_interval[i]] = left_border + i;
          } else {
            sa_interval[i] = F::remove_flag(sa_interval[i]);
          }
        }
        if constexpr(measure_writing) tWrite.end();
        if constexpr(measure_writing) millisWrite += tWrite.millis();
        previous_border = stop;
      }

      if (gsaca_unlikely(sg_count >= sg_count_threshold)) {
        free(subgroup_border);
      }

      left_border += gsize;
    }
  }

  free(memory);

  if constexpr(measure_sorting) {
    LOG_STATS << "sorting" << millisSort;
    LOG_STATS << "sorting_avg_n" << sort_n_sum / sort_cnt;
    for (int i = 1; i < 66; ++i) {
      std::cout << i << ": " << sort_hists[i] << std::endl;
    }
    size_t sum = 0;
    for (int i = 1; i < 33; ++i) {
      sum += sort_hists[i];
    }
    std::cout << "sum32: " << sum << std::endl;
    sum = 0;
    for (int i = 33; i < 66; ++i) {
      sum += sort_hists[i];
    }
    std::cout << "sumR:  " << sum << std::endl;
  }
  if constexpr(measure_keyfetch) LOG_STATS << "fetching" << millisFetch;
  if constexpr(measure_subgrouping) LOG_STATS << "subgrouping" << millisSg;
  if constexpr(measure_writing) LOG_STATS << "writing" << millisWrite;
}

} // namespace gsaca_lyndon
