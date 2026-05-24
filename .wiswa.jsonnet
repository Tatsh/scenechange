{
  uses_user_defaults: true,
  security_policy_supported_versions: { '0.2.x': ':white_check_mark:' },
  project_name: 'scenechange',
  version: '0.2.0',
  license: 'LGPL-2.1-or-later',
  description: 'Scene change detection plugin for VapourSynth.',
  authors+: [
    { 'family-names': 'Motofumi', 'given-names': 'Oka', email: 'chikuzen.mo@gmail.com' },
  ],
  custom_project_badges: [
    {
      anchor: '[![Tests](https://github.com/Tatsh/scenechange/actions/workflows/tests.yml/badge.svg)]',
      href: 'https://github.com/Tatsh/scenechange/actions/workflows/tests.yml',
    },
    {
      anchor: '[![Coverage Status](https://coveralls.io/repos/github/Tatsh/scenechange/badge.svg?branch=master)]',
      href: 'https://coveralls.io/github/Tatsh/scenechange?branch=master',
    },
  ],
  clang_format_args: 'src/*.c',
  keywords: ['plugin', 'vapoursynth'],
  want_codeql: false,
  want_main: false,
  want_tests: false,
  want_snap: false,
  want_appimage: false,
  cz+: {
    commitizen+: {
      version_files+: ['src/scenechange.c', 'src/temporalsoften.c', 'temporalsoften2.py'],
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
  prettierignore+: ['*.c', '*.h'],
  vscode+: {
    c_cpp+: {
      configurations: [
        {
          cStandard: 'gnu23',
          compilerPath: '/usr/bin/gcc',
          cppStandard: 'gnu++23',
          includePath: [
            '${workspaceFolder}/src/**',
          ],
          name: 'Linux',
        },
      ],
    },
  },
  project_type: 'c',
}
