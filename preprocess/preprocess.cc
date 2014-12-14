#include <getopt.h>
#include <stdint.h>
#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>
#define FILE_NAME_MAX_SIZE 1024
#define EPS 1e-6

struct Partition {
  int sum;
  std::vector<int> columns;

  Partition(): sum(0) {}

  friend bool operator <(const Partition &a, const Partition &b) {
    if (a.sum != b.sum) return a.sum > b.sum;
    else return a.columns.size() > b.columns.size();
  }
};

void Help();
inline int GetLines(const char *file_name);
inline void Transform(const std::string &line, int dim,
                      std::vector<int> &features);
inline void FillChunks(const std::vector<int> &features,
                       uint8_t *chunks);
inline void LoadData(const char *file, int dim,
                     std::vector<std::vector<int> > &data);
inline void CalOnesPerColumn(const std::vector<std::vector<int> > &data,
                             int dim, int *num_ones);
void ProcessDataWithoutStrategy(const char *input_file,
                                const char *output_file, int dim);
void ProcessDataWithStrategy(const char *input_file,
                             const char *output_file,
                             const char *mapping_file, int dim,
                             int num_partitions);
std::vector<int> DivideAndGetMapping(std::vector<std::pair<int, int> > scores,
                                     int num_partitions);
inline void Divide(const std::vector<std::pair<int, int> > &scores,
                   int num_partitions,
                   std::priority_queue<Partition> &holder,
                   std::priority_queue<Partition> &remainder);
inline void WriteMappingFile(const char *mapping_file,
                             const std::vector<int> &mapping);

void Help() {
  fprintf(stderr, "Invalid argument!\n");
  fprintf(stderr, "usage:\n \
 -i, --input-file\t\t input file\n\
  -o, --output-file\t\t output file\n\
  -d, --dimension\t\t data dimension\n\
  -z, --optimize\t\t optimize the structure of output data\n\
                                 (optional)\n\
  -m, --mapping-file\t\t mapping file\n\
                                 (needed when enable optimization)\n\
  -n, --num-partitions\t\t the number of partitions\n\
                                 (needed when enable optimization)\n");
}

inline int GetLines(const char *file_name) {
  std::ifstream fin(file_name);
  std::string line;
  int num_total_lines = 0;

  while (std::getline(fin, line)) ++ num_total_lines;

  fin.close();
  return num_total_lines;
}

inline void Transform(const std::string &line, int dim,
                      std::vector<int> &features) {
  std::istringstream sin(line);
  double num;
  while (sin >> num) {
    if (fabs(num) < EPS) features.push_back(0);
    else features.push_back(1);
  }
  assert(dim == features.size());
}

inline void FillChunks(const std::vector<int> &features,
                       uint8_t *chunks) {
  for (size_t i = 0; i < features.size(); i += 8) {
    uint32_t num = 0;
    for (int j = 0; j < 8; ++ j) {
      //num <<= 1;
      //num |= features[i + j];
      num |= features[i + j] << j;
    }
    chunks[i / 8] = num;
  }
}

void ProcessDataWithoutStrategy(const char *input_file,
                                const char *output_file, int dim) {
  int num_total_lines = GetLines(input_file);
  std::ifstream fin(input_file);
  std::ofstream fout(output_file, std::ios::binary | std::ios::out);
  std::string line;
  int dim_over_8 = dim / 8;
  uint8_t *chunks = new uint8_t[dim_over_8];

  printf("\ntotal lines of file:\t%d\n", num_total_lines);

  fout.write((char *)&num_total_lines, sizeof(int));
  while(std::getline(fin, line)) {
    std::vector<int> features;
    features.reserve(dim);
    Transform(line, dim, features);
    FillChunks(features, chunks);
    fout.write((char *)chunks, sizeof(uint8_t) * dim_over_8);
  }

  fin.close();
  fout.close();
  delete[] chunks;
  chunks = NULL;
}

inline void LoadData(const char *file, int dim,
                     std::vector<std::vector<int> > &data) {
  std::ifstream fin(file);
  std::string line;

  while (std::getline(fin, line)) {
    std::istringstream sin(line);
    std::vector<int> features;
    features.reserve(dim);
    Transform(line, dim, features);
    data.push_back(features);
  }

  fin.close();
}

inline void CalOnesPerColumn(const std::vector<std::vector<int> > &data,
                             int dim, int *num_ones) {
  memset(num_ones, 0, sizeof(int) * dim);
  for (size_t i = 0; i < data.size(); ++ i) {
    const std::vector<int> &item = data[i];
    for (int j = 0; j < dim; ++ j)
      num_ones[j] += item[j];
  }
}

inline void Divide(const std::vector<std::pair<int, int> > &scores,
                   int num_partitions,
                   std::priority_queue<Partition> &holder,
                   std::priority_queue<Partition> &remainder) {
  for (int i = 0; i < num_partitions; ++ i)
    holder.push(Partition());

  int dim = scores.size();
  int index = 0;
  while (index < dim) {
    while (!holder.empty() && index < dim) {
      Partition partion = holder.top();
      holder.pop();
      partion.sum += scores[index].first;
      partion.columns.push_back(scores[index ++].second);
      remainder.push(partion);
    }
    std::swap(holder, remainder);
  }
}

std::vector<int> DivideAndGetMapping(std::vector<std::pair<int, int> > scores,
                                     int num_partitions) {
  std::sort(scores.begin(), scores.end(),
            std::greater<std::pair<int, int> >());

  std::priority_queue<Partition> holder, remainder;

  Divide(scores, num_partitions, holder, remainder);

  std::vector<int> mapping;
  mapping.reserve(scores.size());

  while (!holder.empty()) {
    const Partition partion = holder.top();
    holder.pop();
    for (size_t i = 0; i < partion.columns.size(); ++ i) {
      mapping.push_back(partion.columns[i]);
    }
  }

  while (!remainder.empty()) {
    const Partition partion = remainder.top();
    remainder.pop();
    for (size_t i = 0; i < partion.columns.size(); ++ i) {
      mapping.push_back(partion.columns[i]);
    }
  }

  return mapping;
}

void ProcessDataWithStrategy(const char *input_file,
                             const char *output_file,
                             const char *mapping_file, int dim,
                             int num_partitions) {
  std::vector<std::vector<int> > data;
  LoadData(input_file, dim, data);

  int num_total_items = data.size();
  int *num_ones = new int[dim];

  CalOnesPerColumn(data, dim, num_ones);

  std::vector<std::pair<int, int> > scores;
  for (int i = 0; i < dim; ++ i) {
    scores.push_back(
        std::make_pair(std::min(num_ones[i], num_total_items - num_ones[i]),
                       i));
  }
  std::vector<int> mapping = DivideAndGetMapping(scores, num_partitions);

  printf("\ntotal lines of file:\t%d\n", num_total_items);

  std::ofstream fout(output_file, std::ios::binary | std::ios::out);
  std::vector<int> line;
  line.reserve(dim);
  int dim_over_8 = dim / 8;
  uint8_t *chunks = new uint8_t[dim_over_8];

  fout.write((char *)&num_total_items, sizeof(int));
  for (size_t i = 0; i < data.size(); ++ i) {
    const std::vector<int> &item = data[i];
    line.clear();
    for (int j = 0; j < dim; ++ j)
      line.push_back(item[mapping[j]]);
    FillChunks(line, chunks);
    fout.write((char *)chunks, sizeof(uint8_t) * dim_over_8);
  }

  WriteMappingFile(mapping_file, mapping);

  delete[] num_ones;
  delete[] chunks;
  num_ones = NULL;
  chunks = NULL;
}

inline void WriteMappingFile(const char *mapping_file,
                             const std::vector<int> &mapping) {
  std::ofstream fout(mapping_file);
  int *line = new int[mapping.size()];
  int size = mapping.size();

  for (int i = 0; i < size; ++ i)
    line[i] = mapping[i];

  fout.write((char *)&size, sizeof(int));
  fout.write((char *)line, sizeof(int) * size);

  fout.close();
  delete[] line;
  line = NULL;
}

int main(int argc, char *argv[]) {
  struct option long_options[] = {
    {"input-file", required_argument, NULL, 'i'},
    {"output-file", required_argument, NULL, 'o'},
    {"mapping-file", required_argument, NULL, 'm'},
    {"dimension", required_argument, NULL, 'd'},
    {"optimize", no_argument, NULL, 'z'},
    {"num-partitions", required_argument, NULL, 'n'},
    {NULL, no_argument, NULL, 0},
  };

  char input_file[FILE_NAME_MAX_SIZE] = {};
  char output_file[FILE_NAME_MAX_SIZE] = {};
  char mapping_file[FILE_NAME_MAX_SIZE] = {};
  int dim = 0;
  bool optimized = false;
  int num_partitions = 0;

  int c;
  while ((c = getopt_long(argc, argv, "i:o:d:z", long_options, NULL)) != EOF) {
    switch(c) {
      case 'i':
        snprintf(input_file, FILE_NAME_MAX_SIZE, "%s", optarg);
        break;
      case 'o':
        snprintf(output_file, FILE_NAME_MAX_SIZE, "%s", optarg);
        break;
      case 'd':
        sscanf(optarg, "%d", &dim);
        break;
      case 'z':
        optimized = true;
        break;
      case 'n':
        sscanf(optarg, "%d", &num_partitions);
        break;
      case 'm':
        snprintf(mapping_file, FILE_NAME_MAX_SIZE, "%s", optarg);
        break;
      default:
        break;
    }
  }

  if (!strlen(input_file) || !strlen(output_file) || !dim ||
      (optimized && (!num_partitions || !strlen(mapping_file)))) {
    Help();
    return -EXIT_FAILURE;
  }

  printf("input file:\t\t%s\n", input_file);
  printf("output file:\t\t%s\n", output_file);
  printf("dimension:\t\t\t%d\n", dim);
  printf("optimized:\t\t\t%s\n", optimized ? "true" : "false");
  if (optimized) {
    printf("mapping file:\t\t%s\n", mapping_file);
    printf("the number of partitions:\t%d\n", num_partitions);
  }

///////////////////////////////////////////////////////////////////////////////

  if (!optimized) ProcessDataWithoutStrategy(input_file, output_file, dim);
  else ProcessDataWithStrategy(input_file, output_file, mapping_file,
                               dim, num_partitions);

  return EXIT_SUCCESS;
}
