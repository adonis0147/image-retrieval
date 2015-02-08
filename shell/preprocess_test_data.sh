#!/bin/bash

mkdir -p ../data

./generate_test_data.py --input=/home/adonis/Downloads/jd_info/features_test.txt \
                        --output=../data/features_jd_test1.dat \
                        --dimension=1024

echo

./generate_test_data.py --input=/home/adonis/Downloads/jd_info/features_test.txt \
                        --output=../data/features_jd_test2.dat \
                        --mapping-file=../data/mapping.dat \
                        --dimension=1024

echo
