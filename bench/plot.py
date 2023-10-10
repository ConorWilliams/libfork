import numpy as np
from matplotlib import pyplot as plt
import argparse
import json
import re
from statistics import mean, stdev, median, geometric_mean


'''

cmake --build --preset=rel && ./build/rel/bench/benchmark  --benchmark_time_unit=ms --benchmark_filter="matmul" --benchmark_out_format=json --benchmark_out=bench/data/laptop/matmul.json --benchmark_repetitions=5

'''


def stat(x):
     
     x = sorted(x)[:-1] # remove outliers

     err = stdev(x) / np.sqrt(len(x)) if len(x) > 1 else 0

     return geometric_mean(x), err, min(x)


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
    if(not regex.search(name)): 
        continue

    if(name not in benchmarks):
        benchmarks[name] = {}

    num_threads = int(bench["green_threads"] + 0.5)

    if(num_threads not in benchmarks[name]):
        benchmarks[name][num_threads] = []

    benchmarks[name][num_threads].append(bench["real_time"])

# ----------Parse nowa ----------- #
 
if args.nowa_root is not None:
    for name in ["fib", "integrate", "matmul", "nqueens"]:
        
        data = np.loadtxt(f"{args.nowa_root}/nowa/{name}.csv", skiprows=1)

        if(not regex.search(name)): 
            continue

        name = f"{name}_nowa"

        for row in data:
            num_threads = int(row[0])
            time = row[2]

            if(name not in benchmarks):
                benchmarks[name] = {}

            if(num_threads not in benchmarks[name]):
                benchmarks[name][num_threads] = []

            benchmarks[name][num_threads].append(time*1000)

        # print(data)

benchmarks = [(k, sorted(v.items())) for k, v in benchmarks.items()]

# print(benchmarks)
      
benchmarks.sort()

    
# find the serial benchmark

tS = -1
tSerr = -1

for k, v in benchmarks:
    if ("serial" in k):
        tS, tSerr, _ = stat(v[0][1])
        break

# Plot the results

fig, (ax_rel, ax_abs) = plt.subplots(1, 2, figsize=(12, 6))

fig.suptitle(args.input_file)

ymax = 0
xmax = 0

for k, v in benchmarks:

    label = "_".join(k.split("_")[1:])

    if (label.startswith("serial")):
        continue;
    
    # if label.startswith("lib"):
    #     label = f"{label[8:8+4]}:{label[-4:-1]}"

    x = np.asarray([t[0] for t in v])
    y, err, mi = map(np.asarray, zip(*[stat(d[1]) for d in v]))

    xmax = max(xmax, max(x))

    # --------------- #

    m, c = np.polyfit(x, y[0] / mi, 1)
    print(f"{label:>40}: {m}")

    # --------------- #

    t =  y[0] / y
    ferr = err / y
    terr = t * np.sqrt((ferr ** 2 + ferr[0]**2))

    ax_rel.errorbar(x, t, yerr=terr, label=label.capitalize(), capsize=2)
    # ax_rel.plot(x, y[0] / mi, "kx")

    # --------------- #

    if tS < 0:
        continue

    Y, Yerr = (tS, tSerr) if tS > 0 else (1, 0)

    t = Y / y
    ferr = err / y
    terr = t * np.sqrt((ferr ** 2 + (Yerr/ Y)**2))
    terr[0] = 0
   
    ax_abs.errorbar(x, t, yerr=terr, label=label.capitalize(), capsize=2)

    ymax = max(ymax, max(t))

print(f"ymax: {ymax}")
print(f"xmax: {xmax}")

ideal_abs = range(1, int(xmax+1.5))
ideal_rel = range(1, int(xmax+1.5))
    
ax_abs.plot(ideal_abs, ideal_abs, color="black", linestyle="dashed", label="Ideal")
ax_rel.plot(ideal_rel, ideal_rel, color="black", linestyle="dashed", label="Ideal")

ax_rel.set_yticks(range(0, int(xmax+1.5), 14))
ax_rel.set_xticks(range(0, int(xmax+1.5), 14))



# Set log y axis
# ax_abs.set_yscale('log')


ax_rel.legend(loc="best")

ax_abs.set_title("Absolute")
ax_abs.set_xlabel("Number of threads")
ax_abs.set_ylabel("Speedup (Ts / Tn)")
ax_abs.set_yscale('log', base=2)
ax_abs.set_xscale('log', base=2)

# plt.gca().set_xscale('log', basex=2)

ax_rel.set_title("Relative")
ax_rel.set_xlabel("Number of threads")
ax_rel.set_ylabel("Speedup (T1 / Tn)")

# ax_abs.set_xlim(0, 33)
# ax_rel.set_xlim(0, 33)

fig.tight_layout()

if args.output_file is not None:
    plt.savefig(args.output_file)








