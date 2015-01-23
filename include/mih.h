#ifndef IR_INCLUDE_MIH_H
#define IR_INCLUDE_MIH_H
#pragma once

#include "global.h"
#include "bucket_group.h"
#include "result.h"
#include <stdint.h>

class MIH {
  public:
    MIH(const uint8_t *data, int num_data, int dim, size_t num_bucket_groups)
        : data_(data), num_data_(num_data), dim_(dim),
          num_bucket_groups_(num_bucket_groups),
          bucket_groups_(new BucketGroup *[num_bucket_groups]),
          bucket_bits_(ceil(dim * 8 / (num_bucket_groups + 1e-8))) {
      remainder_index_ = dim * 8 - num_bucket_groups * (bucket_bits_ - 1);
      for (size_t i = 0; i < num_bucket_groups_; ++ i) {
        if (i < remainder_index_)
          bucket_groups_[i] = new BucketGroup(bucket_bits_);
        else
          bucket_groups_[i] = new BucketGroup(bucket_bits_ - 1);
      }

      Build();
    }

    ~MIH() {
      for (size_t i = 0; i < num_bucket_groups_; ++ i) {
        delete bucket_groups_[i];
        bucket_groups_[i] = NULL;
      }
      delete[] bucket_groups_;
      bucket_groups_ = NULL;
      num_data_ = num_bucket_groups_ = bucket_bits_ = remainder_index_ = 0;
    }

    size_t num_bucket_groups() const { return num_bucket_groups_; }
    const BucketGroup * const *bucket_groups() const { return bucket_groups_; }

    void Query(const uint8_t *query, int search_radius, Result &result);

  private:
    DISALLOW_COPY_AND_ASSIGN(MIH);
    void Build();

    const uint8_t *data_;
    int num_data_;
    int dim_;
    size_t num_bucket_groups_;
    BucketGroup **bucket_groups_;
    int bucket_bits_;
    size_t remainder_index_;
};

#endif
