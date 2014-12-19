#include "mih.h"
#include "global.h"
#include "result.h"
#include <cmath>
#include <vector>

void MIH::Build() {
  uint32_t *chunks = new(std::nothrow) uint32_t[num_bucket_groups_];
  if (chunks == NULL) return;

  for (int i = 0; i < num_data_; ++ i) {
    const uint8_t *item = data_ + dim_ * i;
    bitops::Split(item, dim_, num_bucket_groups_, chunks);
    for (size_t j = 0; j < num_bucket_groups_; ++ j) {
      bucket_groups_[j]->Insert(chunks[j], i);
    }
  }

  delete[] chunks;
  chunks = NULL;
}

void MIH::Query(const uint8_t *query, int search_radius, Result &result) {
  uint32_t *chunks = new(std::nothrow) uint32_t[num_bucket_groups_];
  if (chunks == NULL) return;

  bitops::Split(query, dim_, num_bucket_groups_, chunks);
  size_t total_num = 0;
  uint32_t permutation_mask;

  for (int d = 0; d <= search_radius; ++ d) {
    for (size_t i = 0; i < num_bucket_groups_; ++ i) {
      const std::vector<int> *buckets = bucket_groups_[i]->buckets();

      if (i < remainder_index_)
        permutation_mask = ~((1 << bucket_bits_) - 1);
      else
        permutation_mask = ~((1 << (bucket_bits_ - 1)) - 1);
      uint32_t mask = (1u << d) - 1;

      while ((mask & permutation_mask) == 0) {
        const std::vector<int> &bucket = buckets[chunks[i] ^ mask];
        for (size_t j = 0; j < bucket.size(); ++ j) {
          int id = bucket[j];
          if (!result.IsDuplicateItem(id)) {
            int hamming_distance = bitops::Match(data_ + dim_ * id, query, dim_);
            result.AddOrIgnore(id, hamming_distance);
          }
        }
        if (!d) break;
        else mask = bitops::NextPermutaion(mask);
      }

      total_num += result.GetResultsNumAtDistance(d * num_bucket_groups_ + i);
      if (total_num >= result.max_num()) goto out;
    }
  }

out:
  delete[] chunks;
  chunks = NULL;
}
