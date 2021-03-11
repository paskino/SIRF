#!/usr/bin/env bash

# #SBATCH -n 8
# #SBATCH -p gpu

#SBATCH --exclusive 
#SBATCH -n 32
#SBATCH -C  "scarf20"
#SBATCH -t 7-00:00

# defaults
gamma=2.8284
alpha=0.5
epochs=200
precond="None"
environment="mcir"

while getopts hpa:g:e:c: option
 do
 case "${option}"
 in
  a) alpha=${OPTARG};;
  g) gamma=${OPTARG};;
  e) epochs=${OPTARG};;
  p) precond="precond";;
  c) environment=${OPTARG};;
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
if [ "${environment}" = "mcir" ] ; then
  # build in Feb2021
  module load GCC/8.1.0-2.30
  source activate mcir
  source /work3/cse/synerbi/mcir_build/install/bin/env_sirf.sh
elif [ "${environment}" = "sirf_mcir" ] ; then
  # this is a copy of the sirf environment which is currently broken
  source activate ${environment}
  source ~/devel/install/bin/env_ccppetmr.sh
elif [ "${environment}" = "icc_cuda_mcir" ] ; then
  source activate ${environment}
  source /work3/cse/synerbi/build_cuda/INSTALL/bin/env_sirf.sh
# source activate sirf
# # # conda list
# source ~/devel/install/bin/env_ccppetmr.sh
# bash /home/vol05/scarf595/MCIR/SIRF/examples/Python/PETMR/gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs}
fi


# source activate mcir_intel
# I copied the env_sirf.sh to the envs/mcir_intel/etc/conda/activate.d/
# so the below source is not strictly needed
# source /work3/cse/synerbi/mcir_build/intel_install/bin/env_sirf.sh

if [ "${precond}" = "None" ]; then
  bash /work3/cse/synerbi/mcir_build/SIRF-Contribs/src/Python/sirf/contrib/MCIR/rebin_gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs}
else
  bash /work3/cse/synerbi/mcir_build/SIRF-Contribs/src/Python/sirf/contrib/MCIR/rebin_gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs} -p
fi
