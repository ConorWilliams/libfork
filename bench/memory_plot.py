# A small python utility to measure memory usage using the GNU time utility


import argparse
import numpy as np

from matplotlib import pyplot as plt


# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(description="Plot memory")

parser.add_argument("data_file", type=str, help="Input csv")

args = parser.parse_args()

data = {}

with open(args.data_file, "r") as file:
    for line in file:
        #
        name, _, threads, med, dev = line.split(",")

        if name not in data:
            data[name] = ([], [], [])

        data[name][0].append(int(threads))
        data[name][1].append(int(med))
        data[name][2].append(int(dev))


y_zero = data["calibrate"][1][0]
y_serial = data["serial"][1][0]


for k, v in data.items():
    if k == "serial" or k == "calibrate" or k == "taskflow":
        continue

    print(k, v)

    x = np.asarray(v[0])
    y = np.asarray(v[1])
    err = np.asarray(v[2])

    y = (y - y_zero) / (y_serial - y_zero)
    err = (err) / (y_serial - y_zero)

    plt.errorbar(x, y, yerr=err, label=k, capsize=2)

    # stack = y - y_zero - x * (y[0] - y_serial) * 0.2

    # plt.plot(x, stack, label=k)


def mem(n):
    return n * per_thread + stack(n)


# plt.plot(x, x, label="bound")

# plt.yscale("log")
# plt.xscale("log")

plt.xlabel("Threads")
plt.ylabel("Relative maximum RSS")

plt.legend()

plt.savefig("test.pdf")

# Memory looks like:
#
#   m = static + n * per_thread + stack(n)
#
# Hence:
#
#   (m - static) / n = per_thread + stack(n) / n

#   (m(1) - static) = per_thread + stack(1)

#   m(1) - m_serial = per_thread

#  stack(n) = m(n) - static - n * (m(1) - m_serial)

#  m_serial - static = stack(1)
