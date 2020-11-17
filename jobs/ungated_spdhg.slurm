#!/usr/bin/env bash

#SBATCH -n 32
#SBATCH --exclusive 
#SBATCH -C  "scarf20"
##SBATCH --account=scddevel
##SBATCH --reservation=scddevel
# SBATCH -t 06:10:00

module purge
source activate sirf
# conda list
source ~/devel/install/bin/env_ccppetmr.sh

bash /home/vol05/scarf595/MCIR/SIRF/examples/Python/PETMR/ungated_spdhg.sh
