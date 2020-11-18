#! /bin/bash

work_dir=/work3/cse/synerbi/
base_result=${work_dir}/results
##############    RUN NAMES    ################
run_names=("run_gated_view_gates" "ungated_pdhg" "ungated_spdhg" "gated_pdhg" "gated_gates")

current_dir=`pwd`
for run_name in ${run_names[@]}
do 
  echo "should rsync ${base_result}/${run_name}"
  cd ${base_result}/${run_name}
  rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
  --progress recons/ \
  sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons/${run_name}/recons
  rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
  --progress *.log \
  sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons/${run_name}/recons
#   ls recons
done

cd ${current_dir}

# https://www.digitalocean.com/community/tutorials/how-to-copy-files-with-rsync-over-ssh
# rsync -avz -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --progress ungated_pdhg/recons/ sirfuser@130.246.214.97:~/data/cardiac_resp/pet/recons