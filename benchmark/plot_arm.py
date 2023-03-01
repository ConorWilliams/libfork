import matplotlib.pyplot as plt
import numpy as np
import glob
import json

globs = [
    ["benchmark/data/arm-fork-reduce-100_000_000.*", "Reduce"],
    ["benchmark/data/arm-fork-matmul-n=1024*", "Matmul"],
    ["benchmark/data/arm-fork-dfs-7,7*",    "DFS"],
    ["benchmark/data/arm-fork-fib-30*",    "Fibonacci"],
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
        t = np.asarray(t) # time/ns per operation

        print(name)

        plt.plot(n, 2*t[1]/t, linestyle='--', marker='+', label=name.split('-')[2])


# plt.gca().set_yscale('log')
# plt.gca().set_xscale('log')
plt.title('AWS Graviton3 (arm64)(c7g.16xlarge)')
plt.xlabel('Number of threads (n)')
plt.ylabel('Speedup 2T[2]/T[n]')

plt.legend()

plt.savefig(f"benchmark/figs/arm.svg", format='svg')

plt.show()





# # matmul_tbb = [[263_098_869.00, 0.2],   [131_379_362.00, 0.1],   [ 92_545_894.00, 0.8],   [ 68_723_140.00, 0.5],   [ 57_315_249.00, 1.0],   [ 48_542_074.00, 1.9],   [ 41_352_373.00, 0.4],   [ 36_300_965.00, 0.3],   [ 33_353_970.00, 1.6],   [ 30_331_572.00, 1.4],   [ 27_880_437.00, 0.5],   [ 25_946_996.00, 0.3],   [ 25_670_770.00, 0.9],   [ 23_582_526.00, 0.8],   [ 22_298_078.00, 1.3],   [ 22_709_981.00, 0.7],]
# # matmul_fork = [[356_510_555.00,  0.1], [177_554_477.00,  0.1], [127_360_291.00,  2.1], [ 94_613_606.00,  2.5], [ 78_345_074.00,  4.0], [ 64_422_306.00,  2.6], [ 56_367_303.00,  0.7], [ 49_651_425.00,  0.6], [ 44_789_555.00,  2.0], [ 41_681_570.00,  0.6], [ 38_440_439.00,  0.9], [ 36_460_249.00,  0.7], [ 34_002_242.00,  1.2], [ 33_233_023.00,  1.4], [ 32_568_424.00,  0.8], [ 30_304_954.00,  0.7],]
# # matmul_omp = [[256_645_116.00, 0.1],[287_280_595.00, 0.1],[160_232_008.00, 0.1],[130_485_102.00, 1.5],[ 92_620_325.00, 0.3],[ 91_985_935.00, 4.0],[ 86_202_730.00, 3.0],[ 84_879_949.00, 1.4],[ 93_035_367.00, 1.4],[103_041_120.00, 3.0],[111_659_827.00, 1.6],[122_151_195.00, 4.7],[124_330_019.00, 1.1],[137_135_311.00, 1.5],[146_125_801.00, 3.2],[158_638_674.00, 2.9],]

# # plt.errorbar([i + 1 for i in range(16)], [x[0] for x in matmul_tbb], yerr=[x[0] * x[1] * 0.01 for x in matmul_tbb], fmt='+', label='tbb matmul')
# # plt.plot([i + 1 for i in range(16)], [x[0] for x in matmul_tbb])
# # plt.errorbar([i + 1 for i in range(16)], [x[0] for x in matmul_fork], yerr=[x[0] * x[1] * 0.01 for x in matmul_fork], fmt='+', label='fork matmul')
# # plt.errorbar([i + 1 for i in range(16)], [x[0] for x in matmul_omp], yerr=[x[0] * x[1] * 0.01 for x in matmul_omp], fmt='+', label='omp matmul')


















