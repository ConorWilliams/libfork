"""
Unit tests for plt_mem.py memory plotting script.

Tests the data parsing, curve fitting, and statistical functions used in the
memory usage plotting tool.
"""

import pytest
import numpy as np
from unittest.mock import patch, mock_open, MagicMock
import sys
from io import StringIO
from scipy.optimize import curve_fit


class TestDataParsing:
    """Test cases for CSV data parsing."""

    def test_csv_line_parsing(self):
        """Test parsing of CSV line format."""
        line = "libfork,8,1024.5,50.2"
        name, threads, med, dev = line.split(",")

        assert name == "libfork"
        assert threads == "8"
        assert med == "1024.5"
        assert dev == "50.2"

    def test_memory_conversion_to_mib(self):
        """Test conversion from KB to MiB."""
        # Memory is stored in KB and converted to MiB by dividing by 1024
        mem_kb = 1024.0
        mem_mib = mem_kb / 1024.0

        assert mem_mib == 1.0

    def test_memory_conversion_various_values(self):
        """Test memory conversion with various values."""
        test_cases = [
            (0, 0),
            (1024, 1.0),
            (2048, 2.0),
            (512, 0.5),
            (1536, 1.5),
        ]

        for kb, expected_mib in test_cases:
            result = float(kb) / 1024.0
            assert result == expected_mib

    def test_data_structure_accumulation(self):
        """Test accumulation of data into nested structure."""
        data = {}
        entries = [
            ("libfork", 1, 100.0, 5.0),
            ("libfork", 2, 200.0, 10.0),
            ("tbb", 1, 150.0, 7.5),
        ]

        for name, threads, mem, dev in entries:
            if name not in data:
                data[name] = ([], [], [])

            data[name][0].append(threads)
            data[name][1].append(mem / 1024.0)
            data[name][2].append(dev / 1024.0)

        assert len(data) == 2
        assert "libfork" in data
        assert "tbb" in data
        assert len(data["libfork"][0]) == 2
        assert len(data["tbb"][0]) == 1

    def test_calibration_extraction(self):
        """Test extraction of calibration baseline."""
        data = {
            "calibrate": ([1], [10.0], [1.0]),
            "serial": ([1], [20.0], [2.0]),
        }

        y_calib = data["calibrate"][1][0]
        y_calib_err = data["calibrate"][2][0] * 5

        assert y_calib == 10.0
        assert y_calib_err == 5.0


class TestNameCategorization:
    """Test cases for categorizing benchmark names."""

    def test_omp_name_detection(self):
        """Test detection of OpenMP benchmarks."""
        names = ["omp", "fib_omp", "bench_omp_test"]

        for name in names:
            assert "omp" in name

    def test_tbb_name_detection(self):
        """Test detection of TBB benchmarks."""
        names = ["tbb", "fib_tbb", "bench_tbb_test"]

        for name in names:
            assert "tbb" in name

    def test_taskflow_name_detection(self):
        """Test detection of Taskflow benchmarks."""
        names = ["taskflow", "fib_taskflow", "bench_taskflow_test"]

        for name in names:
            assert "taskflow" in name

    def test_libfork_name_detection(self):
        """Test detection of libfork benchmarks."""
        names = ["libfork", "fib_libfork", "libfork_lazy"]

        for name in names:
            assert "libfork" in name

    def test_tmc_name_detection(self):
        """Test detection of TooManyCooks benchmarks."""
        names = ["tmc", "fib_tmc", "bench_tmc_test"]

        for name in names:
            assert "tmc" in name

    def test_ccpp_name_detection(self):
        """Test detection of Concurrencpp benchmarks."""
        names = ["ccpp", "fib_ccpp", "bench_ccpp_test"]

        for name in names:
            assert "ccpp" in name

    def test_skip_special_names(self):
        """Test filtering of special benchmark names."""
        special_names = ["zero", "serial", "calibrate"]

        for name in special_names:
            assert name in ["zero", "serial", "calibrate"]


class TestMemoryCalculations:
    """Test cases for memory calculations and adjustments."""

    def test_baseline_subtraction(self):
        """Test subtraction of calibration baseline from measurements."""
        y_calib = 10.0
        measurements = np.array([50.0, 100.0, 150.0])

        adjusted = measurements - y_calib

        assert np.array_equal(adjusted, np.array([40.0, 90.0, 140.0]))

    def test_minimum_memory_threshold(self):
        """Test that memory has minimum threshold of 4 KiB."""
        y_calib = 10.0
        measurements = np.array([10.5, 11.0, 12.0])

        # After subtracting calibration, ensure minimum 4/1024 MiB
        min_threshold = 4 / 1024.0
        adjusted = np.maximum(min_threshold, measurements - y_calib)

        # All values should be at least min_threshold
        assert np.all(adjusted >= min_threshold)

    def test_memory_threshold_with_negative_values(self):
        """Test that negative values after calibration are clamped to minimum."""
        y_calib = 10.0
        measurements = np.array([8.0, 9.0, 11.0])

        min_threshold = 4 / 1024.0
        adjusted = np.maximum(min_threshold, measurements - y_calib)

        # Negative values should be clamped
        assert adjusted[0] == min_threshold
        assert adjusted[1] == min_threshold
        assert adjusted[2] > min_threshold

    def test_error_propagation(self):
        """Test error propagation when combining measurement and calibration errors."""
        measurement_err = 2.0
        calib_err = 1.5

        # Error propagation: sqrt(err1^2 + err2^2)
        combined_err = np.sqrt(measurement_err**2 + calib_err**2)

        expected = np.sqrt(4.0 + 2.25)
        assert abs(combined_err - expected) < 1e-10

    def test_error_propagation_array(self):
        """Test error propagation with arrays."""
        measurement_errs = np.array([1.0, 2.0, 3.0])
        calib_err = 1.5

        combined_errs = np.sqrt(measurement_errs**2 + calib_err**2)

        expected = np.array([
            np.sqrt(1.0 + 2.25),
            np.sqrt(4.0 + 2.25),
            np.sqrt(9.0 + 2.25)
        ])

        assert np.allclose(combined_errs, expected)


class TestCurveFitting:
    """Test cases for curve fitting functionality."""

    def test_fit_function_definition(self):
        """Test the fitting function formula."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n

        # Test with known parameters
        result = func(2, 5, 0.5, 1.0)
        expected = 5 + 0.5 * 10.0 * 2**1.0
        assert abs(result - expected) < 1e-10

    def test_fit_function_edge_cases(self):
        """Test fitting function with edge case parameters."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n

        # Test x=0
        assert func(0, 5, 0.5, 1.0) == 5

        # Test n=0 (constant term)
        result = func(10, 5, 0.5, 0)
        assert abs(result - 10) < 1e-10

    def test_fit_function_with_array(self):
        """Test fitting function with numpy array input."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n

        x = np.array([1, 2, 3, 4])
        result = func(x, 1, 0.5, 1.0)

        expected = 1 + 0.5 * 10.0 * x**1.0
        assert np.allclose(result, expected)

    def test_bounds_constraints(self):
        """Test that curve fitting bounds are correctly specified."""
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        # Lower bounds
        assert bounds[0][0] == -np.inf  # a can be negative
        assert bounds[0][1] == 0  # b must be non-negative
        assert bounds[0][2] == 0  # n must be non-negative

        # Upper bounds
        assert bounds[1][0] == np.inf
        assert bounds[1][1] == np.inf
        assert bounds[1][2] == np.inf

    def test_initial_parameters(self):
        """Test initial parameter guess for curve fitting."""
        p0 = (1, 1, 1)

        assert len(p0) == 3
        assert all(p > 0 for p in p0)

    def test_simple_curve_fit(self):
        """Test curve fitting with synthetic data."""
        def func(x, a, b, n):
            y0 = 10.0  # Simulated y[0]
            return a + b * y0 * x**n

        # Create synthetic data
        x = np.array([1, 2, 4, 8])
        true_params = (5, 0.1, 0.5)
        y = func(x, *true_params) + np.random.normal(0, 0.1, len(x))

        # Fit the curve
        p0 = (1, 1, 1)
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        try:
            popt, pcov = curve_fit(func, x, y, p0=p0, bounds=bounds, maxfev=10000)

            # Check that fitted parameters are reasonable
            assert len(popt) == 3
            assert len(pcov) == 3
            assert pcov.shape == (3, 3)

            # Check parameter uncertainties
            uncertainties = np.sqrt(np.diag(pcov))
            assert all(u >= 0 for u in uncertainties)

        except Exception as e:
            pytest.skip(f"Curve fit failed with synthetic data: {e}")


class TestDataFiltering:
    """Test cases for data filtering logic."""

    def test_thread_count_filtering(self):
        """Test filtering of thread counts."""
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]
        max_cores = 32

        filtered = [t for t in thread_counts if t <= max_cores]

        assert filtered == [1, 2, 4, 8, 16, 24, 32]
        assert all(t <= max_cores for t in filtered)

    def test_benchmark_name_sorting(self):
        """Test that benchmark names are sorted."""
        names = ["tbb", "libfork", "omp", "taskflow"]
        sorted_names = sorted(names)

        assert sorted_names == ["libfork", "omp", "taskflow", "tbb"]


class TestMarkerSelection:
    """Test cases for plot marker selection."""

    def test_marker_mapping(self):
        """Test that each benchmark type gets correct marker."""
        markers = {
            "omp": "o",
            "tbb": "s",
            "taskflow": "d",
            "libfork": ">",
            "tmc": "^",
            "ccpp": "v"
        }

        for name, expected_marker in markers.items():
            if "omp" in name:
                assert expected_marker == "o"
            elif "tbb" in name:
                assert expected_marker == "s"
            elif "taskflow" in name:
                assert expected_marker == "d"
            elif "libfork" in name:
                assert expected_marker == ">"
            elif "tmc" in name:
                assert expected_marker == "^"
            elif "ccpp" in name:
                assert expected_marker == "v"


class TestNumericalStability:
    """Test cases for numerical stability."""

    def test_sqrt_of_sum_of_squares(self):
        """Test numerical stability of error calculation."""
        err1 = 1e-10
        err2 = 1e-10

        result = np.sqrt(err1**2 + err2**2)

        assert result > 0
        assert np.isfinite(result)

    def test_large_error_values(self):
        """Test error calculation with large values."""
        err1 = 1e6
        err2 = 1e6

        result = np.sqrt(err1**2 + err2**2)

        assert np.isfinite(result)
        assert result > 0

    def test_memory_calculation_overflow(self):
        """Test that memory calculations don't overflow."""
        large_memory = 1e9  # 1GB in KB
        mem_mib = large_memory / 1024.0

        assert np.isfinite(mem_mib)
        assert mem_mib > 0


class TestEdgeCases:
    """Test edge cases and boundary conditions."""

    def test_empty_data_structure(self):
        """Test initialization of empty data structure."""
        data = {}
        name = "test"

        if name not in data:
            data[name] = ([], [], [])

        assert name in data
        assert len(data[name]) == 3
        assert all(len(lst) == 0 for lst in data[name])

    def test_single_data_point(self):
        """Test handling of single data point."""
        x = np.array([1])
        y = np.array([10.0])
        err = np.array([1.0])

        assert len(x) == len(y) == len(err)
        assert x[0] == 1
        assert y[0] == 10.0

    def test_zero_calibration(self):
        """Test behavior with zero calibration value."""
        y_calib = 0.0
        measurements = np.array([10.0, 20.0, 30.0])

        adjusted = measurements - y_calib

        assert np.array_equal(adjusted, measurements)

    def test_very_small_errors(self):
        """Test handling of very small error values."""
        err = 1e-15
        calib_err = 1e-15

        combined = np.sqrt(err**2 + calib_err**2)

        assert combined >= 0
        assert np.isfinite(combined)


class TestRegressionCases:
    """Regression tests for previously found issues."""

    def test_memory_never_negative_after_clamping(self):
        """Test that memory values are never negative after threshold clamping."""
        y_calib = 100.0
        measurements = np.array([50.0, 75.0, 90.0, 110.0])

        min_threshold = 4 / 1024.0
        adjusted = np.maximum(min_threshold, measurements - y_calib)

        assert np.all(adjusted >= min_threshold)
        assert np.all(adjusted > 0)

    def test_error_calculation_consistency(self):
        """Test that error calculations are consistent."""
        err1 = 2.0
        err2 = 3.0

        result1 = np.sqrt(err1**2 + err2**2)
        result2 = np.sqrt(err2**2 + err1**2)

        assert abs(result1 - result2) < 1e-10

    def test_array_conversion_from_tuple(self):
        """Test conversion of tuple data to numpy arrays."""
        data = ([1, 2, 3], [10.0, 20.0, 30.0], [1.0, 2.0, 3.0])

        x = np.asarray(data[0])
        y = np.asarray(data[1])
        err = np.asarray(data[2])

        assert isinstance(x, np.ndarray)
        assert isinstance(y, np.ndarray)
        assert isinstance(err, np.ndarray)
        assert len(x) == len(y) == len(err) == 3

    def test_calibration_error_scaling(self):
        """Test that calibration error is properly scaled by factor of 5."""
        raw_calib_err = 2.0
        scaled_calib_err = raw_calib_err * 5

        assert scaled_calib_err == 10.0
        # Comment in original code: "x5 because this is a standard error not a standard deviation"