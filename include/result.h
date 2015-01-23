#ifndef IR_INCLUDE_RESULT_H
#define IR_INCLUDE_RESULT_H
#pragma once

#include "global.h"
#include <cstdio>
#include <vector>

class Result {
  public:
    Result(size_t max_num, size_t max_distance, size_t num_data)
        : max_num_(max_num), max_distance_(max_distance), num_data_(num_data),
          is_duplicate_(new bool[num_data]()),
          results_per_distance_(new std::vector<int>[max_distance + 1]) {
      for (size_t i = 0; i <= max_distance_; ++ i)
        results_per_distance_[i].reserve(max_num_);
    }

    ~Result() {
      delete[] is_duplicate_;
      is_duplicate_ = NULL;
      delete[] results_per_distance_;
      results_per_distance_ = NULL;
      max_num_ = max_distance_ = num_data_ = 0;
    }

    size_t max_num() const { return max_num_; }
    size_t max_distance() const { return max_distance_; }

    bool IsDuplicateItem(size_t id) const;
    void AddOrIgnore(size_t id, size_t distance);
    int GetResultsNumAtDistance(size_t distance) const;
    int GetSizeOfItems() const;
    std::vector<int> Get() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Result);

    size_t max_num_;
    size_t max_distance_;
    size_t num_data_;
    bool *is_duplicate_;
    std::vector<int> *results_per_distance_;
};

inline bool Result::IsDuplicateItem(size_t id) const {
  if (id >= num_data_) return true;
  return is_duplicate_[id];
}

inline void Result::AddOrIgnore(size_t id, size_t distance) {
  if (id >= num_data_) return;
  if (!is_duplicate_[id] && distance <= max_distance_) {
    std::vector<int> &results = results_per_distance_[distance];
    if (results.size() < max_num_)
      results.push_back(id);
  }
  is_duplicate_[id] = true;
}

inline int Result::GetResultsNumAtDistance(size_t distance) const {
  if (distance > max_distance_) return 0;
  return results_per_distance_[distance].size();
}

inline int Result::GetSizeOfItems() const {
  int total_num = 0;
  for (size_t i = 0; i < num_data_; ++ i)
    total_num += is_duplicate_[i];
  return total_num;
}

inline std::vector<int> Result::Get() const {
  std::vector<int> results;
  results.reserve(max_num_);
  for (size_t i = 0; i <= max_distance_; ++ i) {
    for (size_t j = 0; j < results_per_distance_[i].size(); ++ j) {
      results.push_back(results_per_distance_[i][j]);
      if (results.size() >= max_num_) return results;
    }
  }
  return results;
}

#endif
