#include "global.h"
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <fstream>

bool Read(const char *file_name, const int dim,
          uint8_t *&samples, int &num_samples) {
  std::ifstream fin(file_name, std::ios::in | std::ios::binary);
  if (fin == NULL) goto error;

  num_samples = 0;
  fin.read((char *)&num_samples, sizeof(num_samples));
  samples = new(std::nothrow) uint8_t[num_samples * dim];
  if (samples == NULL) goto error;
  fin.read((char *)samples, sizeof(uint8_t) * (num_samples * dim));

  fin.close();
  return true;

error:
  fprintf(stderr, "Read data from file failed[%s]: %s\n",
          file_name, strerror(errno));
  fin.close();
  return false;
}

std::vector<int> LinearQuery(const uint8_t *query, int max_num, int max_dis,
                             const uint8_t *data, int num_data, int dim) {
  std::vector<int> results;
  std::vector<int> *results_per_distances = new(std::nothrow)
      std::vector<int> [max_dis + 1];
  if (results_per_distances == NULL) return results;

  for (int i = 0; i < num_data; ++ i) {
    const uint8_t *item = data + dim * i;
    int hamming_distance = bitops::Match(query, item, dim);
    if (hamming_distance <= max_dis)
      results_per_distances[hamming_distance].push_back(i);
  }

  for (int i = 0; i <= max_dis; ++ i) {
    for (size_t j = 0; j < results_per_distances[i].size(); ++ j) {
      results.push_back(results_per_distances[i][j]);
      if (results.size() >= static_cast<size_t>(max_num)) goto out;
    }
  }

out:
  delete[] results_per_distances;
  results_per_distances = NULL;

  return results;
}
