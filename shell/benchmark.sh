#!/bin/bash

../bin/benchmark --train-file=../data/features_jd_train1.dat \
                --test-file=../data/features_jd_test1.dat \
                --dimension=512 --bucket-groups-num=256 --search-radius=0 \
                --neighbors-num=10

echo

../bin/benchmark --train-file=../data/features_jd_train2.dat \
                --test-file=../data/features_jd_test2.dat \
                --dimension=512 --bucket-groups-num=256 --search-radius=0 \
                --neighbors-num=10

echo

../bin/benchmark --train-file=../data/features_jd_train1.dat \
                --test-file=../data/features_jd_test1.dat \
                --dimension=512 --bucket-groups-num=256 --search-radius=1 \
                --neighbors-num=10

echo

../bin/benchmark --train-file=../data/features_jd_train2.dat \
                --test-file=../data/features_jd_test2.dat \
                --dimension=512 --bucket-groups-num=256 --search-radius=1 \
                --neighbors-num=10

echo
