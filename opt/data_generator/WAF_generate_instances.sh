#!/bin/bash

scenario="WAF"
hub="ESALG"
at_hub=2

iname=$1

# ALL POSSIBLE COMBINATIONS:
# discretisation=(
#   8
# )
# weeks=(
#   4
# )
# min_handling=(
#   1
#   1
# )
# max_handling=(
#   1
#   2
# )
# min_tr=69
# max_tr=82
# speeds=(
#   1
#   3
#   5
# )
# bunker_price=(
#   250
#   300
#   375
#   450
#   500
# )
# penalty=(
#   0
#   -0.01
#   -0.05
#   -0.10
# )
# demand=(
#   1.0
#   0.8
#   0.6
#   0.4
# )
# min_tw=(
#   0
# )
# max_tw=(
#   0
# )

# BASE INSTANCES:
# discretisation=(
#   8
# )
# weeks=(
#   4
# )
# min_handling=(
#   1
#   1
# )
# max_handling=(
#   1
#   2
# )
# min_tr=69
# max_tr=82
# speeds=(
#   3
# )
# bunker_price=(
#   375
# )
# penalty=(
#   0
# )
# demand=(
#   1.0
# )
# min_tw=(
#   0
# )
# max_tw=(
#   0
# )

# BUNKER INSTANCES
# discretisation=(
#   8
# )
# weeks=(
#   4
# )
# min_handling=(
#   1
#   1
# )
# max_handling=(
#   1
#   2
# )
# min_tr=69
# max_tr=82
# speeds=(
#   3
# )
# bunker_price=(
#   250
#   300
#   450
#   500
# )
# penalty=(
#   0
# )
# demand=(
#   1.0
# )
# min_tw=(
#   0
# )
# max_tw=(
#   0
# )

# SPEED INSTANCES
# discretisation=(
#   8
# )
# weeks=(
#   4
# )
# min_handling=(
#   1
#   1
# )
# max_handling=(
#   1
#   2
# )
# min_tr=69
# max_tr=82
# speeds=(
#   1
#   5
# )
# bunker_price=(
#   375
# )
# penalty=(
#   0
# )
# demand=(
#   1.0
# )
# min_tw=(
#   0
# )
# max_tw=(
#   0
# )

# DEMAND INSTANCES
# discretisation=(
#   8
# )
# weeks=(
#   4
# )
# min_handling=(
#   1
#   1
# )
# max_handling=(
#   1
#   2
# )
# min_tr=69
# max_tr=82
# speeds=(
#   3
# )
# bunker_price=(
#   375
# )
# penalty=(
#   0
# )
# demand=(
#   0.8
#   0.6
#   0.4
# )
# min_tw=(
#   0
# )
# max_tw=(
#   0
# )

# PENALTIES INSTANCES
# discretisation=(
#   8
# )
# weeks=(
#   4
# )
# min_handling=(
#   1
#   1
# )
# max_handling=(
#   1
#   2
# )
# min_tr=69
# max_tr=82
# speeds=(
#   3
# )
# bunker_price=(
#   375
# )
# penalty=(
#   -0.01
#   -0.05
#   -0.10
# )
# demand=(
#   1.0
# )
# min_tw=(
#   0
# )
# max_tw=(
#   0
# )

for dsc_id in "${!discretisation[@]}"
do

  for hdl_id in "${!min_handling[@]}"
  do

    for spd in "${speeds[@]}"
    do

      for bnk in "${bunker_price[@]}"
      do

        for pen in "${penalty[@]}"
        do

          for dem in "${demand[@]}"
          do

            for trn in {true,false}
            do

              trn_str=""

              if [[ "$trn" = false ]]
              then
                trn_str="no_no"
              else
                trn_str="${min_tr}_${max_tr}"
              fi

              # Without time windows
              ./data_generator.rb \
                --scenario="${scenario}" \
                --hub="${hub}" \
                --discretisation="${discretisation[dsc_id]}" \
                --weeks="${weeks[dsc_id]}" \
                --time-spent-at-hub="${at_hub}" \
                --min-handling="${min_handling[hdl_id]}" \
                --max-handling="${max_handling[hdl_id]}" \
                --speeds="${spd}" \
                --bunker-price="${bnk}" \
                --tw=false \
                --min-tw=0 \
                --max-tw=0 \
                --transfer="${trn}" \
                --min-transfer="${min_tr}" \
                --max-transfer="${max_tr}" \
                --penalty-coefficient="${pen}" \
                --demand-coefficient="${dem}" > "../../data/new/${iname}/${scenario}_${discretisation[dsc_id]}_${weeks[dsc_id]}_${min_handling[hdl_id]}_${max_handling[hdl_id]}_${spd}_${bnk}_${pen}_${dem}_no_no_${trn_str}.json"

              for mtiw in "${min_tw[@]}"
              do

                for Mtiw in "${max_tw[@]}"
                do

                  # With time windows
                  ./data_generator.rb \
                    --scenario="${scenario}" \
                    --hub="${hub}" \
                    --discretisation="${discretisation[dsc_id]}" \
                    --weeks="${weeks[dsc_id]}" \
                    --time-spent-at-hub="${at_hub}" \
                    --min-handling="${min_handling[hdl_id]}" \
                    --max-handling="${max_handling[hdl_id]}" \
                    --speeds="${spd}" \
                    --bunker-price="${bnk}" \
                    --tw=true \
                    --min-tw="${mtiw}" \
                    --max-tw="${Mtiw}" \
                    --transfer="${trn}" \
                    --min-transfer="${min_tr}" \
                    --max-transfer="${max_tr}" \
                    --penalty-coefficient="${pen}" \
                    --demand-coefficient="${dem}" > "../../data/new/${iname}/${scenario}_${discretisation[dsc_id]}_${weeks[dsc_id]}_${min_handling[hdl_id]}_${max_handling[hdl_id]}_${spd}_${bnk}_${pen}_${dem}_${mtiw}_${Mtiw}_${trn_str}.json"

                done #Mtiw

              done #mtiw

            done #trn

          done #dem

        done #pen

      done #bnk

    done #spd

  done #hdl_id

done #dsc_id
