#pragma once

#include <vector>
#include <iostream>

namespace gsaca_lyndon {

[[maybe_unused]]
static std::string abs_and_rel_string(size_t const &m, size_t const &n) {
  std::stringstream str;
  str << m << " = " << std::setprecision(2) << std::fixed
      << ((10000 * m) / n) / 100.0 << "%";
  return str.str();
}

[[maybe_unused]]
static std::string relative_string(size_t const &m, size_t const &n) {
  std::stringstream str;
  str << std::setprecision(2) << std::fixed << ((10000 * m) / n) / 100.0;
  return str.str();
}

template<typename t1, typename t2>
static std::string sanity_string(t1 const &a, t2 const &b) {
  return "sanity: " + std::to_string(a) + " = " + std::to_string(b) + " [" +
         ((a == b) ? "sane" : "INSANE") + "]";
}

template<typename lyndon_type>
static void
print_lyndon_histogram(lyndon_type const *const lyndon, size_t const max,
                       size_t const n) {


  std::vector<size_t> hist(max + 1);
  for (size_t i = 0; i < n; ++i) {
    ++hist[std::min(max, (size_t) lyndon[i])];
  }

  size_t total = 0;
  for (size_t j = 1; j < max; ++j) {
    std::cout << j << ": " << hist[j] << ", relative:";
    std::cout << " (exactly " << j << ") ";
    std::cout << relative_string(hist[j], n);
    std::cout << " (at most " << j << ") ";
    std::cout << relative_string(total + hist[j], n);
    std::cout << " (at least " << j << ") ";
    std::cout << relative_string(n - total, n) << std::endl;
    total += hist[j];
  }
  std::cout << max << "(+): " << hist[max] << ", relative:";
  std::cout << " (at least " << max << ") ";
  std::cout << relative_string(n - total, n) << std::endl;
}


template<typename lyndon_type, typename pss_type>
static auto count_inducees_and_inducers(lyndon_type const *const lyndon,
                                        pss_type const *const pss,
                                        size_t const threshold,
                                        size_t const n) {
  size_t inducers_a = 0;
  size_t inducers_b = 0;
  size_t inducees = 0;
  for (size_t i = 1; i < n - 1; ++i) {
    if (lyndon[i] > threshold) {
      ++inducees;
    }
    if (lyndon[pss[i]] > threshold) {
      if (i + lyndon[i] - pss[i] > threshold) {
        ++inducers_b;
      }
      ++inducers_a;
    }
  }
  return std::tuple<size_t, size_t, size_t>(inducees, inducers_a, inducers_b);
}


template<typename lyndon_type>
static auto count_reduced(lyndon_type const *const lyndon,
                          size_t const threshold,
                          size_t const n) {
  size_t induced = 0;
  size_t reduced = 0;
  for (size_t i = 1; i < n - 1;) {
    if (lyndon[i] > threshold) {
      ++induced;
      size_t const target = i + threshold;
      do ++i; while (i + lyndon[i] < target);
    } else {
      ++reduced;
      i += lyndon[i];
    }
  }
  return std::pair<size_t, size_t>(induced, reduced + induced);
}


template<typename lyndon_type, typename pss_type>
static void
print_inducer_histogram(lyndon_type const *const lyndon,
                        pss_type const *const pss, size_t const max,
                        size_t const n) {

  auto relative_string = [](size_t const &m, size_t const &n) {
      std::stringstream str;
      str << std::setprecision(2) << std::fixed << ((10000 * m) / n) / 100.0;
      return str.str();
  };

  for (size_t i = 1; i <= max; ++i) {
    auto c = count_inducees_and_inducers(lyndon, pss, i, n);
    auto inducees = std::get<0>(c);
    auto inducersA = std::get<1>(c);
    auto inducersB = std::get<2>(c);
    std::cout << i << ": ";
    std::cout << "(inducees) " << inducees << " = "
              << relative_string(inducees, n) << "% ";
    std::cout << "(inducers A) " << inducersA << " = "
              << relative_string(inducersA, n) << "% ";
    std::cout << "(inducers B) " << inducersB << " = "
              << relative_string(inducersB, n) << "% " << std::endl;
  }
}

template<typename lyndon_type>
static void print_reducer_histogram(lyndon_type const *const lyndon,
                                    size_t const max,
                                    size_t const n) {

  auto relative_string = [](size_t const &m, size_t const &n) {
      std::stringstream str;
      str << std::setprecision(2) << std::fixed << ((10000 * m) / n) / 100.0;
      return str.str();
  };

  for (size_t i = 1; i <= max; ++i) {
    auto c = count_reduced(lyndon, i, n);
    auto inducers = std::get<0>(c);
    auto reducers = std::get<1>(c);
    std::cout << i << ": ";
    std::cout << "(induced) " << inducers << " = "
              << relative_string(inducers, n) << "% ";
    std::cout << "(reduced) " << reducers << " = "
              << relative_string(reducers, n) << "%" << std::endl;
  }
}

template<typename value_type, typename index_type>
static void check_increasing(value_type const *const text,
                             index_type const *const indices,
                             size_t const number_of_indices) {

  std::cout << "Checking index array of length " << number_of_indices << std::endl;

  auto llex = [&](index_type const &a, index_type const &b) {
      index_type lce = 0;
      while (text[a + lce] == text[b + lce]) ++lce;
      return (text[a + lce] < text[b + lce]);
  };
  for (size_t i = 1; i < number_of_indices; ++i) {
    if (!llex(indices[i - 1], indices[i])) {
      std::cout << "WRONG: " << i - 1 << ": " << indices[i - 1] << "; " << i
                << ": " << indices[i] << std::endl;
      for (index_type j = indices[i - 1];
           j <
           std::min(indices[i - 1] + 20, (index_type) number_of_indices); ++j) {
        std::cout << (unsigned int)text[j] << " ";
      }
      std::cout << "<====" << std::endl;
      for (index_type j = indices[i];
           j < std::min(indices[i] + 20, (index_type) number_of_indices); ++j) {
        std::cout << (unsigned int)text[j] << " ";
      }
      std::cout << "<====" << std::endl;
    }
  }

  std::cout << "Check done" << std::endl;
}


}