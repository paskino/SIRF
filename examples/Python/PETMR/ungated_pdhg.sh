#! /bin/bash

# defaults
gamma=1.0
alpha=1.0
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


# base directory
mcir_dir=/home/vol05/scarf595/MCIR
work_dir=/work3/cse/synerbi/
loc_data=${work_dir}/cardiac_resp
loc_algo=${mcir_dir}/SIRF/examples/Python/PETMR

base_result=${work_dir}/results
##############    RUN NAME    ################
run_name=rescaled_gamma_${gamma}_alpha_${alpha}_ungated_pdhg

loc_reco=${base_result}/${run_name}/recons
loc_param=${base_result}/${run_name}/params
                       
# create the run directory and get in there
mkdir -p ${base_result}/${run_name}
cd ${base_result}/${run_name}                      

cp ${loc_algo}/PET_MCIR_PD.py ${base_result}/${run_name}
cd ${base_result}/${run_name}

###############   CONFIGURATION   ###################

#####   TEST   #####
#epochs=2
#update_interval=48      
#####   RUN   ##### 
# epochs=5000
update_interval=10

python PET_MCIR_PD.py                                   \
-o ungated_pdhg                                         \
--algorithm=pdhg                                        \
-r FGP_TV                                               \
--outpath=$loc_reco                                     \
--param_path=$loc_param                                 \
-e ${epochs}                                            \
--update_obj_fn_interval=${update_interval}             \
--descriptive_fname                                     \
-v 0                                                    \
-S "$loc_data/pet/STIRungated/sinospan11_f1g1d0b0.hs"   \
-R "$loc_data/pet/STIRungated/total_background.hs"      \
-n "$loc_data/pet/NORM.n.hdr"                           \
-a "$loc_data/pet/MU.hv"                                \
--nifti                                                 \
--alpha=${alpha}                                        \
--gamma=${gamma}
--dxdy=3.12117                                          \
--nxny=180                                              \
--numThreads=32
