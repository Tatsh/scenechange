local utils = import 'utils.libsonnet';

{
  uses_user_defaults: true,
  local settings = self,
  security_policy_supported_versions: { '0.2.x': ':white_check_mark:' },
  project_name: 'scenechange',
  pypi_project_name: 'vapoursynth-scenechange',
  version: '0.3.0',
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
  custom_project_badges: [
    {
      anchor: '[![PyPI - Version](https://img.shields.io/pypi/v/vapoursynth-scenechange)]',
      href: 'https://pypi.org/project/vapoursynth-scenechange/',
    },
    {
      anchor: '[![Tests](https://github.com/Tatsh/scenechange/actions/workflows/tests.yml/badge.svg)]',
      href: 'https://github.com/Tatsh/scenechange/actions/workflows/tests.yml',
    },
    {
      anchor: '[![Coverage Status](https://coveralls.io/repos/github/Tatsh/scenechange/badge.svg?branch=master)]',
      href: 'https://coveralls.io/github/Tatsh/scenechange?branch=master',
    },
  ],
  github+: {
    workflows+: {
      release_gate_workflows: ['CMake', 'Native Tests'],
    },
  },
  pyproject+: {
    'build-system': {
      'build-backend': 'scikit_build_core.build',
      requires: ['scikit-build-core>=0.12.2'],
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
          'CMakeLists.txt',
          'native/scenechange.c',
          'native/temporalsoften.c',
          'vcpkg.json',
        ],
      },
      hatch:: null,
      'scikit-build': {
        cmake: {
          version: '>=3.28',
          define: {
            BUILD_DOCS: 'OFF',
            BUILD_PYTHON_PACKAGE: 'ON',
            BUILD_TESTS: 'OFF',
          },
        },
        'minimum-version': 'build-system.requires',
        sdist: {
          exclude: ['**'],
          include: [
            '/CMakeLists.txt',
            '/LICENSE.txt',
            '/README.md',
            '/pyproject.toml',
            '/scenechange/**/*.py',
            '/scenechange/py.typed',
            '/native/**/*.c',
            '/native/**/*.h',
            '/native/CMakeLists.txt',
          ],
        },
        wheel: {
          exclude: ['**/*.c', '**/*.h', '**/CMakeLists.txt'],
          'py-api': 'py3',
          packages: ['scenechange'],
        },
      },
    },
  },
  vcpkg+: {
    features: {
      tests: {
        description: 'Build the unit-test suite.',
        dependencies: ['cmocka'],
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
  prettierignore+: ['*.c', '*.h', '*.in'],
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
