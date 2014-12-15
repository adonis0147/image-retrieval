#!/bin/bash

mkdir -p ../data

../bin/preprocess --input=/home/adonis/Downloads/data/features_jd_train.txt \
                  --output=../data/features_jd_train1.dat \
                  --dimension=4096

echo

../bin/preprocess --input=/home/adonis/Downloads/data/features_jd_train.txt \
                  --output=../data/features_jd_train2.dat \
                  --mapping-file=../data/mapping.dat \
                  --dimension=4096 --optimize --num-partitions=205

echo
