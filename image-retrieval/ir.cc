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
#define BUFFER_MAX_SIZE 1024

struct Request {
  int num_neighbors;
  int search_radius;
  int dim;
  uint8_t *data;
};

struct RequestPackage {
  struct bufferevent *bev;
  struct Request *request;
};

struct Wrapper {
  struct event_base *base;
  LockFreeQueue<RequestPackage> *queue;
};

void Help();
void StartServer(LockFreeQueue<RequestPackage> &queue);
void DoAccept(evutil_socket_t listener, short event, void *arg);
void HandleReadEvent(struct bufferevent *bev, void *arg);
void HandleErrorEvent(struct bufferevent *bev, short error, void *arg);
struct Request *CreateRequest(char *buffer, int size);
void FreeRequestPackage(struct RequestPackage &package);

void Help() {
  fprintf(stderr, "Invalid argument!\n");
  fprintf(stderr, "usage:\n \
 -s, --train-file\t\t train file\n\
  -t, --test-file\t\t test file\n\
  -d, --dimension\t\t data dimension\n\
  -b, --bucket-groups-num\t the number of bucket groups\n");
}

void StartServer(LockFreeQueue<RequestPackage> &queue) {
  struct sockaddr_in server_sockaddr = {
    .sin_family = AF_INET,
    .sin_port = htons(PORT),
  };
  evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);
  evutil_make_listen_socket_reuseable(listener);

  if (bind(listener, (struct sockaddr *)&server_sockaddr,
           sizeof server_sockaddr) || listen(listener, BACKLOG) < 0) {
    close(listener);
    return;
  }

  printf("\nListening: %s:%d\n", inet_ntoa(server_sockaddr.sin_addr),
         ntohs(server_sockaddr.sin_port));

  event_base *base = event_base_new();
  struct Wrapper wrapper = {
    .base = base,
    .queue = &queue,
  };
  event *listen_event = event_new(base, listener, EV_READ | EV_PERSIST,
                                  DoAccept,
                                  reinterpret_cast<void *>(&wrapper));
  event_add(listen_event, NULL);
  event_base_dispatch(base);
}

void DoAccept(evutil_socket_t listener, short event, void *arg) {
  struct Wrapper *wrapper = reinterpret_cast<Wrapper *>(arg);
  event_base *base = wrapper->base;
  LockFreeQueue<RequestPackage> *queue = wrapper->queue;
  struct sockaddr_in client_sockaddr;
  socklen_t socklen;
  int fd = accept(listener, (struct sockaddr *)&client_sockaddr, &socklen);

  printf("Connection: %s:%d\n", inet_ntoa(client_sockaddr.sin_addr),
         ntohs(client_sockaddr.sin_port));

  if (fd < 0) {
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    evutil_make_socket_nonblocking(fd);
    struct bufferevent *bev = bufferevent_socket_new(base, fd,
                                                     BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, HandleReadEvent, NULL, HandleErrorEvent,
                      reinterpret_cast<void *>(queue));
    bufferevent_enable(bev, EV_READ|EV_WRITE);
  }
}

void HandleReadEvent(struct bufferevent *bev, void *arg) {
  LockFreeQueue<RequestPackage> *queue =
      reinterpret_cast<LockFreeQueue<RequestPackage> *>(arg);
  char buffer[BUFFER_MAX_SIZE] = {};
  int size = bufferevent_read(bev, buffer, BUFFER_MAX_SIZE);
  struct Request *request = CreateRequest(buffer, size);

  if (request == NULL) {
    bufferevent_free(bev);
  } else {
    struct RequestPackage package = {
      .bev = bev,
      .request = request,
    };
  }
}

struct Request *CreateRequest(char *buffer, int size) {
  if (size <= 0) return NULL;

  struct Request *request = new Request();

  printf("%d\n", size);

  int offset = 0;
  memcpy((char *)&request->num_neighbors, buffer + offset, sizeof(int));

  offset += sizeof(int);
  memcpy((char *)&request->search_radius, buffer + offset, sizeof(int));

  offset += sizeof(int);
  memcpy((char *)&request->dim, buffer + offset, sizeof(int));

  request->data = new uint8_t[request->dim];
  offset += sizeof(int);
  memcpy((char *)request->data, buffer + offset,
         sizeof(uint8_t) * request->dim);

  return request;
}

void HandleErrorEvent(struct bufferevent *bev, short error, void *arg) {
  bufferevent_free(bev);
}

void FreeRequestPackage(struct RequestPackage &package) {
  delete[] package.request->data;
  package.request->data = NULL;
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

  LockFreeQueue<RequestPackage> queue(0xffff);

  StartServer(queue);

  return EXIT_SUCCESS;
}
