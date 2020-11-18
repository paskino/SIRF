#! /bin/bash

# base directory
mcir_dir=/home/vol05/scarf595/MCIR
work_dir=/work3/cse/synerbi/
loc_data=${work_dir}/cardiac_resp
loc_algo=${mcir_dir}/SIRF/examples/Python/PETMR

base_result=${work_dir}/results
run_name=ungated_spdhg_test_scarf18
loc_reco=${base_result}/${run_name}/recons
loc_param=${base_result}/${run_name}/params
                       
# create the run directory and get in there
mkdir -p ${base_result}/${run_name}
cd ${base_result}/${run_name}

# run configuration
#epochs=500
#update_interval=480
      
# test configuration
epochs=2
update_interval=48      

cp ${loc_algo}/PET_MCIR_PD.py ${base_result}/${run_name}
cd ${base_result}/${run_name}

python PET_MCIR_PD.py                                 \
-o ungated_spdhg                                      \
--algorithm=spdhg                                     \
-r FGP_TV                                             \
--outpath=$loc_reco                                   \
--param_path=$loc_param                               \
-e ${epochs}                                          \
--update_obj_fn_interval=${update_interval}           \
--descriptive_fname                                   \
-v 0                                                  \
-S "$loc_data/pet/STIRungated/sinospan11_f1g1d0b0.hs" \
-R "$loc_data/pet/STIRungated/total_background.hs"    \
-n "$loc_data/pet/NORM.n.hdr"                         \
-a "$loc_data/pet/MU.hv"                              \
--nifti                                               \
--alpha=5.0                                           \
--precond                                             \
--dxdy=3.12117                                        \
--nxny=180                                            \
--numSubsets=48                                       \
--numThreads=24

