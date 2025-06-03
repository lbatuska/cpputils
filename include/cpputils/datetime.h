#pragma once

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace cpputils {
namespace datetime {

// When we use ISO-8601 we technially mean RFC 3339 and not ISO 8601-1:2019
// eg: 2025-05-18T18:53:46Z
// "I'd just like to interject for a moment. What you're referring to as ISO-8601,
// is in fact, RFC-3339, or as I've recently taken to calling it ISO-8601 plus RFC-3339."

inline auto print_iso8601_utc(std::chrono::system_clock::time_point tp)
    -> std::string {
  using namespace std::chrono;

  time_t t = system_clock::to_time_t(tp);
  std::tm utc_tm = *std::gmtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
};

inline auto print_iso8601_local(std::chrono::system_clock::time_point tp)
    -> std::string {
  using namespace std::chrono;

  std::time_t time = system_clock::to_time_t(tp);
  std::tm local_tm = *std::localtime(&time);
  std::ostringstream oss;

  oss << std::put_time(&local_tm, "%Y-%m-%dT%H:%M:%S");
  return oss.str();
}

inline int64_t now() {
  using namespace std::chrono;
  return static_cast<int64_t>(
      duration_cast<milliseconds>(system_clock::now().time_since_epoch())
          .count());
}

inline std::chrono::system_clock::time_point to_time_point(
    int64_t timestamp_ms) {
  using namespace std::chrono;
  return system_clock::time_point(milliseconds(timestamp_ms));
}

inline int64_t to_timestamp(std::chrono::system_clock::time_point tp) {
  using namespace std::chrono;
  return duration_cast<milliseconds>(tp.time_since_epoch()).count();
}

}  // namespace datetime
}  // namespace cpputils
