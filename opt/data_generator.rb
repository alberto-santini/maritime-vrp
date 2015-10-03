#!/usr/bin/env ruby

require 'json'
require_relative 'parser'
require_relative 'port'
require_relative 'request'
require_relative 'vessel_class'

class DataGenerator
  def initialize
    @parser = Parser.new
    @options = @parser.options
    
    read_requests
    read_ports
    reorder_ports
    calculate_request_data
    read_vessel_classes
    calculate_vessel_class_data
    read_distances
    calculate_port_data
    edit_ports_with_eccess_capacity
    calculate_port_penalties
  end

  def read_requests
    @requests = Array.new

    File.open("../data/linerlib/Demand_#{@options[:scenario]}.csv").each_with_index do |line, index|
      @requests << Request.new(*(line.split("\t")[0..3])) unless index == 0
    end
  end
  
  def read_ports
    @ports = Array.new

    @requests.each do |r|
      unless @ports.collect(&:name).include? r.origin
        File.open("../data/linerlib/ports.csv").each_with_index do |line, index|
          unless index == 0
            name, city, country, cabotage, region, lon, lat, draught, container_cost, transhipment_cost, fee_fixed, fee_variable = line.split("\t")
            if name == r.origin
              @ports << Port.new(@options, name, draught, container_cost, fee_fixed, fee_variable)
            end
          end
        end
      end
  
      unless @ports.collect(&:name).include? r.destination
        File.open("../data/linerlib/ports.csv").each_with_index do |line, index|
          unless index == 0
            name, city, country, cabotage, region, lon, lat, draught, container_cost, transhipment_cost, fee_fixed, fee_variable = line.split("\t")
            if name == r.destination
              @ports << Port.new(@options, name, draught, container_cost, fee_fixed, fee_variable)
            end
          end
        end
      end
    end
  end

  def calculate_request_data
    @max_req = 0
    @min_req = 2 ** (0.size * 8 - 2) - 1

    @requests.each do |r|
      dest_port = @ports.find { |p| p.name == r.destination }
      orig_port = @ports.find { |p| p.name == r.origin }
  
      throw "Malformed request!" if dest_port.is_hub == orig_port.is_hub
    
      if orig_port.is_hub
        dest_port.delivery_demand = r.quantity.to_i
        dest_port.delivery_revenue = r.revenue.to_i
        dest_port.delivery_unit_revenue = r.unit_revenue.to_i
      end
  
      if dest_port.is_hub
        orig_port.pickup_demand = r.quantity.to_i
        orig_port.pickup_revenue = r.revenue.to_i
        orig_port.pickup_unit_revenue = r.unit_revenue.to_i
      end
  
      if r.quantity < @min_req
        @min_req = r.quantity
      end
  
      if r.quantity > @max_req
        @max_req = r.quantity
      end
    end
  end
  
  def read_vessel_classes
    @vessel_class_names_and_quantities = Hash.new

    File.open("../data/linerlib/fleet_#{@options[:scenario]}.csv").each_with_index do |line, index|
      @vessel_class_names_and_quantities.store(*line.split("\t")) unless index == 0
    end
  end

  def calculate_vessel_class_data
    @vessel_classes = Array.new
    @max_capacity = 0

    File.open("../data/linerlib/fleet_data.csv").each_with_index do |line, index|
      unless index == 0
        vessel_class = VesselClass.new(@options, *(line.split("\t")[0..8]))
    
        if @vessel_class_names_and_quantities.include? vessel_class.name
          vessel_class.quantity = [1, @vessel_class_names_and_quantities[vessel_class.name].to_i / @options[:weeks]].max
          if vessel_class.capacity > @max_capacity
            @max_capacity = vessel_class.capacity
          end
          @vessel_classes << vessel_class
        end
      end
    end
  end
  
  def read_distances
    @distances = Array.new(@ports.size) {Array.new(@ports.size) {0.0}}
    port_names = @ports.collect(&:name)

    File.open("../data/linerlib/dist_dense.csv").each_with_index do |line, index|
      unless index == 0
        from, to, distance, draught, panama, suez = line.split("\t")
        if port_names.include? from and port_names.include? to
          @distances[port_names.index(from)][port_names.index(to)] = distance.to_f
        end
      end
    end
  end

  def set_handling_time_for!(p)
    pdem = [p.pickup_demand, @max_capacity].min
    ddem = [p.delivery_demand, @max_capacity].min
    minreq = [@min_req, @max_capacity].min
    maxreq = [@max_req, @max_capacity].min
    p.pickup_handling = (((pdem - minreq) * (@options[:max_handling] - @options[:min_handling])).to_f / (maxreq - minreq).to_f + @options[:min_handling]).round
    p.delivery_handling = (((ddem - minreq) * (@options[:max_handling] - @options[:min_handling])).to_f / (maxreq - minreq).to_f + @options[:min_handling]).round
  end

  def calculate_port_data
    @ports.each_with_index do |p, port_index|
      p.allowed_vc = Array.new(@vessel_classes.size)
      p.call_fee_vc = Array.new(@vessel_classes.size)
  
      p.pickup_demand ||= 0
      p.delivery_demand ||= 0
    
      #p.pickup_demand = [p.pickup_demand, @max_capacity].min
      #p.delivery_demand = [p.delivery_demand, @max_capacity].min
  
      @vessel_classes.each_with_index do |vc, index|
        p.allowed_vc[index] = (vc.draught <= p.draught)
        p.call_fee_vc[index] = p.call_fee_fixed + p.call_fee_variable * vc.capacity
      end
  
      unless p.is_hub
        set_handling_time_for! p
      end
  
      p.distances = Marshal.load(Marshal.dump(@distances[port_index]))
  
      if p.is_hub or !@options[:tw]
        p.num_tw = 0
        p.tw_start = []
        p.tw_end = []
      else
        num_days_in_time_horizon = 7 * @options[:weeks] - @options[:time_intervals_at_hub] / @options[:time_intervals_per_day]
        num_days_in_time_horizon = num_days_in_time_horizon + 1 if @options[:time_intervals_at_hub] % @options[:time_intervals_per_day] == 0
        first_ti_that_models_0am_to_2am = 0
    
        time_window_centres = Array.new
        num_days_in_time_horizon.times do |i|
          time_window_centres << first_ti_that_models_0am_to_2am + i * @options[:time_intervals_per_day]
        end
    
        tw_width = @options[:min_tw] + rand(@options[:max_tw] - @options[:min_tw] + 1)
    
        p.num_tw = time_window_centres.size
        p.tw_start = Array.new(p.num_tw)
        p.tw_end = Array.new(p.num_tw)
    
        # Other time windows
        0.upto(p.num_tw - 1) do |n|
          p.tw_start[n] = [[time_window_centres[n] - tw_width, 0].max, @options[:time_intervals_in_time_horizon]].min
          p.tw_end[n] = [time_window_centres[n] + tw_width, @options[:time_intervals_in_time_horizon]].min
        end
      end
  
      if p.is_hub or !@options[:transfer]
        p.pickup_transfer = @options[:time_intervals_in_time_horizon]
        p.delivery_transfer = @options[:time_intervals_in_time_horizon]
      else
        p.pickup_transfer = @options[:min_transfer] + rand(@options[:max_transfer] - @options[:min_transfer] + 1)
        p.delivery_transfer = @options[:min_transfer] + rand(@options[:max_transfer] - @options[:min_transfer] + 1)
      end
    end
  end
  
  def edit_ports_with_eccess_capacity    
    @ports.each_with_index do |port, index|
      if port.pickup_demand > @max_capacity || port.delivery_demand > @max_capacity
        excess_pickup = [0, port.pickup_demand - @max_capacity].max
        excess_delivery = [0, port.delivery_demand - @max_capacity].max
        
        port.pickup_demand = @max_capacity
        port.delivery_demand = @max_capacity
        port.pickup_revenue = port.pickup_unit_revenue * port.pickup_demand
        port.delivery_revenue = port.delivery_unit_revenue * port.delivery_demand
        
        new_port = Marshal.load(Marshal.dump(port))
        
        new_port.pickup_demand = excess_pickup
        new_port.delivery_demand = excess_delivery
        new_port.pickup_revenue = new_port.pickup_unit_revenue * new_port.pickup_demand
        new_port.delivery_revenue = new_port.delivery_unit_revenue * new_port.delivery_demand
        new_port.name = "#{new_port.name}-2"
        set_handling_time_for! new_port
        new_port.distances << 0.0
        
        @ports.each_with_index do |dport, dindex|
          dport.distances << @distances[dindex][index]
          @distances[dindex] << @distances[dindex][index]
        end
        
        @distances[@distances.size] = Marshal.load(Marshal.dump(new_port.distances))
        
        @ports << new_port
      end
    end
  end
  
  def calculate_port_penalties
    @ports.each do |port|
      port.penalty_if_not_served_pickup = @options[:penalty_coefficient] * port.pickup_revenue.to_f
      port.penalty_if_not_served_delivery = @options[:penalty_coefficient] * port.delivery_revenue.to_f
    end
  end
  
  def reorder_ports
    @ports.sort_by! do |p1, p2|
      p1.is_hub ? -1 : 1
    end
  end
  
  def print_json
    data = Hash.new

    data[:num_ports] = @ports.size
    data[:num_vessel_classes] = @vessel_classes.size
    data[:num_time_intervals] = @options[:time_intervals_in_time_horizon]
    data[:ports] = @ports
    data[:vessel_classes] = @vessel_classes
    data[:distances] = @distances

    puts JSON.pretty_generate data
  end 
end

DataGenerator.new.print_json