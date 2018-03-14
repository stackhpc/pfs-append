#!/bin/bash

ARGS="testfile.dat"

if [[ $# -gt 1 ]]
then
    ARGS=$*
fi

./append-write -n $SLURM_PROCID $ARGS

