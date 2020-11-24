#! /bin/bash

loc_data=/home/sirfuser/data/cardiac_resp
loc_algo=~/devel/claire/SIRF/examples/Python/PETMR
loc_reco=~/devel/claire/gated/recons
loc_param=~/devel/claire/gated_pdhg_notrans/params
                       

python $loc_algo/PET_MCIR_PD.py             \
-o gated_pdhg_notrans                         \
--algorithm=pdhg                            \
-r FGP_TV                                   \
--outpath=$loc_reco                        \
--param_path=$loc_param                    \
-e 500                                     \
--update_obj_fn_interval=10                \
--descriptive_fname                         \
-v 0                                        \
-S "$loc_data/pet/EM_g*.hs"                  \
-R "$loc_data/pet/total_background_g*.hs"      \
-n "$loc_data/pet/NORM.n.hdr"                   \
-a "$loc_data/pet/MU.hv"                \
--nifti                                   \
--alpha=5.0                                \
--dxdy=3.12117                             \
--nxny=180                                 \
--numThreads=27                            \
--gamma=2.8284271247461903                 
