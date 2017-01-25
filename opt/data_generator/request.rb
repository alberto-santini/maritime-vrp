class Request
  attr_accessor :origin, :destination, :quantity, :unit_revenue

  def initialize(options, origin, destination, quantity, unit_revenue)
    @origin = origin
    @destination = destination
    @quantity = (quantity.to_f * options[:demand_coefficient].to_f).to_i
    @unit_revenue = unit_revenue.to_f
  end

  def revenue
    @unit_revenue * @quantity
  end
end
