# A small python utility to measure memory usage using the GNU time utility


import os
import argparse
import subprocess
import re
from statistics import median, stdev
from math import sqrt

# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(
    description="Measure memory usage of libfork's benchmarks."
)


parser.add_argument("binary", type=str, help="The benchmark binary to run")
parser.add_argument("cores", type=int, help="max number of cores")

args = parser.parse_args()

bench = "fib"

if not bench.startswith("T"):
    libfork = [
        "libfork.*lazy.*fan",
    ]
else:
    libfork = [
        "libfork.*_coalloc_.*lazy.*fan",
    ]

with open(f"memory.{bench.strip()}.csv", "w") as file:
    for kind in [
        "zero",
        "calibrate",
        "serial",
        *libfork,
        "tmc",
        "ccpp",
        "omp",
        "tbb",
        "taskflow",
    ]:
        print(f"Running {kind} {bench.strip()}")

        is_serial = kind == "serial" or kind == "calibrate" or kind == "zero"

        for i in [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]:
            #
            if i > args.cores:
                break

            if kind == "zero":
                reg = "NONNAMEDTHIS"
            elif kind == "calibrate":
                reg = "calibrate"
            elif bench.startswith("T"):
                reg = f"uts.*{kind}.*{bench}"
            else:
                reg = f"{bench}.*{kind}"

            if not is_serial:
                reg += f".*/{i}/"
            elif i > 1:
                break

            mem = []

            for r in range(25 if is_serial else 5):
                command = f'/usr/bin/time -f"MEMORY=%M"  -- {args.binary} --benchmark_filter="{reg}" --benchmark_time_unit=ms'

                # print(f"{command=}")

                output = subprocess.run(
                    command, shell=True, check=True, stderr=subprocess.PIPE
                ).stderr

                match = re.search(".*MEMORY=([1-9][0-9]*)", str(output))

                if match:
                    val = int(match.group(1))
                    mem.append(val)
                else:
                    raise "No memory found"

            x = median(mem)
            e = stdev(mem) / sqrt(len(mem))

            print(f"mems={mem} -> {x}, {e}")

            file.write(f"{kind},{i},{x},{e}\n")
            file.flush()
