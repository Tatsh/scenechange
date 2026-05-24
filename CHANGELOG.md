<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Added

- CMake-based build system (CMake 3.28 or newer, C99) that produces the `libscenechange.so`
  (namespace `scd`) and `libtemporalsoften2.so` (namespace `focus2`) plugins as MODULE libraries.
- VapourSynth discovery via `pkg-config`, with the install destination taken from
  `pkg-config --variable=pluginsdir vapoursynth`, falling back to
  `${CMAKE_INSTALL_FULL_LIBDIR}/vapoursynth` and overridable with `-DVAPOURSYNTH_PLUGINS_DIR=...`.
- SPDX `LGPL-2.1-or-later` identifiers in both C sources.
- Module `__version__` and `__all__` exports in `temporalsoften2.py`.
- Full PEP 484 type annotations for the `temporalsoften2` Python wrapper.

### Changed

- C sources now include the system `<VapourSynth.h>` header instead of a vendored copy.
- MSVC warning suppressions moved from in-source `#pragma warning` directives to
  `/wd4224 /wd4244 /wd4996` compiler flags configured in CMake.
- Bumped `TEMPORALSOFTEN2_VERSION` from `0.1.1` to `0.2.0` to align with the project version.
- Polished the `temporalsoften2` module docstring, including corrections to the `site-packages`
  path and the `TemporalSoften` usage example.

## [0.2.0] - 2017-04-08

First version.

[unreleased]: https://github.com/Tatsh/scenechange/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/Tatsh/scenechange/releases/tag/v0.2.0
