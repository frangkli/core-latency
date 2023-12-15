import matplotlib.pyplot as plt;

# Parse tlb_results.csv
csvfile = open("data/tlb_results.csv")

parsed_data = []
for line in csvfile:
    line_no_endl = line.rstrip("\n")
    line_data = int(line);
    parsed_data.append(line_data)

# Visualize
unique_values = []
for point in parsed_data:
    if point not in unique_values:
        unique_values.append(point);
plt.plot(parsed_data)
plt.title("Output Latency Per Element")
plt.xlabel("Element Index")
plt.ylabel("Output Latency (Nanoseconds)")
plt.savefig("figures/tlb_graph.png")