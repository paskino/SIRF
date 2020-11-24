#!/usr/bin/env bash

#SBATCH -n 32
#SBATCH --exclusive 
#SBATCH -C  "scarf20"
##SBATCH --account=scddevel
##SBATCH --reservation=scddevel
#SBATCH -t 7-00:00

module purge
source activate sirf
# conda list
source ~/devel/install/bin/env_ccppetmr.sh

bash /home/vol05/scarf595/MCIR/SIRF/examples/Python/PETMR/gated_gates.sh -a 5.0 -g 0.1