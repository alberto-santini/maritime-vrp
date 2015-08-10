class Port
  attr_accessor :name, :draught, :cost_per_container, :call_fee_fixed,
                :call_fee_variable, :call_fee_vc, :is_hub, :pickup_demand,
                :delivery_demand, :pickup_revenue, :delivery_revenue,
                :allowed_vc, :pickup_handling, :delivery_handling, :distances,
                :num_tw, :tw_start, :tw_end, :pickup_transfer, :delivery_transfer,
                :pickup_unit_revenue, :delivery_unit_revenue
  
  def initialize(options, name, draught, cost_per_container, call_fee_fixed, call_fee_variable)
    @options = options
    @name = name
    @draught = draught.to_f
    @cost_per_container = cost_per_container.to_f
    @call_fee_fixed = call_fee_fixed.to_f
    @call_fee_variable = call_fee_variable.to_f
    @is_hub = (name == @options[:hub_name])
  end
  
  def is_same_as(other)
    @name[0..4] == other.name[0..4]
  end
  
  def to_json(*opt)
    "{
      \"unlo_code\": \"#{@name}\",
      \"draught\": #{@draught.round(4)},
      \"cost_per_loaded_or_unloaded_ffe_in_dollars\": #{@cost_per_container.round(4)},
      \"total_movement_cost_pickup\": #{(@cost_per_container * (@is_hub ? 0 : @pickup_demand.to_i)).round(4)},
      \"total_movement_cost_delivery\": #{(@cost_per_container * (@is_hub ? 0 : @delivery_demand.to_i)).round(4)},
      \"call_fee_fixed_in_dollars\": #{@call_fee_fixed.round(4)},
      \"call_fee_per_ffe_of_vessel_capacity_in_dollars\": #{@call_fee_variable.round(4)},
      \"call_fee_per_vessel_class_in_dollars\": #{@call_fee_vc.map{|c|c.round(4)}.to_json},
      \"is_hub\": #{@is_hub},
      \"pickup_demand_in_ffe\": #{@is_hub ? 0 : @pickup_demand.to_i},
      \"delivery_demand_in_ffe\": #{@is_hub ? 0 : @delivery_demand.to_i},
      \"total_revenue_for_pickup\": #{@pickup_revenue.to_f.round(4)},
      \"total_revenue_for_delivery\": #{@delivery_revenue.to_f.round(4)},
      \"allowed_vessel_classes\": #{@allowed_vc.to_json},
      \"pickup_handling_time_in_time_intervals\": #{@is_hub ? 0 : @pickup_handling.to_i},
      \"delivery_handling_time_in_time_intervals\": #{@is_hub ? 0 : @delivery_handling.to_i},
      \"number_of_time_windows\": #{@num_tw.to_i},
      \"time_windows_start_time_intervals\": #{@tw_start.to_json},
      \"time_windows_end_time_intervals\": #{@tw_end.to_json},
      \"pickup_max_transit_time_in_time_intervals\": #{@pickup_transfer},
      \"delivery_max_transit_time_in_time_intervals\": #{@delivery_transfer},
      \"distances\": #{@distances.map{|d|d.round(4)}.to_json}
    }"
  end
end