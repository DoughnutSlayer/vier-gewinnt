#!/bin/bash

for (( i = 4; i > 1; i-- )); do
    export PROCESS_NUMBER=$i
    for (( j = 2; j <= 4; j++ )); do
        for (( k = 2; k <= 4; k++ )); do
            export BOARD_WIDTH=$j BOARD_HEIGHT=$k
            make -B
            logFileName="./logs/${j}x$k-$i"
            rm -f $logFileName
            for (( l = 0; l < 5; l++ )); do
                sbatch bench.slurm
                cat job.out >> $logFileName
            done
        done
    done
done
