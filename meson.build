project(
  'scenechange',
  'c',
  default_options: ['c_std=c99'],
  license: 'LGPL-2.1-or-later',
  meson_version: '>=1.3.0',
  version: '0.5.0',
)

fs = import('fs')
cc = meson.get_compiler('c')
is_msvc = cc.get_argument_syntax() == 'msvc'

if is_msvc
  common_c_args = ['/wd4224', '/wd4244', '/wd4996']
else
  common_c_args = ['-Wall', '-Wshadow']
endif

version_parts = meson.project_version().split('.')

vapoursynth_dep = dependency('vapoursynth', required: false)
if not vapoursynth_dep.found()
  vapoursynth_include = ''
  python = import('python').find_installation(required: false)
  # Since R74 VapourSynth installs its headers inside the Python package rather
  # than a system include directory, so ask it where they are before giving up.
  # See https://vapoursynth.com/doc/packaging.html.
  if python.found()
    probe = run_command(
      python,
      '-c',
      'import vapoursynth; print(vapoursynth.get_include())',
      check: false,
    )
    if probe.returncode() == 0
      candidate = probe.stdout().strip()
      if candidate != '' and fs.exists(candidate / 'VapourSynth4.h')
        message('Found VapourSynth headers via Python: ' + candidate)
        vapoursynth_include = candidate
      endif
    endif
  endif
  # VapourSynth4.h only depends on stdint.h and stddef.h, so fetching this single
  # header is enough to compile the plugins. The VapourSynth runtime is provided
  # by the host process at load time, so no library is linked. This path is used
  # by the Python wheel builds and by platforms with no VapourSynth wheel, where
  # no system VapourSynth is installed.
  if vapoursynth_include == '' and get_option('fetch_vapoursynth_headers') and python.found()
    headers_ref = get_option('vapoursynth_headers_ref')
    headers_dir = meson.current_build_dir() / 'vapoursynth-headers'
    message('VapourSynth headers not found; downloading VapourSynth4.h (' + headers_ref + ').')
    run_command(
      python,
      files('tools/fetch_vapoursynth_header.py'),
      '--ref',
      headers_ref,
      '--destination',
      headers_dir,
      check: true,
    )
    vapoursynth_include = headers_dir
  endif
  if vapoursynth_include == ''
    error(
      'VapourSynth headers not found. Install VapourSynth via your package manager '
      + '(apt install vapoursynth, brew install vapoursynth), install the vapoursynth '
      + 'Python package, or enable the fetch_vapoursynth_headers option.',
    )
  endif
  # The headers usually live outside the source and build trees, which
  # include_directories() does not accept, so pass the flag directly. MSVC
  # understands the -I spelling as well.
  vapoursynth_dep = declare_dependency(compile_args: ['-I' + vapoursynth_include])
endif

plugins_dir = get_option('plugins_dir')
if plugins_dir == ''
  plugins_dir = get_option('libdir') / 'vapoursynth'
endif

subdir('native')

install_data(
  ['LICENSE.txt', 'README.md'],
  install_dir: get_option('datadir') / 'doc' / meson.project_name(),
)

build_tests = get_option('tests')
if not build_tests.disabled()
  subdir('tests')
endif

if get_option('docs')
  doxygen = find_program('doxygen', required: true)
  doxyfile_conf = configuration_data()
  doxyfile_conf.set('PROJECT_NAME', meson.project_name())
  doxyfile_conf.set('PROJECT_VERSION', meson.project_version())
  doxyfile_conf.set('SOURCE_ROOT', meson.current_source_dir())
  doxyfile_conf.set('OUTPUT_ROOT', meson.current_build_dir() / 'docs')
  doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: doxyfile_conf,
  )
  custom_target(
    'docs',
    build_by_default: true,
    command: [doxygen, doxyfile],
    output: 'docs',
  )
endif

summary(
  {
    'Documentation': get_option('docs'),
    'Native tests': not build_tests.disabled(),
    'Plugins directory': plugins_dir,
    'SIMD backend': kernel_backend,
  },
  section: 'scenechange',
)
