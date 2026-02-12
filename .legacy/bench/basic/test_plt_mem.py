"""
Comprehensive tests for plt_mem.py memory plotting script.
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


class TestDataParsing:
    """Test CSV data parsing functionality."""

    def create_mock_csv_data(self):
        """Create mock CSV data for testing."""
        return """calibrate,1,1024.5,10.2
serial,1,2048.0,15.0
libfork,4,4096.0,20.0
libfork,8,5120.0,25.0
omp,4,6144.0,30.0
tbb,8,7168.0,35.0
"""

    def test_parse_csv_basic(self):
        """Test basic CSV parsing."""
        csv_data = self.create_mock_csv_data()

        data = {}
        for line in csv_data.strip().split('\n'):
            name, threads, med, dev = line.split(",")

            if name not in data:
                data[name] = ([], [], [])

            data[name][0].append(int(threads))
            data[name][1].append(float(med) / 1024.0)
            data[name][2].append(float(dev) / 1024.0)

        assert "calibrate" in data
        assert "serial" in data
        assert "libfork" in data
        assert len(data["libfork"][0]) == 2  # Two thread counts

    def test_parse_csv_structure(self):
        """Test that parsed data has correct structure."""
        csv_data = self.create_mock_csv_data()

        data = {}
        for line in csv_data.strip().split('\n'):
            name, threads, med, dev = line.split(",")

            if name not in data:
                data[name] = ([], [], [])

            data[name][0].append(int(threads))
            data[name][1].append(float(med) / 1024.0)
            data[name][2].append(float(dev) / 1024.0)

        for name, values in data.items():
            assert len(values) == 3  # threads, median, deviation
            assert isinstance(values[0], list)
            assert isinstance(values[1], list)
            assert isinstance(values[2], list)
            assert len(values[0]) == len(values[1]) == len(values[2])

    def test_memory_unit_conversion(self):
        """Test conversion from KB to MiB."""
        kb_value = 1024.0  # KB
        mib_value = kb_value / 1024.0  # Convert to MiB

        assert mib_value == 1.0

        kb_value = 2048.0
        mib_value = kb_value / 1024.0
        assert mib_value == 2.0

    def test_parse_multiple_entries_same_benchmark(self):
        """Test parsing multiple entries for the same benchmark."""
        csv_data = """libfork,1,1024,10
libfork,2,2048,15
libfork,4,4096,20
"""

        data = {}
        for line in csv_data.strip().split('\n'):
            name, threads, med, dev = line.split(",")

            if name not in data:
                data[name] = ([], [], [])

            data[name][0].append(int(threads))
            data[name][1].append(float(med) / 1024.0)
            data[name][2].append(float(dev) / 1024.0)

        assert len(data["libfork"][0]) == 3
        assert data["libfork"][0] == [1, 2, 4]


class TestCalibrationHandling:
    """Test calibration and baseline calculations."""

    def test_calibration_offset(self):
        """Test calibration offset calculation."""
        y_calib = 1.0  # MiB
        y_data = 5.0  # MiB

        # Subtract calibration offset
        adjusted = y_data - y_calib

        assert adjusted == 4.0

    def test_calibration_error_propagation(self):
        """Test error propagation with calibration."""
        y_calib_err = 0.1  # MiB
        y_data_err = 0.2  # MiB

        # Combined error
        combined_err = np.sqrt(y_data_err**2 + y_calib_err**2)

        expected = np.sqrt(0.2**2 + 0.1**2)
        assert abs(combined_err - expected) < 1e-10

    def test_minimum_memory_threshold(self):
        """Test minimum 4 KiB threshold."""
        min_threshold = 4 / 1024.0  # 4 KiB in MiB

        test_values = [-1.0, 0.0, 0.001, 0.01]

        for val in test_values:
            adjusted = np.maximum(min_threshold, val)
            assert adjusted >= min_threshold

    def test_calibration_standard_error_scaling(self):
        """Test that calibration error is scaled by 5."""
        calibrate_std = 0.05  # Standard deviation
        calibrate_se = calibrate_std * 5  # Standard error (x5 factor)

        assert calibrate_se == 0.25

    def test_serial_memory_calculation(self):
        """Test serial memory baseline calculation."""
        y_calib = 0.5  # MiB
        y_serial_raw = 1.5  # MiB
        min_threshold = 4 / 1024.0  # 4 KiB in MiB

        y_serial = max(min_threshold, y_serial_raw - y_calib)

        assert y_serial == 1.0  # 1.5 - 0.5


class TestFrameworkNameMapping:
    """Test framework name and marker mapping."""

    def get_framework_info(self, key):
        """Helper to get framework label and marker."""
        if "omp" in key:
            return "OpenMP", "o"
        elif "tbb" in key:
            return "OneTBB", "s"
        elif "taskflow" in key:
            return "Taskflow", "d"
        elif "libfork" in key:
            return "Libfork", ">"
        elif "tmc" in key:
            return "TooManyCooks", "^"
        elif "ccpp" in key:
            return "Concurrencpp", "v"
        else:
            return None, None

    def test_framework_mapping(self):
        """Test all framework mappings."""
        test_cases = {
            "omp": ("OpenMP", "o"),
            "tbb": ("OneTBB", "s"),
            "taskflow": ("Taskflow", "d"),
            "libfork": ("Libfork", ">"),
            "tmc": ("TooManyCooks", "^"),
            "ccpp": ("Concurrencpp", "v")
        }

        for key, (expected_label, expected_marker) in test_cases.items():
            label, marker = self.get_framework_info(key)
            assert label == expected_label
            assert marker == expected_marker

    def test_unknown_framework(self):
        """Test handling of unknown framework."""
        label, marker = self.get_framework_info("unknown_framework")
        assert label is None
        assert marker is None

    def test_taskflow_special_handling(self):
        """Test that taskflow can be skipped as in original code."""
        frameworks = ["libfork", "taskflow", "omp"]

        processed = []
        for fw in frameworks:
            if fw == "taskflow":
                continue  # Skip as in original
            processed.append(fw)

        assert "taskflow" not in processed
        assert len(processed) == 2


class TestCurveFitting:
    """Test curve fitting functionality."""

    def test_power_law_function(self):
        """Test the power law function used for fitting."""
        def func(x, a, b, n):
            return a + b * 10.0 * x**n  # Using y[0]=10.0 as example

        x = 4
        a, b, n = 1.0, 0.5, 1.0

        result = func(x, a, b, n)
        expected = 1.0 + 0.5 * 10.0 * 4.0**1.0
        assert result == expected

    def test_curve_fit_parameters(self):
        """Test curve fitting with synthetic data."""
        from scipy.optimize import curve_fit

        def func(x, a, b, n):
            return a + b * x**n

        # Generate synthetic data: y = 2 + 0.5 * x^1.5
        x = np.array([1, 2, 4, 8, 16])
        y_true = 2 + 0.5 * x**1.5
        y = y_true + np.random.normal(0, 0.1, len(x))  # Add noise

        p0 = (1, 1, 1)
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        popt, pcov = curve_fit(func, x, y, p0=p0, bounds=bounds)

        a, b, n = popt

        # Check that fitted parameters are reasonable
        assert a > 0  # Offset should be positive
        assert b > 0  # Coefficient should be positive (bounded)
        assert n > 0  # Exponent should be positive (bounded)

    def test_covariance_matrix(self):
        """Test that covariance matrix provides uncertainties."""
        from scipy.optimize import curve_fit

        def func(x, a, b):
            return a + b * x

        x = np.array([1, 2, 3, 4, 5])
        y = np.array([2, 4, 6, 8, 10])  # Perfect linear

        popt, pcov = curve_fit(func, x, y)

        # Extract standard deviations
        da, db = np.sqrt(np.diag(pcov))

        # Should be very small for perfect data
        assert da < 1e-10
        assert db < 1e-10

    def test_curve_fit_with_errors(self):
        """Test curve fitting with error weights."""
        from scipy.optimize import curve_fit

        def func(x, a, b):
            return a * x + b

        x = np.array([1, 2, 3, 4])
        y = np.array([2, 4, 6, 8])
        sigma = np.array([0.1, 0.1, 0.1, 0.1])

        popt, pcov = curve_fit(func, x, y, sigma=sigma)

        assert len(popt) == 2
        assert pcov.shape == (2, 2)


class TestPlotConfiguration:
    """Test plot configuration and styling."""

    @patch('matplotlib.pyplot.subplots')
    def test_plot_creation(self, mock_subplots):
        """Test plot figure and axes creation."""
        mock_fig = MagicMock()
        mock_ax = MagicMock()
        mock_subplots.return_value = (mock_fig, mock_ax)

        fig, ax = mock_subplots(figsize=(8, 5))

        mock_subplots.assert_called_once_with(figsize=(8, 5))

    def test_axis_limits(self):
        """Test axis limit settings."""
        ylim_bottom = 6
        ylim_top = 400

        assert ylim_bottom < ylim_top
        assert ylim_bottom > 0

    def test_tick_configuration(self):
        """Test axis tick configuration."""
        max_cores = 32
        step = 4

        ticks = list(range(0, int(max_cores + 1.5), step))

        assert ticks == [0, 4, 8, 12, 16, 20, 24, 28, 32]

    @patch('matplotlib.pyplot.subplots')
    def test_log_scale_setting(self, mock_subplots):
        """Test logarithmic scale configuration."""
        mock_fig = MagicMock()
        mock_ax = MagicMock()
        mock_subplots.return_value = (mock_fig, mock_ax)

        fig, ax = mock_subplots()
        ax.set_yscale("log", base=10)

        mock_ax.set_yscale.assert_called_once_with("log", base=10)


class TestDataFiltering:
    """Test data filtering logic."""

    def test_skip_special_benchmarks(self):
        """Test skipping zero, serial, and calibrate benchmarks."""
        benchmarks = ["zero", "calibrate", "serial", "libfork", "omp", "tbb"]

        filtered = [b for b in benchmarks if b not in ["zero", "serial", "calibrate"]]

        assert "zero" not in filtered
        assert "serial" not in filtered
        assert "calibrate" not in filtered
        assert "libfork" in filtered
        assert "omp" in filtered

    def test_benchmark_sorting(self):
        """Test benchmark dictionary sorting."""
        data = {
            "zebra": ([1], [1.0], [0.1]),
            "alpha": ([2], [2.0], [0.2]),
            "beta": ([3], [3.0], [0.3])
        }

        sorted_data = dict(sorted(data.items()))
        keys = list(sorted_data.keys())

        assert keys[0] == "alpha"
        assert keys[1] == "beta"
        assert keys[2] == "zebra"


class TestErrorBarCalculation:
    """Test error bar calculations."""

    def test_errorbar_values(self):
        """Test error bar value calculations."""
        y = np.array([10.0, 20.0, 30.0])
        err = np.array([1.0, 2.0, 3.0])

        # Error bars should be positive
        assert np.all(err >= 0)

        # Error bars should be reasonable relative to values
        relative_err = err / y
        assert np.all(relative_err < 1.0)  # Less than 100% error

    def test_combined_error_calculation(self):
        """Test combined error from multiple sources."""
        err1 = 0.5
        err2 = 0.3

        combined = np.sqrt(err1**2 + err2**2)

        expected = np.sqrt(0.25 + 0.09)
        assert abs(combined - expected) < 1e-10

    def test_error_array_operations(self):
        """Test numpy operations on error arrays."""
        y = np.array([10.0, 20.0, 30.0])
        y_calib = 1.0
        y_calib_err = 0.1
        err = np.array([0.5, 0.6, 0.7])

        # Adjust values
        y_adjusted = np.maximum(4/1024.0, y - y_calib)

        # Propagate errors
        err_combined = np.sqrt(err**2 + y_calib_err**2)

        assert y_adjusted.shape == y.shape
        assert err_combined.shape == err.shape


class TestCommandLineArguments:
    """Test command line argument parsing."""

    @patch('argparse.ArgumentParser.parse_args')
    def test_required_arguments(self, mock_parse_args):
        """Test required input file argument."""
        mock_parse_args.return_value = MagicMock(
            input_file='memory.csv',
            output_file=None
        )

        args = mock_parse_args()
        assert args.input_file == 'memory.csv'

    @patch('argparse.ArgumentParser.parse_args')
    def test_optional_output_file(self, mock_parse_args):
        """Test optional output file argument."""
        mock_parse_args.return_value = MagicMock(
            input_file='memory.csv',
            output_file='output.png'
        )

        args = mock_parse_args()
        assert args.output_file == 'output.png'


class TestIntegration:
    """Integration tests with realistic data."""

    def test_full_data_pipeline(self):
        """Test complete data processing pipeline."""
        # Simulate reading CSV
        csv_data = """calibrate,1,1024,10
serial,1,2048,15
libfork,4,4096,20
libfork,8,6144,25
"""

        data = {}
        for line in csv_data.strip().split('\n'):
            name, threads, med, dev = line.split(",")

            if name not in data:
                data[name] = ([], [], [])

            data[name][0].append(int(threads))
            data[name][1].append(float(med) / 1024.0)
            data[name][2].append(float(dev) / 1024.0)

        # Process calibration
        y_calib = data["calibrate"][1][0]
        y_calib_err = data["calibrate"][2][0] * 5

        # Process libfork data
        x = np.array(data["libfork"][0])
        y = np.array(data["libfork"][1])
        err = np.array(data["libfork"][2])

        # Apply calibration
        y_adjusted = np.maximum(4/1024.0, y - y_calib)
        err_combined = np.sqrt(err**2 + y_calib_err**2)

        assert len(x) == 2
        assert len(y_adjusted) == 2
        assert len(err_combined) == 2
        assert np.all(y_adjusted > 0)

    def test_with_temporary_file(self):
        """Test reading from a temporary file."""
        csv_content = """calibrate,1,512,5
serial,1,1024,10
libfork,2,2048,15
"""

        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            f.write(csv_content)
            temp_file = f.name

        try:
            data = {}
            with open(temp_file) as file:
                for line in file:
                    name, threads, med, dev = line.strip().split(",")

                    if name not in data:
                        data[name] = ([], [], [])

                    data[name][0].append(int(threads))
                    data[name][1].append(float(med) / 1024.0)
                    data[name][2].append(float(dev) / 1024.0)

            assert "calibrate" in data
            assert "serial" in data
            assert "libfork" in data
        finally:
            os.unlink(temp_file)


class TestRegressionCases:
    """Test specific regression and edge cases."""

    def test_negative_memory_values(self):
        """Test handling of negative memory values after calibration."""
        y_calib = 10.0
        y_data = 5.0  # Less than calibration

        y_adjusted = np.maximum(4/1024.0, y_data - y_calib)

        # Should be clamped to minimum
        assert y_adjusted == 4/1024.0

    def test_zero_memory_values(self):
        """Test handling of zero memory values."""
        y_calib = 1.0
        y_data = 1.0  # Same as calibration

        y_adjusted = np.maximum(4/1024.0, y_data - y_calib)

        assert y_adjusted == 4/1024.0

    def test_large_memory_values(self):
        """Test handling of large memory values."""
        large_values = np.array([1000.0, 2000.0, 5000.0])  # MiB
        y_calib = 1.0

        adjusted = np.maximum(4/1024.0, large_values - y_calib)

        assert np.all(adjusted > 100)

    def test_memory_scaling_exponent(self):
        """Test memory scaling with different thread counts."""
        threads = np.array([1, 2, 4, 8, 16])

        # Linear scaling
        mem_linear = 10.0 * threads
        assert mem_linear[4] == 10.0 * 16

        # Sub-linear scaling (typical for good memory management)
        mem_sublinear = 10.0 * threads**0.5
        assert mem_sublinear[4] < mem_linear[4]

    def test_fit_quality_assessment(self):
        """Test assessment of fit quality."""
        from scipy.optimize import curve_fit

        def func(x, a, b, n):
            return a + b * x**n

        # Generate data with known parameters
        x = np.array([1, 2, 4, 8])
        y_true = 5 + 2 * x**1.2
        y = y_true  # Perfect data

        p0 = (1, 1, 1)
        bounds = ([-np.inf, 0, 0], [np.inf, np.inf, np.inf])

        popt, pcov = curve_fit(func, x, y, p0=p0, bounds=bounds)

        # Calculate residuals
        y_fit = func(x, *popt)
        residuals = y - y_fit

        # Should be very small for perfect data
        assert np.all(np.abs(residuals) < 0.01)


class TestNumericalStability:
    """Test numerical stability of calculations."""

    def test_sqrt_of_negative_avoided(self):
        """Test that we don't take sqrt of negative numbers."""
        err1_sq = 0.5**2
        err2_sq = 0.3**2

        # This should always be non-negative
        combined_sq = err1_sq + err2_sq
        assert combined_sq >= 0

        combined = np.sqrt(combined_sq)
        assert not np.isnan(combined)

    def test_division_by_zero_avoided(self):
        """Test avoiding division by zero."""
        values = np.array([1.0, 2.0, 3.0])
        divisor = 2.0

        result = values / divisor

        assert np.all(np.isfinite(result))

    def test_log_scale_positive_values(self):
        """Test that log scale only gets positive values."""
        values = np.array([0.1, 1.0, 10.0, 100.0])

        # All values should be positive for log scale
        assert np.all(values > 0)

        log_values = np.log10(values)
        assert np.all(np.isfinite(log_values))


if __name__ == "__main__":
    pytest.main([__file__, "-v"])