#!/bin/bash

scenario="WAF"
hub="ESALG"
discretisation=8
weeks=4
at_hub=2

min_handling=(
  1
  1
)
max_handling=(
  1
  2
)

min_tr=69
max_tr=82

bunker_price=(
  250
  375
  500
)
penalty=(
  0
  1
  5
  100
)

min_tw=(
  0
)
max_tw=(
  0
)

for hdl_id in "${!min_handling[@]}"
do

  for bnk in "${bunker_price[@]}"
  do
    
    for pen in "${penalty[@]}"
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
        
        ./data_generator.rb \
          --scenario="${scenario}" \
          --hub="${hub}" \
          --discretisation="${discretisation}" \
          --weeks="${weeks}" \
          --time-spent-at-hub="${at_hub}" \
          --min-handling="${min_handling[hdl_id]}" \
          --max-handling="${max_handling[hdl_id]}" \
          --bunker-price="${bnk}" \
          --tw=false \
          --min-tw=0 \
          --max-tw=0 \
          --transfer="${trn}" \
          --min-transfer="${min_tr}" \
          --max-transfer="${max_tr}" \
          --penalty-coefficient="${pen}" > "../data/new/${scenario}_${min_handling[hdl_id]}_${max_handling[hdl_id]}_${bnk}_${pen}_no_no_${trn_str}.json"

        for mtiw in "${min_tw[@]}"
        do

          for Mtiw in "${max_tw[@]}"
          do

            ./data_generator.rb \
              --scenario="${scenario}" \
              --hub="${hub}" \
              --discretisation="${discretisation}" \
              --weeks="${weeks}" \
              --time-spent-at-hub="${at_hub}" \
              --min-handling="${min_handling[hdl_id]}" \
              --max-handling="${max_handling[hdl_id]}" \
              --bunker-price="${bnk}" \
              --tw=true \
              --min-tw="${mtiw}" \
              --max-tw="${Mtiw}" \
              --transfer="${trn}" \
              --min-transfer="${min_tr}" \
              --max-transfer="${max_tr}" \
              --penalty-coefficient="${pen}" > "../data/new/${scenario}_${min_handling[hdl_id]}_${max_handling[hdl_id]}_${bnk}_${pen}_${mtiw}_${Mtiw}_${trn_str}.json"
            
          done
          
        done
  
      done

    done

  done

done