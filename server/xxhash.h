#pragma once

#include <stdint.h>

uint32_t XXH32(const void* input, uint32_t length, uint32_t seed);
uint64_t XXH3_64bits(const void* input, uint32_t len, uint64_t seed);
