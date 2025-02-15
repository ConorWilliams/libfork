catch2_dep = dependency('catch2')
catch2_with_main_dep = dependency('catch2-with-main')

sources = [
  'src/zip.cpp',
]

test_name = '@0@_test'.format(meson.project_name())

libfork_test_exe = executable(
  test_name,
  sources,
  include_directories: inc,
  link_with: libfork_lib,
  dependencies: [catch2_dep, catch2_with_main_dep],
)

test(test_name, libfork_test_exe)
