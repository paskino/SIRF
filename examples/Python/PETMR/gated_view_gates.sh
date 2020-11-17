#! /bin/bash

loc_data=/home/sirfuser/data/cardiac_resp
loc_algo=~/devel/claire/SIRF/examples/Python/PETMR
loc_reco=~/devel/claire/gated/recons
loc_param=~/devel/claire/gated/params
                       

python $loc_algo/PET_MCIR_PD.py             \
-o view_gates_spdhg                           \
--algorithm=spdhg                            \
-r FGP_TV                                   \
--outpath=$loc_reco                        \
--param_path=$loc_param                    \
-e 500                                      \
--update_obj_fn_interval=480                \
--descriptive_fname                         \
-v 0                                        \
-S "$loc_data/pet/EM_g*.hs"                  \
-R "$loc_data/pet/total_background_g*.hs"      \
-n "$loc_data/pet/NORM.n.hdr"               \
-a "$loc_data/pet/MU_g*.nii"                  \
-T "$loc_data/pet/transf_g*.nii"            \
-t def                                    \
--nifti                                   \
--alpha=40.0                                \
--precond                                  \
--dxdy=3.12117                             \
--nxny=180                                 \
--numSubsets=6                             \
--numThreads=27

