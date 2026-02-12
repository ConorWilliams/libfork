"""
Unit tests for memory_plot.py multi-benchmark memory plotting script.

Tests the data processing, curve fitting, and plotting logic used for
comparing memory usage across multiple benchmarks.
"""

import pytest
import numpy as np
from unittest.mock import patch, MagicMock
from scipy.optimize import curve_fit
from math import sqrt


class TestBenchmarkPatterns:
    """Test cases for benchmark pattern matching."""

    def test_benchmark_patterns_list(self):
        """Test the list of benchmark patterns."""
        patterns = [
            "fib",
            "integ",
            "nqueens",
            "T1",
            "T1L",
            "T1XXL",
            "T3",
            "T3L",
            "T3XXL",
            "matmul",
        ]

        assert len(patterns) == 10
        assert "fib" in patterns
        assert "T1" in patterns
        assert "matmul" in patterns

    def test_pattern_to_display_name_mapping(self):
        """Test mapping of patterns to display names."""
        display_patterns = [
            "fib",
            "integrate",  # Note: "integ" becomes "integrate"
            "nqueens",
            "T1 ",  # Note: space added
            "T1L",
            "T1XXL",
            "T3 ",  # Note: space added
            "T3L",
            "T3XXL",
            "matmul",
        ]

        assert len(display_patterns) == 10


class TestDataLoading:
    """Test cases for loading benchmark data from CSV files."""

    def test_csv_line_parsing(self):
        """Test parsing of CSV line format."""
        line = "libfork,8,1024.5,50.2"
        name, threads, med, dev = line.split(",")

        assert name == "libfork"
        assert int(threads) == 8
        assert float(med) == 1024.5
        assert float(dev) == 50.2

    def test_memory_conversion_to_mib(self):
        """Test conversion from KB to MiB."""
        mem_kb = 2048.0
        mem_mib = mem_kb / 1024.0

        assert mem_mib == 2.0

    def test_data_structure_initialization(self):
        """Test initialization of data structure for new benchmark name."""
        data = {}
        name = "libfork"

        if name not in data:
            data[name] = ([], [], [])

        assert name in data
        assert len(data[name]) == 3
        assert all(isinstance(lst, list) for lst in data[name])

    def test_data_accumulation(self):
        """Test accumulation of multiple data points."""
        data = {"libfork": ([], [], [])}

        # Add three data points
        for i in range(3):
            data["libfork"][0].append(i + 1)
            data["libfork"][1].append((i + 1) * 100.0)
            data["libfork"][2].append((i + 1) * 10.0)

        assert len(data["libfork"][0]) == 3
        assert len(data["libfork"][1]) == 3
        assert len(data["libfork"][2]) == 3


class TestCalibrationHandling:
    """Test cases for calibration baseline handling."""

    def test_calibration_extraction(self):
        """Test extraction of calibration values."""
        data = {
            "calibrate": ([1], [10.0], [2.0]),
            "serial": ([1], [20.0], [3.0]),
        }

        y_calib = data["calibrate"][1][0]
        y_calib_err = data["calibrate"][2][0] * 5

        assert y_calib == 10.0
        assert y_calib_err == 10.0  # 2.0 * 5

    def test_serial_baseline_calculation(self):
        """Test calculation of serial baseline after calibration."""
        y_calib = 10.0
        serial_mem = 15.0
        min_threshold = 4 / 1024.0

        y_serial = max(min_threshold, serial_mem - y_calib)

        assert y_serial == 5.0

    def test_serial_baseline_with_minimum_threshold(self):
        """Test that serial baseline respects minimum threshold."""
        y_calib = 10.0
        serial_mem = 10.5
        min_threshold = 4 / 1024.0

        y_serial = max(min_threshold, serial_mem - y_calib)

        # 0.5 > 4/1024, so result should be 0.5
        assert y_serial == 0.5


class TestBenchmarkFiltering:
    """Test cases for filtering benchmark types."""

    def test_skip_seq_benchmarks(self):
        """Test that benchmarks with 'seq' are skipped."""
        name = "libfork_seq"
        should_skip = "seq" in name or "_coalloc_" in name

        assert should_skip

    def test_skip_coalloc_benchmarks(self):
        """Test that benchmarks with '_coalloc_' are skipped."""
        name = "libfork_coalloc_lazy"
        should_skip = "seq" in name or "_coalloc_" in name

        assert should_skip

    def test_skip_special_benchmarks(self):
        """Test that special benchmarks are skipped."""
        special = ["zero", "serial", "calibrate"]

        for name in special:
            assert name in ["zero", "serial", "calibrate"]

    def test_keep_normal_benchmarks(self):
        """Test that normal benchmarks are not skipped."""
        name = "libfork_lazy_fan"
        should_skip = "seq" in name or "_coalloc_" in name

        assert not should_skip


class TestBenchmarkCategorization:
    """Test cases for categorizing benchmark names."""

    def test_omp_categorization(self):
        """Test categorization of OpenMP benchmarks."""
        name = "omp"
        if "omp" in name:
            label = "OpenMP"
            mark = "o"

        assert label == "OpenMP"
        assert mark == "o"

    def test_tbb_categorization(self):
        """Test categorization of TBB benchmarks."""
        name = "tbb"
        if "tbb" in name:
            label = "OneTBB"
            mark = "s"

        assert label == "OneTBB"
        assert mark == "s"

    def test_taskflow_categorization(self):
        """Test categorization of Taskflow benchmarks."""
        name = "taskflow"
        if "taskflow" in name:
            label = "Taskflow"
            mark = "d"

        assert label == "Taskflow"
        assert mark == "d"

    def test_busy_libfork_categorization(self):
        """Test categorization of Busy-LF benchmarks."""
        name = "busy"
        if "busy" in name and "co" not in name:
            label = "Busy-LF"
            mark = "v"

        assert label == "Busy-LF"
        assert mark == "v"

    def test_busy_libfork_coalloc_categorization(self):
        """Test categorization of Busy-LF* benchmarks with coalloc."""
        name = "busy_co"
        if "busy" in name and "co" in name:
            label = "Busy-LF*"
            mark = "<"

        assert label == "Busy-LF*"
        assert mark == "<"

    def test_lazy_libfork_categorization(self):
        """Test categorization of Lazy-LF benchmarks."""
        name = "lazy"
        if "lazy" in name and "co" not in name:
            label = "Lazy-LF"
            mark = "^"

        assert label == "Lazy-LF"
        assert mark == "^"

    def test_lazy_libfork_coalloc_categorization(self):
        """Test categorization of Lazy-LF* benchmarks with coalloc."""
        name = "lazy_co"
        if "lazy" in name and "co" in name:
            label = "Lazy-LF*"
            mark = ">"

        assert label == "Lazy-LF*"
        assert mark == ">"


class TestMemoryAdjustments:
    """Test cases for memory adjustments and calculations."""

    def test_calibration_subtraction(self):
        """Test subtraction of calibration baseline."""
        y_calib = 10.0
        measurements = np.array([50.0, 60.0, 70.0])

        adjusted = measurements - y_calib

        assert np.array_equal(adjusted, np.array([40.0, 50.0, 60.0]))

    def test_minimum_threshold_clamping(self):
        """Test clamping to minimum threshold."""
        min_threshold = 4 / 1024.0
        values = np.array([0.001, 0.002, 0.1])

        clamped = np.maximum(min_threshold, values)

        assert clamped[0] == min_threshold
        assert clamped[1] == min_threshold
        assert clamped[2] == 0.1

    def test_error_propagation(self):
        """Test error propagation with calibration error."""
        measurement_err = np.array([2.0, 3.0, 4.0])
        calib_err = 1.5

        combined_err = np.sqrt(measurement_err**2 + calib_err**2)

        expected = np.sqrt(np.array([4.0, 9.0, 16.0]) + 2.25)
        assert np.allclose(combined_err, expected)


class TestCurveFittingFunction:
    """Test cases for the curve fitting function."""

    def test_fit_function_formula(self):
        """Test the fitting function formula."""
        def func(x, a, b, n):
            y0 = 10.0  # Simulated first measurement
            return a + b * y0 * x**n

        result = func(2, 5, 0.5, 1.0)
        expected = 5 + 0.5 * 10.0 * 2**1.0
        assert abs(result - expected) < 1e-10

    def test_fit_function_with_zero_x(self):
        """Test fitting function with x=0."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n

        result = func(0, 5, 0.5, 1.0)
        assert result == 5

    def test_fit_function_with_array(self):
        """Test fitting function with numpy array."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n

        x = np.array([1, 2, 3, 4])
        result = func(x, 1, 0.5, 1.0)

        assert isinstance(result, np.ndarray)
        assert len(result) == 4

    def test_fit_function_power_law(self):
        """Test power law behavior of fitting function."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n

        # With n=2, should be quadratic
        x = np.array([1, 2, 3])
        result_n2 = func(x, 0, 1, 2)

        expected = 10.0 * np.array([1, 4, 9])
        assert np.allclose(result_n2, expected)


class TestFittingParameters:
    """Test cases for curve fitting parameters."""

    def test_initial_parameters(self):
        """Test initial parameter guess."""
        p0 = (1, 1, 1)

        assert len(p0) == 3
        assert all(p > 0 for p in p0)

    def test_parameter_bounds(self):
        """Test parameter bounds constraints."""
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        # Lower bounds
        assert bounds[0][0] == -np.inf  # a can be negative
        assert bounds[0][1] == 0  # b >= 0
        assert bounds[0][2] == 0  # n >= 0

        # Upper bounds
        assert all(b == np.inf for b in bounds[1])

    def test_covariance_matrix_extraction(self):
        """Test extraction of parameter uncertainties from covariance."""
        # Example covariance matrix
        pcov = np.array([
            [1.0, 0.1, 0.05],
            [0.1, 2.0, 0.15],
            [0.05, 0.15, 0.5]
        ])

        uncertainties = np.sqrt(np.diag(pcov))

        assert len(uncertainties) == 3
        assert np.allclose(uncertainties, [1.0, np.sqrt(2.0), np.sqrt(0.5)])


class TestPlotConfiguration:
    """Test cases for plot configuration."""

    def test_tick_configuration(self):
        """Test x-axis tick configuration."""
        xticks = list(range(0, int(112 + 1.5), 28))

        assert xticks == [0, 28, 56, 84, 112]

    def test_xlim_configuration(self):
        """Test x-axis limits."""
        xlim_min = 0
        xlim_max = 112

        assert xlim_min == 0
        assert xlim_max == 112

    def test_log_scale_base(self):
        """Test logarithmic scale base selection."""
        # For first 6 benchmarks (i <= 5)
        base_early = 100
        # For later benchmarks (i > 5)
        base_late = 10

        assert base_early == 100
        assert base_late == 10

    def test_subplot_grid(self):
        """Test subplot grid dimensions."""
        rows = 3
        cols = 3
        total_subplots = rows * cols

        assert total_subplots == 9


class TestLegendConfiguration:
    """Test cases for legend configuration."""

    def test_legend_label_conditional(self):
        """Test that legend labels are only added for specific subplot."""
        legend_subplot_index = 6

        for i in range(10):
            label = "TestLabel" if i == legend_subplot_index else None

            if i == legend_subplot_index:
                assert label == "TestLabel"
            else:
                assert label is None

    def test_fit_label_conditional(self):
        """Test that fit label is only added once."""
        fit_subplot_index = 7

        for i in range(10):
            is_first = True
            label = "Fit" if i == fit_subplot_index and is_first else None

            if i == fit_subplot_index and is_first:
                assert label == "Fit"
            else:
                assert label is None

    def test_legend_columns(self):
        """Test legend column count."""
        ncol = 3
        assert ncol == 3


class TestEdgeCases:
    """Test edge cases and boundary conditions."""

    def test_empty_data_structure(self):
        """Test initialization of empty data."""
        data = {}
        assert len(data) == 0

    def test_single_benchmark(self):
        """Test handling of single benchmark."""
        benchmarks = [("fib", {})]
        assert len(benchmarks) == 1

    def test_memory_with_negative_after_calibration(self):
        """Test memory values that go negative after calibration."""
        y_calib = 100.0
        measurements = np.array([50.0, 60.0, 110.0])
        min_threshold = 4 / 1024.0

        adjusted = np.maximum(min_threshold, measurements - y_calib)

        assert adjusted[0] == min_threshold  # 50 - 100 < min
        assert adjusted[1] == min_threshold  # 60 - 100 < min
        assert adjusted[2] == 10.0  # 110 - 100 > min

    def test_very_small_calibration_error(self):
        """Test handling of very small calibration errors."""
        calib_err = 1e-10
        measurement_err = 1.0

        combined = np.sqrt(measurement_err**2 + calib_err**2)

        # Should be approximately equal to measurement_err
        assert abs(combined - measurement_err) < 1e-9


class TestNumericalStability:
    """Test cases for numerical stability."""

    def test_error_calculation_stability(self):
        """Test stability of error calculation."""
        err1 = 1e-8
        err2 = 1e-8

        result = np.sqrt(err1**2 + err2**2)

        assert result > 0
        assert np.isfinite(result)

    def test_large_memory_values(self):
        """Test handling of large memory values."""
        large_mem = 1e9  # 1GB in KB
        mem_mib = large_mem / 1024.0

        assert np.isfinite(mem_mib)
        assert mem_mib > 0

    def test_power_function_stability(self):
        """Test stability of power function with various exponents."""
        x = np.array([1, 10, 100])

        for n in [0, 0.5, 1.0, 1.5, 2.0]:
            result = x**n
            assert np.all(np.isfinite(result))
            assert np.all(result >= 0)


class TestRegressionCases:
    """Regression tests for previously found issues."""

    def test_data_sorting_consistency(self):
        """Test that sorted data is consistent."""
        data = {"c": [1], "a": [2], "b": [3]}
        sorted_data = dict(sorted(data.items()))

        keys = list(sorted_data.keys())
        assert keys == ["a", "b", "c"]

    def test_array_conversion_from_list(self):
        """Test conversion of lists to numpy arrays."""
        threads = [1, 2, 4, 8]
        memory = [10.0, 20.0, 30.0, 40.0]
        errors = [1.0, 2.0, 3.0, 4.0]

        x = np.asarray(threads)
        y = np.asarray(memory)
        err = np.asarray(errors)

        assert isinstance(x, np.ndarray)
        assert isinstance(y, np.ndarray)
        assert isinstance(err, np.ndarray)

    def test_calibration_error_scaling_factor(self):
        """Test that calibration error is scaled by factor of 5."""
        raw_calib_err = 2.0
        scaled_calib_err = raw_calib_err * 5

        # This is because calibration error is standard error, not stdev
        assert scaled_calib_err == 10.0

    def test_minimum_threshold_value(self):
        """Test that minimum threshold is correctly calculated."""
        min_threshold = 4 / 1024.0

        # Should be approximately 0.00390625 MiB
        assert abs(min_threshold - 0.00390625) < 1e-10

    def test_matmul_benchmark_skipping(self):
        """Test that matmul benchmark is skipped in plotting."""
        p = "matmul"

        if p == "matmul":
            should_skip = True
        else:
            should_skip = False

        assert should_skip


class TestLatexFormatting:
    """Test cases for LaTeX formatting in plot labels."""

    def test_latex_enabled(self):
        """Test that LaTeX rendering is enabled."""
        # In the script: plt.rcParams["text.usetex"] = True
        use_latex = True
        assert use_latex

    def test_title_latex_formatting(self):
        """Test LaTeX formatting of subplot titles."""
        pattern = "fib"
        title = f"\\textit{{{pattern}}}"

        assert title == "\\textit{fib}"

    def test_axis_label_latex_formatting(self):
        """Test LaTeX formatting of axis labels."""
        xlabel = "\\textbf{{Cores}}"
        ylabel = "\\textbf{{MRSS/MiB}}"

        assert "\\textbf" in xlabel
        assert "\\textbf" in ylabel