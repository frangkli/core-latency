import os.path
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

# Helper Functions
def saveHeatmap(parsed_data, op):
    df = pd.Series(parsed_data).unstack()
    plt.figure(figsize=(25, 15))
    heatmap_plot = sns.heatmap(df, annot=True, fmt="d", cmap="Blues", annot_kws={"fontsize":28})
    heatmap_plot.tick_params(axis='both', which='major', labelbottom=False, bottom=False, top=False, labeltop=True)
    heatmap_plot.tick_params(length=0, labelsize=28)
    heatmap_plot.set_title(f"{op.title()} Latency (Nanoseconds) Per CPU Core Pair\n", fontsize=32)
    heatmap_plot.figure.axes[-1].tick_params(labelsize=28)
    fig = heatmap_plot.get_figure()
    fig.savefig(f"figures/cco_{op}_table.png")

def parseData(op):
    csvfile = open(f"data/cco_{op}_results.csv")
    header = csvfile.readline();
    parsed_data = {}
    for line in csvfile:
        line_no_endl = line.rstrip("\n")
        line_data = [int(x) for x in line_no_endl.split(",")]
        assert len(line_data) == 3, "Malformed results file."
        parsed_data[(line_data[0], line_data[1])] = line_data[2]
    return parsed_data

if os.path.isfile("data/cco_read_results.csv"):
    saveHeatmap(parseData("read"), "read")
if os.path.isfile("data/cco_write_results.csv"):
    saveHeatmap(parseData("write"), "write")