gtest_dep = dependency('gtest')
gtest_main_dep = dependency('gtest_main')

catch2_dep = dependency('catch2')

sources = [
  'src/stack.cpp',
]

libfork_test_exe = executable(
  'libfork_test',
  sources,
  include_directories: inc,
  link_with: libfork_lib,
  dependencies: [gtest_dep, gtest_main_dep],
)

test('libfork_test', libfork_test_exe)
