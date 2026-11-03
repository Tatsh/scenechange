{
  build_command: 'meson compile -C build-scan --verbose',
  configure_command: 'meson setup build-scan -Dtests=disabled',
  documentation_steps: [
    {
      name: 'Build project homepage',
      uses: 'actions/jekyll-build-pages@44a6e6beabd48582f863aeeb6cb2151cc1716697',
      with: {
        destination: 'build/html',
      },
    },
  ],
  env: {},
  setup_steps: [
    {
      name: 'Install current CMake',
      run: 'pipx install cmake',
    },
    {
      name: 'Install Meson',
      run: 'pipx install meson',
    },
  ],
}
