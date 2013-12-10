Maritime VRP
============

Intro
-----

This project is part of my Master Thesis. The code has been written from scratch, but often in a hurry and lacks proper documentation. It is a solver for a quite particular case of VRP with:

* Multiple time windows
* Pickups and deliveries
* Homogeneous fleet
* Multiple sailing speeds (and costs)
* Maximum transit times

Todo
----

* `MPIntegerSolution` and `MPLinearSolution` should derive from a same base class `MPSolution`
* DRY `subproblem/sp_solver`, `subproblem/heuristics_solver`, `subproblem/exact_solver`
* `Label` and `ElementaryLabel` should derive from a same base class
* Clean-up `util` as we don't use `util/knapsack` anymore
* DRY the removal of additional arcs in `graph/preprocessing`
* DRY `is_feasible` and `is_integer_feasible` in `branching/bb_node`
* The whole `-DLOG` thing should be replaced by a proper logging system with levels