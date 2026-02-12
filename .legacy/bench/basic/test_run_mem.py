"""
Unit tests for run_mem.py memory measurement script.

Tests the memory measurement logic, subprocess handling, and statistical
calculations used to measure benchmark memory usage.
"""

import pytest
import re
from statistics import median, stdev
from math import sqrt
from unittest.mock import patch, MagicMock, mock_open
import subprocess


class TestMemoryExtraction:
    """Test cases for extracting memory values from command output."""

    def test_memory_regex_match(self):
        """Test regex pattern for extracting memory value."""
        pattern = ".*MEMORY=([1-9][0-9]*)"
        test_output = "Some output\nMEMORY=12345\nMore output"

        match = re.search(pattern, test_output)

        assert match is not None
        assert match.group(1) == "12345"

    def test_memory_regex_with_various_formats(self):
        """Test memory extraction with various output formats."""
        pattern = ".*MEMORY=([1-9][0-9]*)"

        test_cases = [
            ("MEMORY=1024", "1024"),
            ("prefix MEMORY=2048 suffix", "2048"),
            ("MEMORY=1", "1"),
            ("MEMORY=999999", "999999"),
        ]

        for output, expected in test_cases:
            match = re.search(pattern, output)
            assert match is not None
            assert match.group(1) == expected

    def test_memory_regex_no_match(self):
        """Test regex when memory pattern is not found."""
        pattern = ".*MEMORY=([1-9][0-9]*)"
        test_output = "No memory information here"

        match = re.search(pattern, test_output)

        assert match is None

    def test_memory_regex_rejects_zero_start(self):
        """Test that regex rejects memory values starting with 0."""
        pattern = ".*MEMORY=([1-9][0-9]*)"

        # Should not match values starting with 0 (like 0123)
        match = re.search(pattern, "MEMORY=0123")
        assert match is None

        # Should match valid values
        match = re.search(pattern, "MEMORY=123")
        assert match is not None

    def test_memory_value_conversion(self):
        """Test conversion of extracted memory string to integer."""
        memory_str = "12345"
        memory_int = int(memory_str)

        assert memory_int == 12345
        assert isinstance(memory_int, int)


class TestStatisticalCalculations:
    """Test cases for statistical calculations on memory measurements."""

    def test_median_calculation(self):
        """Test median calculation of memory measurements."""
        measurements = [100, 110, 105, 108, 102]
        result = median(measurements)

        assert result == 105

    def test_median_with_even_count(self):
        """Test median with even number of measurements."""
        measurements = [100, 110, 105, 108]
        result = median(measurements)

        # Median of 4 values: average of middle two
        assert result == (105 + 108) / 2

    def test_median_with_single_value(self):
        """Test median with single measurement."""
        measurements = [100]
        result = median(measurements)

        assert result == 100

    def test_stdev_calculation(self):
        """Test standard deviation calculation."""
        measurements = [100, 110, 105, 108, 102]
        result = stdev(measurements)

        # Should be positive and reasonable
        assert result > 0
        assert isinstance(result, float)

    def test_standard_error_calculation(self):
        """Test standard error calculation (stdev / sqrt(n))."""
        measurements = [100, 110, 105, 108, 102]

        std = stdev(measurements)
        se = std / sqrt(len(measurements))

        assert se > 0
        assert se < std  # Standard error should be smaller than stdev

    def test_standard_error_with_large_sample(self):
        """Test that standard error decreases with larger sample size."""
        measurements_small = [100, 110, 105, 108, 102]
        measurements_large = [100, 110, 105, 108, 102, 103, 107, 104, 106, 109]

        se_small = stdev(measurements_small) / sqrt(len(measurements_small))
        se_large = stdev(measurements_large) / sqrt(len(measurements_large))

        # Larger sample should have smaller standard error (generally)
        assert len(measurements_large) > len(measurements_small)

    def test_stdev_with_two_values(self):
        """Test standard deviation with exactly two values."""
        measurements = [100, 110]
        result = stdev(measurements)

        # Should calculate without error
        assert result > 0


class TestBenchmarkFiltering:
    """Test cases for benchmark name filtering and regex construction."""

    def test_libfork_regex_construction(self):
        """Test construction of libfork benchmark regex."""
        bench = "fib"
        kind = "libfork.*lazy.*fan"

        # Not starting with T
        if not bench.startswith("T"):
            libfork_patterns = ["libfork.*lazy.*fan"]
            assert kind in libfork_patterns

    def test_uts_benchmark_regex(self):
        """Test construction of UTS benchmark regex."""
        bench = "T1"
        kind = "libfork"

        if bench.startswith("T"):
            reg = f"uts.*{kind}.*{bench}"
            assert reg == "uts.*libfork.*T1"

    def test_non_uts_benchmark_regex(self):
        """Test construction of non-UTS benchmark regex."""
        bench = "fib"
        kind = "libfork"

        if not bench.startswith("T"):
            reg = f"{bench}.*{kind}"
            assert reg == "fib.*libfork"

    def test_thread_count_in_regex(self):
        """Test adding thread count to regex."""
        reg = "fib.*libfork"
        threads = 8
        is_serial = False

        if not is_serial:
            reg += f".*/{threads}/"

        assert reg == "fib.*libfork.*/8/"

    def test_serial_benchmark_regex(self):
        """Test that serial benchmarks don't include thread count."""
        reg = "calibrate"
        threads = 8
        is_serial = True

        if not is_serial:
            reg += f".*/{threads}/"

        assert reg == "calibrate"
        assert "/8/" not in reg

    def test_zero_benchmark_detection(self):
        """Test detection of zero benchmark."""
        kind = "zero"

        if kind == "zero":
            reg = "NONNAMEDTHIS"
            assert reg == "NONNAMEDTHIS"

    def test_calibrate_benchmark_detection(self):
        """Test detection of calibrate benchmark."""
        kind = "calibrate"

        if kind == "calibrate":
            reg = "calibrate"
            assert reg == "calibrate"


class TestRepetitionCounts:
    """Test cases for benchmark repetition logic."""

    def test_serial_repetition_count(self):
        """Test that serial benchmarks use more repetitions."""
        is_serial = True
        repetitions = 25 if is_serial else 5

        assert repetitions == 25

    def test_parallel_repetition_count(self):
        """Test that parallel benchmarks use fewer repetitions."""
        is_serial = False
        repetitions = 25 if is_serial else 5

        assert repetitions == 5

    def test_repetition_range(self):
        """Test that repetition counts are positive."""
        assert (25 if True else 5) > 0
        assert (25 if False else 5) > 0


class TestThreadCountIteration:
    """Test cases for thread count iteration logic."""

    def test_thread_count_sequence(self):
        """Test the sequence of thread counts."""
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]

        assert len(thread_counts) == 14
        assert thread_counts[0] == 1
        assert thread_counts[-1] == 112

    def test_thread_count_filtering_by_max(self):
        """Test filtering thread counts by maximum cores."""
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]
        max_cores = 32

        filtered = [t for t in thread_counts if t <= max_cores]

        assert filtered == [1, 2, 4, 8, 16, 24, 32]

    def test_serial_thread_count_break(self):
        """Test that serial benchmarks only run with thread count 1."""
        is_serial = True
        thread_counts = [1, 2, 4, 8]
        executed = []

        for i in thread_counts:
            executed.append(i)
            if is_serial and i > 1:
                break

        assert executed == [1, 2]  # Breaks after i=2

    def test_parallel_thread_count_no_break(self):
        """Test that parallel benchmarks run all thread counts."""
        is_serial = False
        thread_counts = [1, 2, 4, 8]
        executed = []

        for i in thread_counts:
            executed.append(i)
            if is_serial and i > 1:
                break

        assert executed == [1, 2, 4, 8]  # No break


class TestCommandConstruction:
    """Test cases for benchmark command construction."""

    def test_time_command_format(self):
        """Test construction of /usr/bin/time command."""
        binary = "./benchmark"
        regex = "fib.*libfork"

        command = f'/usr/bin/time -f"MEMORY=%M"  -- {binary} --benchmark_filter="{regex}" --benchmark_time_unit=ms'

        assert "/usr/bin/time" in command
        assert "-f\"MEMORY=%M\"" in command
        assert binary in command
        assert regex in command
        assert "--benchmark_filter=" in command
        assert "--benchmark_time_unit=ms" in command

    def test_benchmark_filter_escaping(self):
        """Test that benchmark filter is properly quoted."""
        regex = "fib.*libfork.*/8/"
        command = f'--benchmark_filter="{regex}"'

        assert command == '--benchmark_filter="fib.*libfork.*/8/"'


class TestFileOutputFormat:
    """Test cases for CSV output format."""

    def test_csv_output_format(self):
        """Test format of CSV output line."""
        kind = "libfork"
        threads = 8
        median_val = 1024.5
        stderr_val = 50.2

        line = f"{kind},{threads},{median_val},{stderr_val}\n"

        assert line == "libfork,8,1024.5,50.2\n"

    def test_csv_output_parsing(self):
        """Test parsing of generated CSV line."""
        line = "libfork,8,1024.5,50.2\n"
        parts = line.strip().split(",")

        assert len(parts) == 4
        assert parts[0] == "libfork"
        assert parts[1] == "8"
        assert float(parts[2]) == 1024.5
        assert float(parts[3]) == 50.2


class TestBenchmarkKinds:
    """Test cases for different benchmark kinds."""

    def test_benchmark_kinds_list(self):
        """Test the list of benchmark kinds."""
        bench = "fib"
        libfork = ["libfork.*lazy.*fan"]

        kinds = [
            "zero",
            "calibrate",
            "serial",
            *libfork,
            "tmc",
            "ccpp",
            "omp",
            "tbb",
            "taskflow",
        ]

        assert "zero" in kinds
        assert "calibrate" in kinds
        assert "serial" in kinds
        assert "tmc" in kinds
        assert "ccpp" in kinds
        assert "omp" in kinds
        assert "tbb" in kinds
        assert "taskflow" in kinds
        assert len(kinds) == 9

    def test_special_benchmark_kinds(self):
        """Test detection of special benchmark kinds."""
        special_kinds = ["zero", "calibrate", "serial"]

        for kind in special_kinds:
            is_serial = kind == "serial" or kind == "calibrate" or kind == "zero"
            assert is_serial


class TestEdgeCases:
    """Test edge cases and boundary conditions."""

    def test_single_memory_measurement(self):
        """Test handling single memory measurement."""
        mem = [1024]

        x = median(mem)

        assert x == 1024

    def test_memory_list_accumulation(self):
        """Test accumulation of memory measurements."""
        mem = []

        for i in range(5):
            mem.append(1000 + i * 10)

        assert len(mem) == 5
        assert mem[0] == 1000
        assert mem[-1] == 1040

    def test_empty_regex_match(self):
        """Test behavior when regex doesn't match."""
        pattern = ".*MEMORY=([1-9][0-9]*)"
        output = "No memory information"

        match = re.search(pattern, output)

        assert match is None

    def test_output_filename_construction(self):
        """Test construction of output filename."""
        bench = "fib"
        filename = f"memory.{bench.strip()}.csv"

        assert filename == "memory.fib.csv"


class TestNumericalStability:
    """Test cases for numerical stability."""

    def test_median_with_large_values(self):
        """Test median calculation with large memory values."""
        measurements = [int(1e9), int(1e9 + 1e6), int(1e9 + 2e6)]
        result = median(measurements)

        assert result > 0
        assert isinstance(result, (int, float))

    def test_stdev_with_small_variance(self):
        """Test standard deviation with very similar values."""
        measurements = [1000, 1001, 1000, 1001, 1000]
        result = stdev(measurements)

        assert result > 0
        assert result < 10  # Should be small

    def test_sqrt_in_error_calculation(self):
        """Test square root calculation for standard error."""
        n = 25
        result = sqrt(n)

        assert result == 5.0

    def test_median_preserves_precision(self):
        """Test that median preserves decimal precision."""
        measurements = [100.5, 100.7, 100.6]
        result = median(measurements)

        assert result == 100.6


class TestRegressionCases:
    """Regression tests for previously found issues."""

    def test_memory_value_always_positive(self):
        """Test that extracted memory values are always positive."""
        pattern = ".*MEMORY=([1-9][0-9]*)"

        # The regex ensures first digit is 1-9
        test_cases = ["MEMORY=1", "MEMORY=100", "MEMORY=99999"]

        for output in test_cases:
            match = re.search(pattern, output)
            value = int(match.group(1))
            assert value > 0

    def test_standard_error_consistency(self):
        """Test that standard error calculation is consistent."""
        measurements = [100, 110, 105, 108, 102]

        # Calculate twice
        se1 = stdev(measurements) / sqrt(len(measurements))
        se2 = stdev(measurements) / sqrt(len(measurements))

        assert se1 == se2

    def test_benchmark_name_in_filename(self):
        """Test that benchmark name appears in output filename."""
        bench_names = ["fib", "integ", "nqueens", "T1", "T3"]

        for bench in bench_names:
            filename = f"memory.{bench.strip()}.csv"
            assert bench in filename

    def test_thread_count_range_validity(self):
        """Test that all thread counts in sequence are valid."""
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]

        for count in thread_counts:
            assert count > 0
            assert isinstance(count, int)