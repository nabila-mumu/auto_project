import json
import random
import os

# ===============================
# Load config
# ===============================
def load_config(path="config.json"):
    with open(path, "r") as f:
        return json.load(f)

# ===============================
# Generate CONNECTED random graph
# ===============================
def generate_connected_graph(n, m):
    edges = set()

    # Step 1: make a spanning tree (ensures connectivity)
    nodes = list(range(n))
    random.shuffle(nodes)

    for i in range(1, n):
        u = nodes[i]
        v = random.choice(nodes[:i])
        edges.add((min(u, v), max(u, v)))

    # Step 2: add remaining random edges
    possible_edges = [(i, j) for i in range(n) for j in range(i+1, n)]

    while len(edges) < m:
        e = random.choice(possible_edges)
        edges.add(e)

    return list(edges)

# ===============================
# Write graph.txt
# ===============================
def write_graph_txt(edges, bandwidth, latency, filename="graph.txt"):
    with open(filename, "w") as f:
        for u, v in edges:
            f.write(f"{u} {v} {bandwidth} {latency}\n")

# ===============================
# Generate RingNode.ned
# ===============================
def generate_ring_ned(n):
    lines = []
    lines.append("package src;\n")
    lines.append("simple RingNode\n{\n")
    lines.append("    parameters:\n")
    lines.append("        int nodeId;\n")
    lines.append('        @display("i=block/routing");\n')
    lines.append("    gates:\n")
    lines.append("        input inGate[];\n")
    lines.append("        output outGate[];\n")
    lines.append("}\n\n")

    lines.append("network RingNetwork\n{\n")
    lines.append(f"    parameters:\n        int numNodes = default({n});\n\n")
    lines.append("    submodules:\n")
    lines.append("        node[numNodes]: RingNode {\n")
    lines.append("            parameters:\n")
    lines.append("                nodeId = index;\n")
    lines.append("        }\n\n")

    lines.append("    connections allowunconnected:\n")

    for i in range(n):
        j = (i + 1) % n
        lines.append(f"    node[{i}].outGate++ --> LossyChannel --> node[{j}].inGate++;\n")

    lines.append("}\n")

    with open("/home/nabila/omnetpp-6.3.0/samples/auto_project/src/RingNode.ned", "w") as f:
        f.writelines(lines)

# ===============================
# Generate Channels.ned
# ===============================
def generate_channels_ned():
    lines = []
    lines.append("package src;\n\n")
    lines.append("channel LossyChannel extends ned.DatarateChannel\n{\n")
    lines.append("    parameters:\n")
    lines.append("        double lossRate = default(0.2);\n")
    lines.append("        delay = uniform(5ms, 20ms);\n")
    lines.append("        datarate = 1Mbps;\n")
    lines.append("}\n")

    with open("/home/nabila/omnetpp-6.3.0/samples/auto_project/src/Channels.ned", "w") as f:
        f.writelines(lines)


# ===============================
# Generate BullyNode.ned
# ===============================
def generate_bully_ned(n, edges):
    lines = []
    lines.append("package src;\n\n")

    lines.append("simple BullyNode\n{\n")
    lines.append("    parameters:\n")
    lines.append("        int nodeId;\n")
    lines.append("        double crashTime @unit(s) = default(-1s);\n")
    lines.append("        double electionTimeout @unit(s) = default(0.5s);\n")
    lines.append("        double stopDelay @unit(s) = default(1s);\n")
    lines.append('        @display("i=block/routing");\n')
    lines.append("    gates:\n")
    lines.append("        input inGate[];\n")
    lines.append("        output outGate[];\n")
    lines.append("}\n\n")

    lines.append("network BullyNetwork\n{\n")
    lines.append(f"    parameters:\n        int numNodes = default({n});\n")

    lines.append("    submodules:\n")
    lines.append("        node[numNodes]: BullyNode {\n")
    lines.append("            parameters:\n")
    lines.append("                nodeId = index;\n")
    lines.append("        }\n\n")

    lines.append("    connections allowunconnected:\n\n")

    # Add bidirectional edges
    for u, v in edges:
        lines.append(f"    node[{u}].outGate++ --> LossyChannel --> node[{v}].inGate++;\n")
        lines.append(f"    node[{v}].outGate++ --> LossyChannel --> node[{u}].inGate++;\n\n")

    lines.append("}\n")

    with open("/home/nabila/omnetpp-6.3.0/samples/auto_project/src/BullyNode.ned", "w") as f:
        f.writelines(lines)

    

# ===============================
# Generate omnetpp.ini
# ===============================
def generate_ini(n, failure_rate):
    lines = []
    lines.append("[General]\n")
    lines.append("network = src.BullyNetwork\n")
    lines.append("# sim-time-limit = 1000s\n\n")

    lines.append("# Node failure configuration\n")
    for i in range(n):
        mean_time = 1 / failure_rate
        lines.append(f"*.node[{i}].crashTime = exponential({mean_time}s)\n")

    lines.append("# Optional (for terminal mode)\n")
    lines.append("cmdenv-express-mode = false\n")
    lines.append("**.allowunconnected = true\n\n")
    lines.append("# cmdenv-log-level = info\n")
    lines.append("**.cmdenv-log-level = debug\n")
    with open("/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/omnetpp.ini", "w") as f:
        f.writelines(lines)

# ===============================
# MAIN
# ===============================
def main():
    config = load_config()

    n = config["num_nodes"]
    m = config["num_edges"]
    bandwidth = config["bandwidth"]
    latency = config["latency"]
    failure_rate = config["failure_rate"]

    edges = generate_connected_graph(n, m)

    write_graph_txt(edges, bandwidth, latency)
    generate_ring_ned(n)
    generate_bully_ned(n, edges)
    generate_ini(n, failure_rate)

    print("✅ Files generated:")
    print("- graph.txt")
    print("- RingNode.ned")
    print("- BullyNode.ned")
    print("- omnetpp.ini")

if __name__ == "__main__":
    main()