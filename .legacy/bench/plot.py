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
    x = sorted(x)[:]

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


patterns = [
    "fib",
    "integ",
    "matmul",
    "nqueens",
    "uts_libfork_alloc",
    "uts_flow",
    "uts_omp",
    "uts_tbb",
]


for file in patterns:
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
                print("found", name)
                benchmarks[name] = {}

            num_threads = int(bench["green_threads"] + 0.5)

            if num_threads not in benchmarks[name]:
                benchmarks[name][num_threads] = []

            benchmarks[name][num_threads].append(bench["real_time"])

benchmarks = [(k, sorted(v.items())) for k, v in benchmarks.items()]

benchmarks.sort()

fig, axs = plt.subplots(5, 2, figsize=(6, 10), sharex="col", sharey=None)

count = 0

patterns = [
    "fib",
    "integrate",
    "matmul",
    "nqueens",
    "T1 ",
    "T3 ",
    "T1L",
    "T3L",
    "T1XXL",
    "T3XXL",
]

for (ax_abs), p in zip(axs.flatten(), patterns):
    # find the serial benchmark

    print(f"Plotting {p}")

    tS = -1
    tSerr = -1

    for k, v in benchmarks:
        #
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

    for k, v in benchmarks:
        if p not in k:
            # print(f"skipping {k} wich does not contain {p}")
            continue

        #
        label = "_".join(k.split("_")[1:])

        if label.startswith("serial"):
            continue

        if label.startswith("lib"):
            label = f"libfork ({label[8:8+4]})"

        x = np.asarray([t[0] for t in v])
        y, err, mi = map(np.asarray, zip(*[stat(d[1]) for d in v]))

        xmax = max(xmax, max(x))

        m, c = np.polyfit(x[:10], y[0] / mi[:10], 1)
        print(f"{label:>40}: pre: {m}")

        m, c = np.polyfit(x[10:], y[0] / mi[10:], 1)
        print(f"{label:>40}: ost: {m}")

        if label == "omp":
            label = "OpenMP"
        elif label == "tbb":
            label = "OneTBB"
        elif label == "ztaskflow":
            label = "Taskflow"
        else:
            label = label.capitalize()

        # --------------- #

        if tS < 0:
            continue

        t = tS / y

        f_yerr = err / y
        f_yerr = tSerr / tS

        terr = t * np.sqrt((f_yerr**2 + f_yerr**2))

        if args.rel:
            t /= x
            terr /= x

        if count == 0:
            ax_abs.errorbar(x, t, yerr=terr, label=label, capsize=2)
        else:
            ax_abs.errorbar(x, t, yerr=terr, capsize=2)

        ymax = max(ymax, max(t))
        ymin = min(ymin, min(t))

    # ax_abs.set_ylim(ymin, 112)

    # print(f"ymax: {ymax}")
    # print(f"xmax: {xmax}")

    ideal_abs = range(1, int(xmax + 1.5))
    # ideal_rel = range(1, int(xmax + 1.5))

    # if args.rel:
    #     ax_abs.plot(
    #         ideal_abs,
    #         ideal_abs,
    #         color="black",
    #         linestyle="dashed",
    #         label="Ideal" if count == 0 else None,
    #     )

    #     ax_abs.set_ylim(top=112)

    #     # ax_rel.plot(ideal_rel, ideal_rel, color="black", linestyle="dashed")

    #     ax_abs.set_yticks(range(0, int(xmax + 1.5), 14))

    ax_abs.set_xticks(range(0, int(xmax + 1.5), 14))

    ax_abs.set_title(f"\\textit{{{p}}}")

    # if count < 2:
    # ax_abs.set_yscale("log", base=2)
    # ax_abs.set_xscale("log", base=2)

    # ax_abs.yaxis.set_label_position("right")

    ax_abs.set_xlim(0, 112)

    ax_abs.set_ylim(bottom=0)

    if p.startswith("T"):
        if args.rel:
            ax_abs.set_ylim(0, 1)
        else:
            ax_abs.set_ylim(0, 80)

    # ax_abs.set_xlim(0, 112)

    # ax_abs.set_ylim(top=112)
    # ax_rel.set_ylim(top=112)

    count += 1

# fig.legend()

# fig.set_

fig.supxlabel("\\textbf{{Cores}}")

if args.rel:
    fig.supylabel("\\textbf{{Efficiency}}")
else:
    fig.supylabel("\\textbf{{Speedup}}")


fig.legend(
    loc="upper center",
    # bbox_to_anchor=(0, 0),
    ncol=3,
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 0.95, 0.95))

if args.output_file is not None:
    plt.savefig(args.output_file)
