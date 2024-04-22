import numpy as np
from matplotlib import pyplot as plt
import argparse
import json
import re
from statistics import mean, stdev, median, geometric_mean

plt.rcParams["text.usetex"] = True

"""

cmake --build --preset=rel && ./build/rel/bench/benchmark  --benchmark_time_unit=ms --benchmark_filter="fib_[^l]|fib_libfork<lazy_pool, numa_strategy::fan>" --benchmark_out_format=json --benchmark_out=bench/data/laptop/fib.json --benchmark_repetitions=5

"""

from functools import cache


@cache
def fib_tasks(n):

    if n < 2:
        return 1

    return 1 + fib_tasks(n - 1) + fib_tasks(n - 2)


def stat(x):
    x = sorted(x)[:-1]

    err = stdev(x) / (np.sqrt(len(x)) if len(x) > 1 else 0)
    #
    return median(x), err, min(x)


# Parse the input file and benchmark name and optional output file.
parser = argparse.ArgumentParser()

parser.add_argument("input_file", type=str, help="The output of the fib benchmark.")

parser.add_argument("fib", type=int, help="Fib number being calculated.")

parser.add_argument("-o", "--output_file", help="Output file")
parser.add_argument("-r", "--rel", help="plot relative speedup", action="store_true")
args = parser.parse_args()

benchmarks = {}

# Read the input file as json
with open(args.input_file) as f:
    data = json.load(f)

for bench in data["benchmarks"]:
    if bench["run_type"] != "iteration":
        continue

    name = bench["name"].split("/")

    name = [n for n in name if n != "real_time" and not n.isnumeric()]

    name = "".join(name)

    name = name.replace("seq", "fan")

    if name not in benchmarks:
        benchmarks[name] = {}

    num_threads = int(bench["green_threads"] + 0.5)

    if num_threads not in benchmarks[name]:
        benchmarks[name][num_threads] = []

    benchmarks[name][num_threads].append(bench["real_time"])

benchmarks = [(k, sorted(v.items())) for k, v in benchmarks.items()]

benchmarks.sort()


fig, ax = plt.subplots(figsize=(6, 5))

count = 0

tS = -1
tSerr = -1

for k, v in benchmarks:
    if "serial" in k:
        tS, tSerr, _ = stat(v[0][1])
        break


ymax = 0
xmax = 0

ymin = 112


for k, v in benchmarks:
    #
    label = "_".join(k.split("_")[1:])

    if label.startswith("serial"):
        continue

    x = np.asarray([t[0] for t in v])
    y, err, mi = map(np.asarray, zip(*[stat(d[1]) for d in v]))

    xmax = max(xmax, max(x))

    # --------------- #

    g1, c = np.polyfit(x[:10], y[0] / mi[:10], 1)
    # g2, c = np.polyfit(x[10:], y[0] / mi[10:], 1)

    g2 = g1

    # print(f"{label:>40}: pre: {m}")
    # print(f"{label:>40}: ost: {m}")

    # --------------- #

    if "omp" in k:
        label = "OpenMP"
        mark = "o"
    elif "tbb" in k:
        label = "OneTBB"
        mark = "s"
    elif "taskflow" in k:
        label = "Taskflow"
        mark = "d"
    elif "libfork" in k:
        label = "Libfork"
        mark = ">"
    elif "tmc" in k:
        label = "TooManyCooks"
        mark = "^"
    elif "ccpp" in k:
        label = "Concurrencpp"
        mark = "v"
    else:
        raise "error"

    print(
        f"  {label:<10}: T_s/T_112 = {tS / y[-1]:<7.2f} T_1/T_s = {y[0]/tS:<7.3f} T_1/T_112 = {y[0]/y[-1]:<7.1f} g1 = {g1:.3f} g2 = {g2:.3f}"
    )

    # --------------- #

    if tS < 0:
        continue

    t = fib_tasks(args.fib) / y

    f_yerr = err / y
    terr = t * f_yerr

    if args.rel:
        t /= x
        terr /= x

    if count == 0:
        ax.errorbar(x, t, yerr=terr, label=label, capsize=2, marker=mark, markersize=4)
    else:
        ax.errorbar(x, t, yerr=terr, capsize=2, marker=mark, markersize=4)

    ymax = max(ymax, max(t))
    ymin = min(ymin, min(t))

# ax_abs.set_ylim(ymin, 112)

# print(f"ymax: {ymax}")
# print(f"xmax: {xmax}")

# ideal = range(1, int(ymax + 1.5)) if not args.rel else x

# ax_abs.plot(
#     ideal,
#     ideal if not args.rel else [1] * len(ideal),
#     color="black",
#     linestyle="dashed",
#     label="Ideal" if count == 0 else None,
# )

ax.set_xticks(range(0, int(xmax + 1.5), 14))

# ax.set_title(f"\\textit{{{p}}}")

# if count < 2:
# ax_abs.set_yscale("log", base=2)
# ax_abs.set_xscale("log", base=2)

# ax_abs.yaxis.set_label_position("right")

ax.set_xlim(0, max(x))


count += 1


ax.set_xlabel("Cores")
ax.set_ylabel("Tasks per second")


fig.legend(
    loc="upper center",
    # bbox_to_anchor=(0, 0),
    ncol=3,
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 1, 0.90))

if args.output_file is not None:
    plt.savefig(args.output_file, bbox_inches="tight")
