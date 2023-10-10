import numpy as np
from matplotlib import pyplot as plt
import argparse
import json
import re
from statistics import mean, stdev, median, geometric_mean

plt.rcParams["text.usetex"] = True

# python3 ./bench/counts.py -o ./bench/counts.pdf ./bench/data/sapphire/counts.json "T3L|inte|mat|nq|fib"


def stat(x):
    err = stdev(x) / np.sqrt(len(x)) if len(x) > 1 else 0

    # print(x)

    return median(x), err, min(x)


# Parse the input file and benchmark name and optional output file.
parser = argparse.ArgumentParser()
parser.add_argument("input_file", help="Input file")
parser.add_argument("regex", help="regex to match benchmark names")
parser.add_argument("-o", "--output_file", help="Output file")
parser.add_argument("-n", "--nowa_root", help="nowa data root")
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

    name = bench["name"].split("/")

    name = [n for n in name if n != "real_time" and not n.isnumeric()]

    name = "".join(name)

    # Check name matches regex
    if not regex.search(name):
        continue

    if "steal" not in bench:
        continue

    if name not in benchmarks:
        benchmarks[name] = {}

    num_threads = int(bench["green_threads"] + 0.5)

    if num_threads not in benchmarks[name]:
        benchmarks[name][num_threads] = []

    count = bench["alloc"] / bench["iterations"]

    benchmarks[name][num_threads].append(count)


benchmarks = [(k, sorted(v.items())) for k, v in benchmarks.items()]

benchmarks.sort()

# find the serial benchmark

fig, (ax) = plt.subplots(1, figsize=(5, 4))

xmax = 0

for k, v in benchmarks:
    #

    if "fan" in k or "busy" in k:
        continue

    label = k.split("_")[0]

    if label == "uts":
        label = f"\\textit{{uts-T3L}}"
    else:
        label = f"\\textit{{{label}}}"

    x = np.asarray([t[0] for t in v])
    y, err, mi = map(np.asarray, zip(*[stat(d[1]) for d in v]))

    xmax = max(xmax, max(x))

    # --------------- #

    m, c = np.polyfit(x, y, 1)
    print(f"{label:>40}: {m}")

    # --------------- #

    stacks = y + 8 * x
    mem = stacks * 1
    err = err

    ax.errorbar(x, mem / x, yerr=err / x, label=label, capsize=2)

    # break


# print(f"ymax: {ymax}")

ax.set_xticks(range(0, int(xmax + 1.5), 14))


# Set log y axis
# ax.set_yscale("log")


ax.legend(loc="best")

ax.set_xlabel("Number of threads")
ax.set_ylabel("Async-stacks per thread")


fig.tight_layout()

if args.output_file is not None:
    plt.savefig(args.output_file)
