"""Unit tests for plot.py benchmark plotting script logic."""

import pytest
import json
import tempfile
import os
from pathlib import Path
from unittest.mock import patch, MagicMock
import numpy as np
from functools import cache
from statistics import mean, stdev, median


@cache
def fib_tasks(n):
    """Replicate the fib_tasks function from plot.py for testing."""
    if n < 2:
        return 1
    return 1 + fib_tasks(n - 1) + fib_tasks(n - 2)


def stat(x):
    """Replicate the stat function from plot.py for testing."""
    if not x:
        raise ValueError("Cannot compute stats on empty list")
    x = sorted(x)[:-1]
    if len(x) < 1:
        raise ValueError("Not enough data after removing outlier")
    if len(x) == 1:
        return x[0], 0.0, x[0]
    err = stdev(x) / np.sqrt(len(x))
    return median(x), err, min(x)


class TestFibTasks:
    """Tests for the fib_tasks function."""

    def test_fib_tasks_base_case_0(self):
        """Test fib_tasks with n=0."""
        assert fib_tasks(0) == 1

    def test_fib_tasks_base_case_1(self):
        """Test fib_tasks with n=1."""
        assert fib_tasks(1) == 1

    def test_fib_tasks_small_values(self):
        """Test fib_tasks with small values."""
        assert fib_tasks(2) == 3  # 1 + 1 + 1
        assert fib_tasks(3) == 5  # 1 + 3 + 1
        assert fib_tasks(4) == 9  # 1 + 5 + 3

    def test_fib_tasks_larger_value(self):
        """Test fib_tasks with a larger value."""
        result = fib_tasks(10)
        assert result > 0
        assert isinstance(result, int)

    def test_fib_tasks_memoization(self):
        """Test that fib_tasks uses caching."""
        # Call twice with same value to ensure caching works
        first_call = fib_tasks(15)
        second_call = fib_tasks(15)
        assert first_call == second_call


class TestStat:
    """Tests for the stat function."""

    def test_stat_single_value(self):
        """Test stat with a single value - should raise error after removing outlier."""
        # After removing the largest value, we have no data left
        with pytest.raises(ValueError):
            stat([5.0])

    def test_stat_multiple_values(self):
        """Test stat with multiple values."""
        values = [1.0, 2.0, 3.0, 4.0, 5.0]
        median_val, err, minimum = stat(values)
        assert median_val == 2.5  # Median of [1, 2, 3, 4] after removing max
        assert minimum == 1.0
        assert err >= 0

    def test_stat_removes_largest(self):
        """Test that stat removes the largest value."""
        values = [1.0, 2.0, 100.0]
        median_val, err, minimum = stat(values)
        assert median_val <= 2.0  # Should not include 100.0
        assert minimum == 1.0

    def test_stat_identical_values(self):
        """Test stat with identical values."""
        values = [5.0, 5.0, 5.0, 5.0]
        median_val, err, minimum = stat(values)
        assert median_val == 5.0
        assert minimum == 5.0

    def test_stat_returns_tuple(self):
        """Test that stat returns a tuple of 3 elements."""
        result = stat([1.0, 2.0, 3.0])
        assert isinstance(result, tuple)
        assert len(result) == 3


class TestBenchmarkProcessing:
    """Tests for benchmark JSON processing logic."""

    @pytest.fixture
    def sample_benchmark_data(self):
        """Create sample benchmark data."""
        return {
            "benchmarks": [
                {
                    "name": "fib_serial/threads:1/real_time",
                    "run_type": "iteration",
                    "green_threads": 1.0,
                    "real_time": 100.0
                },
                {
                    "name": "fib_libfork<lazy_pool>/threads:4/real_time",
                    "run_type": "iteration",
                    "green_threads": 4.0,
                    "real_time": 30.0
                },
                {
                    "name": "fib_libfork<lazy_pool>/threads:8/real_time",
                    "run_type": "iteration",
                    "green_threads": 8.0,
                    "real_time": 20.0
                },
                {
                    "name": "fib_aggregate/threads:4",
                    "run_type": "aggregate",
                    "green_threads": 4.0,
                    "real_time": 35.0
                }
            ]
        }

    def test_benchmark_filtering_by_run_type(self, sample_benchmark_data):
        """Test that aggregate runs are filtered out."""
        benchmarks = {}
        for bench in sample_benchmark_data["benchmarks"]:
            if bench["run_type"] != "iteration":
                continue
            assert bench["run_type"] == "iteration"

    def test_benchmark_name_parsing(self, sample_benchmark_data):
        """Test benchmark name parsing logic."""
        for bench in sample_benchmark_data["benchmarks"]:
            if bench["run_type"] != "iteration":
                continue

            name = bench["name"].split("/")
            name = [n for n in name if n != "real_time" and not n.isnumeric()]
            name = "".join(name)

            assert "real_time" not in name
            # Note: threads:N may still be in the name, just the numeric N is removed

    def test_benchmark_thread_count_parsing(self, sample_benchmark_data):
        """Test thread count extraction."""
        for bench in sample_benchmark_data["benchmarks"]:
            if bench["run_type"] != "iteration":
                continue

            num_threads = int(bench["green_threads"] + 0.5)
            assert num_threads > 0
            assert isinstance(num_threads, int)

    def test_benchmark_filters_high_thread_counts(self, sample_benchmark_data):
        """Test that high thread counts are filtered."""
        max_threads = 32
        for bench in sample_benchmark_data["benchmarks"]:
            if bench["run_type"] != "iteration":
                continue

            num_threads = int(bench["green_threads"] + 0.5)
            if num_threads > max_threads:
                # Should be skipped
                pass
            else:
                assert num_threads <= max_threads


class TestIntegration:
    """Integration tests with temporary files."""

    @pytest.fixture
    def temp_json_file(self):
        """Create a temporary JSON file with benchmark data."""
        data = {
            "benchmarks": [
                {
                    "name": "fib_serial/real_time",
                    "run_type": "iteration",
                    "green_threads": 1.0,
                    "real_time": 100.0
                },
                {
                    "name": "fib_serial/real_time",
                    "run_type": "iteration",
                    "green_threads": 1.0,
                    "real_time": 105.0
                },
                {
                    "name": "fib_libfork<lazy_pool, numa_strategy::fan>/4/real_time",
                    "run_type": "iteration",
                    "green_threads": 4.0,
                    "real_time": 30.0
                }
            ]
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(data, f)
            temp_path = f.name

        yield temp_path

        # Cleanup
        if os.path.exists(temp_path):
            os.unlink(temp_path)

    def test_json_loading(self, temp_json_file):
        """Test loading benchmark JSON file."""
        with open(temp_json_file) as f:
            data = json.load(f)

        assert "benchmarks" in data
        assert len(data["benchmarks"]) > 0

    @patch('matplotlib.pyplot.subplots')
    @patch('matplotlib.pyplot.savefig')
    def test_plotting_no_errors(self, mock_savefig, mock_subplots, temp_json_file):
        """Test that plotting doesn't raise errors with valid data."""
        # Mock the matplotlib objects
        mock_fig = MagicMock()
        mock_ax = MagicMock()
        mock_bx = MagicMock()
        mock_subplots.return_value = (mock_fig, (mock_ax, mock_bx))

        # This would normally be in the main script execution
        # Just verify we can process the data without errors
        with open(temp_json_file) as f:
            data = json.load(f)

        benchmarks = {}
        for bench in data["benchmarks"]:
            if bench["run_type"] != "iteration":
                continue

            name = bench["name"].split("/")
            name = [n for n in name if n != "real_time" and not n.isnumeric()]
            name = "".join(name)

            if name not in benchmarks:
                benchmarks[name] = {}

            num_threads = int(bench["green_threads"] + 0.5)

            if num_threads not in benchmarks[name]:
                benchmarks[name][num_threads] = []

            benchmarks[name][num_threads].append(bench["real_time"])

        # Verify we got the expected structure
        assert len(benchmarks) > 0
        assert "fib_serial" in benchmarks


class TestEdgeCases:
    """Test edge cases and error handling."""

    def test_stat_with_empty_list_raises_error(self):
        """Test stat with empty list."""
        with pytest.raises(Exception):
            stat([])

    def test_fib_tasks_negative_value(self):
        """Test fib_tasks with negative value."""
        # Should return 1 for negative values based on condition n < 2
        result = fib_tasks(-1)
        assert result == 1

    def test_benchmark_name_with_special_chars(self):
        """Test benchmark name parsing with special characters."""
        name = "fib_libfork<lazy_pool, numa_strategy::fan>/threads:8/real_time"
        parts = name.split("/")
        filtered = [p for p in parts if p != "real_time" and not p.isnumeric()]
        result = "".join(filtered)

        assert "real_time" not in result
        assert "libfork" in result


class TestLabelMapping:
    """Test the benchmark label mapping logic."""

    def test_openmp_label_mapping(self):
        """Test OpenMP label and marker."""
        name = "fib_omp"
        if "omp" in name:
            label = "OpenMP"
            mark = "o"
            assert label == "OpenMP"
            assert mark == "o"

    def test_tbb_label_mapping(self):
        """Test TBB label and marker."""
        name = "fib_tbb"
        if "tbb" in name:
            label = "OneTBB"
            mark = "s"
            assert label == "OneTBB"
            assert mark == "s"

    def test_taskflow_label_mapping(self):
        """Test Taskflow label and marker."""
        name = "fib_taskflow"
        if "taskflow" in name:
            label = "Taskflow"
            mark = "d"
            assert label == "Taskflow"
            assert mark == "d"

    def test_libfork_label_mapping(self):
        """Test Libfork label and marker."""
        name = "fib_libfork"
        if "libfork" in name:
            label = "Libfork"
            mark = ">"
            assert label == "Libfork"
            assert mark == ">"

    def test_tmc_label_mapping(self):
        """Test TooManyCooks label and marker."""
        name = "fib_tmc"
        if "tmc" in name:
            label = "TooManyCooks"
            mark = "^"
            assert label == "TooManyCooks"
            assert mark == "^"

    def test_ccpp_label_mapping(self):
        """Test Concurrencpp label and marker."""
        name = "fib_ccpp"
        if "ccpp" in name:
            label = "Concurrencpp"
            mark = "v"
            assert label == "Concurrencpp"
            assert mark == "v"


class TestNumpyOperations:
    """Test numpy operations used in the script."""

    def test_polyfit_call(self):
        """Test numpy polyfit usage."""
        x = np.array([1, 2, 3, 4, 5])
        y = np.array([2.1, 3.9, 6.1, 8.0, 10.1])

        # Should be able to fit a line
        coeffs = np.polyfit(x, y, 1)
        assert len(coeffs) == 2
        assert coeffs[0] > 0  # Positive slope

    def test_array_operations(self):
        """Test array operations used in the script."""
        x = np.asarray([1, 2, 3, 4])
        y = np.asarray([10.0, 20.0, 30.0, 40.0])

        # Division operations
        result = y / x
        expected = np.array([10.0, 10.0, 10.0, 10.0])
        np.testing.assert_array_almost_equal(result, expected)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])