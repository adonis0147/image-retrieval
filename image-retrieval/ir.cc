#include "lock_free_queue.h"
#include "global.h"
#include "mih.h"
#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#define FILE_NAME_MAX_SIZE 1024
#define PORT 5000
#define BACKLOG 32
#define REQUEST_BUFFER_SIZE 1024

void Help();

void Help() {
  fprintf(stderr, "Invalid argument!\n");
  fprintf(stderr, "usage:\n \
 -s, --train-file\t\t train file\n\
  -t, --test-file\t\t test file\n\
  -d, --dimension\t\t data dimension\n\
  -b, --bucket-groups-num\t the number of bucket groups\n");
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
  int dim = 0, num_bucket_groups = 0;

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
      default:
        break;
    }
  }

  if (!strlen(train_file) || !strlen(test_file) ||
      !dim || !num_bucket_groups) {
    Help();
    return EXIT_FAILURE;
  }

  printf("train file:\t%s\n", train_file);
  printf("test file:\t%s\n", test_file);
  printf("data dimension:\t\t%d\n", dim);
  printf("bucket groups:\t\t%d\n", num_bucket_groups);

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

  printf("\n");
  printf("Initializing ...\n");

  MIH mih(train_data, num_train_data, dim, num_bucket_groups);

  printf("Done!\n");

  return EXIT_SUCCESS;
}
