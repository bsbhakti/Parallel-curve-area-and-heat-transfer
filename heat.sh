#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow

srun python /home/bsb10/cmpt431_A2/test_scripts/heat_transfer_tester.pyc --execPath=/home/bsb10/cmpt431_A2/heat_transfer_parallel --scriptPath=/home/bsb10/cmpt431_A2/test_scripts/heat_transfer_evaluator.pyc