#!/bin/bash

compute_product() {
    local r1=$1
    local r2=$2
    local r3=$3
    local x=$4
    local total_ratio=$((r1 + r2 + r3))

    if [[ $total_ratio -eq 0 ]]; then
        echo "Error: Total ratio is zero!"
        return 0
    fi

    local a=$(( (x * r1) / total_ratio ))
    local b=$(( (x * r2) / total_ratio ))
    local c=$(( (x * r3) / total_ratio ))

    echo $((a * b * c))
}

find_best_ratio() {
    local x=$1
    local max_product=0
    local best_ratio=""
    local best_values=""

    declare -a ratios=("1 1 1" "2 2 1" "4 3 2" "3 2 2")
    max_p=$(perl -E "say int($x**(1/3) + 0.5)")

    echo $max_p

    for ratio in "${ratios[@]}"; do
    for((a=0;a<max_p;a++)); do
            read -r r1 r2 r3 <<< "$ratio"
            echo $r1 $r2 $r3 $a
            product=$(compute_product $r1 $r2 $r3 $x)
            if [[ $product -le $x && $product -gt $max_product ]]; then
                max_product=$product
                best_ratio="$r1:$r2:$r3"
                total_ratio=$((r1 + r2 + r3))
                a=$(( (x * r1) / total_ratio ))
                b=$(( (x * r2) / total_ratio ))
                c=$(( (x * r3) / total_ratio ))
                best_values="$a $b $c"
            fi
        fi
    done

    echo "Best ratio: $best_ratio"
    echo "Numbers: $best_values"
}

find_best_ratio $1

