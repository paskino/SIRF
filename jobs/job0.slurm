#!/usr/bin/env bash

#SBATCH -n 24
#SBATCH --exclusive 
#SBATCH -C  "scarf18"
##SBATCH --account=scddevel
##SBATCH --reservation=scddevel
#SBATCH -t 01:10:00

module purge
source activate sirf
conda list
source ~/devel/install/bin/env_ccppetmr.sh

python -c "from ccpi.framework import ImageGeometry"
cwd=`pwd`
cd ~/devel/buildVM/sources/CCPi-Framework/Wrappers/Python/test
python -m unittest test_functions.TestKullbackLeiblerNumba
cd ${cwd}
