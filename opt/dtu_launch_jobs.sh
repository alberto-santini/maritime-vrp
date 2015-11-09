#!/bin/bash

cmdline_args=($*)
base_dir="/zhome/fc/e/102910/maritime-vrp"

for file in $(printf "%s%s%s%s" "${base_dir}" "/data/new/" "${cmdline_args[0]}" "*.json")
do
  base_name=$(basename $file)
  grep "${base_name}" solved_instances.txt >/dev/null 2>&1
  return_code=$?
	
  if [[ "${return_code}" == "1" ]]
  then
    echo "Launching job for ${file}"
    sh_name=$(echo ${base_name} | awk '{ gsub(".json", ".sh", $0); print $0 }')
		qsub $(printf "%s%s" "launchers/" "${sh_name}")
  else
    echo "Skipping job for ${file}"
  fi
done