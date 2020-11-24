#! /bin/bash

work_dir=/work3/cse/synerbi
base_result=${work_dir}/results
##############    RUN NAMES    ################
run_names=(\
#"run_gated_view_gates" "ungated_pdhg" "ungated_spdhg" "gated_pdhg" "gated_gates" "ungated_pdhg_5000" \
#"alpha_0.1_gated_gates" "alpha_0.5_gated_gates" "alpha_1_gated_gates"   "alpha_5_gated_gates"  "alpha_10_gated_gates"\
#  "alpha_15_gated_gates"  "alpha_20_gated_gates"  "alpha_25_gated_gates"  "alpha_30_gated_gates"  \
#  "alpha_35_gated_gates"  
  #"noprecond_alpha_5_gated_gates" 
  "noprecond_alpha_5_gated_pdhg" 
  #"precond_alpha_5_gated_pdhg")
)

current_dir=`pwd`
for run_name in ${run_names[@]}
do 
  echo "should rsync ${base_result}/${run_name}"
  cd ${base_result}/${run_name}
  echo `pwd`
  # sync reconstructions
  rsync -avz --exclude "tmp_*.hs" --exclude "tmp_*.s" \
     -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
     --progress . \
     sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons/${run_name}
  
  # # sync parameters
  # rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
  # --progress params \
  # sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons/${run_name}/params
  # # sync algorithm log
  # rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
  # --progress *.log \
  # sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons/${run_name}/recons

#   ls recons
done

cd ${current_dir}

# https://www.digitalocean.com/community/tutorials/how-to-copy-files-with-rsync-over-ssh
# rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress ungated_pdhg/recons/ sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons
