import numpy as np
from matplotlib import pyplot as plt
import argparse
import json
import re
from statistics import mean, stdev


def stat(x):
    # Take the mean and standard deviation of the best 80% of the data
    x.sort()
    n = len(x)
    x = x[0:int(n*0.8)]
    assert(len(x) > 3)
    return mean(x), stdev(x)



# Parse the input file and benchmark name and optional output file.
parser = argparse.ArgumentParser()
parser.add_argument("input_file", help="Input file")
parser.add_argument("regex", help="regex to match benchmark names")
parser.add_argument("-o", "--output_file", help="Output file")
args = parser.parse_args()

# Compile the regex
regex = re.compile(args.regex)

# Read the input file as json
with open(args.input_file) as f:
    data = json.load(f)

# Build a dictionary of benchmarks

benchmarks = {}

for bench in data["benchmarks"]:

    if bench["run_type"] != "iteration":
        continue

    name = bench["name"].split("/")[0]

    # Check name matches regex
    if(not regex.match(name)):
        continue

    if(name not in benchmarks):
        benchmarks[name] = {}

    num_threads = bench["per_family_instance_index"] + 1

    if(num_threads not in benchmarks[name]):
        benchmarks[name][num_threads] = []

    benchmarks[name][num_threads].append(bench["real_time"])



# find the serial benchmark


for k, v in benchmarks.items():
    if ("serial" in k):
        tS = stat(v[1])[0]
        break



# Plot the results

fig, ax = plt.subplots()

for k, v in benchmarks.items():

    label = "_".join(k.split("_")[1:])

    if (label.startswith("serial")):
        continue;

    x = [t for t in v.keys()]
    y, err = zip(*[stat(d) for d in v.values()])

    x = np.asarray(x)
    y = np.asarray(y)
    err = np.asarray(err)

    ferr = err / y

    y = y[0] / y
    err = y * ferr

    ax.errorbar(x, y, yerr=err, label=label.capitalize(), capsize=2)

fig.legend()

ax.set_xlabel("Number of threads")
ax.set_ylabel("Time/ms")

fig.tight_layout()

if args.output_file is not None:
    plt.savefig(args.output_file)








