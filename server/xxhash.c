#include <string.h>
#include "xxhash.h"

// xxh3_64.c - Minimal standalone implementation of XXH3_64bits

// 64-bit primes
const unsigned long long XXH_PRIME64_1 = 0x9e3779b185ebca87ull;
const unsigned long long XXH_PRIME64_2 = 0xc2b2ae3d27d4eb4full;
const unsigned long long XXH_PRIME64_3 = 0x165667b19e3779f9ull;
const unsigned long long XXH_PRIME64_4 = 0x85ebca77c2b2ae63ull;
const unsigned long long XXH_PRIME64_5 = 0x27d4eb2f165667c5ull;
const unsigned long long XXH_AVALANCHE = 0x165667919E3779F9ull;

static const uint8_t XXH3_kSecret[192] = {
  0xb8,0xfe,0x6c,0x39,0x23,0xa4,0x4b,0xbe,
  0x1f,0x22,0x32,0x1c,0x7b,0xe0,0x72,0x91,
  0x8b,0xea,0x4b,0x61,0xe8,0xcb,0xb3,0x9d,
  0x4b,0xdb,0x0c,0xa1,0xcb,0x04,0x5f,0x61,
  0x6b,0xed,0x3d,0x42,0x9a,0xb8,0xbc,0x1e,
  0xb3,0x82,0xf4,0xda,0x88,0x5b,0x2b,0x99,
  0x0f,0xac,0x1e,0x6f,0xe4,0x5c,0xca,0xea,
  0xd2,0x4b,0x6e,0xd0,0x3c,0xcb,0x8b,0x30,
  // Fill rest with zeros
};

inline static uint64_t read64(const void* memPtr) {
  uint64_t val;
  memcpy(&val, memPtr, sizeof(val));
  return val;
}

inline static uint64_t rol64(uint64_t x, int r) {
  return (x << r) | (x >> (64 - r));
}

inline static uint64_t avalanche(uint64_t h64) {
  h64 ^= h64 >> 37;
  h64 *= XXH_AVALANCHE;
  h64 ^= h64 >> 32;
  return h64;
}

inline static uint64_t mix16B(const void* data, const void* key, uint64_t seed) {
  const uint64_t* d = (const uint64_t*)data;
  const uint64_t* k = (const uint64_t*)key;
  uint64_t input_lo = read64(d) ^ (read64(k) + seed);
  uint64_t input_hi = read64(d + 1) ^ (read64(k + 1) - seed);
  return rol64(input_lo, 31) * XXH_PRIME64_1 + rol64(input_hi, 27) * XXH_PRIME64_2;
}

uint64_t XXH3_64bits(const void* input, uint32_t length, uint64_t seed) {
  const uint8_t* data = (const uint8_t*)input;

  if (length <= 16) {
    if (length >= 9) {
      uint64_t input_lo = read64(data);
      uint64_t input_hi = read64(data + length - 8);
      uint64_t bitflip = read64(XXH3_kSecret + 24) ^ read64(XXH3_kSecret + 32);
      uint64_t keyed = input_lo ^ input_hi ^ bitflip;
      return avalanche(rol64(keyed, 37) * XXH_PRIME64_1);
    }
    if (length >= 4) {
      uint32_t input1, input2;
      memcpy(&input1, data, 4);
      memcpy(&input2, data + length - 4, 4);
      uint64_t combined = ((uint64_t)input1 << 32) | input2;
      uint64_t keyed = combined ^ (read64(XXH3_kSecret + 8) + seed);
      return avalanche(rol64(keyed, 11) * XXH_PRIME64_1);
    }
    if (length > 0) {
      uint8_t c1 = data[0];
      uint8_t c2 = data[length >> 1];
      uint8_t c3 = data[length - 1];
      uint32_t combined = ((uint32_t)c1 << 16) | ((uint32_t)c2 << 8) | c3;
      uint64_t keyed = combined ^ (read64(XXH3_kSecret) + seed);
      return avalanche(keyed * XXH_PRIME64_1);
    }
    return avalanche(read64(XXH3_kSecret + 56) ^ read64(XXH3_kSecret + 64));
  }

  if (length <= 128) {
    uint64_t acc = length * XXH_PRIME64_1;
    size_t i;
    for (i = 0; i < length; i += 16) {
      acc += mix16B(data + i, XXH3_kSecret + i, seed);
    }
    return avalanche(acc);
  }

  // Truncate: for minimal version, just hash the first and last 64 bytes
  uint64_t acc = length * XXH_PRIME64_1;
  acc += mix16B(data, XXH3_kSecret, seed);
  acc += mix16B(data + length - 16, XXH3_kSecret + 16, seed);
  return avalanche(acc);
}

// 32-bit primes
const unsigned long long XXH_PRIME32_1 = 0x9E3779B1u;
const unsigned long long XXH_PRIME32_2 = 0x85EBCA77u;
const unsigned long long XXH_PRIME32_3 = 0xC2B2AE3Du;
const unsigned long long XXH_PRIME32_4 = 0x27D4EB2Fu;
const unsigned long long XXH_PRIME32_5 = 0x165667B1u;

static inline uint32_t rotl32(uint32_t x, int r) {
  return (x << r) | (x >> (32 - r));
}

uint32_t XXH32(const void* input, uint32_t length, uint32_t seed) {
  const uint8_t* p = (const uint8_t*)input;
  const uint8_t* end = p + length;
  uint32_t h32;

  if (length >= 16) {
    const uint8_t* const limit = end - 16;
    uint32_t v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
    uint32_t v2 = seed + XXH_PRIME32_2;
    uint32_t v3 = seed + 0;
    uint32_t v4 = seed - XXH_PRIME32_1;

    do {
      v1 += (*(const uint32_t*)p) * XXH_PRIME32_2;
      v1 = rotl32(v1, 13);
      v1 *= XXH_PRIME32_1;
      p += 4;

      v2 += (*(const uint32_t*)p) * XXH_PRIME32_2;
      v2 = rotl32(v2, 13);
      v2 *= XXH_PRIME32_1;
      p += 4;

      v3 += (*(const uint32_t*)p) * XXH_PRIME32_2;
      v3 = rotl32(v3, 13);
      v3 *= XXH_PRIME32_1;
      p += 4;

      v4 += (*(const uint32_t*)p) * XXH_PRIME32_2;
      v4 = rotl32(v4, 13);
      v4 *= XXH_PRIME32_1;
      p += 4;
    } while (p <= limit);

    h32 = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
  } else {
    h32 = seed + XXH_PRIME32_5;
  }

  h32 += (uint32_t)length;

  while (p + 4 <= end) {
    h32 += (*(const uint32_t*)p) * XXH_PRIME32_3;
    h32 = rotl32(h32, 17) * XXH_PRIME32_4;
    p += 4;
  }

  while (p < end) {
    h32 += (*p) * XXH_PRIME32_5;
    h32 = rotl32(h32, 11) * XXH_PRIME32_1;
    p++;
  }

  // Final avalanche
  h32 ^= h32 >> 15;
  h32 *= XXH_PRIME32_2;
  h32 ^= h32 >> 13;
  h32 *= XXH_PRIME32_3;
  h32 ^= h32 >> 16;

  return h32;
}
