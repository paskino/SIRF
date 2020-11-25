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

while getopts ha:g:e: option
 do
 case "${option}"
 in
  a) alpha=${OPTARG};;
  g) gamma=${OPTARG};;
  e) epochs=${OPTARG};;
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
source activate sirf
# conda list
source ~/devel/install/bin/env_ccppetmr.sh

bash /home/vol05/scarf595/MCIR/SIRF/examples/Python/PETMR/gated_pdhg.sh -a ${alpha} -g ${gamma} -e ${epochs}