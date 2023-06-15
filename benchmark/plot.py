import matplotlib.pyplot as plt
import numpy as np
import glob
import json



globs = [
    ["benchmark/data/*-reduce-100_000_000.*", "Reduce"],
    ["benchmark/data/*-matmul-n=1024.json", "Matmul"],
    ["benchmark/data/*-dfs-7,7.json",    "DFS"],
    ["benchmark/data/*-fib-30.json",    "Fibonacci"],
]

for g, title in globs:
    files = glob.glob(g)

    files.sort()

    for name in files:
        n = []
        t = []
        err = []

        with open(name, 'r') as f:
            for i, thread in enumerate(json.load(f)['results'], 1):
                n.append(i)
                t.append(thread['median(elapsed)'])
                err.append(t[-1] *  thread['medianAbsolutePercentError(elapsed)'])

        n = np.asarray(n)
        t = np.asarray(t)


        plt.plot(n, 1 / t, linestyle='--', marker='+', label=name.split('-')[0].split('\\')[1])


    plt.title(f'{title} benchmark')
    plt.xlabel('Number of threads')
    plt.ylabel('Operations per second')

    # plt.gca().set_yscale('log')
    # plt.gca().set_xscale('log')
    plt.legend()

    plt.savefig(f"benchmark/figs/{title.lower()}.svg", format='svg')

    plt.show()


















