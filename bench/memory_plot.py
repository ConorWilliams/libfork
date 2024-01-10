import argparse
import numpy as np

from matplotlib import pyplot as plt

from math import sqrt

from scipy.optimize import curve_fit

plt.rcParams["text.usetex"] = True

# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(description="Plot memory")

parser.add_argument("-o", "--output_file", help="Output file")

args = parser.parse_args()

Benchmarks = []

patterns = [
    "fib",
    "integ",
    "nqueens",
    "T1",
    "T1L",
    "T1XXL",
    "T3",
    "T3L",
    "T3XXL",
    "matmul",
]

for bm in patterns:
    #
    data = {}

    with open(f"./bench/data/sapphire/v5/memory.{bm}.csv", "r") as file:
        for line in file:
            #
            name, threads, med, dev = line.split(",")

            if name not in data:
                data[name] = ([], [], [])

            data[name][0].append(int(threads))
            data[name][1].append(float(med) / 1024.0)
            data[name][2].append(float(dev) / 1024.0)

    Benchmarks.append(data)


fig, axs = plt.subplots(3, 3, figsize=(6, 6.5), sharex="col", sharey="row")

patterns = [
    "fib",
    "integrate",
    "nqueens",
    "T1 ",
    "T1L",
    "T1XXL",
    "T3 ",
    "T3L",
    "T3XXL",
    "matmul",
]

from itertools import cycle

for (ax), p, data, i in zip(cycle(axs.flatten()), patterns, Benchmarks, range(1000)):
    #
    print(f"{p=}")

    # y_zero = data["zero"][1][0]

    y_calib = data["calibrate"][1][0]
    # x5 because this is a standard error not a standard deviation
    y_calib_err = data["calibrate"][2][0] * 5

    y_serial = max(4 / 1024.0, data["serial"][1][0] - y_calib)  # must be at least 4 KiB

    first = True

    for k, v in data.items():
        #
        if "seq" in k or "_coalloc_" in k:
            # print("Skipping", k)
            continue

        if k == "zero" or k == "serial" or k == "calibrate":
            continue

        if "omp" in k:
            label = "OpenMP"
            mark = "o"
        elif "tbb" in k:
            label = "OneTBB"
            mark = "s"
        elif "taskflow" in k:
            label = "Taskflow"
            mark = "d"
        elif "busy" in k and "co" in k:
            label = "Busy-LF*"
            mark = "<"
        elif "busy" in k:
            label = "Busy-LF"
            mark = "v"
        elif "lazy" in k and "co" in k:
            label = "Lazy-LF*"
            mark = ">"
        elif "lazy" in k:
            label = "Lazy-LF"
            mark = "^"
        else:
            raise "error"

        x = np.asarray(v[0])
        y = np.asarray(v[1])
        err = np.asarray(v[2])

        y = np.maximum(4 / 1024.0, y - y_calib)  # Subtract zero offset
        err = np.sqrt(err**2 + y_calib_err**2)

        def func(x, a, b, n):
            return a + b * y[0] * x**n

        p0 = (1, 1, 1)
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])
        popt, pcov = curve_fit(
            func,
            x,
            y,
            p0=p0,
            bounds=bounds,
            maxfev=10000,
            sigma=err,
            # absolute_sigma=True,
        )

        a, b, n = popt

        da, db, dn = np.sqrt(np.diag(pcov))

        print(
            f"{label:>12}: {a:>10.1f} + {b:.2e}({db:.2e}) * x^{n:.2f} +- {dn:.2f} mem_112={y[-1]:.0f} "
        )

        if p == "matmul":
            continue

        if k == "taskflow":
            pass
            # continue

        ax.errorbar(
            x,
            y,
            yerr=err,
            label=label if i == 6 else None,
            capsize=2,
            marker=mark,
            markersize=4,
            linestyle="None",
        )

        # if "*" not in label:
        ax.plot(
            x,
            func(x, *popt),
            "k-",
            # linewidth=0.75,
            label="Fit" if i == 7 and first else None,
        )

        first = False

        ax.set_title(f"\\textit{{{p}}}")
        ax.set_xticks(range(0, int(112 + 1.5), 28))
        ax.set_xlim(0, 112)

        # ax.set_xlim(0, 112)
        # ax.set_ylim(bottom=0)

        ax.set_yscale("log", base=10)
        # ax.set_xscale("log", base=10)


fig.supxlabel("\\textbf{{Cores}}")
fig.supylabel("\\textbf{{MRSS/MiB}}")

fig.legend(
    loc="upper center",
    # bbox_to_anchor=(0, 0),
    ncol=3,
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 1, 0.925), h_pad=0.5, w_pad=0.1)

if args.output_file is not None:
    plt.savefig(args.output_file, bbox_inches="tight")
