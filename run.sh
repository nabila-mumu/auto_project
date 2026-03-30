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
printf "%s,%s,%s,%s,%s,%s,%s,%s," \
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

echo "====================================="
echo "Step 6: Run BullyNetwork"
echo "====================================="

sed -i 's/^network[[:space:]]*=.*/network = src.BullyNetwork/' simulations/omnetpp.ini
./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini

echo "====================================="
echo "Step 7: Switch to RingNetwork"
echo "====================================="

sed -i 's/^network[[:space:]]*=.*/network = src.RingNetwork/' simulations/omnetpp.ini

echo "====================================="
echo "Step 8: Run RingNetwork"
echo "====================================="

./auto_project -u Cmdenv -n . -f simulations/omnetpp.ini

echo "====================================="
echo "Done ✅"
echo "====================================="