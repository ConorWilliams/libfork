from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build.cppstd import check_min_cppstd


class libfork(ConanFile):

    name = "libfork"
    version = "3.6.0"
    package_type = "header-library"

    # Optional metadata
    license = "MPL-2.0"
    author = "Conor Williams"
    url = "https://github.com/ConorWilliams/libfork"
    description = (
        "A bleeding-edge, lock-free, wait-free, continuation-stealing tasking library."
    )
    topics = (
        "multithreading",
        "fork-join",
        "parallelism",
        "framework",
        "continuation-stealing",
        "lock-free",
        "wait-free",
    )

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # WHAT IS THIS?
    # no_copy_source = True

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "include/*", "cmake/*", "LICENSE.md"

    #    ---

    def validate(self):
        # We need coroutine support
        check_min_cppstd(self, "20")

    def build_requirements(self):
        self.test_requires("boost/1.83.0")
        self.test_requires("hwloc/2.9.3")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        # Generate build system
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["libfork"]
