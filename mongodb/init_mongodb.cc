#include "bson.h"
#include "mongoc.h"
#include <getopt.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#define ARGUMENT_SIZE 1024

void Help();

void Help() {
  fprintf(stderr, "Invalid argument!\n");
  fprintf(stderr, "usage:\n \
 -f, --file\t\t\t data file\n\
  -d, --database\t\t database in mongodb\n\
  -c, --collection\t\t collection in database\n");
}

int main(int argc, char *argv[]) {
  struct option long_options[] = {
    {"file", required_argument, NULL, 'f'},
    {"database", required_argument, NULL, 'd'},
    {"collection", required_argument, NULL, 'c'},
  };

  char file[ARGUMENT_SIZE] = {}, database_name[ARGUMENT_SIZE] = {},
       collection_name[ARGUMENT_SIZE] = {};

  int c;
  while ((c = getopt_long(argc, argv, "f:d:c:", long_options, NULL)) != EOF) {
    switch(c) {
      case 'f':
        snprintf(file, ARGUMENT_SIZE, "%s", optarg);
        break;
      case 'd':
        snprintf(database_name, ARGUMENT_SIZE, "%s", optarg);
        break;
      case 'c':
        snprintf(collection_name, ARGUMENT_SIZE, "%s", optarg);
        break;
      default:
        break;
    }
  }

  if (!strlen(file) || !strlen(database_name) || !strlen(collection_name)) {
    Help();
    return -EXIT_FAILURE;
  }

  printf("data file:\t\t%s\n", file);
  printf("database:\t\t%s\n", database_name);
  printf("collection:\t\t%s\n", collection_name);
  printf("\n");

///////////////////////////////////////////////////////////////////////////////

  std::ifstream fin(file);
  std::string line;
  std::string info;
  int index = 0;

  mongoc_init();
  mongoc_client_t *client = mongoc_client_new("mongodb://localhost:27017/");
  mongoc_collection_t *collection =
      mongoc_client_get_collection(client, database_name, collection_name);
  bson_error_t error;

  while (std::getline(fin, line)) {
    std::istringstream sin(line);
    sin >> info;
    bson_t *doc = bson_new();
    BSON_APPEND_INT32(doc, "id", index ++);
    BSON_APPEND_UTF8(doc, "path", info.c_str());
    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE,
                                  doc, NULL, &error)) {
      printf("%s\n", error.message);
    }
    bson_destroy(doc);
  }

  printf("\nThe number of items inserted: %d\n", index);

  mongoc_collection_destroy(collection);
  mongoc_client_destroy(client);

  return EXIT_SUCCESS;
}
