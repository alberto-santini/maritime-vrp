#!/usr/bin/env ruby

require 'json'

PARAMETERS = [
  {
    :name => 'greedy_max_outarcs',
    :values => [-1, 1, 5, 10, 20],
    :disable_via => 'try_fast_heuristics',
    :disable_also => 'greedy_reruns'
  },
  {
    :name => 'greedy_reruns',
    :values => [-1, 10, 50, 100, 500],
    :disable_via => 'try_fast_heuristics',
    :disable_also => 'greedy_max_outarcs'
  },
  {
    :name => 'max_cols_to_solve_mp',
    :values => [50, 100, 500, 100000]
  },
  {
    :name => 'smart_min_chance',
    :values => [-1, 0.0, 0.1, 0.2, 0.3],
    :disable_via => 'try_smart_graph_reduction',
    :disable_also => 'smart_max_chance'
  },
  {
    :name => 'smart_max_chance',
    :values => [-1, 0.4, 0.6, 0.8, 1.0],
    :disable_via => 'try_smart_graph_reduction',
    :disable_also => 'smart_min_chance'
  },
  {
    :name => 'elementary_labelling_every_n_nodes',
    :values => [-1, 1, 5, 15, 30],
    :disable_via => 'try_elementary_labelling'
  },
  {
    :name => 'try_reduced_labelling',
    :values => ['true', 'false']
  }
]

BASIC_PARAMS = {
  :greedy_max_outarcs => 10,
  :greedy_reruns => 100,
  :max_cols_to_solve_mp => 10000,
  :try_fast_heuristics => true,
  :try_elementary_labelling => true,
  :try_smart_graph_reduction => true,
  :try_reduced_labelling => true,
  :elementary_labelling_every_n_nodes => 5,
  :smart_min_chance => 0.1,
  :smart_max_chance => 0.6,
  :dummy_column_price => 1000000000000000,
  :cplex_cores => 2,
  :time_limit_in_s => 900,
  :parallel_labelling => true,
  :early_branching => false,
  :early_branching_timeout => 60
}

def tune_for(param_name)
  param = PARAMETERS.find{|p| p[:name] == param_name}

  throw "Unrecognised parameter: #{param_name}" unless param
  throw "Not in basic params: #{param_name}" unless BASIC_PARAMS.has_key?(param_name.to_sym)

  json_params = Array.new

  param[:values].each do |val|
    new_params = BASIC_PARAMS.clone
    new_params[param_name.to_sym] = val

    if val < 0
      new_params[param[:disable_via].to_sym] = false
      new_params[param[:disable_also].to_sym] = val
    end

    json_params << new_params
  end

  return json_params
end

j = tune_for('greedy_max_outarcs')

j.each do |jparams|
  File.write('tuning_params.json', jparams.to_json)
  Dir.foreach('~/src/data/tuning/*.json') do |instance|
    system("cd ~/src/maritime-vrp/build && oarsub -n \"tuning\" -O \"#{File.basename(instance, '.json')}.out\" -E \"#{File.basename(instance, '.json')}.err\" -p \"network_address!='drbl10-201-201-21'\" -l /nodes=1/core=2,walltime=5 \"LD_LIBRARY_PATH=~/local/lib64 ~/src/maritime-vrp/build/maritime_vrp tuning_params.json #{instance}\"")
  end
  File.unlink('tuning_params.json')
end
