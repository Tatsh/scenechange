local utils = import 'utils.libsonnet';

{
  uses_user_defaults: true,
  local settings = self,
  security_policy_supported_versions: { '0.2.x': ':white_check_mark:' },
  project_name: 'scenechange',
  pypi_project_name: 'vapoursynth-scenechange',
  version: '0.5.0',
  license: 'LGPL-2.1-or-later',
  // vapoursynth>=75 (matching vs-jetpack) only supports Python 3.12+.
  supported_python_versions: ['3.12', '3.13', '3.14'],
  description: 'Scene change detection plugin for VapourSynth.',
  authors+: [
    {
      'family-names': 'Motofumi',
      'given-names': 'Oka',
      email: 'chikuzen.mo@gmail.com',
      name: '%s %s' % [self['given-names'], self['family-names']],
    },
  ],
  want_codeql: false,
  keywords: ['plugin', 'vapoursynth'],
  clang_format_args: 'native/*.c',
  github+: {
    workflows+: {
      release_gate_workflows: ['Meson', 'Native Tests'],
    },
  },
  pyproject+: {
    'build-system': {
      'build-backend': 'hatchling.build',
      requires: [
        'hatchling>=1.27.0',
        'meson>=1.3.0',
        'ninja>=1.11.0',
        'packaging>=25.0',
      ],
    },
    'dependency-groups'+: {
      dev+: ['hatchling>=1.27.0', 'meson>=1.3.0', 'ninja>=1.11.0'],
    },
    project+: {
      name: 'vapoursynth-scenechange',
      classifiers: utils.pyprojectClassifiers(settings, [
        'Environment :: Plugins',
        'Operating System :: MacOS',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C',
        'Topic :: Multimedia :: Video',
      ]),
      dependencies: ['vapoursynth>=75'],
    },
    tool+: {
      commitizen+: {
        remove_path_prefixes: ['include', 'native', 'scenechange'],
        version_files+: [
          'meson.build',
          'native/scenechange.c',
          'native/temporalsoften.c',
        ],
      },
      ruff+: {
        'namespace-packages': ['docs', 'tests', 'tools'],
      },
      hatch: {
        build: {
          targets: {
            sdist: {
              include: [
                '/Doxyfile.in',
                '/LICENSE.txt',
                '/README.md',
                '/hatch_build.py',
                '/meson.build',
                '/meson.options',
                '/native',
                '/pyproject.toml',
                '/scenechange',
                '/tools',
              ],
            },
            wheel: {
              artifacts: [
                'vapoursynth/plugins/*.dll',
                'vapoursynth/plugins/*.dylib',
                'vapoursynth/plugins/*.so',
              ],
              hooks: { custom: { path: 'hatch_build.py' } },
              include: ['/scenechange', '/vapoursynth/plugins'],
            },
          },
        },
      },
    },
  },
  package_json+: {
    cspell+: {
      ignorePaths+: [
        '*.patch',
        '.docs/*.tag.xml',
        '.docs/*.tags',
      ],
    },
  },
  prettierignore+: ['*.c', '*.h', '*.in', '*.wrap', 'meson.build', 'meson.options'],
  vscode+: {
    c_cpp+: {
      configurations: [
        {
          cStandard: 'gnu23',
          compilerPath: '/usr/bin/gcc',
          cppStandard: 'gnu++23',
          includePath: [
            '${workspaceFolder}/native/**',
          ],
          name: 'Linux',
        },
      ],
    },
  },
}
