"""Hatchling build hook that compiles the Meson project and packages the plugins."""

from __future__ import annotations

from pathlib import Path
from typing import Any
import os
import shutil
import subprocess as sp
import sys

from hatchling.builders.hooks.plugin.interface import BuildHookInterface
from packaging import tags

__all__ = ('CustomHook',)

PLUGIN_SUFFIXES = ('.dll', '.dylib', '.so')
"""Shared-library suffixes copied into the wheel.

:meta hide-value:
"""


class CustomHook(BuildHookInterface[Any]):
    """Compile the plugins with Meson and stage them for the wheel."""

    source_dir = Path('build-wheel')
    """Meson build directory, kept separate from a developer's own ``build``."""
    target_dir = Path('vapoursynth') / 'plugins'
    """Directory the plugins are staged in, mirroring their location in ``site-packages``."""

    def initialize(self, version: str, build_data: dict[str, Any]) -> None:  # ruff: ignore[unused-method-argument]
        """
        Build the plugins and copy them next to the VapourSynth package.

        VapourSynth recursively autoloads every native plugin under
        ``<site-packages>/vapoursynth/plugins``, so installing the wheel is all
        that is needed to make ``scd`` and ``focus2`` available.

        Parameters
        ----------
        version : str
            Build target version. Unused.
        build_data : dict[str, Any]
            Build metadata consumed by the wheel builder.
        """
        build_data['pure_python'] = False
        # The plugins are loaded by VapourSynth itself rather than by CPython, so
        # they depend only on the operating system and architecture. Tag the
        # wheel accordingly instead of inferring an interpreter-specific tag,
        # which would require one wheel per Python version.
        platform_tag = os.environ.get('SCENECHANGE_WHEEL_PLATFORM_TAG') or next(
            tags.platform_tags()
        )
        build_data['tag'] = f'py3-none-{platform_tag}'
        meson = (sys.executable, '-m', 'mesonbuild.mesonmain')
        setup = [*meson, 'setup', str(self.source_dir), '-Dtests=disabled']
        if (self.source_dir / 'meson-info').is_dir():
            # ``--vsenv`` is read-only after the first configure, so it may only
            # be passed when the build directory is created.
            setup.append('--reconfigure')
        else:
            # Activate the Visual Studio environment on Windows, where MSVC is
            # required. Ignored elsewhere.
            setup.append('--vsenv')
        sp.run(setup, check=True)
        sp.run([*meson, 'compile', '-C', str(self.source_dir)], check=True)
        self.target_dir.mkdir(parents=True, exist_ok=True)
        for path in (self.source_dir / 'native').glob('*'):
            if path.is_file() and path.suffix in PLUGIN_SUFFIXES:
                shutil.copy2(path, self.target_dir)

    def finalize(self, version: str, build_data: dict[str, Any], artifact_path: str) -> None:  # ruff: ignore[unused-method-argument]
        """
        Remove the staged plugin tree once the wheel has been written.

        Parameters
        ----------
        version : str
            Build target version. Unused.
        build_data : dict[str, Any]
            Build metadata. Unused.
        artifact_path : str
            Path of the built wheel. Unused.
        """
        shutil.rmtree(self.target_dir.parent, ignore_errors=True)
