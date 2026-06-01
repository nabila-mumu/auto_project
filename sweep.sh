#!/bin/bash

CONFIG_FILE="config.json"

# Parameter sets
bandwidths=(0.01 0.1 1 10 100)
latencies=(20 50 80)
failures=(0.8 0.5 0.2 0.1 0.05 0.02 0.01) 

for n in $(seq 4 50); do
    min_edges=$((n - 1))
    max_edges=$((n * (n - 1) / 2))

    desired_samples=10
    total_possible=$((max_edges - min_edges + 1))

    edge_values=()

    # If possible edge count <= 10 → use all possible values
    if [ $total_possible -le $desired_samples ]; then

        for ((edge=min_edges; edge<=max_edges; edge++)); do
            edge_values+=($edge)
        done

    else
        # Uniformly distribute 10 integer samples
        for ((i=0; i<desired_samples; i++)); do

            edge=$(
                awk "BEGIN {
                    print int($min_edges + \
                    ($i * ($max_edges-$min_edges))/($desired_samples-1) + 0.5)
                }"
            )

            edge_values+=($edge)

        done

        # remove duplicates caused by rounding
        edge_values=($(printf "%s\n" "${edge_values[@]}" | sort -nu))
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