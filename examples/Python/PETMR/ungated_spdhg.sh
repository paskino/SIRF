#! /bin/bash

loc_data=/home/sirfuser/data/cardiac_resp
loc_algo=~/devel/claire/SIRF/examples/Python/PETMR
loc_reco=~/devel/claire/cluster_test/recons
loc_param=~/devel/claire/cluster_test/params
                       

python $loc_algo/PET_MCIR_PD.py             \
-o ungated_spdhg                              \
--algorithm=spdhg                            \
-r FGP_TV                                   \
--outpath=$loc_reco                        \
--param_path=$loc_param                    \
-e 500                                     \
--update_obj_fn_interval=480                  \
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
--numSubsets=48                             \
--numThreads=27                             


                                                          
                                    