# A small python utility to measure memory usage using the GNU time utility


import os
import argparse
import subprocess
import re
from statistics import median, stdev

# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(
    description="Measure memory usage of libfork's benchmarks."
)


parser.add_argument("binary", type=str, help="The benchmark binary to run")
parser.add_argument("bench", type=str, help="The regex benchmark")
parser.add_argument("cores", type=int, help="max number of cores")

args = parser.parse_args()

bench = args.bench

if not bench.startswith("T"):
    libfork = [
        "libfork.*lazy.*fan",
        "libfork.*busy.*fan",
        "libfork.*lazy.*seq",
        "libfork.*busy.*seq",
    ]
else:
    libfork = [
        "libfork.*_alloc_.*lazy.*fan",
        "libfork.*_alloc_.*busy.*fan",
        "libfork.*_alloc_.*lazy.*seq",
        "libfork.*_alloc_.*busy.*seq",
        "libfork.*_coalloc_.*lazy.*fan",
        "libfork.*_coalloc_.*busy.*fan",
        "libfork.*_coalloc_.*lazy.*seq",
        "libfork.*_coalloc_.*busy.*seq",
    ]


with open(f"memory.{bench.strip()}.csv", "w") as file:
    for kind in [
        "calibrate",
        "serial",
        *libfork,
        "omp",
        "tbb",
        "taskflow",
    ]:
        print(f"Running {kind} {bench.strip()}")

        for i in [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]:
            #
            if i > args.cores:
                break

            if not bench.startswith("T"):
                reg = f"{bench}.*{kind}"
            else:
                reg = f"uts.*{kind}.*{bench}"

            if kind != "serial":
                reg += f".*/{i}/"
            elif i > 1:
                break

            if kind == "calibrate" and i > 1:
                break

            mem = []

            for r in range(5):
                command = f'/usr/bin/time -f"MEMORY=%M"  -- {args.binary} --benchmark_filter="{reg}" --benchmark_time_unit=ms'

                output = subprocess.run(
                    command, shell=True, check=True, stderr=subprocess.PIPE
                ).stderr

                match = re.search(".*MEMORY=([1-9][0-9]*)", str(output))

                if match:
                    val = int(match.group(1))
                    mem.append(val)
                else:
                    raise "No memory found"

            print(f"mems={mem}")

            file.write(
                f"{kind},{bench.strip()},{i},{median(mem)},{round(stdev(mem))}\n"
            )
            file.flush()
