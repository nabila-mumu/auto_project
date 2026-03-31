import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv("dataset.csv")

# Skip rows where bully_time == 0
df = df[df["bully_time"] != 0]

# -------------------------------
# 1. Plot: Compare FAILURE RATE
# (same failure rate → one line)
# -------------------------------
plt.figure(figsize=(10, 6))

for fr in sorted(df["failure_rate"].unique()):
    subset = df[df["failure_rate"] == fr]

    # Plot ALL points (no averaging)
    plt.scatter(subset["num_nodes"], subset["bully_time"], label=f"FR={fr}", alpha=0.6)

plt.xlabel("Number of Nodes")
plt.ylabel("Bully Time")
plt.title("Bully Time vs Nodes (Grouped by Failure Rate)")
plt.legend()
plt.grid(True)
plt.show()


# -------------------------------
# 2. Plot: Compare BANDWIDTH
# -------------------------------
plt.figure(figsize=(10, 6))

for bw in sorted(df["bandwidth"].unique()):
    subset = df[df["bandwidth"] == bw]
    plt.scatter(subset["num_nodes"], subset["bully_time"], label=f"BW={bw}", alpha=0.6)

plt.xlabel("Number of Nodes")
plt.ylabel("Bully Time")
plt.title("Bully Time vs Nodes (Grouped by Bandwidth)")
plt.legend()
plt.grid(True)
plt.show()


# -------------------------------
# 3. Plot: Compare LATENCY
# -------------------------------
plt.figure(figsize=(10, 6))

for lat in sorted(df["latency"].unique()):
    subset = df[df["latency"] == lat]
    plt.scatter(subset["num_nodes"], subset["bully_time"], label=f"LAT={lat}", alpha=0.6)

plt.xlabel("Number of Nodes")
plt.ylabel("Bully Time")
plt.title("Bully Time vs Nodes (Grouped by Latency)")
plt.legend()
plt.grid(True)
plt.show()


# -------------------------------
# 4. Plot: Compare GRAPH DENSITY
# -------------------------------
plt.figure(figsize=(10, 6))

for gd in sorted(df["graph_density"].unique()):
    subset = df[df["graph_density"] == gd]
    plt.scatter(subset["num_nodes"], subset["bully_time"], label=f"Density={gd:.2f}", alpha=0.6)

plt.xlabel("Number of Nodes")
plt.ylabel("Bully Time")
plt.title("Bully Time vs Nodes (Grouped by Graph Density)")
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')  # avoid clutter
plt.grid(True)
plt.show()