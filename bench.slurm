#!/bin/bash

#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --partition=west
#SBATCH --error=job.err

. /etc/profile.d/wr-spack.sh
spack load --dependencies mpi

if [ "${SLURM_PARTITION}" = 'abu' ]
then
	export MPICH_NEMESIS_NETMOD=ib
fi

mpirun -n ${PROCESS_NUMBER} ./vier-gewinnt bench
