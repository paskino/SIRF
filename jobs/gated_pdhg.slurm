#!/usr/bin/env bash

#SBATCH -n 32
#SBATCH --exclusive 
#SBATCH -C  "scarf20"
##SBATCH --account=scddevel
##SBATCH --reservation=scddevel
#SBATCH -t 7-00:00

# defaults
gamma=2.8284
alpha=0.5
epochs=200
precond="None"

while getopts hpa:g:e: option
 do
 case "${option}"
 in
  a) alpha=${OPTARG};;
  g) gamma=${OPTARG};;
  e) epochs=${OPTARG};;
  p) precond="precond";;
  h)
   echo "Usage: $0 -a alpha -g gamma -e epochs"
   exit 
   ;;
  *)
   echo "Wrong option passed. Use the -h option to get some help." >&2
   exit 1
  ;;
 esac
done


module purge
# source activate sirf
# # # conda list
# source ~/devel/install/bin/env_ccppetmr.sh
# bash /home/vol05/scarf595/MCIR/SIRF/examples/Python/PETMR/gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs}

# build in Feb2021
module load GCC/8.1.0-2.30

source activate mcir
source /work3/cse/synerbi/mcir_build/install/bin/env_sirf.sh

# source activate mcir_intel
# I copied the env_sirf.sh to the envs/mcir_intel/etc/conda/activate.d/
# so the below source is not strictly needed
# source /work3/cse/synerbi/mcir_build/intel_install/bin/env_sirf.sh

if [ "${precond}" = "None" ]; then
  bash /work3/cse/synerbi/mcir_build/SIRF-Contribs/src/Python/sirf/contrib/MCIR/rebin_gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs}
else
  bash /work3/cse/synerbi/mcir_build/SIRF-Contribs/src/Python/sirf/contrib/MCIR/rebin_gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs} -p
fi
