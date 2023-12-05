# A small python utility to measure memory usage using the GNU time utility


import os
import argparse
import pathlib


# Parse location of benchmark and regex base.

parser = argparse.ArgumentParser(
    description="Measure memory usage of libfork's benchmarks."
)


parser.add_argument("binary", type=str, help="The benchmark binary to run")
parser.add_argument("cores", type=int, help="max number of cores")

args = parser.parse_args()


for kind in [
    "serial",
    "libfork.*lazy.*fan",
    "libfork.*busy.*fan",
    "omp",
    "tbb",
    "taskflow",
]:
    for bench in ["T1 "]:
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

            print(f"Running reg={reg}")

            os.system(
                f'/usr/bin/time -a -o mem.log -f"{reg}: mem=%M"  -- {args.binary} --benchmark_filter="{reg}" --benchmark_time_unit=ms --benchmark_repetitions=5'
            )
