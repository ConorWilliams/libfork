import argparse
import numpy as np

from matplotlib import pyplot as plt

from math import sqrt

from scipy.optimize import curve_fit

plt.rcParams["text.usetex"] = True

# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(description="Plot memory")

args = parser.parse_args()

Benchmarks = []

patterns = [
    "fib",
    "integ",
    "matmul",
    "nqueens",
    "T1",
    "T3",
    "T1L",
    "T3L",
    "T1XXL",
    "T3XXL",
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


fig, axs = plt.subplots(5, 2, figsize=(6, 10), sharex="col", sharey=False)

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

for (ax), p, data, i in zip(axs.flatten(), patterns, Benchmarks, range(1000)):
    #
    print(f"{p=}")

    # y_zero = data["zero"][1][0]

    y_calib = data["calibrate"][1][0]

    y_serial = max(4 / 1024.0, data["serial"][1][0] - y_calib)  # must be at least 4 KiB

    first = True

    for k, v in data.items():
        #
        if "seq" in k:
            continue

        if k == "zero" or k == "serial" or k == "calibrate":
            continue

        # if "lib" not in k:
        #     continue

        x = np.asarray(v[0])
        y = np.asarray(v[1])
        err = np.asarray(v[2])

        y = np.maximum(4 / 1024.0, y - y_calib)  # Subtract zero offset

        def func(x, a, b, n):
            return a + b * y[0] * x**n

        p0 = (1, 1, 1)
        bounds = ([0, 0, 0], [np.inf, np.inf, np.inf])
        popt, pcov = curve_fit(
            func,
            x,
            y,
            p0=p0,
            bounds=bounds,
            maxfev=10000,
            # sigma=err,
            # absolute_sigma=True,
        )

        a, b, n = popt

        da, db, dn = np.sqrt(np.diag(pcov))

        print(f"{k:>30}: {a:>16.1f} + {b:>16.4f} * x^{n:<8.2f} +- {dn:<8.2f}")

        if k == "taskflow":
            pass
            continue

        ax.plot(
            x,
            func(x, *popt),
            "k--",
            label="$y = a + bM_1x^n$" if i == 0 and first else None,
        )

        first = False

        # if np.any(y < y_serial):
        #     print(y_serial, y)

        # assert np.all(y > y_serial)

        # eff = np.maximum(4 / 1024.0, y - y_serial)

        ax.errorbar(x, y, yerr=err, label=k if i == 0 else None, capsize=2)

        ax.set_title(f"\\textit{{{p}}}")

        ax.set_xticks(range(0, int(112 + 1.5), 14))

        # ax.set_xlim(0, 112)
        # ax.set_ylim(bottom=0)

        # ax.set_yscale("log", base=10)
        # ax.set_xscale("log", base=10)


fig.supxlabel("\\textbf{{Threads/cores}}")
fig.supylabel("\\textbf{{$\Delta$MRSS/MiB}}")


fig.legend(
    loc="upper center",
    # bbox_to_anchor=(0, 0),
    ncol=3,
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 0.95, 0.95))

# if args.output_file is not None:
plt.savefig("test.pdf")
