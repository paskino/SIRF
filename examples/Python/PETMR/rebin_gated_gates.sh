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
if [ -f $loc_data/pet/${initial} ] ;
then
run_name=rebin_initial_rescaled_gamma_${gamma}_noprecond_alpha_${alpha}_gated_gates
else
run_name=rebin_noinitial_rescaled_gamma_${gamma}_noprecond_alpha_${alpha}_gated_gates
fi
if [ ${transform} == "pet_space_def*.nii" ] 
then run_name="new_motion_${run_name}"
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
update_interval=80

if [ -f $loc_data/pet/${initial} ] ;
then 
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
-T "$loc_data/${transform}"                 \
-t def                                      \
--nifti                                     \
--dxdy=3.12117                              \
--nxny=180                                  \
--alpha=${alpha}                            \
--gamma=${gamma}                            \
-I "$loc_data/pet/${initial}"               \
--numSegsToCombine=11                       \
--numViewsToCombine=2                       \
--numThreads=32 2>&1 > script.log
else
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
-T "$loc_data/${transform}"                 \
-t def                                      \
--nifti                                     \
--dxdy=3.12117                              \
--nxny=180                                  \
--alpha=${alpha}                            \
--gamma=${gamma}                            \
--numSegsToCombine=11                       \
--numViewsToCombine=2                       \
--numThreads=32 2>&1 > script.log

fi
