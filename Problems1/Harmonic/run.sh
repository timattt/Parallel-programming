#!/bin/bash
#PBS -l walltime=00:01:00,nodes=3
#PBS -N out
#PBS -q batch
cd $PBS_O_WORKDIR
mpirun --hostfile $PBS_NODEFILE -np 3
