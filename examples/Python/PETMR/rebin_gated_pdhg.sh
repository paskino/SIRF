#! /bin/bash
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
# base directory
mcir_dir=/home/vol05/scarf595/MCIR
work_dir=/work3/cse/synerbi/
loc_data=${work_dir}/cardiac_resp
loc_algo=${mcir_dir}/SIRF/examples/Python/PETMR

base_result=${work_dir}/results
##############    RUN NAME    ################
if [ ${transform} == "None" ]
then run_name=notrans_rebin_rescaled_gamma_${gamma}_noprecond_alpha_${alpha}_gated_pdhg
else run_name=new_motion_rebin_rescaled_gamma_${gamma}_noprecond_alpha_${alpha}_gated_pdhg
fi
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
                       
if [ ${transform} == "None" ]
then 
python PET_MCIR_PD.py                         \
-o gated_pdhg                                 \
--algorithm=pdhg                              \
-r FGP_TV                                     \
--outpath=$loc_reco                           \
--param_path=$loc_param                       \
-e ${epochs}                                  \
--update_obj_fn_interval=${update_interval}   \
--descriptive_fname                           \
-v 0                                          \
-S "$loc_data/pet/EM_g*.hs"                   \
-R "$loc_data/pet/total_background_g*.hs"     \
-n "$loc_data/pet/NORM.n.hdr"                 \
-a "$loc_data/pet/MU_g*.nii"                  \
-t def                                        \
--nifti                                       \
--alpha=${alpha}                              \
--gamma=${gamma}                              \
--dxdy=3.12117                                \
--nxny=180                                    \
--numSegsToCombine=11                         \
--numViewsToCombine=2                         \
--numThreads=32 2>&1 > script.log

else
python PET_MCIR_PD.py                         \
-o gated_pdhg                                 \
--algorithm=pdhg                              \
-r FGP_TV                                     \
--outpath=$loc_reco                           \
--param_path=$loc_param                       \
-e ${epochs}                                  \
--update_obj_fn_interval=${update_interval}   \
--descriptive_fname                           \
-v 0                                          \
-S "$loc_data/pet/EM_g*.hs"                   \
-R "$loc_data/pet/total_background_g*.hs"     \
-n "$loc_data/pet/NORM.n.hdr"                 \
-a "$loc_data/pet/MU_g*.nii"                  \
-T "$loc_data/${transform}"                   \
-t def                                        \
--nifti                                       \
--alpha=${alpha}                              \
--gamma=${gamma}                              \
--dxdy=3.12117                                \
--nxny=180                                    \
--numSegsToCombine=11                         \
--numViewsToCombine=2                         \
--numThreads=32 2>&1 > script.log
# -T "$loc_data/pet/transf_g*.nii"              \
fi
