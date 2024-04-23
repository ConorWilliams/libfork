# Building with libfork

## Quick start

If you know what you are doing and have all the dependencies installed, you can install libfork with the following commands:

Get the source:

```sh
git clone git@github.com:ConorWilliams/libfork
```

Configure:

```sh
cmake -B libfork/build -S libfork
```

Build and install:

```sh
cmake --install libfork/build
```

Note: The last step may need `sudo` privileges if you are installing to a system directory.

## Pre-requisites

### Dependencies

Outside of hwloc, libfork has no more required dependencies. However, libfork can use some optional dependencies to enable additional features, the tests/benchmarks/docs have their own dependencies and, the developer's lint and miscellaneous targets need a few additional dependencies. A complete list of libfork's dependencies are as follows:

<!-- TODO: test with a fresh wsl ubuntu -->

Core:

- __CMake__ 3.14 or greater (version 3.28 is recommended).
- __C++20 compiler__ (C++23 is preferred) - see [compiler support](#compiler-support) section.
- __hwloc__ - see [below](#hwloc).

Optional:

- __boost-atomic__ - recommended for performance if using the clang compiler.

Docs:

- __python 3 + pip deps__ - see [requirements.txt](docs/requirements.txt).
- __doxygen__

Tests:

- __catch2__

Benchmarks:

- __google benchmark__
- __taskflow__
- __tbb__
- __concurrencpp__

Lint/developer

- __cppcheck__
- __clang-tidy__
- __clang-format__
- __codespell__

#### vcpkg

The easiest way to manage libfork's main dependencies (some of which are required for the [additional targets](#additional-targets)) is with vcpkg; at the [configuration stage](#configure-and-build) append the following flags to fetch the required dependencies:

```sh
-DCMAKE_TOOLCHAIN_FILE=<path to vcpkg>/scripts/buildsystems/vcpkg.cmake -DVCPKG_MANIFEST_FEATURES="<features>"
```

where `<path to vcpkg>` is the path to your vcpkg installation and ``<features>`` is a colon-separated list of one or more of the available features: `test`, `benchmark`, `boost` and, `hwloc`. The `test` and `benchmark` features include the dependencies __required__  for the test and benchmark suits respectively. 

#### Hwloc

Hwloc enables libfork to determine the topology of the system and use this information to optimize work-stealing. Unfortunately, hwloc does not have native CMake support hence, it is recommended to use the system's hwloc installation if available, e.g. on Ubuntu/Debian:

```sh
sudo apt install libhwloc-dev
```

### Compiler support

Some very new C++ features are used in libfork, most compilers have buggy implementations of coroutines, we do our best to work around known bugs/deficiencies:

- __clang__ Libfork compiles on versions 15.x-18.x however for versions 16.x and below bugs [#63022](https://github.com/llvm/llvm-project/issues/63022) and [#47179](https://github.com/llvm/llvm-project/issues/47179) will cause crashes for optimized builds in multithreaded programs. We work around these bugs by isolating access to `thread_local` storage in non-inlined functions however, this introduces a performance penalty in these versions.

- __gcc__ Libfork is tested on versions 11.x-13.x however gcc [does not perform a guaranteed tail call](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100897) for coroutine's symmetric transfer unless compiling with optimization greater than or equal to `-O2` and sanitizers are not in use. This will result in stack overflows for some programs in un-optimized builds.

- __Apple's clang__ Libfork is compatible with the standard library that Apple ships with Xcode 2015 but, the mitigations for the old clang versions (mentioned above) degrade performance.

- __msvc__ Libfork compiles on versions 19.35-19.37 however due to [this bug](https://developercommunity.visualstudio.com/t/Incorrect-code-generation-for-symmetric/1659260?scope=follow) (duplicate [here](https://developercommunity.visualstudio.com/t/Using-symmetric-transfer-and-coroutine_h/10251975?scope=follow&q=symmetric)) it will always seg-fault at runtime due to an erroneous double delete. Note that, by default, MSVC is not standards compliant and you need to pass some flags to make it behave properly - see the `flags-windows` preset in the [CMakePresets.json](CMakePresets.json) file.

### Apple silicon

CMake supports building on Apple Silicon properly since 3.20.1. Make sure you have the [latest version][1] installed.

## Getting the source

All libfork's dependencies can be managed through vcpkg which is supplied as a submodule. Hence, if you want to use vcpkg, when cloning the source you must clone recursively as follows:

```sh
git clone --recursive git@github.com:ConorWilliams/libfork.git
```

If you don't want to use vcpkg or use a central vcpkg installation you can skip the recursive clone and set `CMAKE_TOOLCHAIN_FILE` as appropriate.

## Configure and build with CMake

### Building

This project doesn't require any special command-line flags to build to keep things simple.

For building in release mode with a single-configuration generator, like the Unix Makefiles one:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or with a multi-configuration generator, like the Visual Studio ones:

```sh
cmake -S . -B build
cmake --build build --config Release
```

Note, the build step won't do anything as libfork is a header-only library and no tests, benchmarks, etc. have been requested. We cover how to enable these [additional targets](#additional-targets) below.

## Install

The following commands require at least CMake 3.15 to run, because that is the
version in which [Install a Project][2] was added. As a pre, the [above commands](#configure-and-build) must have been run.

To install the release artifacts (headers) with a single-configuration generator, like the Unix Makefiles one:

```sh
cmake --install build
```

Or with a multi-configuration generator, like the Visual Studio ones:

```sh
cmake --install build --config Release
```

### Note to packagers

The `CMAKE_INSTALL_INCLUDEDIR` is set to a path other than just `include` if
the project is configured as a top level project to avoid indirectly including
other libraries when installed to a common prefix. Please review the
[install-rules.cmake](cmake/install-rules.cmake) file for the full set of
install rules.

[1]: https://cmake.org/download/
[2]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project

## Additional targets

Build system targets that are primarily useful for developers of this project are hidden if the `libfork_DEV_MODE` option is disabled. Enabling this option makes tests and other developer targets and options available. Not enabling this option means that you are a consumer of this project and thus you
have no need for these targets and options.

The following targets you may invoke using the build command from above, with an additional `-t <target>` flag. Make sure you have also installed any required dependencies

- `test` Enabled with `BUILD_TESTING` (and by default with dev-mode). This target builds and runs the test suit. The test binary will be placed in `<binary-dir>/test` by default.

- `benchmark` Available if `BUILD_BENCHMARKS` is enabled. This target builds the included benchmarking suit. The benchmarks use [google benchmark](https://github.com/google/benchmark) see their [user guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md) for how to use it. The benchmark binary will be placed in `<binary-dir>/benchmark` by default.

- `docs` Available if `BUILD_DOCS` is enabled. Builds to documentation using Doxygen and Sphinx. The output will go to `<binary-dir>/docs` by default (customizable using `DOXYGEN_OUTPUT_DIRECTORY`).

- `coverage` Available if `ENABLE_COVERAGE` is enabled. This target processes the output of the previously run tests when built with coverage configuration. The commands this target runs can be found in the `COVERAGE_TRACE_COMMAND` and `COVERAGE_HTML_COMMAND` cache variables. The trace command produces an info file by default, which can be submitted to services with CI integration. The HTML command uses the trace command's output to generate an HTML document to `<binary-dir>/coverage_html` by default.

- `format-check` and `format-fix` These targets run the clang-format tool on the codebase to check errors and to fix them respectively. Customization available using the `FORMAT_PATTERNS` and `FORMAT_COMMAND` cache variables.

- `spell-check` and `spell-fix` These targets run the codespell tool on the codebase to check errors and to fix them respectively. Customization available using the `SPELL_COMMAND` cache variable.

## For developers

Here is some wisdom to help you build and test this project as a developer and potential contributor. If you plan to contribute, please read the [CONTRIBUTING](CONTRIBUTING.md) guide. This project makes use of [presets][1] to simplify the process of configuring
the project. As a developer, you are recommended to always have the [latest
CMake version][2] installed to make use of the latest Quality-of-Life
additions.

As a developer, you should create a `CMakeUserPresets.json` file at the root of the project:

```json
{
  "version": 2,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 14,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "dev",
      "binaryDir": "${sourceDir}/build/dev",
      "inherits": ["dev-mode", "ci-<os>"],
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug"
    }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

You should replace `<os>` in your newly created presets file with the name of
the operating system you have, which may be `win64`, `linux` or `darwin`. You
can see what these correspond to in the
[`CMakePresets.json`](CMakePresets.json) file.

`CMakeUserPresets.json` is also the perfect place in which you can put all
sorts of things that you would otherwise want to pass to the configure command
in the terminal.

### Configure, build and test

If you followed the above instructions, then you can configure, build and test
the project respectively with the following commands from the project root on
any operating system with any build system:

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

If you are using a compatible editor (e.g. VSCode) or IDE (e.g. CLion, VS), you
will also be able to select the above created user presets for automatic
integration.

Please note that both the build and test commands accept a `-j` flag to specify
the number of jobs to use, which should ideally be specified to the number of
threads your CPU has. You may also want to add that to your preset using the
`jobs` property, see the [presets documentation][1] for more details.

### Git hooks

If you have set up the above developer presets then you may want to use the provided git-hook to check your commits before pushing to CI. To do this run the following command from the project root:

```sh
cp git-hooks/pre-push .git/hooks/pre-push; chmod 700 .git/hooks/pre-push
```

Now the lints and tests will run before each push and the single-header file will be updated if necessary.
