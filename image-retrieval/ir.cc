#include "lock_free_queue.h"
#include "global.h"
#include "mih.h"
#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#define FILE_NAME_MAX_SIZE 1024
#define BACKLOG 32
#define BUFFER_MAX_SIZE 1024

struct Request {
  int num_neighbors;
  int search_radius;
  int dim;
  uint8_t *data;
};

struct RequestPackage {
  int fd;
  struct Request *request;
};

struct Wrapper {
  struct event_base *base;
  LockFreeQueue<RequestPackage> *queue;
  size_t protocol_size;
};

struct Worker {
  pthread_t id;
  LockFreeQueue<RequestPackage> *queue;
  MIH *mih;
};

void Help();
void StartWorkers(int num_threads, LockFreeQueue<RequestPackage> &queue,
                  MIH *mih);
void *DoWork(void *arg);
void StartServer(int port, size_t protocol_size,
                 LockFreeQueue<RequestPackage> &queue);
void DoAccept(evutil_socket_t listener, short event, void *arg);
void HandleReadEvent(struct bufferevent *bev, void *arg);
void HandleErrorEvent(struct bufferevent *bev, short error, void *arg);
inline struct Request *CreateRequest(char *buffer, int size);
inline evbuffer *CreateResponse(const Result &result);
inline void FreeRequestPackage(struct RequestPackage &package);

void Help() {
  fprintf(stderr, "Invalid argument!\n");
  fprintf(stderr, "usage:\n \
 -s, --train-file\t\t train file\n\
  -t, --test-file\t\t test file\n\
  -d, --dimension\t\t data dimension\n\
  -b, --bucket-groups-num\t the number of bucket groups\n\
  -p, --port\t\t\t listen port\n\
  -n, --threads-num\t\t the number of threads\n");
}

void StartWorkers(int num_threads, LockFreeQueue<RequestPackage> &queue,
                  MIH &mih) {
  Worker *workers = new Worker[num_threads];
  for (int i = 0; i < num_threads; ++ i) {
    workers[i].queue = &queue;
    workers[i].mih = &mih;
  }

  for (int i = 0; i < num_threads; ++ i) {
    pthread_create(&workers[i].id, NULL, DoWork,
                   reinterpret_cast<void *>(&workers[i]));
  }
}

void *DoWork(void *arg) {
  Worker *worker = reinterpret_cast<Worker *>(arg);
  LockFreeQueue<RequestPackage> *queue = worker->queue;
  MIH *mih = worker->mih;
  RequestPackage package;

  while(true) {
    if (queue->pop(package)) {
      const Request *request = package.request;
      Result result(request->num_neighbors, request->dim * 8, mih->num_data());
      mih->Query(request->data, request->search_radius, result);
      evbuffer *response = CreateResponse(result);
      evbuffer_write(response, package.fd);
      evbuffer_free(response);
      FreeRequestPackage(package);
    } else {
      usleep(1000);
    }
  }
  return NULL;
}

inline evbuffer *CreateResponse(const Result &result) {
  std::vector<int> items = result.Get();
  int num_items = items.size();
  evbuffer *response = evbuffer_new();
  char buffer[BUFFER_MAX_SIZE] = {};

  memcpy(buffer, (char *)&num_items, sizeof(int));
  int buffer_size = sizeof(int);
  for (int i = 0; i < num_items; ++ i) {
    int id = items[i];
    memcpy(buffer + buffer_size, (char *)&id, sizeof(int));
    buffer_size += sizeof(int);
  }

  evbuffer_add(response, buffer, buffer_size);
  return response;
}

void StartServer(int port, size_t protocol_size,
                 LockFreeQueue<RequestPackage> &queue) {
  struct sockaddr_in server_sockaddr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
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
    .protocol_size = protocol_size,
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
  size_t protocol_size = wrapper->protocol_size;
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
    bufferevent_setwatermark(bev, EV_READ, 0, protocol_size);
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
      .fd = bufferevent_getfd(bev),
      .request = request,
    };

    while (!queue->push(package)) {
      sched_yield();
    }
  }
}

inline struct Request *CreateRequest(char *buffer, int size) {
  if (size <= 0) return NULL;

  struct Request *request = new Request();

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

inline void FreeRequestPackage(struct RequestPackage &package) {
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
    {"port", required_argument, NULL, 'p'},
    {"threads-num", required_argument, NULL, 'n'},
    {NULL, no_argument, NULL, 0},
  };

  char train_file[FILE_NAME_MAX_SIZE] = {}, test_file[FILE_NAME_MAX_SIZE] = {};
  int dim = 0, num_bucket_groups = 0;
  int port = 5000;
  int num_threads = 4;

  int c;
  while ((c = getopt_long(argc, argv, "s:t:d:b:r:k:p:n:",
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
      case 'p':
        sscanf(optarg, "%d", &port);
        break;
      case 'n':
        sscanf(optarg, "%d", &num_threads);
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
  printf("threads:\t\t%2d\n", num_threads);

///////////////////////////////////////////////////////////////////////////////

  uint8_t *train_data = NULL;
  int num_train_data;

  if (!Read(train_file, dim, train_data, num_train_data)) {
    delete[] train_data;
    train_data = NULL;
    return EXIT_FAILURE;
  }

  printf("\n");
  printf("the number of train data:\t %d\n", num_train_data);

  printf("\n");
  printf("Initializing ...\n");

  MIH mih(train_data, num_train_data, dim, num_bucket_groups);

  printf("Done!\n");

  LockFreeQueue<RequestPackage> queue(0xffff);
  size_t protocol_size = sizeof(int) * 3 + sizeof(uint8_t) * dim;

  StartWorkers(num_threads, queue, mih);
  StartServer(port, protocol_size, queue);

  return EXIT_SUCCESS;
}
