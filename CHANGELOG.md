<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Changed

- Migrated both plugins from the VapourSynth API 3 header (`VapourSynth.h`) to API 4
  (`VapourSynth4.h`). The entry point is now `VapourSynthPluginInit2`, filters are created through
  `createVideoFilter` with explicit frame-request dependencies, frame properties are accessed via
  the `mapGet`/`mapSet` and `getFrameProperties*` functions, and the argument and return strings
  use the `vnode` type. VapourSynth R55 or newer is now required; API 3 is no longer supported.

## [0.3.0] - 2026-05-24

### Added

- CMake-based build system (CMake 3.28 or newer, C99) that produces the `libscenechange.so`
  (namespace `scd`) and `libtemporalsoften2.so` (namespace `focus2`) plugins as MODULE libraries.
- VapourSynth discovery via `pkg-config`, with the install destination taken from
  `pkg-config --variable=pluginsdir vapoursynth`, falling back to
  `${CMAKE_INSTALL_FULL_LIBDIR}/vapoursynth` and overridable with `-DVAPOURSYNTH_PLUGINS_DIR=...`.
- SPDX `LGPL-2.1-or-later` identifiers in both C sources.
- Module `__version__` and `__all__` exports in `temporalsoften2.py`.
- Full PEP 484 type annotations for the `temporalsoften2` Python wrapper.
- Per-architecture SIMD backend layout for `temporalsoften`, with the legacy SSE2 kernel moved
  into its own backend file alongside new aarch64 NEON and portable scalar generic backends.
- `force-generic` and coverage CMake options for opting into the scalar backend and for
  producing coverage instrumentation.
- CPack zip packaging for binary distribution.
- Windows VERSIONINFO resource and macOS `Info.plist` metadata embedded into the built plugin
  binaries.
- GitHub Actions workflows for CMake builds, CodeQL analysis, releases, and tests, including a
  release gate that requires the Tests workflow to succeed before publishing.
- cmocka-based unit-test suite that exercises the `temporalsoften` kernel against the SSE2,
  NEON, and generic backends, preferring the system or vcpkg cmocka and falling back to
  `FetchContent` when neither is available.
- Vendored VapourSynth headers used by the Linux CI workflows to keep builds hermetic.
- Credit for Oka Motofumi in the project authors.

### Changed

- C sources now include the system `<VapourSynth.h>` header instead of a vendored copy.
- MSVC warning suppressions moved from in-source `#pragma warning` directives to
  `/wd4224 /wd4244 /wd4996` compiler flags configured in CMake.
- Bumped `TEMPORALSOFTEN2_VERSION` from `0.1.1` to `0.2.0` to align with the project version.
- Polished the `temporalsoften2` module docstring, including corrections to the `site-packages`
  path and the `TemporalSoften` usage example.
- Debug builds now upload their artefacts from the CMake workflow alongside Release builds.

### Fixed

- MSVC compilation errors in the C sources.

## [0.2.0] - 2017-04-08

First version.

[unreleased]: https://github.com/Tatsh/scenechange/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/Tatsh/scenechange/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/Tatsh/scenechange/releases/tag/v0.2.0
