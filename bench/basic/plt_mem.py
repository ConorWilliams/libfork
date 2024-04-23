import argparse
import numpy as np

from matplotlib import pyplot as plt
from matplotlib import ticker

from math import sqrt

from scipy.optimize import curve_fit

parser = argparse.ArgumentParser(description="Plot memory")
parser.add_argument("input_file", type=str, help="The output of the memory benchmark.")
parser.add_argument("-o", "--output_file", help="Output file")
args = parser.parse_args()


data = {}

with open(args.input_file) as file:
    for line in file:
        #
        name, threads, med, dev = line.split(",")

        if name not in data:
            data[name] = ([], [], [])

        data[name][0].append(int(threads))
        data[name][1].append(float(med) / 1024.0)
        data[name][2].append(float(dev) / 1024.0)


fig, ax = plt.subplots(figsize=(8, 5))


y_calib = data["calibrate"][1][0]

y_calib_err = (
    data["calibrate"][2][0] * 5
)  # x5 because this is a standard error not a standard deviation

y_serial = max(4 / 1024.0, data["serial"][1][0] - y_calib)  # must be at least 4 KiB

first = True

data = dict(sorted(data.items()))

for k, v in data.items():

    if k == "zero" or k == "serial" or k == "calibrate":
        continue

    if "omp" in k:
        label = "OpenMP"
        mark = "o"
    elif "tbb" in k:
        label = "OneTBB"
        mark = "s"
    elif "taskflow" in k:
        continue
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

    ax.errorbar(
        x,
        y,
        yerr=err,
        label=label,
        capsize=2,
        marker=mark,
        markersize=4,
        # linestyle="None",
    )

    # # if "*" not in label:
    # ax.plot(
    #     x,
    #     func(x, *popt),
    #     "k-",
    #     # linewidth=0.75,
    #     # label="Fit" if first else None,
    # )

    first = False

    ax.set_xticks(range(0, int(32 + 1.5), 4))
    # ax.set_xlim(0, max(x))
    # ax.tick_params("x", rotation=90)

    # ax.set_xlim(0, 112)
    ax.set_ylim(bottom=6, top=400)

    # ax.set_xscale("log", base=10)
    ax.set_yscale("log", base=10)

    # base = 100

    # locmajy = ticker.LogLocator(base=base, numticks=100)
    # locminy = ticker.LogLocator(base=base, subs=np.arange(0, 10) * 0.1, numticks=100)

    # ax.yaxis.set_major_locator(locmajy)
    # ax.yaxis.set_minor_locator(locminy)
    # ax.yaxis.set_minor_formatter(ticker.NullFormatter())

    # ax.xaxis.set_ticks_position("left")


ax.set_xlabel("Cores")
ax.set_ylabel("Memory/MiB")

fig.legend(
    loc="upper center",
    # bbox_to_anchor=(0, 0),
    ncol=5,
    frameon=False,
)

fig.tight_layout(rect=(0, 0, 1, 0.95))

if args.output_file is not None:
    plt.savefig(args.output_file, bbox_inches="tight")
