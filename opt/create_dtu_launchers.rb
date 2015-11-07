#!/usr/bin/env ruby

base_dir = '/zhome/fc/e/102910/maritime-vrp'
preamble = <<-EOM
#!/bin/bash
#PBS -l walltime=4:00:00
#PBS -l nodes=1:ppn=2
#PBS -l feature=XeonX5550
#PBS -l mem=32G
EOM

Dir.glob("../data/new/*.json") do |filename|
  instance_name = File.basename(filename, '.json')
  launcher_filename = "launchers/#{instance_name}.sh"
  config_filename = "../data/old_thesis_data/program_params.json"
  File.open(launcher_filename, 'w') do |file|
    file.write preamble
    file.write "\#PBS -N #{instance_name}\n"
    file.write "cd #{base_dir}/build\n"
    file.write "./maritime_vrp #{config_filename} #{filename}\n"
  end
end