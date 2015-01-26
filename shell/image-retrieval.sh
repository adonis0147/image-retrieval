#!/bin/bash

../bin/ir --train-file=../data/features_jd_train2.dat \
          --test-file=../data/features_jd_test2.dat \
          --dimension=512 --bucket-groups-num=205
