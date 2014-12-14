#!/usr/bin/env python

import struct
import getopt
import sys
from math import fabs
from pprint import pprint

def Help():
    print('Invalid arguments!')
    print("""usage:
  -i, --input\t\t input file
  -o, --output\t\t output file
  -m, --mapping-file\t mapping file (optional)
  -d, --dimension\t data dimension
          """)

def GetMapping(mapping_file):
    with open(mapping_file, 'rb') as f:
        mapping_size = struct.unpack('<I', f.read(4))[0]
        binary_format = '<' + 'I' * mapping_size
        mapping = struct.unpack(binary_format, f.read(4 * mapping_size))
        return mapping


def GetData(input_file):
    epsilon = 1e-6
    data = []
    with open(input_file, 'r') as f:
        for line in f:
            data.append(map(lambda x: int(not (fabs(float(x)) < epsilon)),
                            line.split()))
    return data


def Transform(mapping, data):
    for i, item in enumerate(data):
        data[i] = [item[mapping[j]] for j in xrange(len(item))]
    return data


def Compress(data):
    chunks = []
    for i in xrange(0, len(data), 8):
        num = 0
        for j in xrange(8):
            num |= data[i + j] << j
        chunks.append(num)
    return chunks


def ProcessData(input_file, output_file, mapping_file, dim):
    if mapping_file is None:
        mapping = range(0, dim)
    else:
        mapping = GetMapping(mapping_file)

    data = GetData(input_file)
    data = Transform(mapping, data)

    print('\ntotal lines of file:\t%d' % len(data))

    with open(output_file, 'wb') as f:
        f.write(struct.pack('<I', len(data)))
        dim_over_8 = len(data[0]) / 8
        binary_format = '<' + 'B' * dim_over_8
        for item in data:
            f.write(struct.pack(binary_format, *Compress(item)))


if __name__ == '__main__':
    opts, args = getopt.getopt(sys.argv[1:], "i:o:m:d:",
                               ["input=", "output=", "mapping-file=",
                                "dimension="])
    input_file = None
    output_file = None
    mapping_file = None
    dim = 0
    for opt, arg in opts:
        if opt in ('-i', '--input'):
            input_file = arg
        elif opt in ('-o', '--output'):
            output_file = arg
        elif opt in ('-m', '--mapping-file'):
            mapping_file = arg
        elif opt in ('-d', '--dimension'):
            dim = int(arg)

    if input_file is None or output_file is None or dim == 0:
        Help()
        exit()

    print('input file:\t%s' % input_file)
    print('output file:\t%s' % output_file)
    if mapping_file is not None:
        print('mapping-file:\t%s' % mapping_file)
    print('data dimension:\t\t%d' % dim)

    ProcessData(input_file, output_file, mapping_file, dim)
