#ifndef MIH_SRC_GLOBAL_H
#define MIH_SRC_GLOBAL_H
#pragma once

#include <stdint.h>
#include <assert.h>
#include <cstdio>
#include <cmath>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &); \
    TypeName &operator =(const TypeName &)

#define pop_count __builtin_popcountll

namespace bitops {
inline void Split(const uint8_t *data, int dim, int num_chunks,
                  uint32_t *chunks) {
  int bits = ceil(dim * 8.0 / num_chunks);
  assert(bits <= 32);

  int remainder_index = dim * 8 - num_chunks * (bits - 1);
  uint32_t temp = 0, mask = (1llu << bits) - 1;
  int index = 0, bits_to_shift = 0;
  for (int i = 0; i < num_chunks; ++ i) {
    while (bits_to_shift < bits) {
      temp |= data[index ++] << bits_to_shift;
      bits_to_shift += 8;
    }
    chunks[i] = temp & mask;
    temp >>= bits;
    bits_to_shift -= bits;

    if (i + 1 == remainder_index) {
      -- bits;
      mask = (1llu << bits) - 1;
    }
  }
}

inline uint32_t NextPermutaion(uint32_t x) {
  uint32_t lower_bits = x & -x;
  uint32_t higher_bits = x + lower_bits;
  uint32_t new_lower_bits = ((x ^ higher_bits) / lower_bits) >> 2;
  return higher_bits | new_lower_bits;
}

inline int Match(const uint8_t *u, const uint8_t *v, int dim) {
  int hamming_dis = 0;
  uint64_t hold_u, hold_v;
  for (int i = 0; i < dim; i += 8) {
    hold_u = hold_v = 0;
    for (int j = 0; j < 8 && i + j < dim; ++ j) {
      hold_u = (hold_u << 8) | u[i + j];
      hold_v = (hold_v << 8) | v[i + j];
    }
    hamming_dis += pop_count(hold_u ^ hold_v);
  }
  return hamming_dis;
}
} /* bitops */

#endif
