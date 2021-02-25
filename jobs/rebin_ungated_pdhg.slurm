#!/usr/bin/env bash

#SBATCH -n 32
#SBATCH --exclusive 
#SBATCH -C  "scarf20"
##SBATCH --account=scddevel
##SBATCH --reservation=scddevel
# SBATCH -t 7-0

# defaults
gamma=1.0
alpha=0.5
epochs=200
transform="pet/transf_g*.nii"

while getopts ha:g:e:i:t: option
 do
 case "${option}"
 in
  a) alpha=${OPTARG};;
  g) gamma=${OPTARG};;
  e) epochs=${OPTARG};;
  i) initial=${OPTARG};;
  t) transform=${OPTARG};;
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
# # conda list
# source ~/devel/install/bin/env_ccppetmr.sh
# build in Feb2021
module load GCC/8.1.0-2.30

source activate mcir
source /work3/cse/synerbi/mcir_build/install/bin/env_sirf.sh

# bash /home/vol05/scarf595/MCIR/SIRF/examples/Python/PETMR/rebin_ungated_pdhg.sh -a ${alpha} -e ${epochs} -g ${gamma}
/work3/cse/synerbi/mcir_build/SIRF-Contribs/src/Python/sirf/contrib/MCIR/rebin_gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs}