#!/bin/bash

mkdir -p ../data

../bin/preprocess --input=/home/adonis/Downloads/jd_info/features_train.txt \
                  --output=../data/features_jd_train1.dat \
                  --dimension=1024

echo

../bin/preprocess --input=/home/adonis/Downloads/jd_info/features_train.txt \
                  --output=../data/features_jd_train2.dat \
                  --mapping-file=../data/mapping.dat \
                  --dimension=1024 --optimize --num-partitions=55

echo
