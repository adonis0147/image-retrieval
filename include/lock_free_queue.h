#ifndef IR_INCLUDE_LOCK_FREE_QUEUE_H
#define IR_INCLUDE_LOCK_FREE_QUEUE_H
#pragma once

#include "global.h"
#define CAS(ptr, old_val, new_val) \
    __sync_bool_compare_and_swap(ptr, old_val, new_val)


template<typename T>
class LockFreeQueue {
 public:
  explicit LockFreeQueue(size_t size)
      : buffer_(new T[size]()), size_(size),
        read_count_(0), write_count_(0), last_commit_count_(0) {}

  ~LockFreeQueue() {
    delete[] buffer_;
    buffer_ = NULL;
    read_count_ = write_count_ = last_commit_count_ = 0;
  }

  bool push(const T &element) {
    unsigned int current_read_count;
    unsigned int current_write_count;

    do {
      current_read_count = read_count_;
      current_write_count = write_count_;

      if (Index(current_write_count + 1) == Index(current_read_count))
        return false;
    } while (!CAS(&write_count_, current_write_count,
                  current_write_count + 1));

    buffer_[Index(current_write_count)] = element;

    __sync_synchronize();

    ++ last_commit_count_;
    return true;
  }

  bool pop(T &element) {
    unsigned int current_read_count;
    unsigned int current_last_commit_count;

    while (true) {
      current_read_count = read_count_;
      current_last_commit_count = last_commit_count_;

      if (Index(current_read_count) == Index(current_last_commit_count))
        return false;

      if (CAS(&read_count_, current_read_count, current_read_count + 1)) {
        element = buffer_[Index(current_read_count)];
        return true;
      }
    }
  }

 private:
  size_t Index(unsigned int count) {
    return count % size_;
  }

  T *buffer_;
  const size_t size_;
  volatile unsigned int read_count_;
  volatile unsigned int write_count_;
  volatile unsigned int last_commit_count_;
};

#endif
