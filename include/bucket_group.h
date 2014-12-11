#ifndef MIH_SRC_BUCKET_GROUP
#define MIH_SRC_BUCKET_GROUP
#pragma once

#include "global.h"
#include <vector>

class BucketGroup {
  public:
    explicit BucketGroup(int bucket_bits)
        : bucket_bits_(bucket_bits), num_buckets_(1u << bucket_bits),
          buckets_(new(std::nothrow) std::vector<int>[1u << bucket_bits]) {
      if (buckets_ == NULL) num_buckets_ = 0;
    }

    ~BucketGroup() {
      delete[] buckets_;
      buckets_ = NULL;
      num_buckets_ = bucket_bits_= 0;
    }

    int bucket_bits() const { return bucket_bits_; }
    size_t num_buckets() const { return num_buckets_; }
    const std::vector<int> *buckets() const { return buckets_; }

    void Insert(size_t bucket_id, int value);

  private:
    DISALLOW_COPY_AND_ASSIGN(BucketGroup);

    int bucket_bits_;
    size_t num_buckets_;
    std::vector<int> *buckets_;
};

inline void BucketGroup::Insert(size_t bucket_id, int value) {
  if (buckets_ == NULL || bucket_id > num_buckets_) return;
  buckets_[bucket_id].push_back(value);
}

#endif
