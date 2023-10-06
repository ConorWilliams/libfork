# Building with CMake

## Build

This project doesn't require any special command-line flags to build to keep
things simple.

Here are the steps for building in release mode with a single-configuration
generator, like the Unix Makefiles one:

TODO: rework hacking/building into a unified guide for consumers/benchmarkers and mention recursive clone + vcpkg!!

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

Here are the steps for building in release mode with a multi-configuration
generator, like the Visual Studio ones:

```sh
cmake -S . -B build
cmake --build build --config Release
```

### Building with MSVC

Note that MSVC by default is not standards compliant and you need to pass some
flags to make it behave properly. See the `flags-windows` preset in the
[CMakePresets.json](CMakePresets.json) file for the flags and with what
variable to provide them to CMake during configuration.

### Building on Apple Silicon

CMake supports building on Apple Silicon properly since 3.20.1. Make sure you
have the [latest version][1] installed.

## Install

This project doesn't require any special command-line flags to install to keep
things simple. As a prerequisite, the project has to be built with the above
commands already.

The below commands require at least CMake 3.15 to run, because that is the
version in which [Install a Project][2] was added.

Here is the command for installing the release mode artifacts with a
single-configuration generator, like the Unix Makefiles one:

```sh
cmake --install build
```

Here is the command for installing the release mode artifacts with a
multi-configuration generator, like the Visual Studio ones:

```sh
cmake --install build --config Release
```

### CMake integration

This project exports a CMake package to be used with the [`find_package`][3] command of CMake:

* Package name: `libfork`
* Target name: `libfork::libfork`

#### If you have installed libfork

```cmake
find_package(libfork REQUIRED)

# Declare the imported target as a build requirement using PRIVATE, where
# project_target is a target created in the consuming project
target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

#### Using CMake's ``FetchContent``

```cmake
include(FetchContent)

FetchContent_Declare(
    libfork
    GIT_REPOSITORY https://github.com/conorwilliams/libfork.git
    GIT_TAG v3.0.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(libfork)

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

#### Using [CMP.cmake](https://github.com/cpm-cmake/CPM.cmake)

Assuming your ``CPM.cmake`` file is the ``cmake`` directory in your project:

```cmake
include(cmake/CPM.cmake)

CPMAddPackage("gh:conorwilliams/libfork#3.0.0")

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

#### Using git submodules

Assuming you cloned libfork as a submodule into ``external/libfork``

```cmake
add_subdirectory(external/libfork)

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

### Note to packagers

The `CMAKE_INSTALL_INCLUDEDIR` is set to a path other than just `include` if
the project is configured as a top level project to avoid indirectly including
other libraries when installed to a common prefix. Please review the
[install-rules.cmake](cmake/install-rules.cmake) file for the full set of
install rules.

[1]: https://cmake.org/download/
[2]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project
[3]: https://cmake.org/cmake/help/latest/command/find_package.html
