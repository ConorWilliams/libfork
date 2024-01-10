import numpy as np
from matplotlib import pyplot as plt
import argparse
import json
import re
from statistics import mean, stdev, median, geometric_mean

plt.rcParams["text.usetex"] = True

"""

cmake --build --preset=rel && ./build/rel/bench/benchmark  --benchmark_time_unit=ms --benchmark_filter="matmul" --benchmark_out_format=json --benchmark_out=bench/data/laptop/matmul.json --benchmark_repetitions=5

"""


# python3 ./bench/xplot.py -o ./bench/test2.pdf


def stat(x):
    x = sorted(x)[:-1]

    err = stdev(x) / (np.sqrt(len(x)) if len(x) > 1 else 0)
    #
    return median(x), err, min(x)


# Parse the input file and benchmark name and optional output file.
parser = argparse.ArgumentParser()
parser.add_argument("-o", "--output_file", help="Output file")
parser.add_argument("-n", "--nowa_root", help="nowa data root")
parser.add_argument("-r", "--rel", help="plot relative speedup", action="store_true")
args = parser.parse_args()

# Build a dictionary of benchmarks

benchmarks = {}

for file in [
    "uts_libfork_alloc",
    "uts_flow",
    "uts_coalloc",
    "uts_omp",
    "uts_tbb",
    "uts_tbb_t3xxl",
]:
    # Read the input file as json
    with open(f"./bench/data/sapphire/v5/csd3.{file}.json") as f:
        #
        data = json.load(f)

        for bench in data["benchmarks"]:
            if bench["run_type"] != "iteration":
                continue

            name = bench["name"].split("/")

            name = [n for n in name if n != "real_time" and not n.isnumeric()]

            name = "".join(name)

            name = name.replace("seq", "fan")

            if name not in benchmarks:
                # print("found", name)
                benchmarks[name] = {}

            num_threads = int(bench["green_threads"] + 0.5)

            if num_threads not in benchmarks[name]:
                benchmarks[name][num_threads] = []

            benchmarks[name][num_threads].append(bench["real_time"])

benchmarks = [(k, sorted(v.items())) for k, v in benchmarks.items()]

benchmarks.sort()

fig, axs = plt.subplots(3, 2, figsize=(6, 7), sharex=True, sharey=True)

count = 0

# patterns = ["T1 ", "T1L", "T1XXL", "T3 ", "T3L", "T3XXL"]

patterns = ["T1 ", "T3 ", "T1L", "T3L", "T1XXL", "T3XXL"]

leg_hi = []
leg_lo = []

for (ax_abs), p in zip(axs.flatten(), patterns):
    # find the serial benchmark

    tS = -1
    tSerr = -1

    for k, v in benchmarks:
        if p not in k:
            continue

        if "serial" in k:
            tS, tSerr, _ = stat(v[0][1])
            break

    # Plot the results

    # fig.suptitle(args.input_file)

    ymax = 0
    xmax = 0

    ymin = 112

    print(p)

    for k, v in benchmarks:
        if p not in k:
            # print(f"skipping {k} wich does not contain {p}")
            continue

        #
        label = "_".join(k.split("_")[1:])

        if label.startswith("serial"):
            continue

        if p == "T3L" and "flow" in label:
            continue

        x = np.asarray([t[0] for t in v])
        y, err, mi = map(np.asarray, zip(*[stat(d[1]) for d in v]))

        xmax = max(xmax, max(x))

        # --------------- #

        m, c = np.polyfit(x[:10], y[0] / mi[:10], 1)
        # print(f"{label:>40}: pre: {m}")

        m, c = np.polyfit(x[10:], y[0] / mi[10:], 1)
        # print(f"{label:>40}: ost: {m}")

        # --------------- #

        if "omp" in label:
            label = "OpenMP"
            mark = "o"
        elif "tbb" in label:
            label = "OneTBB"
            mark = "s"
        elif "taskflow" in label:
            label = "Taskflow"
            mark = "d"
        elif "busy" in label and "co" in label:
            label = "Busy-LF*"
            mark = "<"
        elif "busy" in label:
            label = "Busy-LF"
            mark = "v"
        elif "lazy" in label and "co" in label:
            label = "Lazy-LF*"
            mark = ">"
        elif "lazy" in label:
            label = "Lazy-LF"
            mark = "^"
        else:
            raise "error"

        print(
            f"  {label:<10}: T_s/T_112 = {tS / y[-1]:<4.1f} T_s/T_1 = {tS / y[0]:<4.3f}, T_112 = {y[-1]:.3f}"
        )

        # --------------- #

        t = tS / y

        f_yerr = err / y
        f_serr = tSerr / tS

        terr = t * np.sqrt((f_yerr**2 + f_serr**2))

        if args.rel:
            t /= x
            terr /= x

        if count == 0:
            obj = ax_abs.errorbar(
                x, t, yerr=terr, label=label, capsize=2, marker=mark, markersize=4
            )
            if "LF" in label:
                leg_hi.append(obj)
            else:
                leg_lo.append(obj)
        else:
            ax_abs.errorbar(x, t, yerr=terr, capsize=2, marker=mark, markersize=4)

        ymax = max(ymax, max(t))
        ymin = min(ymin, min(t))

    ax_abs.set_title(f"\\textit{{{p}}}")
    ax_abs.set_xticks(range(0, int(xmax + 1.5), 14))
    ax_abs.set_xlim(0, 112)

    count += 1

fig.supxlabel("\\textbf{{Cores}}")

if args.rel:
    fig.supylabel("\\textbf{{Efficiency}}")
else:
    fig.supylabel("\\textbf{{Speedup}}")


# kw = dict(bbox_to_anchor=(0, 0), edgecolor="w")

leg_lo = fig.legend(
    handles=leg_lo,
    ncol=3,
    loc="upper center",
    bbox_to_anchor=[0.5, 0.965],
    frameon=False,
)

# plt.gca().add_artist(leg_lo)

leg_hi = fig.legend(
    handles=leg_hi,
    ncol=4,
    loc="upper center",
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 1, 0.925))

if args.output_file is not None:
    plt.savefig(args.output_file, bbox_inches="tight")
