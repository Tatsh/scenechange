# scenechange

<!-- WISWA-GENERATED-README:START -->

[![C](https://img.shields.io/badge/C-00599C?logo=c)](<https://en.wikipedia.org/wiki/C_(programming_language)>)
[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/Tatsh/scenechange)](https://github.com/Tatsh/scenechange/tags)
[![License](https://img.shields.io/github/license/Tatsh/scenechange)](https://github.com/Tatsh/scenechange/blob/master/LICENSE.txt)
[![GitHub commits since latest release (by SemVer including pre-releases)](https://img.shields.io/github/commits-since/Tatsh/scenechange/v0.3.0/master)](https://github.com/Tatsh/scenechange/compare/v0.3.0...master)
[![Dependabot](https://img.shields.io/badge/Dependabot-enabled-blue?logo=dependabot)](https://github.com/dependabot)
[![pages-build-deployment](https://github.com/Tatsh/scenechange/actions/workflows/pages/pages-build-deployment/badge.svg)](https://tatsh.github.io/scenechange/)
[![Stargazers](https://img.shields.io/github/stars/Tatsh/scenechange?logo=github&style=flat)](https://github.com/Tatsh/scenechange/stargazers)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit)](https://github.com/pre-commit/pre-commit)
[![CMake](https://img.shields.io/badge/CMake-6E6E6E?logo=cmake)](https://cmake.org/)
[![Prettier](https://img.shields.io/badge/Prettier-black?logo=prettier)](https://prettier.io/)
[![Tests](https://github.com/Tatsh/scenechange/actions/workflows/tests.yml/badge.svg)](https://github.com/Tatsh/scenechange/actions/workflows/tests.yml)
[![Coverage Status](https://coveralls.io/repos/github/Tatsh/scenechange/badge.svg?branch=master)](https://coveralls.io/github/Tatsh/scenechange?branch=master)

[![@Tatsh](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor=did%3Aplc%3Auq42idtvuccnmtl57nsucz72&query=%24.followersCount&label=Follow+%40Tatsh&logo=bluesky&style=social)](https://bsky.app/profile/Tatsh.bsky.social)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Tatsh-black?logo=buymeacoffee)](https://buymeacoffee.com/Tatsh)
[![Libera.Chat](https://img.shields.io/badge/Libera.Chat-Tatsh-black?logo=liberadotchat)](irc://irc.libera.chat/Tatsh)
[![Mastodon Follow](https://img.shields.io/mastodon/follow/109370961877277568?domain=hostux.social&style=social)](https://hostux.social/@Tatsh)
[![Patreon](https://img.shields.io/badge/Patreon-Tatsh2-F96854?logo=patreon)](https://www.patreon.com/Tatsh2)

<!-- WISWA-GENERATED-README:STOP -->

Scene change detection plugin for VapourSynth.

The project builds two VapourSynth plugins:

- `scenechange` (namespace `scd`) detects scene changes and attaches `_SceneChangePrev` and
  `_SceneChangeNext` frame properties (0 or 1) so downstream temporal filters can avoid bleeding
  across cuts.
- `temporalsoften2` (namespace `focus2`) is a temporal-averaging denoiser that honours those scene
  change properties.

A small Python wrapper (`temporalsoften2.py`) is also included; it stitches the two plugins
together so callers only need a single function call.

## Building and installing

Requirements:

- A C99 compiler (GCC, Clang, or MSVC).
- CMake 3.28 or newer, and Ninja (or any other generator).
- VapourSynth R55 or newer (API 4) development headers and `vapoursynth.pc` (`vapoursynth-dev` or
  your distribution's equivalent). The plugins use `VapourSynth4.h` and are not compatible with the
  legacy API 3.

Configure, build, and install:

```shell
cmake -S . -B build -G Ninja
cmake --build build
cmake --install build
```

By default the plugins install to the directory reported by
`pkg-config --variable=pluginsdir vapoursynth`, falling back to
`${CMAKE_INSTALL_FULL_LIBDIR}/vapoursynth` when the pkg-config variable is unset. Override the
location with `-DVAPOURSYNTH_PLUGINS_DIR=/path/to/plugins` at configure time.

The Python helper `temporalsoften2.py` is shipped in the source tree but is intentionally not
installed by CMake. Copy it manually into a directory on your `PYTHONPATH` (for example
`Python3.x/Lib/site-packages` on Windows) if you want the wrapper.

## Usage

### scd.Detect

```python
clip = core.scd.Detect(clip, thresh, interval_h, interval_v, log)
```

Detect scene changes and attach `_SceneChangePrev` and `_SceneChangeNext` properties to the clip.

- `thresh`: threshold for the average of luma differences between the previous and next frames.
  When the average exceeds this value the frame is judged a scene change. Range is 1 to
  `254 * 2 ^ (bitdepth - 8)`; the default (or any out-of-range value) is `15 * 2 ^ (bitdepth - 8)`.
- `interval_h`: horizontal interval, in pixels, used when measuring differences. Range is 1 to the
  clip width; the default is auto-adjusted.
- `interval_v`: vertical interval, in pixels, used when measuring differences. Range is 1 to the
  clip height; the default is auto-adjusted.
- `log`: optional path of a log file. When set, the frame numbers identified as scene changes are
  written as text. An absolute path is recommended. Unset by default.

### scd.ApplyLog

```python
clip = core.scd.ApplyLog(clip, log)
```

Apply `_SceneChangePrev` and `_SceneChangeNext` properties to the clip from a log previously
produced by `scd.Detect`.

Supported colour families are GRAY (8-bit and 16-bit) and YUV (8-bit, 9-bit, 10-bit, and 16-bit).

### focus2.TemporalSoften2

```python
clip = core.focus2.TemporalSoften2(clip, radius, luma_threshold, chroma_threshold,
                                   scenechange, mode)
```

- `radius`: 1 to 7. Default is 4.
- `luma_threshold`: 0 to 255. For RGB clips this value applies to all planes. Default is 4.
- `chroma_threshold`: 0 to 255. Ignored for RGB and Gray clips. Default is 8.
- `scenechange`: when 0 the `_SceneChange*` frame properties are ignored. Default is 1.
- `mode`: 2.

YUV or Gray example:

```python
import vapoursynth as vs

core = vs.Core()
core.std.LoadPlugin('/path/to/libscenechange.so')
core.std.LoadPlugin('/path/to/libtemporalsoften2.so')

clip = core.scd.Detect(clip, thresh=20)
clip = core.focus2.TemporalSoften2(clip)
```

RGB example (scene detection requires GRAY8, so the properties must be copied back):

```python
def copy_sc(n, f):
    fout = f[0].copy()
    fout.props._SceneChange = f[1].props._SceneChange[0]
    return fout


tmp = core.resize.Point(clip, format=vs.GRAY8)
tmp = core.scd.Detect(tmp, thresh=20)
clip = core.std.ModifyFrame([clip, tmp], copy_sc)
clip = core.focus2.TemporalSoften2(clip)
```

### Python wrapper

The `TemporalSoften` class in `temporalsoften2.py` collapses the boilerplate above into a single
call and handles the RGB property-copy step automatically:

```python
from temporalsoften2 import TemporalSoften

core.std.LoadPlugin('/path/to/libscenechange.so')
core.std.LoadPlugin('/path/to/libtemporalsoften2.so')
clip = TemporalSoften(core).soften(clip, luma_threshold=4)
```

## Credits

Original plugins by Oka Motofumi (`chikuzen.mo at gmail dot com`).
