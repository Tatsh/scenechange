cmocka_dep = dependency('cmocka', required: get_option('tests'))

if cmocka_dep.found()
  # The kernel sources and flags come from native/meson.build, so the tests
  # exercise the same backend the plugin is built with.
  test_kernel = executable(
    'test_kernel',
    'test_kernel.c',
    kernel_sources,
    c_args: common_c_args + kernel_c_args,
    dependencies: [cmocka_dep, vapoursynth_dep, m_dep],
    include_directories: include_directories('../native'),
  )
  test('test_kernel', test_kernel)
endif
