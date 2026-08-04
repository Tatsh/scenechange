# Compile the backend that matches the host architecture. The chosen sources are
# reused by the native test suite so it exercises the kernel the host ships.
if get_option('force_generic')
  kernel_backend = 'generic (forced)'
  kernel_sources = files('temporalsoften_generic.c')
  kernel_c_args = []
elif host_machine.cpu_family() == 'x86_64' or host_machine.cpu_family() == 'x86'
  kernel_backend = 'x86 SSE2'
  kernel_sources = files('temporalsoften_x86.c')
  if is_msvc
    kernel_c_args = host_machine.cpu_family() == 'x86' ? ['/arch:SSE2'] : []
  else
    kernel_c_args = ['-msse2', '-mfpmath=sse']
  endif
elif host_machine.cpu_family() == 'aarch64'
  kernel_backend = 'AArch64 NEON'
  kernel_sources = files('temporalsoften_aarch64.c')
  kernel_c_args = []
else
  kernel_backend = 'generic (no SIMD backend for ' + host_machine.cpu_family() + ')'
  kernel_sources = files('temporalsoften_generic.c')
  kernel_c_args = []
endif

m_dep = cc.find_library('m', required: false)

# Per-target metadata: Windows resource files for DLL VERSIONINFO, macOS
# embedded Info.plist via -sectcreate. Both pull the project version and common
# attribution from the project() definition.
scenechange_extra_sources = []
temporalsoften2_extra_sources = []
scenechange_link_args = []
temporalsoften2_link_args = []

if host_machine.system() == 'windows'
  windows = import('windows')
  rc_company = 'Tatsh'
  rc_copyright = 'Copyright (C) 2012 Oka Motofumi; LGPL-2.1-or-later'

  foreach plugin : [
    ['scenechange', 'Scene change detection plugin for VapourSynth'],
    ['temporalsoften2', 'VapourSynth TemporalSoften filter'],
  ]
    rc_conf = configuration_data()
    rc_conf.set('PROJECT_NAME', meson.project_name())
    rc_conf.set('PROJECT_VERSION', meson.project_version())
    rc_conf.set('PROJECT_VERSION_MAJOR', version_parts[0])
    rc_conf.set('PROJECT_VERSION_MINOR', version_parts[1])
    rc_conf.set('PROJECT_VERSION_PATCH', version_parts[2])
    rc_conf.set('SCENECHANGE_RC_COMPANY', rc_company)
    rc_conf.set('SCENECHANGE_RC_COPYRIGHT', rc_copyright)
    rc_conf.set('SCENECHANGE_RC_DESCRIPTION', plugin[1])
    rc_conf.set('SCENECHANGE_RC_INTERNAL_NAME', plugin[0])
    rc_conf.set('SCENECHANGE_RC_ORIGINAL_FILENAME', plugin[0] + '.dll')
    rc_file = configure_file(
      input: 'version.rc.in',
      output: plugin[0] + '.rc',
      configuration: rc_conf,
    )
    if plugin[0] == 'scenechange'
      scenechange_extra_sources += windows.compile_resources(rc_file)
    else
      temporalsoften2_extra_sources += windows.compile_resources(rc_file)
    endif
  endforeach
elif host_machine.system() == 'darwin'
  foreach plugin : ['scenechange', 'temporalsoften2']
    plist_conf = configuration_data()
    plist_conf.set('PROJECT_VERSION', meson.project_version())
    plist_conf.set('SCENECHANGE_PLIST_NAME', plugin)
    plist_conf.set(
      'SCENECHANGE_PLIST_COPYRIGHT',
      'Copyright (C) 2012 Oka Motofumi; LGPL-2.1-or-later',
    )
    configure_file(
      input: 'Info.plist.in',
      output: plugin + '.plist',
      configuration: plist_conf,
    )
    plist_link_args = [
      '-Wl,-sectcreate,__TEXT,__info_plist,'
      + meson.current_build_dir() / (plugin + '.plist'),
    ]
    if plugin == 'scenechange'
      scenechange_link_args += plist_link_args
    else
      temporalsoften2_link_args += plist_link_args
    endif
  endforeach
endif

shared_library(
  'scenechange',
  'scenechange.c',
  scenechange_extra_sources,
  c_args: common_c_args,
  dependencies: vapoursynth_dep,
  install: true,
  install_dir: plugins_dir,
  link_args: scenechange_link_args,
)

shared_library(
  'temporalsoften2',
  'temporalsoften.c',
  kernel_sources,
  temporalsoften2_extra_sources,
  c_args: common_c_args + kernel_c_args,
  dependencies: [vapoursynth_dep, m_dep],
  install: true,
  install_dir: plugins_dir,
  link_args: temporalsoften2_link_args,
)
