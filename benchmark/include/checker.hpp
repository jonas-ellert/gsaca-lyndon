#pragma once

#include <divsufsort.h>
#include <iostream>
#include <vector>
#include "common/uint_types.hpp"

template <typename Array>
inline void print_array(Array arr, size_t n, std::string name = "Array") {
    if ( n == 0 ) {
        std::cout << name << ": []" << std::endl;
        return;
    }
    std::cout << name << ": [";
    for (size_t i = 0; i < n-1; ++i) {
        std::cout << arr[i] << ", ";
    }
    std::cout << arr[n-1] << "]" << std::endl;
}

// checker based on https://www.researchgate.net/publication/47841538_Fast_Lightweight_Suffix_Array_Construction_and_Checking
template<typename value_type>
struct checker_isa {
  value_type const *const text_;
  size_t const n_;
  bool enabled_;

  checker_isa(value_type const *const text, size_t n, bool enabled) : text_(text), n_(n), enabled_(enabled) { }

  template<typename index_type>
  bool check(index_type const *const sa, std::string const &name) {
    if (!enabled_) return true;

    size_t errors = 0;
    std::vector<size_t> first_errors;

    value_type const *const text = (sa[0] == n_-1 && sa[1] == 0ULL) ? text_ : (sa[0] == n_-2 ? text_+1 : nullptr);
    size_t const n = (sa[0] == n_-1 && sa[1] == 0ULL) ? n_ : (sa[0] == n_-2 ? n_-1 : 0);

    // check whether one or two sentinels were appended
    if (text == nullptr) {
        std::cerr
            << "Could not detect sentinels. Given suffix array is incorrect."
            << std::endl;
        std::cerr << "SA[0]=" << sa[0] << ", SA[1]=" << sa[1] << ", n=" << n
                  << "\n"
                  << std::endl;
        return false;
    }

    /*print_array(text, n, "T");
    print_array(sa, n, "SA in ISA");*/

    std::cout << "\n\nChecking correctness of computed suffix array (" << name
              << ")." << std::endl;

    // checking condition i)
    for (size_t i = 0; i < n; ++i) {
        if (sa[i] >= n) {
            if (errors < 20) {
              first_errors.emplace_back(i);
            }
            ++errors;
        }
    }
    if (errors > 0) {
        for (size_t i = 0; i < first_errors.size(); ++i) {
          std::cerr << "Found " << errors
                    << " errors. Error at i=" << first_errors[i] << ": "
                    << sa[first_errors[i]] << " is out of range\n"
                    << std::endl;
        }
        return false;
    }

    // checking condition ii)
    for (size_t i = 1; i < n; ++i) {
        if (text[sa[i-1]] > text[sa[i]]) {
            if (errors < 20) {
              first_errors.emplace_back(i);
            }
            ++errors;
        }
    }
    if (errors > 0) {
        for (size_t i = 0; i < first_errors.size(); ++i) {
          std::cerr << "Found " << errors
                    << " errors. Error at i=" << first_errors[i] << ": "
                    << sa[first_errors[i]] << " vs "
                    << sa[first_errors[i]-1] << "\n"
                    << std::endl;
        }
        return false;
    }

    // computing isa and checking if sa is permutation
    index_type *const isa =
        (index_type *) malloc(n * sizeof(index_type));
    index_type const empty = n;
    for (size_t i = 0; i < n; ++i) {
        isa[i] = empty;
    }
    for (size_t i = 0; i < n; ++i) {
        if (isa[sa[i]] != empty) {
            if (errors < 20) {
              first_errors.emplace_back(i);
            }
            ++errors;
        }
        isa[sa[i]] = i;
    }
    if (errors > 0) {
        for (size_t i = 0; i < first_errors.size(); ++i) {
          std::cerr << "Found " << errors
                    << " errors. Error at i=" << first_errors[i] << ": "
                    << sa[first_errors[i]] << " not unique\n"
                    << std::endl;
        }
        return false;
    }

    // checking condition iii)
    for (size_t i = 1; i < n; ++i) {
        if (sa[i-1] != n-1 && text[sa[i-1]] == text[sa[i]]) {
            size_t j = isa[sa[i-1]+1];
            size_t k = isa[sa[i]+1];
            if (j >= k) {
                if (errors < 20) {
                  first_errors.emplace_back(i);
                }
                ++errors;
            }
        }
    }
    if (errors > 0) {
        for (size_t i = 0; i < first_errors.size(); ++i) {
          std::cerr << "Found " << errors
                    << " errors. Error at i=" << first_errors[i] << ": "
                    << sa[first_errors[i]] << " vs "
                    << sa[first_errors[i]-1] << "\n"
                    << std::endl;
        }
        return false;
    }

    std::cout << "The computed suffix array is CORRECT!\n" << std::endl;
    return true;
  }
};

template<typename value_type>
struct checker {
  value_type const *const text_;
  size_t const n_;
  bool enabled_;
  std::vector<int32_t> correct_sa;


  checker(value_type const *const text, size_t n, bool enabled) : text_(text), n_(n), enabled_(enabled) {
    if (enabled) {
      std::cout << "Preparing check: Computing correct SA using divsufsort."
                << std::endl;
      correct_sa.resize(n_ - 1);
      divsufsort(&(text_[1]), correct_sa.data(), n_ - 1);
    }
  }

  template<typename index_type>
  bool check(index_type const *const sa, std::string const &name) {
    if (!enabled_) return true;

    size_t errors = 0;
    std::vector<size_t> first_errors;
    size_t const n = correct_sa.size();


    print_array(sa, n, "SA ohne ISA");

    std::cout << "\n\nChecking correctness of computed suffix array (" << name
              << ")." << std::endl;

    if (((size_t) sa[0]) == n - 1) {
      std::cout << "Detected sentinel at end." << std::endl;
      for (size_t i = 0; i < n; ++i) {
        if (((int32_t) sa[i]) != correct_sa[i]) {
          if (errors < 20) {
            first_errors.emplace_back(i);
          }
          ++errors;
        }
      }
      if (errors > 0) {
        for (size_t i = 0; i < first_errors.size(); ++i) {
          std::cerr << "Found " << errors
                    << " errors. Error at i=" << first_errors[i] << ": "
                    << sa[first_errors[i]] << " vs "
                    << correct_sa[first_errors[i]] << "\n"
                    << std::endl;
        }
        return false;
      }
    } else if (((size_t) sa[0]) == n && ((size_t) sa[1]) == 0ULL) {
      std::cout << "Detected sentinel at beginning and end." << std::endl;
      for (size_t i = 1; i < n; ++i) {
        if ((int32_t) sa[i + 1] - 1 != correct_sa[i]) {
          if (errors < 20) {
            first_errors.emplace_back(i);
          }
          ++errors;
        }
      }
      if (errors > 0) {
        for (size_t i = 0; i < first_errors.size(); ++i) {
          std::cerr << "Found " << errors << " errors. First error at i=("
                    << first_errors[i] << " + 1): (" << sa[first_errors[i] + 1]
                    << " - 1) vs " << correct_sa[first_errors[i]] << "\n"
                    << std::endl;
        }
        return false;
      }
    } else {
      std::cerr
          << "Could not detect sentinels. Given suffix array is incorrect."
          << std::endl;
      std::cerr << "SA[0]=" << sa[0] << ", SA[1]=" << sa[1] << ", n=" << n
                << "\n"
                << std::endl;
      return false;
    }

    std::cout << "The computed suffix array is CORRECT!\n" << std::endl;
    return true;
  }
};

