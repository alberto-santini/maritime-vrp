#!/bin/bash

cmdline_args=($*)
base_dir="/home/alberto/or-bologna/maritime-vrp"

for file in $(printf "%s%s%s%s" "${base_dir}" "/data/new/" "${cmdline_args[0]}" "*.json")
do
  base_name=$(basename $file)
  grep "${base_name}" solved_instances.txt
  return_code=$?
  
  if [[ "${return_code}" == "1" ]]
  then
    echo "Launching job for ${file}"
    oarsub -n "${base_name}" -O "${base_name}.out" -E "${base_name}.err" --p "network_address!='drbl10-201-201-21'" -l /nodes=1/core=2,walltime=5 "${base_dir}/opt/timeout -m 3145728 ${base_dir}/build/maritime_vrp ${base_dir}/data/old_thesis_data/program_params.json ${base_dir}/data/new/${base_name}"
  else
    echo "Skipping job for ${file}"
  fi
done