#!/bin/bash

set -e

CONFIG="config.json"
GRAPH_FILE="graph.txt"
CSV_FILE="/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv"

echo "====================================="
echo "Step 1: Generate graph"
echo "====================================="

python3 graph_gen.py

echo "====================================="
echo "Step 2: Extract config values"
echo "====================================="

num_nodes=$(jq '.num_nodes' $CONFIG)
num_edges=$(jq '.num_edges' $CONFIG)
bandwidth=$(jq '.bandwidth' $CONFIG)
latency=$(jq '.latency' $CONFIG)
failure_rate=$(jq '.failure_rate' $CONFIG)

echo "====================================="
echo "Step 3: Compute graph metrics"
echo "====================================="

# Graph density = 2E / (N(N-1))
graph_density=$(echo "scale=4; (2*$num_edges)/($num_nodes*($num_nodes-1))" | bc)

# Avg degree = 2E / N
avg_degree=$(echo "scale=4; (2*$num_edges)/$num_nodes" | bc)

# Connected components 
num_components=$(python3 - <<END
from collections import defaultdict, deque

n = $num_nodes
G = defaultdict(list)

for i in range(n):
    G[i] = []

with open("$GRAPH_FILE") as f:
    for line in f:
        u, v, *_ = line.split()
        u, v = int(u), int(v)
        G[u].append(v)
        G[v].append(u)

visited = set()
components = 0

for node in range(n):
    if node not in visited:
        components += 1
        queue = deque([node])
        visited.add(node)

        while queue:
            cur = queue.popleft()
            for nei in G[cur]:
                if nei not in visited:
                    visited.add(nei)
                    queue.append(nei)

print(components)
END
)

echo "====================================="
echo "Step 4: Append to CSV"
echo "====================================="

# echo "$num_nodes,$num_edges,$graph_density,$avg_degree,$num_components,$failure_rate,$bandwidth,$latency," >> $CSV_FILE
printf "\n%s,%s,%s,%s,%s,%s,%s,%s," \
"$num_nodes" "$num_edges" "$graph_density" "$avg_degree" \
"$num_components" "$failure_rate" "$bandwidth" "$latency" >> "$CSV_FILE"

echo "====================================="
echo "Step 5: Build project"
echo "====================================="

make clean
opp_makemake -f --deep -o auto_project
make -j$(nproc)

if [ ! -f "./auto_project" ]; then
    echo "Error: Build failed."
    exit 1
fi

#----------------------------------------------------------

INI_FILE="simulations/omnetpp.ini"
EXEC="./auto_project"
CSV_FILE="/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv"

MAX_RETRY=3
TIME_LIMIT=5s

run_with_retry() {
    local network_name=$1
    local fail_commas=$2   # what to write if all retries fail

    echo "====================================="
    echo "Running $network_name"
    echo "====================================="

    # switch network
    sed -i "s/^network[[:space:]]*=.*/network = src.${network_name}/" $INI_FILE

    attempt=1
    success=0

    while [ $attempt -le $MAX_RETRY ]; do
        echo "Attempt $attempt for $network_name..."

        set +e
        timeout --kill-after=10s $TIME_LIMIT $EXEC -u Cmdenv -n . -f $INI_FILE
        # timeout --signal=KILL 5s $TIME_LIMIT $EXEC -u Cmdenv -n . -f $INI_FILE
        exit_code=$?
        set -e

        # timeout $TIME_LIMIT $EXEC -u Cmdenv -n . -f $INI_FILE
        # exit_code=$?

        if [ $exit_code -eq 0 ]; then
            echo "✅ $network_name succeeded on attempt $attempt"
            success=1
            break
        else
            echo "❌ $network_name failed (timeout or crash)"
        fi

        ((attempt++))
    done

    # If failed all attempts → write commas
    # if [ $success -eq 0 ]; then
    #     echo "⚠️ $network_name failed after $MAX_RETRY attempts"

    #     echo -n "$fail_commas" >> $CSV_FILE
    # fi
    if [ $success -eq 0 ]; then
        echo "⚠️ $network_name failed after $MAX_RETRY attempts"

        # Ensure directory exists
        mkdir -p "$(dirname "$CSV_FILE")"

        # Safe write (disable set -e temporarily)
        set +e
        printf "%s" "$fail_commas" >> "$CSV_FILE"
        write_status=$?
        set -e

        if [ $write_status -ne 0 ]; then
            echo "❌ ERROR: Failed to write to CSV!"
        else
            echo "📝 Wrote failure commas: $fail_commas"
        fi
    fi
}

# STEP 6: BullyNetwork
run_with_retry "BullyNetwork" ",,"

# STEP 7: RingNetwork
run_with_retry "RingNetwork" ",,"

# STEP 8: Bidirectional RingNetwork
./make_bidir.sh
run_with_retry "RingNetwork" ",,"

# STEP 9: RaftNetwork
run_with_retry "RaftNetwork" ","

# echo "====================================="
# echo "Step 6: Switch to and Run BullyNetwork"
# echo "====================================="

# sed -i 's/^network[[:space:]]*=.*/network = src.BullyNetwork/' simulations/omnetpp.ini
# ./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini

# echo "====================================="
# echo "Step 7: Switch to and Run RingNetwork"
# echo "====================================="

# sed -i 's/^network[[:space:]]*=.*/network = src.RingNetwork/' simulations/omnetpp.ini
# ./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini

# echo "====================================="
# echo "Step 8: Switch to and Run Bidirectional RingNetwork"
# echo "====================================="

# ./make_bidir.sh
# ./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini

# echo "====================================="
# echo "Step 9: Switch to and Run RaftNetwork"
# echo "====================================="

# sed -i 's/^network[[:space:]]*=.*/network = src.RaftNetwork/' simulations/omnetpp.ini
# ./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini

echo "====================================="
echo "Done ✅"
echo "====================================="