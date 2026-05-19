AM_CPPFLAGS = -I$(includedir)
AM_LDFLAGS = -L$(libdir) -l$(PACKAGE)

check_PROGRAMS = \
                 cholesky \
                 fft \
                 fib \
                 heat \
                 integrate \
                 knapsack \
                 lu \
                 matmul \
                 nqueens \
                 quicksort \
                 rectmul \
                 strassen

cholesky_LDADD = -lm
fft_LDADD = -lm
heat_LDADD = -lm
lu_LDADD = -lm
strassen_LDADD = -lm

TESTS = $(check_PROGRAMS)
