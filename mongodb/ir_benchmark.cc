#include "global.h"
#include "bson.h"
#include "mongoc.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#define MESSAGE_SIZE 1024

void Send(int fd, const uint8_t *test_data, int test_id,
          int num_neighbors, int search_radius, int dim);
std::vector<int> GetItems(char *buffer, size_t buffer_size);

void Send(int fd, const uint8_t *test_data, int test_id,
          int num_neighbors, int search_radius, int dim) {
  char msg[MESSAGE_SIZE] = {};
  int offset = 0;
  const uint8_t *query = test_data + dim * test_id;

  memcpy(msg + offset, (char *)&num_neighbors, sizeof(int));
  offset += sizeof(int);
  memcpy(msg + offset, (char *)&search_radius, sizeof(int));
  offset += sizeof(int);
  memcpy(msg + offset, (char *)&dim, sizeof(int));
  offset += sizeof(int);
  memcpy(msg + offset, (char *)query, sizeof(uint8_t) * dim);

  write(fd, msg, offset + sizeof(uint8_t) * dim);
}

std::vector<int> GetItems(char *buffer, size_t buffer_size) {
  int num_items = 0;
  int offset = 0;
  int id;

  memcpy((char *)&num_items, buffer, sizeof(int));

  if (sizeof(int) * (num_items + 1) != buffer_size) return std::vector<int>();

  std::vector<int> items;
  items.reserve(num_items);
  for (int i = 0; i < num_items; ++ i) {
    offset += sizeof(int);
    memcpy((char *)&id, buffer + offset, sizeof(int));
    items.push_back(id);
  }
  return items;
}

int main() {
  const char *file = "../data/features_jd_test2.dat";
  uint8_t *test_data = NULL;
  int num_test_data;
  int dim = 512;

  Read(file, dim, test_data, num_test_data);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in host_sockaddr = {
    .sin_family = AF_INET,
    .sin_port = htons(5000),
  };

  connect(fd, (struct sockaddr *)&host_sockaddr, sizeof host_sockaddr);

  char buffer[MESSAGE_SIZE];
  int buffer_size;

  mongoc_init();
  mongoc_client_t *client = mongoc_client_new("mongodb://localhost:27017/");
  mongoc_collection_t *collection = mongoc_client_get_collection(client, "jd", "jd_info_train");

  bson_t *fields = bson_new();
  BSON_APPEND_INT32(fields, "_id", 0);
  const bson_t *doc;
  char index[10] = {};

  printf("\n");
  for (int i = 0; i < num_test_data; ++ i) {
    printf("\r%d / %d", i, num_test_data);
    fflush(stdout);

    Send(fd, test_data, i, 10, 1, 128);
    buffer_size = read(fd, buffer, MESSAGE_SIZE);
    std::vector<int> items = GetItems(buffer, buffer_size);
    //std::sort(items.begin(), items.end());
    //printf("%d\t", i + 1);
    //for (size_t j = 0; j < items.size(); ++ j) {
      //printf("%d ", items[j]);
    //}
    //printf("\n");
    bson_t *query = bson_new();
    bson_t *in = bson_new();
    bson_t *array = bson_new();
    for (size_t j = 0; j < items.size(); ++ j) {
      sprintf(index, "%lu", j);
      BSON_APPEND_INT32(array, index, items[j]);
    }
    BSON_APPEND_ARRAY(in, "$in", array);
    BSON_APPEND_DOCUMENT(query, "id", in);
    mongoc_cursor_t *cursor =
        mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0,
                              query, fields, NULL);
    while (mongoc_cursor_next(cursor, &doc));
    mongoc_cursor_destroy(cursor);
    bson_destroy(array);
    bson_destroy(in);
    bson_destroy(query);
  }

  bson_destroy(fields);
  mongoc_collection_destroy(collection);
  mongoc_client_destroy(client);
  delete[] test_data;
  test_data = NULL;

  return 0;
}

