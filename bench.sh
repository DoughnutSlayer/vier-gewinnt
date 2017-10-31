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
                sbatch -o job-$l.out bench.slurm
            done

            jobsDone="False"
            until [[ "$jobsDone" == "" ]]; do
                jobsDone='squeue | grep elich'
                sleep 10
            done

            for (( l = 0; l < 5; l++ )); do
                cat job-$l.out >> $logFileName
            done
        done
    done
done
