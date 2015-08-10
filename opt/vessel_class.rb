class VesselClass
  attr_accessor :name, :capacity, :cost_tc, :draught, :min_speed,
                :max_speed, :design_speed, :cost_design_speed,
                :cost_idle, :quantity, :speeds, :speeds_costs,
                :min_speed_ti, :max_speed_ti, :design_speed_ti,
                :speeds_ti
  
  def initialize(options, name, capacity, cost_tc, draught, min_speed, max_speed, design_speed, cost_design_speed, cost_idle)
    @options = options
    @name = name
    @capacity = capacity.to_i
    
    @cost_tc = cost_tc.to_f
    @cost_tc_ti = @cost_tc / @options[:time_intervals_per_day]
    
    @draught = draught.to_f
    
    @min_speed = min_speed.to_f
    @max_speed = max_speed.to_f
    @design_speed = design_speed.to_f
    @cost_design_speed = cost_design_speed.to_f
    
    @cost_idle = cost_idle.to_f
    @cost_idle_ti = @cost_idle / @options[:time_intervals_per_day]
    
    @speeds = [@min_speed, (@max_speed + @min_speed) / 2, @max_speed]  
    @speed_costs = @speeds.map{|s| VesselClass.cost_for_speed(s, @design_speed, @cost_design_speed)}
    
    @min_speed_ti = @min_speed * @options[:discretisation]
    @max_speed_ti = @max_speed * @options[:discretisation]
    @design_speed_ti = @design_speed * @options[:discretisation]
    
    @speeds_ti = Array.new(@speeds.size)
    @speeds.each_with_index do |speed, index|
      @speeds_ti[index] = speed * @options[:discretisation]
    end
    
    @speed_costs_dol = @speed_costs.map{|c| c * @options[:bunker_cost_per_ton]}
    
    @speed_costs_dol_ti = Array.new(@speeds.size)
    @speeds.each_with_index do |speed, index|
      @speed_costs_dol_ti[index] = @speed_costs_dol[index] * speed
    end
  end
  
  def self.cost_for_speed(speed, design_speed, cost_design_speed)
    (((speed.to_f / design_speed) ** 3) * cost_design_speed).to_f
  end
  
  def to_json(*opt)
    "{
      \"vessel_class_name\": \"#{@name}\",
      \"capacity_in_ffe\": #{@capacity.to_i},
      \"time_charter_cost_per_day\": #{@cost_tc.round(4)},
      \"time_charter_cost_per_time_interval\": #{@cost_tc_ti.round(4)},
      \"draught\": #{@draught.round(4)},
      \"min_speed_in_knots\": #{@min_speed.round(4)},
      \"max_speed_in_knots\": #{@max_speed.round(4)},
      \"design_speed_in_knots\": #{@design_speed.round(4)},
      \"min_speed_in_miles_per_time_interval\": #{@min_speed_ti.round(4)},
      \"max_speed_in_miles_per_time_interval\": #{@max_speed_ti.round(4)},
      \"design_speed_in_miles_per_time_interval\": #{@design_speed_ti.round(4)},
      \"cost_at_design_speed_in_tons_of_bunker_per_mile\": #{@cost_design_speed.round(4)},
      \"cost_when_idle_in_tons_of_bunker_per_day\": #{@cost_idle.round(4)},
      \"cost_when_idle_in_tons_of_bunker_per_time_interval\": #{@cost_idle_ti.round(4)},
      \"cost_when_idle_in_dollars_per_time_interval\": #{(@cost_idle_ti * @options[:bunker_cost_per_ton]).round(4)},
      \"number_of_available_vessels\": #{@quantity.to_i},
      \"speeds_in_knots\": #{@speeds.map{|s|s.round(4)}.to_json},
      \"speeds_in_miles_per_time_interval\": #{@speeds_ti.map{|s|s.round(4)}.to_json},
      \"speed_costs_in_tons_of_bunker_per_mile\": #{@speed_costs.map{|s|s.round(4)}.to_json},
      \"speed_costs_in_dollars_per_mile\": #{@speed_costs_dol.map{|s|s.round(4)}.to_json},
      \"speed_costs_in_dollars_per_time_interval\": #{@speed_costs_dol_ti.map{|s|s.round(4)}.to_json}
    }"
  end
end