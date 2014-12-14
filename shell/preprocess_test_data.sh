#!/bin/bash

./generate_test_data.py --input=/home/adonis/Downloads/grad/split/features_jd_test.txt \
                        --output=../data/features_jd_test1.dat \
                        --dimension=4096

echo

./generate_test_data.py --input=/home/adonis/Downloads/grad/split/features_jd_test.txt \
                        --output=../data/features_jd_test2.dat \
                        --mapping-file=../data/mapping.dat \
                        --dimension=4096

echo
