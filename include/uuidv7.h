#pragma once

#include <string.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

#include "datetime.h"

namespace cpputils {
class UUIDv7Generator {
  /*
   *
   *    0                   1                   2                   3
   *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |                           unix_ts_ms                          |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |          unix_ts_ms           |  ver  |       rand_a          |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |var|                        rand_b                             |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |                            rand_b                             |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   */

 private:
  std::atomic<int64_t> last_timestamp{0};
  std::atomic<uint8_t> sequence{0};
  std::mt19937_64 rng;

  // inline uint64_t get_current_timestamp_ms() {
  //   using namespace std::chrono;
  //   return static_cast<uint64_t>(
  //       duration_cast<milliseconds>(system_clock::now().time_since_epoch())
  //           .count());
  // }

  inline uint64_t random64() { return rng(); }

  uint8_t next_sequence(int64_t current_timestamp);

 public:
  inline UUIDv7Generator() : rng(std::random_device{}()) {}

  static inline uint64_t get_timestamp_from_uuid(
      std::array<uint8_t, 16>& uuid) {
    uint64_t timestamp = 0;
    for (size_t i = 0; i < 6; ++i) {
      timestamp = (timestamp << 8) | uuid[i];
    }
    return timestamp;
  }

  std::array<uint8_t, 16> generate();

  static std::string uuidToString(const std::array<uint8_t, 16>& uuid);
};
};  // namespace cpputils
