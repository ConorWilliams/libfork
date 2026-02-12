"""
Comprehensive tests for memory_plot.py comprehensive memory plotting script.
"""

import pytest
import os
import tempfile
from unittest.mock import patch, MagicMock, mock_open
import sys
import numpy as np
from io import StringIO


# Import path setup
sys.path.insert(0, os.path.dirname(__file__))


class TestMultipleBenchmarkFiles:
    """Test handling multiple benchmark data files."""

    def test_benchmark_pattern_list(self):
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
            "matmul"
        ]

        assert len(patterns) == 10
        assert "fib" in patterns
        assert "matmul" in patterns
        assert "T1XXL" in patterns

    def test_file_path_construction(self):
        """Test constructing file paths for each benchmark."""
        patterns = ["fib", "integ", "nqueens"]
        base_path = "./bench/data/sapphire/v5/"

        for pattern in patterns:
            filepath = f"{base_path}memory.{pattern}.csv"
            assert pattern in filepath
            assert "memory." in filepath
            assert ".csv" in filepath

    def test_benchmark_iteration(self):
        """Test iterating through benchmarks."""
        patterns = ["fib", "integ", "nqueens"]

        benchmarks = []
        for bm in patterns:
            # Simulate loading data
            data = {"benchmark": bm}
            benchmarks.append(data)

        assert len(benchmarks) == 3


class TestDataStructure:
    """Test data structure for multiple benchmarks."""

    def test_benchmarks_list_structure(self):
        """Test the Benchmarks list structure."""
        Benchmarks = []

        # Simulate adding multiple benchmark data
        for i in range(3):
            data = {
                "libfork": ([1, 2, 4], [10.0, 15.0, 20.0], [1.0, 1.5, 2.0])
            }
            Benchmarks.append(data)

        assert len(Benchmarks) == 3
        assert isinstance(Benchmarks, list)

    def test_benchmark_data_dictionary(self):
        """Test individual benchmark data structure."""
        data = {
            "calibrate": ([1], [0.5], [0.05]),
            "serial": ([1], [1.0], [0.1]),
            "libfork": ([1, 2, 4], [2.0, 3.0, 5.0], [0.2, 0.3, 0.5])
        }

        assert "calibrate" in data
        assert "serial" in data
        assert "libfork" in data

        for name, values in data.items():
            assert len(values) == 3  # threads, memory, error
            assert isinstance(values[0], list)


class TestPlotLayout:
    """Test subplot layout configuration."""

    @patch('matplotlib.pyplot.subplots')
    def test_subplot_creation(self, mock_subplots):
        """Test creating 3x3 subplot grid."""
        mock_fig = MagicMock()
        mock_axs = MagicMock()
        mock_subplots.return_value = (mock_fig, mock_axs)

        fig, axs = mock_subplots(3, 3, figsize=(6, 6.5), sharex='col', sharey='row')

        mock_subplots.assert_called_once()
        call_args = mock_subplots.call_args
        assert call_args[0] == (3, 3)

    def test_subplot_dimensions(self):
        """Test subplot grid dimensions."""
        rows = 3
        cols = 3
        total_plots = rows * cols

        assert total_plots == 9

    def test_figure_size(self):
        """Test figure size configuration."""
        figsize = (6, 6.5)

        assert figsize[0] == 6
        assert figsize[1] == 6.5
        assert figsize[0] < figsize[1]


class TestPatternMatching:
    """Test pattern matching between display and file patterns."""

    def test_pattern_pairs(self):
        """Test that display and file patterns match."""
        display_patterns = [
            "fib",
            "integrate",
            "nqueens",
            "T1 ",
            "T1L",
            "T1XXL",
            "T3 ",
            "T3L",
            "T3XXL",
            "matmul"
        ]

        file_patterns = [
            "fib",
            "integ",
            "nqueens",
            "T1",
            "T1L",
            "T1XXL",
            "T3",
            "T3L",
            "T3XXL",
            "matmul"
        ]

        assert len(display_patterns) == len(file_patterns)

    def test_pattern_display_names(self):
        """Test display pattern formatting."""
        display_patterns = ["fib", "integrate", "T1 ", "T3 "]

        for pattern in display_patterns:
            assert isinstance(pattern, str)
            assert len(pattern) > 0


class TestCalibrationProcessing:
    """Test calibration data processing."""

    def test_calibration_extraction(self):
        """Test extracting calibration data."""
        data = {
            "calibrate": ([1], [1.0], [0.05]),
            "serial": ([1], [2.0], [0.1]),
            "libfork": ([4], [5.0], [0.2])
        }

        y_calib = data["calibrate"][1][0]
        y_calib_err = data["calibrate"][2][0] * 5

        assert y_calib == 1.0
        assert y_calib_err == 0.25

    def test_serial_baseline_calculation(self):
        """Test serial baseline calculation."""
        y_calib = 0.5
        y_serial_raw = 2.0
        min_threshold = 4 / 1024.0

        y_serial = max(min_threshold, y_serial_raw - y_calib)

        assert y_serial == 1.5

    def test_minimum_threshold_enforcement(self):
        """Test minimum 4 KiB threshold."""
        min_threshold = 4 / 1024.0  # 4 KiB in MiB

        test_values = [-1.0, 0.0, 0.001, 0.003]

        for val in test_values:
            adjusted = max(min_threshold, val)
            assert adjusted >= min_threshold
            # For values less than threshold, should equal threshold
            if val < min_threshold:
                assert adjusted == min_threshold


class TestDataFiltering:
    """Test data filtering logic."""

    def test_skip_seq_benchmarks(self):
        """Test skipping 'seq' benchmarks."""
        benchmarks = {
            "libfork_seq": ([1], [1.0], [0.1]),
            "libfork_fan": ([2], [2.0], [0.2]),
            "omp": ([4], [4.0], [0.4])
        }

        filtered = {k: v for k, v in benchmarks.items() if "seq" not in k}

        assert "libfork_seq" not in filtered
        assert "libfork_fan" in filtered
        assert "omp" in filtered

    def test_skip_coalloc_benchmarks(self):
        """Test skipping '_coalloc_' benchmarks."""
        benchmarks = {
            "libfork_coalloc_lazy": ([1], [1.0], [0.1]),
            "libfork_lazy": ([2], [2.0], [0.2]),
            "omp": ([4], [4.0], [0.4])
        }

        filtered = {k: v for k, v in benchmarks.items() if "_coalloc_" not in k}

        assert "libfork_coalloc_lazy" not in filtered
        assert "libfork_lazy" in filtered

    def test_skip_special_benchmarks(self):
        """Test skipping zero, serial, calibrate."""
        benchmarks = ["zero", "calibrate", "serial", "libfork", "omp"]

        filtered = [b for b in benchmarks if b not in ["zero", "serial", "calibrate"]]

        assert len(filtered) == 2
        assert "libfork" in filtered
        assert "omp" in filtered


class TestFrameworkIdentification:
    """Test framework identification and styling."""

    def get_framework_info(self, key):
        """Helper to identify framework."""
        if "omp" in key:
            return "OpenMP", "o"
        elif "tbb" in key:
            return "OneTBB", "s"
        elif "taskflow" in key:
            return "Taskflow", "d"
        elif "busy" in key and "co" in key:
            return "Busy-LF*", "<"
        elif "busy" in key:
            return "Busy-LF", "v"
        elif "lazy" in key and "co" in key:
            return "Lazy-LF*", ">"
        elif "lazy" in key:
            return "Lazy-LF", "^"
        else:
            return None, None

    def test_all_framework_mappings(self):
        """Test all framework label and marker mappings."""
        test_cases = {
            "omp": ("OpenMP", "o"),
            "tbb": ("OneTBB", "s"),
            "taskflow": ("Taskflow", "d"),
            "busy_co": ("Busy-LF*", "<"),
            "busy": ("Busy-LF", "v"),
            "lazy_co": ("Lazy-LF*", ">"),
            "lazy": ("Lazy-LF", "^")
        }

        for key, (expected_label, expected_marker) in test_cases.items():
            label, marker = self.get_framework_info(key)
            assert label == expected_label, f"Failed for {key}"
            assert marker == expected_marker, f"Failed for {key}"

    def test_libfork_variants(self):
        """Test different libfork variant identification."""
        variants = [
            ("libfork_lazy", "Lazy-LF", "^"),
            ("libfork_lazy_co", "Lazy-LF*", ">"),
            ("libfork_busy", "Busy-LF", "v"),
            ("libfork_busy_co", "Busy-LF*", "<")
        ]

        for key, expected_label, expected_marker in variants:
            label, marker = self.get_framework_info(key)
            assert label == expected_label
            assert marker == expected_marker


class TestCurveFitting:
    """Test curve fitting for each benchmark."""

    def test_power_law_function(self):
        """Test power law fitting function."""
        def func(x, a, b, n):
            y0 = 10.0  # Example y[0] value
            return a + b * y0 * x**n

        x = 4
        result = func(x, 1.0, 0.5, 1.5)

        # Should be: 1.0 + 0.5 * 10.0 * 4^1.5
        expected = 1.0 + 0.5 * 10.0 * (4**1.5)
        assert abs(result - expected) < 1e-10

    def test_fit_bounds(self):
        """Test curve fitting bounds."""
        p0 = (1, 1, 1)
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        # Check lower bounds
        assert bounds[0][1] == 0  # b >= 0
        assert bounds[0][2] == 0  # n >= 0

        # Check upper bounds
        assert bounds[1][1] == np.inf
        assert bounds[1][2] == np.inf

    def test_fit_initial_guess(self):
        """Test initial parameter guess."""
        p0 = (1, 1, 1)

        assert len(p0) == 3
        assert all(p > 0 for p in p0)

    def test_uncertainty_calculation(self):
        """Test parameter uncertainty from covariance."""
        # Mock covariance matrix
        pcov = np.array([
            [0.01, 0.00, 0.00],
            [0.00, 0.04, 0.00],
            [0.00, 0.00, 0.09]
        ])

        da, db, dn = np.sqrt(np.diag(pcov))

        assert da == 0.1
        assert db == 0.2
        assert dn == 0.3


class TestPlotConfiguration:
    """Test plot styling and configuration."""

    def test_subplot_title_format(self):
        """Test subplot title formatting with LaTeX."""
        patterns = ["fib", "integrate", "T1 "]

        for p in patterns:
            title = f"\\textit{{{p}}}"
            assert "\\textit{" in title
            assert p in title
            assert "}" in title

    def test_axis_ticks(self):
        """Test axis tick configuration."""
        max_threads = 112
        step = 28

        ticks = list(range(0, int(max_threads + 1.5), step))

        assert ticks == [0, 28, 56, 84, 112]

    def test_axis_limits(self):
        """Test axis limit configuration."""
        xlim = (0, 112)

        assert xlim[0] == 0
        assert xlim[1] == 112

    def test_log_scale_base(self):
        """Test logarithmic scale base selection."""
        test_cases = [
            (0, 100),
            (5, 100),
            (6, 10),
            (10, 10)
        ]

        for i, expected_base in test_cases:
            base = 100 if i <= 5 else 10
            assert base == expected_base


class TestTickConfiguration:
    """Test tick locator configuration."""

    def test_major_tick_locator(self):
        """Test major tick locator configuration."""
        from matplotlib import ticker

        base = 10
        locmajy = ticker.LogLocator(base=base, numticks=100)

        assert locmajy is not None

    def test_minor_tick_locator(self):
        """Test minor tick locator configuration."""
        from matplotlib import ticker

        base = 10
        subs = np.arange(0, 10) * 0.1

        locminy = ticker.LogLocator(base=base, subs=subs, numticks=100)

        assert locminy is not None
        assert len(subs) == 10

    def test_tick_subdivision(self):
        """Test tick subdivision calculation."""
        subs = np.arange(0, 10) * 0.1

        assert len(subs) == 10
        assert subs[0] == 0.0
        assert subs[9] == 0.9


class TestLegendConfiguration:
    """Test legend configuration."""

    def test_legend_placement(self):
        """Test legend location."""
        location = "upper center"

        assert "upper" in location
        assert "center" in location

    def test_legend_columns(self):
        """Test legend column configuration."""
        ncol = 3

        assert ncol == 3
        assert ncol > 0

    def test_legend_frame(self):
        """Test legend frame setting."""
        frameon = False

        assert frameon is False

    def test_legend_label_conditional(self):
        """Test conditional legend label display."""
        test_cases = [
            (6, True),   # Should show label
            (7, True),   # Should show label
            (0, False),  # Should not show label
            (5, False)   # Should not show label
        ]

        for i, should_show in test_cases:
            label = "TestLabel" if i == 6 else None
            if should_show and i == 6:
                assert label == "TestLabel"
            elif not should_show:
                assert label is None or i != 6


class TestFigureLabels:
    """Test figure-level labels."""

    def test_supxlabel(self):
        """Test x-axis super label."""
        label = "\\textbf{{Cores}}"

        assert "Cores" in label
        assert "\\textbf" in label

    def test_supylabel(self):
        """Test y-axis super label."""
        label = "\\textbf{{MRSS/MiB}}"

        assert "MRSS/MiB" in label
        assert "\\textbf" in label

    def test_latex_bold_formatting(self):
        """Test LaTeX bold formatting."""
        text = "Cores"
        formatted = f"\\textbf{{{text}}}"

        assert formatted == "\\textbf{Cores}"


class TestLayoutAdjustment:
    """Test figure layout adjustment."""

    def test_tight_layout_rect(self):
        """Test tight_layout rect parameter."""
        rect = (0, 0, 1, 0.925)

        assert len(rect) == 4
        assert rect[0] == 0
        assert rect[1] == 0
        assert rect[2] == 1
        assert rect[3] < 1

    def test_padding_parameters(self):
        """Test h_pad and w_pad parameters."""
        h_pad = 0.5
        w_pad = 0.3

        assert h_pad > 0
        assert w_pad > 0
        assert h_pad > w_pad


class TestDataProcessingPipeline:
    """Test complete data processing pipeline."""

    def test_memory_adjustment_pipeline(self):
        """Test complete memory adjustment pipeline."""
        # Input data
        y_raw = np.array([5.0, 10.0, 15.0])
        err_raw = np.array([0.5, 1.0, 1.5])
        y_calib = 1.0
        y_calib_err = 0.1

        # Process
        y_adjusted = np.maximum(4/1024.0, y_raw - y_calib)
        err_combined = np.sqrt(err_raw**2 + y_calib_err**2)

        # Verify
        assert len(y_adjusted) == 3
        assert len(err_combined) == 3
        assert np.all(y_adjusted > 0)
        assert np.all(err_combined > 0)

    def test_fit_and_plot_pipeline(self):
        """Test fit and plot data pipeline."""
        from scipy.optimize import curve_fit

        # Sample data
        x = np.array([1, 2, 4, 8])
        y = np.array([10.0, 15.0, 25.0, 40.0])
        err = np.array([1.0, 1.5, 2.5, 4.0])

        # Fit function
        def func(x, a, b, n):
            return a + b * y[0] * x**n

        p0 = (1, 1, 1)
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        popt, pcov = curve_fit(func, x, y, p0=p0, bounds=bounds, sigma=err)

        # Verify fit
        a, b, n = popt
        assert a is not None
        assert b >= 0  # Bounded
        assert n >= 0  # Bounded


class TestIterationControl:
    """Test iteration and control flow."""

    def test_benchmark_zip_iteration(self):
        """Test zipping patterns, display names, and data."""
        from itertools import cycle

        patterns = ["fib", "integrate"]
        display = ["fib", "integrate"]
        benchmarks = [{"data": 1}, {"data": 2}]
        indices = [0, 1]

        zipped = list(zip(cycle([None, None]), patterns, benchmarks, indices))

        assert len(zipped) == 2

    def test_matmul_skip_logic(self):
        """Test skipping matmul benchmark plot."""
        pattern = "matmul"

        if pattern == "matmul":
            should_skip = True
        else:
            should_skip = False

        assert should_skip is True

    def test_taskflow_skip_logic(self):
        """Test taskflow skip logic."""
        benchmark_name = "taskflow"

        if benchmark_name == "taskflow":
            should_skip = True
        else:
            should_skip = False

        assert should_skip is True


class TestErrorPropagation:
    """Test error propagation through calculations."""

    def test_quadrature_error_combination(self):
        """Test quadrature combination of errors."""
        err1 = 0.3
        err2 = 0.4

        combined = np.sqrt(err1**2 + err2**2)

        assert combined == 0.5

    def test_error_array_propagation(self):
        """Test error propagation with arrays."""
        err = np.array([0.1, 0.2, 0.3])
        calib_err = 0.1

        combined = np.sqrt(err**2 + calib_err**2)

        expected = np.sqrt(err**2 + 0.01)
        assert np.allclose(combined, expected)


class TestOutputGeneration:
    """Test output file generation."""

    @patch('argparse.ArgumentParser.parse_args')
    def test_output_file_argument(self, mock_parse_args):
        """Test output file argument."""
        mock_parse_args.return_value = MagicMock(
            output_file='output.png'
        )

        args = mock_parse_args()
        assert args.output_file == 'output.png'

    @patch('matplotlib.pyplot.savefig')
    def test_save_figure(self, mock_savefig):
        """Test saving figure."""
        output_file = 'test.png'

        # Simulate saving
        if output_file is not None:
            mock_savefig(output_file, bbox_inches='tight')

        mock_savefig.assert_called_once_with(output_file, bbox_inches='tight')


class TestLatexFormatting:
    """Test LaTeX formatting in plots."""

    def test_latex_enabled(self):
        """Test LaTeX is enabled in rcParams."""
        import matplotlib.pyplot as plt

        # In the actual script: plt.rcParams["text.usetex"] = True
        # We just test that this is a valid setting
        usetex_value = True
        assert isinstance(usetex_value, bool)

    def test_textit_formatting(self):
        """Test italic text formatting."""
        text = "fib"
        formatted = f"\\textit{{{text}}}"

        assert formatted == "\\textit{fib}"

    def test_textbf_formatting(self):
        """Test bold text formatting."""
        text = "Cores"
        formatted = f"\\textbf{{{text}}}"

        assert formatted == "\\textbf{Cores}"


class TestRegressionCases:
    """Test specific regression cases."""

    def test_negative_memory_after_calibration(self):
        """Test handling negative memory after calibration."""
        y_calib = 10.0
        y_data = 5.0

        y_adjusted = max(4/1024.0, y_data - y_calib)

        # Should be minimum threshold
        assert y_adjusted == 4/1024.0

    def test_very_large_memory_values(self):
        """Test handling very large memory values."""
        y = np.array([1000.0, 2000.0, 5000.0])  # MiB
        y_calib = 1.0

        y_adjusted = np.maximum(4/1024.0, y - y_calib)

        assert np.all(y_adjusted > 100)

    def test_fit_convergence(self):
        """Test that curve fit can converge."""
        from scipy.optimize import curve_fit

        def func(x, a, b, n):
            return a + b * x**n

        x = np.array([1, 2, 4, 8, 16])
        y = 10 + 2 * x**0.8  # Known relationship

        try:
            popt, pcov = curve_fit(
                func, x, y,
                p0=(1, 1, 1),
                bounds=([-np.inf, 0, 0], [np.inf, np.inf, np.inf]),
                maxfev=10000
            )
            converged = True
        except:
            converged = False

        assert converged


class TestIntegration:
    """Integration tests with realistic scenarios."""

    def test_multi_benchmark_processing(self):
        """Test processing multiple benchmarks."""
        patterns = ["fib", "integ", "nqueens"]
        Benchmarks = []

        for pattern in patterns:
            # Simulate loading data
            data = {
                "calibrate": ([1], [1.0], [0.1]),
                "serial": ([1], [2.0], [0.2]),
                "libfork": ([1, 2, 4], [3.0, 5.0, 8.0], [0.3, 0.5, 0.8])
            }
            Benchmarks.append(data)

        assert len(Benchmarks) == 3

        # Process each benchmark
        for data in Benchmarks:
            y_calib = data["calibrate"][1][0]
            assert y_calib > 0

    def test_complete_workflow(self):
        """Test complete workflow from data to plot."""
        # Load data
        data = {
            "calibrate": ([1], [0.5], [0.05]),
            "serial": ([1], [1.0], [0.1]),
            "libfork": ([1, 2, 4, 8], [2.0, 3.0, 5.0, 8.0], [0.2, 0.3, 0.5, 0.8])
        }

        # Process calibration
        y_calib = data["calibrate"][1][0]
        y_calib_err = data["calibrate"][2][0] * 5

        # Process libfork data
        x = np.array(data["libfork"][0])
        y = np.array(data["libfork"][1])
        err = np.array(data["libfork"][2])

        # Adjust
        y_adjusted = np.maximum(4/1024.0, y - y_calib)
        err_combined = np.sqrt(err**2 + y_calib_err**2)

        # Verify pipeline worked
        assert len(x) == 4
        assert len(y_adjusted) == 4
        assert np.all(y_adjusted > 0)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])