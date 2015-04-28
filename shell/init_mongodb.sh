#!/bin/bash

../bin/init_mongodb --file=/home/adonis/Downloads/jd_info/train.txt \
                    --database=jd --collection=jd_info_train

../bin/init_mongodb --file=/home/adonis/Downloads/jd_info/test.txt \
                    --database=jd --collection=jd_info_test
