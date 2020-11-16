#! /bin/bash

# set location of data scripts and outputs

# base directory
mcir_dir=/home/vol05/scarf595/MCIR/

loc_data=${mcir_dir}/cardiac_resp
loc_algo=${mcir_dir}/SIRF/examples/Python/PETMR

base_result=/work3/cse/synerbi/results/
run_name=cluster_test
loc_reco=${base_result}/${run_name}/recons
loc_param=${base_result}/${run_name}/params
                       
# create the run directory and get in there
mkdir -p ${base_result}/${run_name}
cd ${base_result}/${run_name}

# launch the  python script
python $loc_algo/PET_MCIR_PD.py             \
-o ungated                              \
--algorithm=pdhg                            \
-r FGP_TV                                   \
--outpath=$loc_reco                        \
--param_path=$loc_param                    \
-e 200                                     \
--update_obj_fn_interval=1                  \
--descriptive_fname                         \
-v 0                                        \
-S "$loc_data/pet/STIRungated/sinospan11_f1g1d0b0.hs"   \
-R "$loc_data/pet/STIRungated/total_background.hs"      \
-n "$loc_data/pet/NORM.n.hdr"           \
-a "$loc_data/pet/MU.hv"             \
--nifti                                   \
--alpha=5.0                                \
--precond                                  \
--dxdy=3.12117                             \
--nxny=180                                 \
--numThreads=24

                                                          
                                    
