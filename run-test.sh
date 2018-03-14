#!/bin/bash

# An amount of data to begin with in the file
TEST_BASE=$RANDOM

# A record size to write (minimum 100, maximum 10000)
RECORD_SIZE=$((($RANDOM % 9900) + 100))

echo "Running with a base file of $TEST_BASE bytes, records of $RECORD_SIZE"
dd if=/dev/zero of=testfile.dat bs=$TEST_BASE count=1
srun -n 8 -N 2 ./slurm-test.sh $RECORD_SIZE
./append-check testfile.dat -b $TEST_BASE -s $RECORD_SIZE
