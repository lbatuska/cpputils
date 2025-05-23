#include "uuidv7.h"
#include "datetime.h"

#include <cstddef>
#include <cstdio>
#include <string>

using cpputils::UUIDv7Generator;

uint8_t UUIDv7Generator::next_sequence(int64_t current_timestamp) {
  uint64_t prev_timestamp = last_timestamp.load(std::memory_order_relaxed);

  if (current_timestamp > prev_timestamp) {
    last_timestamp.store(current_timestamp, std::memory_order_relaxed);
    sequence.store(0, std::memory_order_relaxed);
    return 0;
  } else {
    return sequence.fetch_add(1, std::memory_order_relaxed) & 0xFF;
  }
}

std::array<uint8_t, 16> UUIDv7Generator::generate() {
  std::array<uint8_t, 16> uuid{};
  int64_t timestamp = cpputils::datetime::now();
  uint8_t seq = next_sequence(timestamp);
  uint64_t rand_a = random64();
  uint64_t rand_b = random64();

  // Encode timestamp (big-endian)
  uuid[0] = (timestamp >> 40) & 0xFF;
  uuid[1] = (timestamp >> 32) & 0xFF;
  uuid[2] = (timestamp >> 24) & 0xFF;
  uuid[3] = (timestamp >> 16) & 0xFF;
  uuid[4] = (timestamp >> 8) & 0xFF;
  uuid[5] = timestamp & 0xFF;

  // Version 7 (UUIDv7)
  uuid[6] = ((timestamp >> 4) & 0x0F) | 0x70;
  uuid[7] = ((timestamp << 4) & 0xF0) | ((seq >> 8) & 0x0F);

  // Variant 1 (RFC 4122 standard)
  uuid[8] = (seq & 0xFF) | 0x80;

  // Fill remaining bytes with randomness
  for (size_t i = 9; i < 16; i++) {
    uuid[i] = (rand_b >> ((15 - i) * 8)) & 0xFF;
  }

  return uuid;
}

std::string UUIDv7Generator::uuidToString(const std::array<uint8_t, 16>& uuid) {
  char buffer[37];
  constexpr int dash_positions[] = {8, 13, 18, 23};
  int j = 0;
  for (size_t i = 0; i < 16; ++i) {
    // Convert each byte to hex
    sprintf(&buffer[j], "%02x", uuid[i]);
    j += 2;

    for (int dash : dash_positions) {
      if (j == dash) {
        buffer[j++] = '-';
        break;
      }
    }
  }
  return std::string(buffer, 36);
}
