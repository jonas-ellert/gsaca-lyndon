#include <parallel_divsufsort.h>
#include <file_util.hpp>
#include <gsaca-hash-ds.hpp>
#include <gsaca-double-sort.hpp>
#include <gsaca-double-sort-par.hpp>
#include <gsaca.h>
#include <run_algorithm.hpp>
#include <sstream>
#include <tlx/cmdline_parser.hpp>
#include <checker.hpp>

using namespace gsaca_lyndon;

struct {
  std::vector<std::string> file_paths;
  uint64_t bytes_per_char = 1;
  uint64_t number_of_runs = 5;
  uint64_t prefix_size = 0;
  std::string contains_all = "";
  std::string contains_any = "";
  std::string not_contains_all = "";
  std::string not_contains_any = "";

  std::string list_of_cores = "1,2,4,8,16";
  bool list = false;
  bool check = false;

  bool matches_cores(const uint64_t cores) const {
    std::stringstream c(list_of_cores);
    while (c.good()) {
      std::string sub;
      getline(c, sub, ',');
      if (sub == std::to_string(cores))
        return true;
    }
    return false;
  }

  bool matches(const std::string algo) const {
    if (contains_all.size() > 0) {
      std::stringstream c_all(contains_all);
      while (c_all.good()) {
        std::string sub;
        getline(c_all, sub, ',');
        if (algo.find(sub) == std::string::npos)
          return false;
      }
    }

    if (contains_any.size() > 0) {
      std::stringstream c_any(contains_any);
      bool success = false;
      while (c_any.good()) {
        std::string sub;
        getline(c_any, sub, ',');
        if (algo.find(sub) != std::string::npos) {
          success = true;
          break;
        }
      }
      if (!success) return false;
    }

    if (not_contains_all.size() > 0) {
      std::stringstream nc_all(not_contains_all);
      bool success = false;
      while (nc_all.good()) {
        std::string sub;
        getline(nc_all, sub, ',');
        if (algo.find(sub) == std::string::npos) {
          success = true;
          break;
        }
        if (!success) return false;
      }
    }

    if (not_contains_any.size() > 0) {
      std::stringstream nc_any(not_contains_any);
      while (nc_any.good()) {
        std::string sub;
        getline(nc_any, sub, ',');
        if (algo.find(sub) != std::string::npos)
          return false;
      }
    }

    return true;
  }

} s;


int main(int argc, char const *argv[]) {
  tlx::CmdlineParser cp;
  cp.set_description("GSACA Lyndon");
  cp.set_author("Nico Bertram <nico.bertram@tu-dortmund.de>; Jonas Ellert <jonas.ellert@tu-dortmund.de>");

  cp.add_stringlist('f', "file", s.file_paths, "Path(s) to the text file(s).");

  cp.add_bytes('r', "runs", s.number_of_runs,
               "Number of repetitions of the algorithm (default = 5).");
  cp.add_bytes('l', "length", s.prefix_size,
               "Length of the prefix of the text that should be considered.");

  cp.add_string('\0', "contains", s.contains_all, "Alias for --contains-all");
  cp.add_string('\0', "contains-any", s.contains_any,
                "Only execute algorithms that contain at least one of the "
                "given strings (comma separated).");
  cp.add_string('\0', "contains-all", s.contains_all,
                "Only execute algorithms that contain all of the "
                "given strings (comma separated).");

  cp.add_string('\0', "not-contains", s.not_contains_any,
                "Alias for --not-contains-any");
  cp.add_string('\0', "not-contains-any", s.not_contains_any,
                "Only execute algorithms that contain none of the given "
                "strings (comma separated).");
  cp.add_string('\0', "not-contains-all", s.not_contains_all,
                "Only execute algorithms that do not contain all of the given "
                "strings (comma separated).");
  cp.add_string('\0', "threads", s.list_of_cores,
                "Execute parallel algorithms using the given number of "
                "OMP threads (multiple options possible; comma separated, e.g. "
                "\"1,2,4,8\").");

  cp.add_flag('\0', "list", s.list, "List the available algorithms.");
  cp.add_flag('\0', "check", s.check,
              "Check the correctness against divsufsort.");

  if (!cp.process(argc, argv)) {
    return -1;
  }

  if (s.list) {
    std::cout << "gsaca_hash_ds" << std::endl;
    std::cout << "gsaca_ds1" << std::endl;
    std::cout << "gsaca_ds2" << std::endl;
    std::cout << "gsaca_ds3" << std::endl;
    std::cout << "gsaca_ds1_par" << std::endl;
    std::cout << "gsaca_ds2_par" << std::endl;
    std::cout << "gsaca_ds3_par" << std::endl;
    return 0;
  }

  for (auto file : s.file_paths) {
    uint32_t sigma = 0;
    std::vector<uint32_t> text_vec =
        file_to_instance(file, s.prefix_size, sigma);
    const std::string info =
        std::string("file=") + file + " sigma=" + std::to_string(sigma);

    auto const *const text = text_vec.data();
    auto const n = text_vec.size();

    checker_isa<uint32_t> checker(text, n, s.check);

#define run_with_sorting_type(name, sa_type, p1_sort, p2_sort, text, n) \
    { \
        std::string name_with_sa_type = std::string(#name) + "-sa" + \
            std::to_string(sizeof(sa_type) * 8); \
        if (s.matches(name_with_sa_type)) { \
          if (s.check) { \
            sa_type * const sa = (sa_type *) malloc(n * sizeof(sa_type)); \
            name<p1_sort,p2_sort>(text, sa, n); \
            checker.check(sa, name_with_sa_type); \
            delete sa; \
          } \
          auto runner = [&](sa_type * const sa) { \
              name<p1_sort,p2_sort>(text, sa, n); \
          }; \
          run_generic<sa_type>(name_with_sa_type, info, n, s.number_of_runs, runner); \
        } \
  }

#define run_without_sorting_type(name, sa_type, text, n) \
    { \
        std::string name_with_sa_type = std::string(#name) + "-sa" + \
            std::to_string(sizeof(sa_type) * 8); \
        if (s.matches(name_with_sa_type)) { \
          if (s.check) { \
            sa_type * const sa = (sa_type *) malloc(n * sizeof(sa_type)); \
            name(text, sa, n); \
            checker.check(sa, name_with_sa_type); \
            delete sa; \
          } \
          auto runner = [&](sa_type * const sa) { \
              name(text, sa, n); \
          }; \
          run_generic<sa_type>(name_with_sa_type, info, n, s.number_of_runs, runner); \
        } \
  }

#define run_parallel(name, sa_type, text, n) \
    for (int p = 1; p < 1025; ++p) { \
        std::string name_with_sa_type = std::string(#name) + "-sa" + \
            std::to_string(sizeof(sa_type) * 8); \
        if (s.matches(name_with_sa_type) && s.matches_cores(p)) { \
            if (s.check) { \
               sa_type * const sa = (sa_type *) malloc(n * sizeof(sa_type)); \
               name(text, sa, n, p); \
               checker.check(sa, name_with_sa_type); \
               delete sa; \
            } \
            auto runner = [&](sa_type * const sa) { \
               name(text, sa, n, p); \
            }; \
            run_generic<sa_type>(name_with_sa_type, info + " threads=" + std::to_string(p), n, \
                   s.number_of_runs, runner); \
        } \
    }

    run_with_sorting_type(gsaca_ds1, uint32_t, MSD, MSD, text, n)
    run_with_sorting_type(gsaca_ds1, uint40_t, MSD, MSD, text, n)
    run_with_sorting_type(gsaca_ds1, uint64_t, MSD, MSD, text, n)

    run_with_sorting_type(gsaca_ds2, uint32_t, MSD, MSD, text, n)
    run_with_sorting_type(gsaca_ds2, uint40_t, MSD, MSD, text, n)
    run_with_sorting_type(gsaca_ds2, uint64_t, MSD, MSD, text, n)

    run_with_sorting_type(gsaca_ds3, uint32_t, MSD, MSD, text, n)
    run_with_sorting_type(gsaca_ds3, uint40_t, MSD, MSD, text, n)
    run_with_sorting_type(gsaca_ds3, uint64_t, MSD, MSD, text, n)

    run_parallel(gsaca_ds1_par, uint32_t, text, n)
    run_parallel(gsaca_ds1_par, uint40_t, text, n)
    run_parallel(gsaca_ds1_par, uint64_t, text, n)

    run_parallel(gsaca_ds2_par, uint32_t, text, n)
    run_parallel(gsaca_ds2_par, uint40_t, text, n)
    run_parallel(gsaca_ds2_par, uint64_t, text, n)

    run_parallel(gsaca_ds3_par, uint32_t, text, n)
    run_parallel(gsaca_ds3_par, uint40_t, text, n)
    run_parallel(gsaca_ds3_par, uint64_t, text, n)
  }
}
