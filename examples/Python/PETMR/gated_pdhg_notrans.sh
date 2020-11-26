#! /bin/bash
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
# base directory
mcir_dir=/home/vol05/scarf595/MCIR
work_dir=/work3/cse/synerbi/
loc_data=${work_dir}/cardiac_resp
loc_algo=${mcir_dir}/SIRF/examples/Python/PETMR

base_result=${work_dir}/results
##############    RUN NAME    ################
run_name=rescaled_gamma_${gamma}_notrans_alpha_${alpha}_gated_pdhg

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
update_interval=10
                       

python PET_MCIR_PD.py                       \
-o gated_pdhg_notrans                         \
--algorithm=pdhg                            \
-r FGP_TV                                   \
--outpath=$loc_reco                        \
--param_path=$loc_param                    \
-e 500                                     \
--update_obj_fn_interval=${update_interval}                \
--descriptive_fname                         \
-v 0                                        \
-S "$loc_data/pet/EM_g*.hs"                  \
-R "$loc_data/pet/total_background_g*.hs"      \
-n "$loc_data/pet/NORM.n.hdr"                   \
-a "$loc_data/pet/MU_g*.nii"                \
--nifti                                   \
--alpha=${alpha}                                \
--dxdy=3.12117                             \
--nxny=180                                 \
--numThreads=27                            \
--gamma=${gamma}                 
