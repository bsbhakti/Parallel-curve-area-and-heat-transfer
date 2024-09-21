#!/bin/bash
#
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00
#SBATCH --mem=1G
#SBATCH --partition=slow

srun python /home/bsb10/cmpt431_A2/test_scripts/curve_area_tester.pyc --execPath=/home/bsb10/cmpt431_A2/curve_area_parallel --scriptPath=/home/bsb10/cmpt431_A2/test_scripts/curve_area_evaluator.pyc