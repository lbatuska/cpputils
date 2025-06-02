#include "uuid.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include "datetime.h"

using cpputils::uuid::V4;
using cpputils::uuid::V7;

std::string cpputils::uuid::uuidToString(const std::array<uint8_t, 16>& uuid) {
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

std::array<uint8_t, 16> V4::generate() {
  std::array<uint8_t, 16> uuid{};
  uint64_t rand_a = random64();
  uint64_t rand_b = random64();
  uint64_t rand_c = random64();
  // First 48 bits
  uuid[0] = (rand_a >> 40) & 0xFF;  // 0-7
  uuid[1] = (rand_a >> 32) & 0xFF;  // 8-15
  uuid[2] = (rand_a >> 24) & 0xFF;  // 16-23
  uuid[3] = (rand_a >> 16) & 0xFF;  // 24-31
  uuid[4] = (rand_a >> 8) & 0xFF;   // 32-39
  uuid[5] = rand_a & 0xFF;          // 40-47

  uuid[6] = (rand_b) & 0x0F;  // fill lower 4 bits with random
  uuid[6] |= 0x40;            // set top 4 bits to 0100
  uuid[7] = (rand_b >> 8) & 0xFF;

  uuid[8] = (rand_b >> 16) & 0xFF;
  uuid[8] |= (0b10 << 6);

  uuid[9] = (rand_c) & 0xFF;
  uuid[10] = (rand_c >> 8) & 0xFF;
  uuid[11] = (rand_c >> 16) & 0xFF;
  uuid[12] = (rand_c >> 24) & 0xFF;
  uuid[13] = (rand_c >> 32) & 0xFF;
  uuid[14] = (rand_c >> 40) & 0xFF;
  uuid[15] = (rand_c >> 48) & 0xFF;

  return uuid;
}

uint8_t V7::next_sequence(int64_t current_timestamp) {
  uint64_t prev_timestamp = last_timestamp.load(std::memory_order_relaxed);

  if (current_timestamp > prev_timestamp) {
    last_timestamp.store(current_timestamp, std::memory_order_relaxed);
    sequence.store(0, std::memory_order_relaxed);
    return 0;
  } else {
    return sequence.fetch_add(1, std::memory_order_relaxed) & 0xFF;
  }
}

std::array<uint8_t, 16> V7::generate() {
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
