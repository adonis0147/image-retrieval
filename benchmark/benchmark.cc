#include "global.h"
#include "mih.h"
#include "result.h"
#include <getopt.h>
#include <stdint.h>
#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>
#define FILE_NAME_MAX_SIZE 1024


void Help();
int CalCorrectItems(const uint8_t *query, const uint8_t *train_data,
                    int num_train_data, const std::vector<int> &mih_results,
                    const std::vector<int> &linear_results);

void Help() {
  fprintf(stderr, "Invalid argument!\n");
  fprintf(stderr, "usage:\n \
 -s, --train-file\t\t train file\n\
  -t, --test-file\t\t test file\n\
  -d, --dimension\t\t data dimension\n\
  -b, --bucket-groups-num\t the number of bucket groups\n\
  -r, --search-radius\t\t search radius\n\
  -k, --neighbors-num\t\t the number of neighbors to be searched\n");
}

int CalCorrectItems(const uint8_t *query, const uint8_t *train_data, int dim,
                    const std::vector<int> &mih_results,
                    const std::vector<int> &linear_results) {
  std::set<int> correct_set;
  int max_dis_in_correct_set = 0;
  int num_correct_items = 0;

  for (size_t i = 0; i < linear_results.size(); ++ i) {
    int id = linear_results[i];
    const uint8_t *item = train_data + dim * id;
    int hamming_distance = bitops::Match(query, item, dim);
    correct_set.insert(id);
    max_dis_in_correct_set = std::max(max_dis_in_correct_set, hamming_distance);
  }

  for (size_t i = 0; i < mih_results.size(); ++ i) {
    int id = mih_results[i];
    if (correct_set.find(id) != correct_set.end()) ++ num_correct_items;
    else {
      const uint8_t *item = train_data + dim * id;
      int hamming_distance = bitops::Match(query, item, dim);
      if (hamming_distance <= max_dis_in_correct_set) ++ num_correct_items;
    }
  }

  return num_correct_items;
}

int main(int argc, char *argv[]) {
  struct option long_options[] = {
    {"train-file", required_argument, NULL, 's'},
    {"test-file", required_argument, NULL, 't'},
    {"dimension", required_argument, NULL, 'd'},
    {"bucket-groups-num", required_argument, NULL, 'b'},
    {"search-radius", required_argument, NULL, 'r'},
    {"neighbors-num", required_argument, NULL, 'k'},
    {NULL, no_argument, NULL, 0},
  };

  char train_file[FILE_NAME_MAX_SIZE] = {}, test_file[FILE_NAME_MAX_SIZE] = {};
  int dim = 0, num_bucket_groups = 0, search_radius = -1, num_neighbors = 0;

  int c;
  while ((c = getopt_long(argc, argv, "s:t:d:b:r:k:",
                          long_options, NULL)) != EOF) {
    switch(c) {
      case 's':
        memcpy(train_file, optarg, strlen(optarg));
        break;
      case 't':
        memcpy(test_file, optarg, strlen(optarg));
        break;
      case 'd':
        sscanf(optarg, "%d", &dim);
        break;
      case 'b':
        sscanf(optarg, "%d", &num_bucket_groups);
        break;
      case 'r':
        sscanf(optarg, "%d", &search_radius);
        break;
      case 'k':
        sscanf(optarg, "%d", &num_neighbors);
      default:
        break;
    }
  }

  if (!strlen(train_file) || !strlen(test_file) ||
      !dim || !num_bucket_groups || search_radius < 0 || !num_neighbors) {
    Help();
    return EXIT_FAILURE;
  }

  printf("train file:\t%s\n", train_file);
  printf("test file:\t%s\n", test_file);
  printf("data dimension:\t\t%d\n", dim);
  printf("bucket groups:\t\t%d\n", num_bucket_groups);
  printf("search radius:\t\t%d\n", search_radius);
  printf("neighbors:\t\t%d\n", num_neighbors);

///////////////////////////////////////////////////////////////////////////////

  uint8_t *train_data = NULL, *test_data = NULL;
  int num_train_data, num_test_data;

  if (!Read(train_file, dim, train_data, num_train_data) ||
      !Read(test_file, dim, test_data, num_test_data)) {
    delete[] train_data;
    delete[] test_data;
    train_data = test_data = NULL;
    return EXIT_FAILURE;
  }

  printf("\n");
  printf("the number of train data:\t %d\n", num_train_data);
  printf("the number of test data:\t %d\n", num_test_data);

  clock_t begin, end, mih_time = 0, linear_time = 0;
  int num_correct_items = 0, num_mih_items = 0, num_linear_items = 0;
  int num_candidates = 0;

  MIH mih(train_data, num_train_data, dim, num_bucket_groups);

  printf("\n");

  for (int i = 0; i < num_test_data; ++ i) {
    printf("\r%d / %d (%.2lf%%)", i + 1, num_test_data,
           (i + 1) * 100.0 / num_test_data);
    fflush(stdout);

    const uint8_t *query = test_data + dim * i;
    int max_dis = dim * 8;

    Result result(num_neighbors, max_dis, num_train_data);
    begin = clock();
    mih.Query(query, search_radius, result);
    end = clock();
    mih_time += end - begin;
    num_candidates += result.GetSizeOfItems();
    const std::vector<int> mih_results = result.Get();

    begin = clock();
    const std::vector<int> linear_results = LinearQuery(query, num_neighbors,
                                                        max_dis, train_data,
                                                        num_train_data, dim);
    end = clock();
    linear_time += end - begin;

    num_mih_items += mih_results.size();
    num_linear_items += linear_results.size();
    num_correct_items += CalCorrectItems(query, train_data, dim,
                                         mih_results, linear_results);
  }

  printf("\n\n");
  printf("candidates ratio:\t\t %lf%% (%lf / %d)\n",
         num_candidates * 100.0 / num_test_data / num_train_data,
         num_candidates / static_cast<double>(num_test_data), num_train_data);
  printf("acceleration ratio:\t\t %lf%% (%lf / %lf)\n",
         linear_time / static_cast<double>(mih_time),
         linear_time / static_cast<double>(num_test_data) / CLOCKS_PER_SEC,
         mih_time / static_cast<double>(num_test_data) / CLOCKS_PER_SEC);
  printf("average correct items:\t\t %lf\n",
         num_correct_items / static_cast<double>(num_test_data));
  printf("average precision:\t\t %lf%%\n",
         num_correct_items * 100.0 / num_mih_items);

  delete[] train_data;
  delete[] test_data;
  train_data = test_data = NULL;
  return EXIT_SUCCESS;
}
