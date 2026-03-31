input_file = "/home/nabila/omnetpp-6.3.0/samples/auto_project/simulations/results/analysis.csv"
output_file = "dataset.csv"

with open(input_file, "r") as fin, open(output_file, "w") as fout:
    for line in fin:
        if line.count(",") <= 15:
            fout.write(line)

print("Done. Cleaned file saved as cleaned.csv")