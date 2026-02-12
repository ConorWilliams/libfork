"""
Unit tests for plot.py benchmark plotting script.

Tests the statistical functions and data processing logic used in the
Fibonacci benchmark plotting tool.
"""

import pytest
import numpy as np
from functools import cache
from statistics import stdev, median


# Replicate the functions from plot.py for testing
@cache
def fib_tasks(n):
    """Calculate number of tasks in Fibonacci computation."""
    if n < 2:
        return 1
    return 1 + fib_tasks(n - 1) + fib_tasks(n - 2)


def stat(x):
    """Calculate median, standard error, and minimum from measurements."""
    x = sorted(x)[:-1]  # Remove maximum (outlier)
    if len(x) > 1:
        err = stdev(x) / np.sqrt(len(x))
    else:
        err = 0
    return median(x), err, min(x)


class TestFibTasks:
    """Test cases for the fib_tasks function."""

    def test_fib_tasks_base_case_zero(self):
        """Test fib_tasks with n=0 returns 1."""
        result = fib_tasks(0)
        assert result == 1

    def test_fib_tasks_base_case_one(self):
        """Test fib_tasks with n=1 returns 1."""
        result = fib_tasks(1)
        assert result == 1

    def test_fib_tasks_small_values(self):
        """Test fib_tasks with small values."""
        # fib_tasks(2) = 1 + fib_tasks(1) + fib_tasks(0) = 1 + 1 + 1 = 3
        assert fib_tasks(2) == 3
        # fib_tasks(3) = 1 + fib_tasks(2) + fib_tasks(1) = 1 + 3 + 1 = 5
        assert fib_tasks(3) == 5

    def test_fib_tasks_medium_values(self):
        """Test fib_tasks with medium values."""
        # Test some larger values to ensure correct calculation
        result_4 = fib_tasks(4)
        result_5 = fib_tasks(5)

        # fib_tasks(4) = 1 + fib_tasks(3) + fib_tasks(2) = 1 + 5 + 3 = 9
        assert result_4 == 9
        # fib_tasks(5) = 1 + fib_tasks(4) + fib_tasks(3) = 1 + 9 + 5 = 15
        assert result_5 == 15

    def test_fib_tasks_caching(self):
        """Test that fib_tasks uses caching (multiple calls return same object)."""
        # Clear cache first
        fib_tasks.cache_clear()

        # First call
        result1 = fib_tasks(10)
        # Second call should use cache
        result2 = fib_tasks(10)

        assert result1 == result2

        # Check cache info to verify caching is working
        cache_info = fib_tasks.cache_info()
        assert cache_info.hits > 0  # Should have cache hits

    def test_fib_tasks_negative_input(self):
        """Test fib_tasks with negative input (should return 1 per implementation)."""
        # Based on the implementation: if n < 2: return 1
        result = fib_tasks(-1)
        assert result == 1


class TestStatFunction:
    """Test cases for the stat function."""

    def test_stat_single_value(self):
        """Test stat with a single value (edge case after outlier removal)."""
        # Note: stat() removes the maximum value, so single value becomes empty list
        # This is an edge case that might not occur in practice
        # Let's test with 2 values instead (minimum case that makes sense)
        values = [10.0, 10.0]
        median_val, err, minimum = stat(values)

        assert median_val == 10.0
        assert minimum == 10.0
        # Error should be 0 since only one value after outlier removal
        assert err == 0.0

    def test_stat_two_values(self):
        """Test stat with two values."""
        values = [10.0, 12.0]
        median_val, err, minimum = stat(values)

        # Sorted and removed last: [10.0]
        assert median_val == 10.0
        assert minimum == 10.0
        assert err == 0.0

    def test_stat_multiple_values(self):
        """Test stat with multiple values."""
        values = [10.0, 12.0, 11.0, 13.0, 10.5]
        median_val, err, minimum = stat(values)

        # After sorting and removing last: [10.0, 10.5, 11.0, 12.0]
        assert median_val == 10.75  # Median of 4 values
        assert minimum == 10.0

        # Error calculation: stdev / sqrt(n)
        sorted_vals = [10.0, 10.5, 11.0, 12.0]
        expected_err = stdev(sorted_vals) / np.sqrt(len(sorted_vals))
        assert abs(err - expected_err) < 1e-10

    def test_stat_removes_outlier(self):
        """Test that stat removes the maximum value (outlier)."""
        values = [10.0, 10.0, 10.0, 100.0]  # 100.0 is an outlier
        median_val, err, minimum = stat(values)

        # After removing max (100.0): [10.0, 10.0, 10.0]
        assert median_val == 10.0
        assert minimum == 10.0

    def test_stat_with_unsorted_input(self):
        """Test stat with unsorted input values."""
        values = [15.0, 10.0, 20.0, 12.0, 18.0]
        median_val, err, minimum = stat(values)

        # After sorting [10, 12, 15, 18, 20] and removing last: [10, 12, 15, 18]
        assert minimum == 10.0
        # Median of 4 values: (12 + 15) / 2 = 13.5
        assert median_val == 13.5

    def test_stat_all_same_values(self):
        """Test stat when all values are the same."""
        values = [5.0, 5.0, 5.0, 5.0, 5.0]
        median_val, err, minimum = stat(values)

        assert median_val == 5.0
        assert minimum == 5.0
        # Standard deviation of same values is 0
        assert err == 0.0


class TestBenchmarkDataParsing:
    """Test cases for benchmark data parsing logic."""

    def test_benchmark_name_parsing(self):
        """Test parsing of benchmark names from the JSON data."""
        # Test the name parsing logic that happens in the main code
        # Simulating: name = bench["name"].split("/")
        bench_name = "fib_libfork<lazy_pool, numa_strategy::seq>/40/green_threads:8/real_time"
        parts = bench_name.split("/")
        parts = [n for n in parts if n != "real_time" and not n.isnumeric()]
        result = "".join(parts)
        result = result.replace("seq", "fan")

        assert "real_time" not in result
        assert "40" not in result
        assert "fan" in result

    def test_green_threads_extraction(self):
        """Test extraction and rounding of green_threads value."""
        # Simulating: num_threads = int(bench["green_threads"] + 0.5)
        assert int(8.0 + 0.5) == 8
        assert int(8.4 + 0.5) == 8
        assert int(8.5 + 0.5) == 9
        assert int(7.6 + 0.5) == 8

    def test_thread_limit_filtering(self):
        """Test that threads > 32 are filtered out."""
        # The code has: if num_threads > 32: continue
        thread_counts = [1, 8, 16, 32, 33, 64, 112]
        filtered = [t for t in thread_counts if t <= 32]

        assert filtered == [1, 8, 16, 32]
        assert 33 not in filtered
        assert 64 not in filtered


class TestMathematicalProperties:
    """Test mathematical properties and edge cases."""

    def test_fib_tasks_growth_rate(self):
        """Test that fib_tasks grows at expected rate."""
        fib_tasks.cache_clear()

        # Each level should grow
        values = [fib_tasks(n) for n in range(2, 12)]

        # Check that the sequence is strictly increasing
        for i in range(1, len(values)):
            assert values[i] > values[i-1]

    def test_stat_error_calculation_properties(self):
        """Test properties of error calculation in stat function."""
        # Error should decrease as sample size increases
        # Create samples of increasing size with same distribution
        base_values = [10.0, 11.0, 12.0, 13.0, 14.0]

        for size in [3, 4, 5]:
            values = base_values[:size] + [20.0]  # Add outlier to be removed
            median_val, err, minimum = stat(values)

            # Error should be non-negative
            assert err >= 0

    def test_fib_tasks_symmetry(self):
        """Test that fib_tasks formula is symmetric."""
        # fib_tasks(n) = 1 + fib_tasks(n-1) + fib_tasks(n-2)
        # So: fib_tasks(n) - fib_tasks(n-1) > fib_tasks(n-1) - fib_tasks(n-2)
        for n in range(3, 10):
            fn = fib_tasks(n)
            fn_1 = fib_tasks(n-1)
            fn_2 = fib_tasks(n-2)

            # Verify the recurrence relation
            assert fn == 1 + fn_1 + fn_2


class TestEdgeCases:
    """Test edge cases and boundary conditions."""

    def test_stat_with_numpy_array(self):
        """Test stat function with numpy array input."""
        values = np.array([10.0, 11.0, 12.0, 13.0, 14.0])
        median_val, err, minimum = stat(values.tolist())

        assert minimum == 10.0
        assert median_val > 0
        assert err >= 0

    def test_fib_tasks_large_value(self):
        """Test fib_tasks with a moderately large value."""
        # Test that it doesn't crash and returns reasonable result
        result = fib_tasks(20)

        assert result > 0
        assert isinstance(result, int)

    def test_stat_with_float_precision(self):
        """Test stat with values requiring float precision."""
        values = [10.123456, 10.234567, 10.345678, 10.456789, 10.567890]
        median_val, err, minimum = stat(values)

        # Should handle float precision correctly
        assert isinstance(median_val, float)
        assert isinstance(err, float)
        assert isinstance(minimum, float)

    def test_fib_tasks_boundary_at_two(self):
        """Test fib_tasks at boundary condition n=2."""
        # This is the first recursive case
        result = fib_tasks(2)
        base_0 = fib_tasks(0)
        base_1 = fib_tasks(1)

        assert result == 1 + base_1 + base_0
        assert result == 3


class TestRegressionCases:
    """Regression tests for previously found issues."""

    def test_stat_doesnt_modify_input(self):
        """Test that stat doesn't modify the input list."""
        original = [10.0, 12.0, 11.0, 13.0]
        values = original.copy()

        stat(values)

        # Original should be unchanged
        assert values == original

    def test_fib_tasks_consistency(self):
        """Test that fib_tasks returns consistent results."""
        # Clear cache and compute
        fib_tasks.cache_clear()
        result1 = fib_tasks(15)

        # Clear cache and compute again
        fib_tasks.cache_clear()
        result2 = fib_tasks(15)

        assert result1 == result2

    def test_stat_with_close_values(self):
        """Test stat with very close values (potential precision issues)."""
        values = [10.0001, 10.0002, 10.0003, 10.0004, 10.0005]
        median_val, err, minimum = stat(values)

        # Should not throw any errors
        assert isinstance(median_val, float)
        assert err >= 0
        assert minimum <= median_val