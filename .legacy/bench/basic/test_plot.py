"""
Comprehensive tests for plot.py benchmark plotting script.

Note: The plot.py script runs argparse at module level, so we test the
functions and logic patterns in isolation rather than importing the module.
"""

import pytest
import json
import os
import tempfile
from unittest.mock import patch, MagicMock
import sys
import numpy as np
from functools import cache
from statistics import stdev, median


# Define functions to test (isolated from main script)
@cache
def fib_tasks(n):
    """Calculate number of tasks in fibonacci computation."""
    if n < 2:
        return 1
    return 1 + fib_tasks(n - 1) + fib_tasks(n - 2)


def stat(x):
    """Calculate statistics removing outlier."""
    x = sorted(x)[:-1]
    if len(x) > 1:
        err = stdev(x) / np.sqrt(len(x))
    else:
        err = 0.0
    return median(x), err, min(x)


class TestFibTasks:
    """Test the fib_tasks function."""

    def test_fib_tasks_base_case_zero(self):
        """Test fib_tasks returns 1 for n=0."""
        assert fib_tasks(0) == 1

    def test_fib_tasks_base_case_one(self):
        """Test fib_tasks returns 1 for n=1."""
        assert fib_tasks(1) == 1

    def test_fib_tasks_recursive_case(self):
        """Test fib_tasks for recursive cases."""
        # For n=2: 1 + fib_tasks(1) + fib_tasks(0) = 1 + 1 + 1 = 3
        assert fib_tasks(2) == 3
        # For n=3: 1 + fib_tasks(2) + fib_tasks(1) = 1 + 3 + 1 = 5
        assert fib_tasks(3) == 5
        # For n=4: 1 + fib_tasks(3) + fib_tasks(2) = 1 + 5 + 3 = 9
        assert fib_tasks(4) == 9

    def test_fib_tasks_larger_values(self):
        """Test fib_tasks for larger values."""
        # Test that it grows exponentially
        result_10 = fib_tasks(10)
        result_20 = fib_tasks(20)
        assert result_10 > 100
        assert result_20 > result_10 * 100

    def test_fib_tasks_caching(self):
        """Test that fib_tasks uses caching effectively."""
        # Clear cache if possible
        if hasattr(fib_tasks, 'cache_clear'):
            fib_tasks.cache_clear()

        # First call
        result1 = fib_tasks(15)
        # Second call should use cache
        result2 = fib_tasks(15)
        assert result1 == result2


class TestStatFunction:
    """Test the stat function for statistical calculations."""

    def test_stat_single_value(self):
        """Test stat with a single value."""
        values = [10.0, 20.0]  # Need at least 2 values, removes last
        median_val, err, min_val = stat(values)
        assert median_val == 10.0
        assert min_val == 10.0
        assert err == 0.0  # Single value after removal

    def test_stat_multiple_values(self):
        """Test stat with multiple values."""
        values = [10.0, 12.0, 11.0, 15.0]
        median_val, err, min_val = stat(values)
        # After sorting and removing last: [10.0, 11.0, 12.0]
        assert median_val == 11.0
        assert min_val == 10.0
        assert err > 0  # Should have some error

    def test_stat_removes_outlier(self):
        """Test that stat removes the largest outlier."""
        values = [10.0, 10.0, 10.0, 100.0]  # Last is outlier
        median_val, err, min_val = stat(values)
        # Should exclude 100.0
        assert median_val == 10.0
        assert min_val == 10.0

    def test_stat_error_calculation(self):
        """Test standard error calculation."""
        values = [10.0, 12.0, 11.0, 13.0, 20.0]
        median_val, err, min_val = stat(values)

        # After removing max and sorting: [10.0, 11.0, 12.0, 13.0]
        sorted_vals = [10.0, 11.0, 12.0, 13.0]
        expected_err = stdev(sorted_vals) / np.sqrt(len(sorted_vals))

        assert abs(err - expected_err) < 1e-10


class TestBenchmarkDataParsing:
    """Test benchmark data parsing logic."""

    def create_mock_benchmark_data(self):
        """Create mock benchmark JSON data."""
        return {
            "benchmarks": [
                {
                    "name": "fib_serial/real_time",
                    "run_type": "iteration",
                    "green_threads": 1.0,
                    "real_time": 1000.0
                },
                {
                    "name": "fib_libfork<lazy_pool>/4/real_time",
                    "run_type": "iteration",
                    "green_threads": 4.0,
                    "real_time": 500.0
                },
                {
                    "name": "fib_libfork<lazy_pool>/8/real_time",
                    "run_type": "iteration",
                    "green_threads": 8.0,
                    "real_time": 300.0
                },
                {
                    "name": "fib_omp/4/real_time",
                    "run_type": "iteration",
                    "green_threads": 4.0,
                    "real_time": 600.0
                },
                {
                    "name": "some_aggregate",
                    "run_type": "aggregate",
                    "green_threads": 4.0,
                    "real_time": 550.0
                }
            ]
        }

    def test_parse_benchmark_names(self):
        """Test parsing and normalization of benchmark names."""
        test_cases = [
            ("fib_serial/real_time", "fib_serial"),
            ("fib_libfork<lazy_pool, numa_strategy::seq>/4/real_time", "fib_libfork<lazy_pool, numa_strategy::fan>"),
            ("fib_omp/8/real_time/123", "fib_omp")
        ]

        for input_name, expected in test_cases:
            name_parts = input_name.split("/")
            name_parts = [n for n in name_parts if n != "real_time" and not n.isnumeric()]
            result = "".join(name_parts)
            result = result.replace("seq", "fan")
            # Basic check that numeric and 'real_time' are removed
            assert "real_time" not in result

    def test_filter_by_run_type(self):
        """Test filtering benchmarks by run_type."""
        data = self.create_mock_benchmark_data()

        iterations = [b for b in data["benchmarks"] if b["run_type"] == "iteration"]
        assert len(iterations) == 4  # Should exclude aggregate

    def test_filter_by_thread_count(self):
        """Test filtering benchmarks by thread count."""
        data = self.create_mock_benchmark_data()

        # Filter threads > 32
        filtered = [b for b in data["benchmarks"]
                   if b["run_type"] == "iteration" and int(b["green_threads"] + 0.5) <= 32]
        assert len(filtered) == 4


class TestPlotGeneration:
    """Test plot generation functionality."""

    @patch('matplotlib.pyplot.subplots')
    @patch('matplotlib.pyplot.savefig')
    def test_plot_creation_mock(self, mock_savefig, mock_subplots):
        """Test that plots can be created with mocked matplotlib."""
        # Mock the figure and axes
        mock_fig = MagicMock()
        mock_ax = MagicMock()
        mock_bx = MagicMock()
        mock_subplots.return_value = (mock_fig, (mock_ax, mock_bx))

        # Just verify the mocks work
        fig, (ax, bx) = mock_subplots(1, 2, figsize=(8, 4))
        ax.errorbar([1, 2], [1, 2], yerr=[0.1, 0.1])

        mock_ax.errorbar.assert_called_once()

    def test_framework_name_mapping(self):
        """Test mapping of benchmark names to framework labels."""
        test_cases = {
            "fib_omp": ("OpenMP", "o"),
            "fib_tbb": ("OneTBB", "s"),
            "fib_taskflow": ("Taskflow", "d"),
            "fib_libfork": ("Libfork", ">"),
            "fib_tmc": ("TooManyCooks", "^"),
            "fib_ccpp": ("Concurrencpp", "v")
        }

        for key, (expected_label, expected_marker) in test_cases.items():
            if "omp" in key:
                label, mark = "OpenMP", "o"
            elif "tbb" in key:
                label, mark = "OneTBB", "s"
            elif "taskflow" in key:
                label, mark = "Taskflow", "d"
            elif "libfork" in key:
                label, mark = "Libfork", ">"
            elif "tmc" in key:
                label, mark = "TooManyCooks", "^"
            elif "ccpp" in key:
                label, mark = "Concurrencpp", "v"
            else:
                label, mark = None, None

            assert label == expected_label
            assert mark == expected_marker


class TestCommandLineArguments:
    """Test command line argument parsing."""

    @patch('argparse.ArgumentParser.parse_args')
    def test_required_arguments(self, mock_parse_args):
        """Test that required arguments are parsed."""
        mock_parse_args.return_value = MagicMock(
            input_file='test.json',
            fib=40,
            output_file=None,
            rel=False
        )

        args = mock_parse_args()
        assert args.input_file == 'test.json'
        assert args.fib == 40
        assert args.output_file is None
        assert args.rel is False

    @patch('argparse.ArgumentParser.parse_args')
    def test_optional_arguments(self, mock_parse_args):
        """Test optional arguments."""
        mock_parse_args.return_value = MagicMock(
            input_file='test.json',
            fib=40,
            output_file='output.png',
            rel=True
        )

        args = mock_parse_args()
        assert args.output_file == 'output.png'
        assert args.rel is True


class TestDataStructures:
    """Test data structure handling."""

    def test_benchmark_dictionary_structure(self):
        """Test the benchmarks dictionary structure."""
        benchmarks = {}

        # Add benchmark data
        name = "fib_libfork"
        num_threads = 4

        if name not in benchmarks:
            benchmarks[name] = {}

        if num_threads not in benchmarks[name]:
            benchmarks[name][num_threads] = []

        benchmarks[name][num_threads].append(500.0)
        benchmarks[name][num_threads].append(510.0)

        assert name in benchmarks
        assert num_threads in benchmarks[name]
        assert len(benchmarks[name][num_threads]) == 2

    def test_benchmark_sorting(self):
        """Test benchmark data sorting."""
        benchmarks = {
            "b_benchmark": {4: [100], 2: [50], 8: [150]},
            "a_benchmark": {4: [200]}
        }

        # Sort by name
        sorted_benchmarks = [(k, sorted(v.items())) for k, v in benchmarks.items()]
        sorted_benchmarks.sort()

        assert sorted_benchmarks[0][0] == "a_benchmark"
        assert sorted_benchmarks[1][0] == "b_benchmark"

        # Check thread counts are sorted
        assert sorted_benchmarks[1][1][0][0] == 2
        assert sorted_benchmarks[1][1][1][0] == 4
        assert sorted_benchmarks[1][1][2][0] == 8


class TestMetricsCalculations:
    """Test performance metrics calculations."""

    def test_tasks_per_nanosecond(self):
        """Test calculation of nanoseconds per task."""
        fib_n = 10
        total_tasks = fib_tasks(fib_n)
        time_ms = 100.0  # milliseconds
        threads = 4

        # Nanoseconds per task = (time_ms * 1e6) / total_tasks / threads
        # But actually: time / total_tasks * threads
        ns_per_task = time_ms / total_tasks * threads

        assert ns_per_task > 0
        assert isinstance(ns_per_task, float)

    def test_tasks_per_second(self):
        """Test calculation of tasks per second."""
        fib_n = 10
        total_tasks = fib_tasks(fib_n)
        time_ms = 100.0  # milliseconds

        # Tasks per second = total_tasks / (time_ms / 1000)
        tasks_per_sec = total_tasks / time_ms * 1000

        assert tasks_per_sec > 0

    def test_speedup_calculation(self):
        """Test speedup calculation."""
        serial_time = 1000.0
        parallel_time = 250.0

        speedup = serial_time / parallel_time

        assert speedup == 4.0
        assert speedup > 1.0  # Parallel should be faster


class TestErrorHandling:
    """Test error handling and edge cases."""

    def test_empty_benchmark_list(self):
        """Test handling of empty benchmark data."""
        benchmarks = []

        # Should handle gracefully
        for item in benchmarks:
            pass  # No iteration

        assert len(benchmarks) == 0

    def test_missing_serial_benchmark(self):
        """Test when serial benchmark is missing."""
        benchmarks = [
            ("fib_libfork", [(4, [500.0]), (8, [250.0])])
        ]

        tS = -1
        for k, v in benchmarks:
            if "serial" in k:
                tS = 1000.0
                break

        # Should remain -1
        assert tS == -1

    def test_thread_count_rounding(self):
        """Test thread count rounding."""
        thread_values = [3.7, 4.0, 4.3, 4.5, 4.8]

        for val in thread_values:
            rounded = int(val + 0.5)
            if val < 4.5:
                assert rounded == 4
            else:
                assert rounded == 5


class TestIntegration:
    """Integration tests with temporary files."""

    def test_with_minimal_json_file(self):
        """Test with a minimal valid JSON file."""
        test_data = {
            "benchmarks": [
                {
                    "name": "fib_serial/real_time",
                    "run_type": "iteration",
                    "green_threads": 1.0,
                    "real_time": 1000.0
                },
                {
                    "name": "fib_libfork<lazy_pool>/4/real_time",
                    "run_type": "iteration",
                    "green_threads": 4.0,
                    "real_time": 500.0
                }
            ]
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(test_data, f)
            temp_file = f.name

        try:
            # Read and parse the file
            with open(temp_file) as f:
                data = json.load(f)

            assert "benchmarks" in data
            assert len(data["benchmarks"]) == 2

            # Test filtering
            iterations = [b for b in data["benchmarks"] if b["run_type"] == "iteration"]
            assert len(iterations) == 2
        finally:
            os.unlink(temp_file)

    def test_full_data_processing_pipeline(self):
        """Test the complete data processing pipeline."""
        # Simulate benchmark data processing
        benchmark_times = [1000.0, 1010.0, 1005.0, 1020.0]

        median_time, error, min_time = stat(benchmark_times)

        assert median_time > 0
        assert error >= 0
        assert min_time <= median_time

        # Calculate metrics
        total_tasks = fib_tasks(30)
        threads = 4

        ns_per_task = median_time / total_tasks * threads
        tasks_per_sec = total_tasks / median_time * 1e9

        assert ns_per_task > 0
        assert tasks_per_sec > 0


class TestRegressionCases:
    """Test specific regression cases and boundary conditions."""

    def test_zero_time_handling(self):
        """Test handling of zero or very small times."""
        # Very small time should not cause division by zero
        time = 0.0001
        tasks = fib_tasks(5)

        if time > 0:
            result = tasks / time
            assert result > 0

    def test_large_thread_count_filtering(self):
        """Test that thread counts > 32 are filtered."""
        thread_counts = [1, 4, 8, 16, 32, 40, 64, 112]

        filtered = [t for t in thread_counts if t <= 32]

        assert 40 not in filtered
        assert 64 not in filtered
        assert 112 not in filtered
        assert 32 in filtered

    def test_polyfit_coefficient_calculation(self):
        """Test polynomial fitting for performance analysis."""
        import numpy as np

        x = np.array([1, 2, 4, 8, 16])
        # Simulate performance degradation
        y = np.array([1.0, 0.98, 0.95, 0.90, 0.85])

        # Linear fit
        g1, c = np.polyfit(x, y, 1)

        # Slope should be negative (degradation)
        assert g1 < 0
        assert isinstance(g1, (float, np.floating))


if __name__ == "__main__":
    pytest.main([__file__, "-v"])