# A small python utility to measure memory usage using the GNU time utility


import argparse
import numpy as np

from matplotlib import pyplot as plt

from scipy.optimize import curve_fit

plt.rcParams["text.usetex"] = True

# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(description="Plot memory")

args = parser.parse_args()

Benchmarks = []
patterns = ["T1", "T1L", "T1XXL", "T3", "T3L", "T3L"]

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
            data[name][1].append(float(med) / 1024)
            data[name][2].append(float(dev) / 1024)

    Benchmarks.append(data)


fig, axs = plt.subplots(2, 3, figsize=(12, 7), sharex="col")

patterns = ["T1", "T1L", "T1XXL", "T3", "T3L", "noop"]

# Benchmarks = Benchmarks[:1]

for (ax), p, data, i in zip(axs.flatten(), patterns, Benchmarks, range(1000)):
    if p == "noop":
        continue

    print(f"{p=}")

    #

    y_zero = data["zero"][1][0]
    y_calib = data["calibrate"][1][0]
    y_serial = data["serial"][1][0]

    for k, v in data.items():
        #
        if "seq" in k or "busy" in k:
            continue

        if k == "zero" or k == "serial" or k == "calibrate":
            continue

        x = np.asarray(v[0])
        y = np.asarray(v[1])
        err = np.asarray(v[2])

        err = err / (y[0] - y_calib)
        y = (y - y_calib) / (y[0] - y_calib)

        def func(x, a, b, n):
            return y_calib + (y_serial - y_calib) * (a + b * (x) ** n)

        p0 = (y_serial, 100, 2)
        bounds = ([0, 0, 1], [np.inf, np.inf, 5])
        popt, pcov = curve_fit(func, x, y, p0=p0, bounds=bounds, maxfev=10000)

        # a, b, n = popt

        # da, db, dn = np.sqrt(np.diag(pcov))

        # print(f"{k:>30}: {a:>16.2f} + {b:>16.2f} * x^{n:>8.1f} +- {dn:<8.1f}")

        if k == "taskflow":
            pass
            # continue

        if i == 0:
            ax.errorbar(x, y, yerr=err, label=k, capsize=2)
        else:
            ax.errorbar(x, y, yerr=err, capsize=2)

        ax.plot(x, func(x, *popt), "k--")

        ax.set_title(f"\\textit{{{p}}}")

        ax.set_xticks(range(0, int(112 + 1.5), 14))

        ax.set_xlim(0, 112)
        # ax.set_ylim(bottom=0)

        # ax.set_yscale("log", base=10)
        # ax.set_xscale("log", base=2)


fig.supxlabel("\\textbf{{Threads/cores}}")
fig.supylabel("\\textbf{{MRRS(x) - MRRS(serial) / MiB}}")


fig.legend(
    loc="upper center",
    # bbox_to_anchor=(0, 0),
    ncol=6,
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 0.95, 0.95))

# if args.output_file is not None:
plt.savefig("test.pdf")
