#!/usr/bin/env ruby

require 'json'
require 'optparse'

OPTIONS = {}

OptionParser.new do |opts|
  # Linerlib scenario: Baltic, WAF, ...
  opts.on "--scenario=MANDATORY" do |s|
    OPTIONS[:scenario] = s
  end
  
  # Hub port UNLO-code, e.g. DEBRV for Baltic
  opts.on "--hub=MANDATORY" do |h|
    OPTIONS[:hub_name] = h
  end
  
  # Time discretisation: 1 time interval = ? hours
  opts.on "--discretisation=MANDATORY" do |d|
    OPTIONS[:discretisation] = d.to_i
  end
  
  # Number of weeks in the time horizon
  opts.on "--weeks=MANDATORY" do |w|
    OPTIONS[:weeks] = w.to_i
  end
  
  # Number of t.i. for handling at the port with max demand
  opts.on "--max-handling=MANDATORY" do |m|
    OPTIONS[:max_handling] = m.to_i
  end
  
  # Number of t.i. for handling at the port with min demand
  opts.on "--min-handling=MANDATORY" do |m|
    OPTIONS[:min_handling] = m.to_i
  end
  
  # Number of t.i. the ship spends at the hub -- for example:
  # If the time discretisation is of 2 hours and the ship spends 1 day
  # at the hub, this parameter will be 12; this would mean that our time
  # horizon goes from Monday 00:00 to Sunday 00:00, while the whole
  # sunday is not modelled, as it is the time the ship spends at the hub
  opts.on "--time-spent-at-hub=MANDATORY" do |t|
    OPTIONS[:time_intervals_at_hub] = t.to_i
  end
  
  # Price in dollar per metric tonne of bunker
  opts.on "--bunker-price=MANDATORY" do |p|
    OPTIONS[:bunker_cost_per_ton] = p.to_f
  end
  
  # Enable closing time windows?
  opts.on "--tw=MANDATORY" do |t|
    OPTIONS[:tw] = (t == "true")
  end
  
  # Min width of a half-time-window, e.g. if this number
  # is n, the min time window will be large 1 + 2 * n
  opts.on "--min-tw=MANDATORY" do |m|
    OPTIONS[:min_tw] = m.to_i
  end
  
  # Max width of a half-time-window, e.g. if this number
  # is n, the max time window will be large 1 + 2 * n
  opts.on "--max-tw=MANDATORY" do |m|
    OPTIONS[:max_tw] = m.to_i
  end
  
  # Enable transfer times?
  opts.on "--transfer=MANDATORY" do |t|
    OPTIONS[:transfer] = (t == "true")
  end
  
  # Min number of t.i. for transfer time
  opts.on "--min-transfer=MANDATORY" do |m|
    OPTIONS[:min_tw] = m.to_i
  end
  
  # Max number of t.i. for transfer time
  opts.on "--max-transfer=MANDATORY" do |m|
    OPTIONS[:max_tw] = m.to_i
  end
end.parse!

TIME_INTERVALS_PER_DAY = (24 / OPTIONS[:discretisation]).to_i
TIME_INTERVALS_IN_TIME_HORIZON = ((OPTIONS[:weeks] * 7 * TIME_INTERVALS_PER_DAY) - OPTIONS[:time_intervals_at_hub]).to_i

class Request
  attr_accessor :origin, :destination, :quantity, :unit_revenue
  
  def initialize(origin, destination, quantity, unit_revenue)
    @origin = origin
    @destination = destination
    @quantity = quantity.to_i
    @unit_revenue = unit_revenue.to_f
  end
  
  def revenue
    @unit_revenue * @quantity
  end
end

class Port
  attr_accessor :name, :draught, :cost_per_container, :call_fee_fixed,
                :call_fee_variable, :call_fee_vc, :is_hub, :pickup_demand,
                :delivery_demand, :pickup_revenue, :delivery_revenue,
                :allowed_vc, :pickup_handling, :delivery_handling, :distances,
                :num_tw, :tw_start, :tw_end, :pickup_transfer, :delivery_transfer
  
  def initialize(name, draught, cost_per_container, call_fee_fixed, call_fee_variable)
    @name = name
    @draught = draught.to_f
    @cost_per_container = cost_per_container.to_f
    @call_fee_fixed = call_fee_fixed.to_f
    @call_fee_variable = call_fee_variable.to_f
    @is_hub = (name == OPTIONS[:hub_name])
  end
  
  def to_json(*opt)
    "{
      \"unlo_code\": \"#{@name}\",
      \"draught\": #{@draught.round(4)},
      \"cost_per_loaded_or_unloaded_ffe_in_dollars\": #{@cost_per_container.round(4)},
      \"call_fee_fixed_in_dollars\": #{@call_fee_fixed.round(4)},
      \"call_fee_per_ffe_of_vessel_capacity_in_dollars\": #{@call_fee_variable.round(4)},
      \"call_fee_per_vessel_class_in_dollars\": #{@call_fee_vc.map{|c|c.round(4)}.to_json},
      \"is_hub\": #{@is_hub},
      \"pickup_demand_in_ffe\": #{@is_hub ? 0 : @pickup_demand.to_i},
      \"delivery_demand_in_ffe\": #{@is_hub ? 0 : @delivery_demand.to_i},
      \"unit_revenue_per_pickup_ffe\": #{@pickup_revenue.to_f.round(4)},
      \"unit_revenue_per_delivery_ffe\": #{@delivery_revenue.to_f.round(4)},
      \"allowed_vessel_classes\": #{@allowed_vc.to_json},
      \"pickup_handling_time_in_time_intervals\": #{@is_hub ? 0 : @pickup_handling.to_i},
      \"delivery_handling_time_in_time_intervals\": #{@is_hub ? 0 : @delivery_handling.to_i},
      \"number_of_time_windows\": #{@num_tw.to_i},
      \"time_windows_start_time_intervals\": #{@tw_start.to_json},
      \"time_windows_end_time_intervals\": #{@tw_end.to_json},
      \"pickup_max_transfer_time\": #{@pickup_transfer},
      \"delivery_max_transfer_time\": #{@delivery_transfer},
      \"distances\": #{@distances.map{|d|d.round(4)}.to_json}
    }"
  end
end

class VesselClass
  attr_accessor :name, :capacity, :cost_tc, :draught, :min_speed,
                :max_speed, :design_speed, :cost_design_speed,
                :cost_idle, :quantity, :speeds, :speeds_costs,
                :min_speed_ti, :max_speed_ti, :design_speed_ti,
                :speeds_ti
  
  def initialize(name, capacity, cost_tc, draught, min_speed, max_speed, design_speed, cost_design_speed, cost_idle)
    @name = name
    @capacity = capacity.to_i
    
    @cost_tc = cost_tc.to_f
    @cost_tc_ti = @cost_tc / TIME_INTERVALS_PER_DAY
    
    @draught = draught.to_f
    
    @min_speed = min_speed.to_f
    @max_speed = max_speed.to_f
    @design_speed = design_speed.to_f
    @cost_design_speed = cost_design_speed.to_f
    
    @cost_idle = cost_idle.to_f
    @cost_idle_ti = @cost_idle / TIME_INTERVALS_PER_DAY
    
    @speeds = [@min_speed, (@max_speed + @min_speed) / 2, @max_speed]  
    @speed_costs = @speeds.map{|s| VesselClass.cost_for_speed(s, @design_speed, @cost_design_speed)}
    
    @min_speed_ti = @min_speed * OPTIONS[:discretisation]
    @max_speed_ti = @max_speed * OPTIONS[:discretisation]
    @design_speed_ti = @design_speed * OPTIONS[:discretisation]
    
    @speeds_ti = Array.new(@speeds.size)
    @speeds.each_with_index do |speed, index|
      @speeds_ti[index] = speed * OPTIONS[:discretisation]
    end
    
    @speed_costs_dol = @speed_costs.map{|c| c * OPTIONS[:bunker_cost_per_ton]}
    
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
      \"cost_when_idle_in_dollars_per_time_interval\": #{(@cost_idle_ti * OPTIONS[:bunker_cost_per_ton]).round(4)},
      \"number_of_available_vesseles\": #{@quantity.to_i},
      \"speeds_in_knots\": #{@speeds.map{|s|s.round(4)}.to_json},
      \"speeds_in_miles_per_time_interval\": #{@speeds_ti.map{|s|s.round(4)}.to_json},
      \"speed_costs_in_tons_of_bunker_per_mile\": #{@speed_costs.map{|s|s.round(4)}.to_json},
      \"speed_costs_in_dollars_per_mile\": #{@speed_costs_dol.map{|s|s.round(4)}.to_json},
      \"speed_costs_in_dollars_per_time_interval\": #{@speed_costs_dol_ti.map{|s|s.round(4)}.to_json}
    }"
  end
end

requests = Array.new

File.open("../data/linerlib/Demand_#{OPTIONS[:scenario]}.csv").each_with_index do |line, index|
  requests << Request.new(*(line.split("\t")[0..3])) unless index == 0
end

ports = Array.new

requests.each do |r|
  unless ports.collect(&:name).include? r.origin
    File.open("../data/linerlib/ports.csv").each_with_index do |line, index|
      unless index == 0
        name, city, country, cabotage, region, lon, lat, draught, container_cost, transhipment_cost, fee_fixed, fee_variable = line.split("\t")
        if name == r.origin
          ports << Port.new(name, draught, container_cost, fee_fixed, fee_variable)
        end
      end
    end
  end
  
  unless ports.collect(&:name).include? r.destination
    File.open("../data/linerlib/ports.csv").each_with_index do |line, index|
      unless index == 0
        name, city, country, cabotage, region, lon, lat, draught, container_cost, transhipment_cost, fee_fixed, fee_variable = line.split("\t")
        if name == r.destination
          ports << Port.new(name, draught, container_cost, fee_fixed, fee_variable)
        end
      end
    end
  end
end

max_req = 0
min_req = 2 ** (0.size * 8 - 2) - 1

requests.each do |r|
  dest_port = ports.find { |p| p.name == r.destination }
  orig_port = ports.find { |p| p.name == r.origin }
  
  throw "Malformed request!" if dest_port.is_hub == orig_port.is_hub
    
  if orig_port.is_hub
    dest_port.delivery_demand = r.quantity.to_i
    dest_port.delivery_revenue = r.revenue.to_i
  end
  
  if dest_port.is_hub
    orig_port.pickup_demand = r.quantity.to_i
    orig_port.pickup_revenue = r.revenue.to_i
  end
  
  if r.quantity < min_req
    min_req = r.quantity
  end
  
  if r.quantity > max_req
    max_req = r.quantity
  end
end

vessel_classes = Array.new
vessel_class_names_and_quantities = Hash.new

File.open("../data/linerlib/fleet_#{OPTIONS[:scenario]}.csv").each_with_index do |line, index|
  vessel_class_names_and_quantities.store(*line.split("\t")) unless index == 0
end

max_capacity = 0

File.open("../data/linerlib/fleet_data.csv").each_with_index do |line, index|
  unless index == 0
    vessel_class = VesselClass.new(*(line.split("\t")[0..8]))
    
    if vessel_class_names_and_quantities.include? vessel_class.name
      vessel_class.quantity = vessel_class_names_and_quantities[vessel_class.name]
      if vessel_class.capacity > max_capacity
        max_capacity = vessel_class.capacity
      end
      vessel_classes << vessel_class
    end
  end
end

distances = Array.new(ports.size) {Array.new(ports.size) {0.0}}
port_names = ports.collect(&:name)

File.open("../data/linerlib/dist_dense.csv").each_with_index do |line, index|
  unless index == 0
    from, to, distance, draught, panama, suez = line.split("\t")
    if port_names.include? from and port_names.include? to
      distances[port_names.index(from)][port_names.index(to)] = distance.to_f
    end
  end
end

ports.each_with_index do |p, port_index|
  p.allowed_vc = Array.new(vessel_classes.size)
  p.call_fee_vc = Array.new(vessel_classes.size)
  
  p.pickup_demand ||= 0
  p.delivery_demand ||= 0
    
  p.pickup_demand = [p.pickup_demand, max_capacity].min
  p.delivery_demand = [p.delivery_demand, max_capacity].min
  
  vessel_classes.each_with_index do |vc, index|
    p.allowed_vc[index] = (vc.draught <= p.draught)
    p.call_fee_vc[index] = p.call_fee_fixed + p.call_fee_variable * vc.capacity
  end
  
  unless p.is_hub 
    p.pickup_handling = (((p.pickup_demand - min_req) * (OPTIONS[:max_handling] - OPTIONS[:min_handling])).to_f / (max_req - min_req).to_f + OPTIONS[:min_handling]).round
    p.delivery_handling = (((p.delivery_demand - min_req) * (OPTIONS[:max_handling] - OPTIONS[:min_handling])).to_f / (max_req - min_req).to_f + OPTIONS[:min_handling]).round
  end
  
  p.distances = distances[port_index]
  
  if p.is_hub or !OPTIONS[:tw]
    p.num_tw = 0
    p.tw_start = []
    p.tw_end = []
  else
    num_days_in_time_horizon = 7 * OPTIONS[:weeks] - OPTIONS[:time_intervals_at_hub] / TIME_INTERVALS_PER_DAY
    first_ti_that_models_0am_to_2am = 0
    
    time_window_centres = Array.new
    num_days_in_time_horizon.times do |i|
      time_window_centres << first_ti_that_models_0am_to_2am + i * TIME_INTERVALS_PER_DAY
    end
    
    tw_width = OPTIONS[:min_tw] + rand(OPTIONS[:max_tw] - OPTIONS[:min_tw] + 1)
    
    p.num_tw = time_window_centres.size + 1
    p.tw_start = Array.new(p.num_tw)
    p.tw_end = Array.new(p.num_tw)
    
    # First time window
    p.tw_start[0] = 0
    p.tw_end[0] = tw_width
    if tw_width > 0
      p.tw_start[p.tw_start.size - 1] = TIME_INTERVALS_IN_TIME_HORIZON - tw_width
      p.tw_end[p.tw_end.size - 1] = TIME_INTERVALS_IN_TIME_HORIZON - 1
    end
    
    # Other time windows
    1.upto(p.num_tw - 2) do |n|
      p.tw_start[n] = time_window_centres[n] - tw_width
      p.tw_end[n] = time_window_centres[n] + tw_width
    end
  end
  
  if p.is_hub or !OPTIONS[:transfer]
    p.pickup_transfer = TIME_INTERVALS_IN_TIME_HORIZON
    p.delivery_transfer = TIME_INTERVALS_IN_TIME_HORIZON
  else
    p.pickup_transfer = OPTIONS[:min_transfer] + rand(OPTIONS[:max_transfer] - OPTIONS[:min_transfer] + 1)
    p.delivery_transfer = OPTIONS[:min_transfer] + rand(OPTIONS[:max_transfer] - OPTIONS[:min_transfer] + 1)
  end
end

data = Hash.new

data[:num_ports] = ports.size
data[:num_vessel_classes] = vessel_classes.size
data[:num_time_intervals] = TIME_INTERVALS_IN_TIME_HORIZON
data[:time_intervals] = TIME_INTERVALS_IN_TIME_HORIZON.times.to_a
data[:ports] = ports
data[:vessel_classes] = vessel_classes
data[:distances] = distances

puts JSON.pretty_generate data