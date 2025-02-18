catch2_dep = dependency('catch2')
catch2_with_main_dep = dependency('catch2-with-main')

sources = [
  'src/test_stack.cpp',
  'src/test_constexpr.cpp',
  'src/test_ev.cpp',
]

test_name = '@0@_test'.format(meson.project_name())

libfork_test_exe = executable(
  test_name,
  sources,
  dependencies: [libfork_dep, catch2_dep, catch2_with_main_dep],
)

test(test_name, libfork_test_exe)
