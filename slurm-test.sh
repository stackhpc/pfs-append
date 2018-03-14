#!/bin/bash

if [[ ! -z "$1" ]]
then
    ARGS="-s $1"
fi

./append-write -n $SLURM_PROCID $ARGS testfile.dat

