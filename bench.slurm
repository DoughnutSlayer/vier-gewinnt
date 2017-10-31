#!/bin/bash

#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --partition=west
#SBATCH --output=job.out
#SBATCH --error=job.err

. /etc/profile.d/wr-spack.sh
spack load --dependencies mpi

if [ "${SLURM_PARTITION}" = 'abu' ]
then
	export MPICH_NEMESIS_NETMOD=ib
fi

srun hostname
./bench.sh
