#pragma once

#include <string>
#include <sstream>

#define LOG_VERBOSE if constexpr (false) std::cout
#define LOG_STATS if constexpr (true) ::gsaca_lyndon::clog

namespace gsaca_lyndon {

namespace logging_internal {

constexpr uint64_t LOG_LIMIT = 1ULL << 20;

struct temporary_logger {
  std::string key;

  template<typename T>
  void operator<<(T &&);
};
}

struct {
private:
  uint64_t size = logging_internal::LOG_LIMIT;
  std::stringstream ss;
public:
  friend class logging_internal::temporary_logger;

  inline auto operator<<(std::string &&str) {
    return logging_internal::temporary_logger{std::move(str)};
  }

  inline std::string get_and_clear_log() {
    auto result = ss.str();
    size = logging_internal::LOG_LIMIT;
    ss = std::stringstream();
    return result;
  }
} clog;

template<typename T>
void logging_internal::temporary_logger::operator<<(T &&t) {
  std::string value = std::to_string(t);
  uint64_t add = 2 + key.size() + value.size();
  if (clog.size + add < logging_internal::LOG_LIMIT) {
    clog.size += add;
    clog.ss << " " << std::move(key) << "=" << std::move(value);
  } else {
    clog.size = 1 + key.size() + value.size();
    clog.ss = std::stringstream();
    clog.ss << std::move(key) << "=" << std::move(value);
  }
}


}
