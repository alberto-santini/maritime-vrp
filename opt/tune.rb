#!/usr/bin/env ruby

SIGNAL_DISABLED = -1

TUNABLE_PARAMS = [
  {
    :name => 'greedy_max_outarcs',
    :values => [5, 10, 20]
  },
  {
    :name => 'greedy_reruns',
    :values => [50, 100, 500]
  },
  {
    :name => 'max_cols_to_solve_mp',
    :values => [100, 500, 100000]
  },
  {
    :name => 'try_fast_heuristics',
    :values => ['true', 'false'],
    :disables => {
      'false' => ['greedy_max_outarcs', 'greedy_reruns']
    }
  },
  {
    :name => 'smart_min_chance',
    :values => [0.1, 0.2, 0.3]
  },
  {
    :name => 'smart_max_chance',
    :values => [0.6, 0.8, 1.0]
  },
  {
    :name => 'try_smart_graph_reduction',
    :values => ['true', 'false'],
    :disables => {
      'false' => ['smart_min_chance', 'smart_max_chance']
    }
  },
  {
    :name => 'elementary_labelling_every_n_nodes',
    :values => [1, 5, 15]
  },
  {
    :name => 'try_elementary_labelling',
    :values => ['true', 'false'],
    :disables => {
      'false' => ['elementary_labelling_every_n_nodes']
    }
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


def parameters_cartesian_product
  couples = TUNABLE_PARAMS.map do |param|
    [param[:name]].product param[:values]
  end

  params = couples[0]
  params = params.product(*couples[1..-1])

  params_without_disabled = Array.new

  params.each do |param_combination|
    new_param_combination = Marshal.load(Marshal.dump(param_combination))

    param_combination.each do |param_and_value|
      param, value = param_and_value
      tparam = TUNABLE_PARAMS.find{|p| p[:name] == param}

      if tparam.has_key? :disables
        tparam[:disables].each do |disabling_value, disabled_params|
          if value == disabling_value
            disabled_params.each do |disabled_param|
              new_param_combination.map! do |new_param_and_value|
                new_param, new_value = new_param_and_value
                if new_param == disabled_param
                  new_value = SIGNAL_DISABLED
                end
                [new_param, new_value]
              end
            end
          end
        end
      end
    end

    params_without_disabled << new_param_combination
  end

  params_without_disabled.uniq!
end
