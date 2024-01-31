from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build.cppstd import check_min_cppstd
from conan.tools.scm import Git
from conan.tools.files import load, update_conandata
from conan.tools.files import copy

# TODO
# - test if source is not in same repository (scm)

class LibforkConan(ConanFile):
    name = "libfork"
    version = "2.1.1" 
    package_type = "header-library"

    # Optional metadata
    license = "MPL-2.0"
    author = "Conor Williams"
    url = "https://github.com/ConorWilliams/libfork"
    description = "A bleeding-edge, lock-free, wait-free, continuation-stealing tasking library."
    topics = ("multithreading", "fork-join", "parallelism", "framework", "continuation-stealing", "lockfree", "wait-free")
    
    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
            "build_tests" : [True, False],
            "build_benchmarks" : [True, False],
            "build_docs" : [True, False],
            "enable_coverage" : [True, False],
        }
    default_options = {
            "build_tests": False,
            "build_docs": False,
            "build_benchmarks": False,
            "enable_coverage": False,
        }

    no_copy_source = True

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "include/*", "cmake/*", "LICENSE.md"

    def validate(self):
        # We need coroutine support
        check_min_cppstd(self, "20")
        
    def build_requirements(self):
        # Some traits like build=True, etc.. will be automatically inferred
        self.tool_requires("cmake/3.28.1")
        if self.options.build_tests:
            self.test_requires("nanobench/4.3.11")
            self.test_requires("catch2/3.5.1")

    def layout(self):
        cmake_layout(self)
        # Mimic the packageConfig files written by the library
        # For header-only packages, libdirs and bindirs are not used
         # so it's necessary to set those as empty.
        self.cpp.package.includedirs = ["include/libfork-2.1.1"]
        self.cpp.package.bindirs = []
        self.cpp.package.libdirs = []
        # By default, this package will be exported as libfork::libfork

    def generate(self):
        # Generate custom packageConfig files
        deps = CMakeDeps(self)
        deps.generate()
        # Generate build system
        tc = CMakeToolchain(self)
        # Set specs for CMake. E.g. we can relieve CPM from downloading
        tc.cache_variables["CPM_USE_LOCAL_PACKAGES"] = True
        tc.cache_variables["BUILD_TESTING"] = self.options.build_tests
        tc.cache_variables["BUILD_BENCHMARKS"] = self.options.build_benchmarks
        tc.cache_variables["BUILD_DOCS"] = self.options.build_docs
        tc.cache_variables["ENABLE_COVERAGE"] = self.options.enable_coverage
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.build_tests:
            cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_id(self):
        self.info.clear()