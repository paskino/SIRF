#! /bin/bash

# base directory
mcir_dir=/home/vol05/scarf595/MCIR
work_dir=/work3/cse/synerbi/
loc_data=${work_dir}/cardiac_resp
loc_algo=${mcir_dir}/SIRF/examples/Python/PETMR

base_result=${work_dir}/results
##############    RUN NAME    ################
run_name=gated_gates

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
epochs=2000
update_interval=80
                       

python PET_MCIR_PD.py                       \
-o gates_spdhg                              \
--algorithm=spdhg                           \
-r FGP_TV                                   \
--outpath=$loc_reco                         \
--param_path=$loc_param                     \
-e ${epochs}                                \
--update_obj_fn_interval=${update_interval} \
--descriptive_fname                         \
-v 0                                        \
-S "$loc_data/pet/EM_g*.hs"                 \
-R "$loc_data/pet/total_background_g*.hs"   \
-n "$loc_data/pet/NORM.n.hdr"               \
-a "$loc_data/pet/MU_g*.nii"                \
-T "$loc_data/pet/transf_g*.nii"            \
-t def                                      \
--nifti                                     \
--alpha=40.0                                \
--precond                                   \
--dxdy=3.12117                              \
--nxny=180                                  \
--numThreads=32

