#!/bin/bash

CONFIG_FILE="config.json"

# Parameter sets
bandwidths=(0.01 0.1 1 10 100)
latencies=(20 50 80)
failures=(0.2 0.5 0.8)

for n in $(seq 3 15); do
    min_edges=$((n - 1))
    max_edges=$((n * (n - 1) / 2))

    # Decide number of samples (3 or 4)
    num_samples=4

    # Generate uniformly spaced edge values
    edge_values=()
    if [ $num_samples -eq 1 ]; then
        edge_values=($min_edges)
    else
        step=$(( (max_edges - min_edges) / (num_samples - 1) ))
        for ((i=0; i<num_samples; i++)); do
            edge=$((min_edges + i * step))
            edge_values+=($edge)
        done
    fi

    for edges in "${edge_values[@]}"; do
        for bw in "${bandwidths[@]}"; do
            for lat in "${latencies[@]}"; do
                for fail in "${failures[@]}"; do

                    # Update config.json using jq
                    jq \
                        --argjson num_nodes "$n" \
                        --argjson num_edges "$edges" \
                        --argjson bandwidth "$bw" \
                        --argjson latency "$lat" \
                        --argjson failure_rate "$fail" \
                        '.num_nodes=$num_nodes |
                         .num_edges=$num_edges |
                         .bandwidth=$bandwidth |
                         .latency=$latency |
                         .failure_rate=$failure_rate' \
                        "$CONFIG_FILE" > tmp.json && mv tmp.json "$CONFIG_FILE"

                    echo "Running: n=$n edges=$edges bw=$bw lat=$lat fail=$fail"

                    # Run your simulation script
                    bash run.sh

                done
            done
        done
    done
done