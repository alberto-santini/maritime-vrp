require 'optparse'

class Parser
  attr_accessor :options

  def initialize
    @options = Hash.new

    OptionParser.new do |opts|
      # Linerlib scenario: Baltic, WAF, ...
      opts.on "--scenario=MANDATORY" do |s|
        @options[:scenario] = s
      end

      # Hub port UNLO-code, e.g. DEBRV for Baltic
      opts.on "--hub=MANDATORY" do |h|
        @options[:hub_name] = h
      end

      # Time discretisation: 1 time interval = ? hours
      opts.on "--discretisation=MANDATORY" do |d|
        @options[:discretisation] = d.to_i
      end

      # Number of weeks in the time horizon
      opts.on "--weeks=MANDATORY" do |w|
        @options[:weeks] = w.to_i
      end

      # Number of t.i. for handling at the port with max demand
      opts.on "--max-handling=MANDATORY" do |m|
        @options[:max_handling] = m.to_i
      end

      # Number of t.i. for handling at the port with min demand
      opts.on "--min-handling=MANDATORY" do |m|
        @options[:min_handling] = m.to_i
      end

      # Number of t.i. the ship spends at the hub -- for example:
      # If the time discretisation is of 2 hours and the ship spends 1 day
      # at the hub, this parameter will be 12; this would mean that our time
      # horizon goes from Monday 00:00 to Sunday 00:00, while the whole
      # sunday is not modelled, as it is the time the ship spends at the hub
      opts.on "--time-spent-at-hub=MANDATORY" do |t|
        @options[:time_intervals_at_hub] = t.to_i
      end

      # Number of speeds to generate
      opts.on "--speeds=MANDATORY" do |s|
        @options[:speeds_n] = s.to_i
      end

      # Price in dollar per metric tonne of bunker
      opts.on "--bunker-price=MANDATORY" do |p|
        @options[:bunker_cost_per_ton] = p.to_f
      end

      # Enable closing time windows?
      opts.on "--tw=MANDATORY" do |t|
        @options[:tw] = (t == "true")
      end

      # Min width of a half-time-window, e.g. if this number
      # is n, the min time window will be large 1 + 2 * n
      opts.on "--min-tw=MANDATORY" do |m|
        @options[:min_tw] = m.to_i
      end

      # Max width of a half-time-window, e.g. if this number
      # is n, the max time window will be large 1 + 2 * n
      opts.on "--max-tw=MANDATORY" do |m|
        @options[:max_tw] = m.to_i
      end

      # Enable transfer times?
      opts.on "--transfer=MANDATORY" do |t|
        @options[:transfer] = (t == "true")
      end

      # Min number of t.i. for min transfer time
      opts.on "--min-transfer=MANDATORY" do |m|
        @options[:min_transfer] = m.to_i
      end

      # Max number of t.i. for max transfer time
      opts.on "--max-transfer=MANDATORY" do |m|
        @options[:max_transfer] = m.to_i
      end

      # Penalty coefficient: if cargo is not picked up, the ship
      # operator pays this number * the lost profit, as a penalty
      opts.on "--penalty-coefficient=MANDATORY" do |p|
        @options[:penalty_coefficient] = p.to_f
      end

      # Demand coefficient: multiply the demand by this number
      opts.on "--demand-coefficient=MANDATORY" do |d|
        @options[:demand_coefficient] = d.to_f
      end
    end.parse!

    @options[:time_intervals_per_day] = (24 / @options[:discretisation]).to_i
    @options[:time_intervals_in_time_horizon] = ((@options[:weeks] * 7 * @options[:time_intervals_per_day]) - @options[:time_intervals_at_hub]).to_i
  end
end
