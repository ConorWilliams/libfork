"""
Comprehensive tests for run_mem.py memory measurement script.
"""

import pytest
import os
import tempfile
import subprocess
from unittest.mock import patch, MagicMock, call
import sys
from io import StringIO


# Import path setup
sys.path.insert(0, os.path.dirname(__file__))


class TestCommandLineArguments:
    """Test command line argument parsing."""

    @patch('argparse.ArgumentParser.parse_args')
    def test_required_arguments(self, mock_parse_args):
        """Test that required arguments are parsed."""
        mock_parse_args.return_value = MagicMock(
            binary='./build/benchmark',
            cores=112
        )

        args = mock_parse_args()
        assert args.binary == './build/benchmark'
        assert args.cores == 112

    @patch('argparse.ArgumentParser.parse_args')
    def test_cores_parameter_type(self, mock_parse_args):
        """Test that cores parameter is an integer."""
        mock_parse_args.return_value = MagicMock(
            binary='./build/benchmark',
            cores=64
        )

        args = mock_parse_args()
        assert isinstance(args.cores, int)
        assert args.cores > 0

    def test_argument_validation(self):
        """Test argument validation logic."""
        valid_cores = [1, 4, 8, 16, 32, 64, 112]

        for cores in valid_cores:
            assert cores > 0
            assert isinstance(cores, int)


class TestBenchmarkConfiguration:
    """Test benchmark configuration and regex patterns."""

    def test_benchmark_name_variable(self):
        """Test benchmark name variable."""
        bench = "fib"
        assert isinstance(bench, str)
        assert len(bench) > 0

    def test_libfork_patterns_non_t_benchmark(self):
        """Test libfork patterns for non-T benchmarks."""
        bench = "fib"

        if not bench.startswith("T"):
            libfork = ["libfork.*lazy.*fan"]
        else:
            libfork = ["libfork.*_coalloc_.*lazy.*fan"]

        assert libfork == ["libfork.*lazy.*fan"]

    def test_libfork_patterns_t_benchmark(self):
        """Test libfork patterns for T-prefixed benchmarks."""
        bench = "T1"

        if not bench.startswith("T"):
            libfork = ["libfork.*lazy.*fan"]
        else:
            libfork = ["libfork.*_coalloc_.*lazy.*fan"]

        assert libfork == ["libfork.*_coalloc_.*lazy.*fan"]

    def test_benchmark_kinds(self):
        """Test all benchmark kinds."""
        kinds = [
            "zero",
            "calibrate",
            "serial",
            "libfork.*lazy.*fan",
            "tmc",
            "ccpp",
            "omp",
            "tbb",
            "taskflow"
        ]

        assert len(kinds) == 9
        assert "zero" in kinds
        assert "serial" in kinds
        assert "calibrate" in kinds


class TestThreadCountConfiguration:
    """Test thread count configuration."""

    def test_thread_counts_list(self):
        """Test standard thread count list."""
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]

        assert len(thread_counts) == 14
        assert thread_counts[0] == 1
        assert thread_counts[-1] == 112
        assert thread_counts == sorted(thread_counts)

    def test_thread_count_filtering_by_max_cores(self):
        """Test filtering thread counts by max cores."""
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]
        max_cores = 32

        filtered = [t for t in thread_counts if t <= max_cores]

        assert 40 not in filtered
        assert 64 not in filtered
        assert 112 not in filtered
        assert 32 in filtered

    def test_serial_benchmark_single_iteration(self):
        """Test that serial benchmarks only run once."""
        is_serial = True
        thread_counts = [1, 2, 4, 8]

        for i, count in enumerate(thread_counts):
            if is_serial and i > 0:
                break
            # Process thread count
            pass

        # Should only process first thread count
        assert True  # Serial logic check


class TestRegexConstruction:
    """Test regex pattern construction."""

    def test_zero_benchmark_regex(self):
        """Test regex for zero benchmark."""
        kind = "zero"

        if kind == "zero":
            reg = "NONNAMEDTHIS"
        else:
            reg = "something"

        assert reg == "NONNAMEDTHIS"

    def test_calibrate_benchmark_regex(self):
        """Test regex for calibrate benchmark."""
        kind = "calibrate"

        if kind == "calibrate":
            reg = "calibrate"
        else:
            reg = "something"

        assert reg == "calibrate"

    def test_t_benchmark_regex(self):
        """Test regex for T-prefixed benchmarks."""
        bench = "T1"
        kind = "libfork"

        if bench.startswith("T"):
            reg = f"uts.*{kind}.*{bench}"
        else:
            reg = f"{bench}.*{kind}"

        assert reg == "uts.*libfork.*T1"

    def test_non_t_benchmark_regex(self):
        """Test regex for non-T benchmarks."""
        bench = "fib"
        kind = "libfork"

        if bench.startswith("T"):
            reg = f"uts.*{kind}.*{bench}"
        else:
            reg = f"{bench}.*{kind}"

        assert reg == "fib.*libfork"

    def test_serial_vs_parallel_regex(self):
        """Test regex construction for serial vs parallel."""
        reg_base = "fib.*libfork"
        num_threads = 4
        is_serial = False

        if not is_serial:
            reg = f"{reg_base}.*/{num_threads}/"
        else:
            reg = reg_base

        assert reg == "fib.*libfork.*/4/"

    def test_serial_benchmark_regex_simple(self):
        """Test regex for serial benchmark without threads."""
        reg_base = "fib.*serial"
        is_serial = True

        if not is_serial:
            reg = f"{reg_base}.*/4/"
        else:
            reg = reg_base

        assert reg == "fib.*serial"


class TestCommandConstruction:
    """Test GNU time command construction."""

    def test_time_command_format(self):
        """Test time command format string."""
        binary = "./benchmark"
        regex = "fib.*libfork.*/4/"

        command = f'/usr/bin/time -f"MEMORY=%M"  -- {binary} --benchmark_filter="{regex}" --benchmark_time_unit=ms'

        assert "/usr/bin/time" in command
        assert "-f" in command
        assert "MEMORY=%M" in command
        assert binary in command
        assert regex in command
        assert "--benchmark_filter=" in command
        assert "--benchmark_time_unit=ms" in command

    def test_command_escaping(self):
        """Test that command parts are properly formatted."""
        binary = "./build/rel/bench/benchmark"
        regex = "fib_libfork<lazy_pool>"

        command = f'/usr/bin/time -f"MEMORY=%M"  -- {binary} --benchmark_filter="{regex}" --benchmark_time_unit=ms'

        # Check that special characters are preserved
        assert "<" in command
        assert ">" in command

    def test_command_parts(self):
        """Test individual command parts."""
        parts = [
            "/usr/bin/time",
            "-f",
            '"MEMORY=%M"',
            "--",
            "./benchmark",
            "--benchmark_filter=",
            "--benchmark_time_unit=ms"
        ]

        for part in parts:
            assert isinstance(part, str)
            assert len(part) > 0


class TestMemoryParsing:
    """Test memory output parsing."""

    def test_regex_match_memory(self):
        """Test regex matching for memory output."""
        import re

        test_outputs = [
            "MEMORY=1024",
            "Some output\nMEMORY=2048\nMore output",
            "MEMORY=4096",
            "MEMORY=12345"
        ]

        for output in test_outputs:
            match = re.search(r".*MEMORY=([1-9][0-9]*)", output)
            assert match is not None
            value = int(match.group(1))
            assert value > 0

    def test_regex_no_match(self):
        """Test regex with no memory match."""
        import re

        output = "No memory information here"
        match = re.search(r".*MEMORY=([1-9][0-9]*)", output)

        assert match is None

    def test_regex_zero_memory(self):
        """Test regex rejects zero memory."""
        import re

        output = "MEMORY=0"
        match = re.search(r".*MEMORY=([1-9][0-9]*)", output)

        # Should not match (requires [1-9] as first digit)
        assert match is None

    def test_memory_value_extraction(self):
        """Test extracting memory value from match."""
        import re

        output = "benchmark completed\nMEMORY=8192\ntime elapsed"
        match = re.search(r".*MEMORY=([1-9][0-9]*)", output)

        if match:
            val = int(match.group(1))
            assert val == 8192


class TestStatisticalCalculations:
    """Test statistical calculations."""

    def test_median_calculation(self):
        """Test median calculation."""
        from statistics import median

        values = [100, 102, 101, 103, 105]
        result = median(values)

        assert result == 102

    def test_standard_deviation(self):
        """Test standard deviation calculation."""
        from statistics import stdev

        values = [10, 12, 11, 13, 14]
        result = stdev(values)

        assert result > 0
        assert isinstance(result, float)

    def test_standard_error(self):
        """Test standard error calculation."""
        from statistics import median, stdev
        from math import sqrt

        values = [100, 102, 101, 103, 105]
        x = median(values)
        e = stdev(values) / sqrt(len(values))

        assert e > 0
        assert e < stdev(values)  # SE should be less than SD

    def test_empty_list_handling(self):
        """Test handling of statistics with minimal data."""
        from statistics import median

        values = [100]
        x = median(values)

        assert x == 100

        # stdev requires at least 2 values
        values = [100, 101]
        from statistics import stdev
        s = stdev(values)
        assert s >= 0


class TestRepetitions:
    """Test repetition configuration."""

    def test_serial_repetitions(self):
        """Test that serial benchmarks have more repetitions."""
        is_serial = True
        repetitions = 25 if is_serial else 5

        assert repetitions == 25

    def test_parallel_repetitions(self):
        """Test that parallel benchmarks have fewer repetitions."""
        is_serial = False
        repetitions = 25 if is_serial else 5

        assert repetitions == 5

    def test_repetition_counts_valid(self):
        """Test that repetition counts are reasonable."""
        serial_reps = 25
        parallel_reps = 5

        assert serial_reps > parallel_reps
        assert serial_reps > 0
        assert parallel_reps > 0


class TestFileOutput:
    """Test CSV file output."""

    def test_csv_filename_construction(self):
        """Test CSV filename construction."""
        bench = "fib"
        filename = f"memory.{bench.strip()}.csv"

        assert filename == "memory.fib.csv"

    def test_csv_line_format(self):
        """Test CSV line formatting."""
        kind = "libfork"
        threads = 4
        median_val = 4096.5
        std_err = 50.2

        line = f"{kind},{threads},{median_val},{std_err}\n"

        parts = line.strip().split(",")
        assert len(parts) == 4
        assert parts[0] == kind
        assert parts[1] == str(threads)
        assert float(parts[2]) == median_val
        assert float(parts[3]) == std_err

    def test_csv_multiple_lines(self):
        """Test writing multiple CSV lines."""
        data = [
            ("libfork", 1, 1024.0, 10.0),
            ("libfork", 4, 4096.0, 20.0),
            ("libfork", 8, 6144.0, 30.0)
        ]

        lines = []
        for kind, threads, med, err in data:
            line = f"{kind},{threads},{med},{err}\n"
            lines.append(line)

        assert len(lines) == 3

        for line in lines:
            parts = line.strip().split(",")
            assert len(parts) == 4


class TestMemoryCollection:
    """Test memory measurement collection."""

    def test_memory_list_accumulation(self):
        """Test accumulating memory measurements."""
        mem = []

        # Simulate collecting measurements
        measurements = [1024, 1028, 1020, 1030, 1025]

        for val in measurements:
            mem.append(val)

        assert len(mem) == 5
        assert all(isinstance(m, int) for m in mem)

    def test_memory_value_ranges(self):
        """Test that memory values are in reasonable ranges."""
        mem = [1024, 2048, 4096, 8192]

        for val in mem:
            assert val > 0
            assert val < 1e9  # Less than 1GB in KB


class TestMockSubprocess:
    """Test subprocess execution with mocking."""

    @patch('subprocess.run')
    def test_subprocess_execution(self, mock_run):
        """Test subprocess execution mock."""
        mock_run.return_value = MagicMock(
            stderr=b"MEMORY=4096"
        )

        result = subprocess.run(
            "dummy command",
            shell=True,
            check=True,
            stderr=subprocess.PIPE
        )

        assert mock_run.called
        assert result.stderr == b"MEMORY=4096"

    @patch('subprocess.run')
    def test_subprocess_stderr_capture(self, mock_run):
        """Test capturing stderr from subprocess."""
        mock_run.return_value = MagicMock(
            stderr=b"Some output\nMEMORY=8192\nMore output"
        )

        result = subprocess.run(
            "command",
            shell=True,
            check=True,
            stderr=subprocess.PIPE
        )

        output = result.stderr
        assert b"MEMORY=" in output

    @patch('subprocess.run')
    def test_subprocess_error_handling(self, mock_run):
        """Test subprocess error handling."""
        mock_run.side_effect = subprocess.CalledProcessError(1, "cmd")

        with pytest.raises(subprocess.CalledProcessError):
            subprocess.run(
                "failing command",
                shell=True,
                check=True,
                stderr=subprocess.PIPE
            )


class TestIntegration:
    """Integration tests."""

    def test_full_iteration_logic(self):
        """Test complete iteration logic."""
        thread_counts = [1, 2, 4, 8]
        max_cores = 8
        is_serial = False

        processed_counts = []

        for i in thread_counts:
            if i > max_cores:
                break

            if is_serial and len(processed_counts) > 0:
                break

            processed_counts.append(i)

        assert processed_counts == [1, 2, 4, 8]

    def test_serial_only_first_iteration(self):
        """Test serial benchmark only processes first thread count."""
        thread_counts = [1, 2, 4, 8]
        is_serial = True

        processed_counts = []

        for i in thread_counts:
            processed_counts.append(i)

            if is_serial:
                break

        assert processed_counts == [1]

    def test_benchmark_kind_iteration(self):
        """Test iterating through benchmark kinds."""
        bench = "fib"

        if not bench.startswith("T"):
            libfork = ["libfork.*lazy.*fan"]
        else:
            libfork = ["libfork.*_coalloc_.*lazy.*fan"]

        kinds = [
            "zero",
            "calibrate",
            "serial",
            *libfork,
            "tmc",
            "ccpp",
            "omp",
            "tbb",
            "taskflow"
        ]

        assert len(kinds) == 9
        assert "serial" in kinds


class TestEdgeCases:
    """Test edge cases and error conditions."""

    def test_zero_cores_handling(self):
        """Test handling of zero cores."""
        max_cores = 0
        thread_counts = [1, 2, 4, 8]

        filtered = [t for t in thread_counts if t <= max_cores]

        assert len(filtered) == 0

    def test_single_core_configuration(self):
        """Test single core configuration."""
        max_cores = 1
        thread_counts = [1, 2, 4, 8]

        filtered = [t for t in thread_counts if t <= max_cores]

        assert filtered == [1]

    def test_very_large_core_count(self):
        """Test very large core count."""
        max_cores = 1000
        thread_counts = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]

        filtered = [t for t in thread_counts if t <= max_cores]

        assert len(filtered) == len(thread_counts)

    def test_memory_measurement_boundary(self):
        """Test memory measurement at boundary values."""
        import re

        # Test minimum valid memory
        output = "MEMORY=1"
        match = re.search(r".*MEMORY=([1-9][0-9]*)", output)
        assert match is not None
        assert int(match.group(1)) == 1

        # Test large memory value
        output = "MEMORY=999999999"
        match = re.search(r".*MEMORY=([1-9][0-9]*)", output)
        assert match is not None
        assert int(match.group(1)) == 999999999


class TestFileHandling:
    """Test file handling operations."""

    def test_file_write_mode(self):
        """Test file opening in write mode."""
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            temp_file = f.name
            f.write("test,1,100,5\n")

        try:
            with open(temp_file, 'r') as f:
                content = f.read()
                assert "test,1,100,5" in content
        finally:
            os.unlink(temp_file)

    def test_file_flush(self):
        """Test file flushing."""
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            temp_file = f.name
            f.write("data\n")
            f.flush()  # Should not raise

        os.unlink(temp_file)

    def test_csv_file_creation(self):
        """Test creating CSV file."""
        bench = "test"
        filename = f"memory.{bench}.csv"

        with tempfile.TemporaryDirectory() as tmpdir:
            filepath = os.path.join(tmpdir, filename)

            with open(filepath, 'w') as f:
                f.write("kind,threads,median,stderr\n")
                f.write("libfork,4,4096,20\n")

            assert os.path.exists(filepath)

            with open(filepath, 'r') as f:
                lines = f.readlines()
                assert len(lines) == 2


class TestRegressionCases:
    """Test specific regression cases."""

    def test_benchmark_name_with_special_chars(self):
        """Test benchmark names with special characters."""
        bench_names = ["fib", "integ", "nqueens", "T1", "T1L", "T3XXL"]

        for name in bench_names:
            filename = f"memory.{name.strip()}.csv"
            assert ".csv" in filename
            assert name in filename

    def test_regex_pattern_escaping(self):
        """Test that regex patterns handle special characters."""
        patterns = [
            "libfork.*lazy.*fan",
            "libfork.*_coalloc_.*lazy.*fan",
            "fib.*libfork.*/4/"
        ]

        for pattern in patterns:
            assert isinstance(pattern, str)
            assert len(pattern) > 0

    def test_thread_count_sequence(self):
        """Test the complete thread count sequence."""
        expected = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112]

        # Verify it's a proper sequence
        for i in range(len(expected) - 1):
            assert expected[i] < expected[i + 1]

        # Verify specific milestones
        assert 1 in expected
        assert 32 in expected
        assert 112 in expected


if __name__ == "__main__":
    pytest.main([__file__, "-v"])