"""Scene-change-aware temporal soften wrapper for VapourSynth."""

from __future__ import annotations

from importlib import resources
from typing import TYPE_CHECKING, Any, cast
import logging

import vapoursynth as vs  # type: ignore[import-untyped]

if TYPE_CHECKING:
    from collections.abc import Sequence

__all__ = ('TemporalSoften',)

log = logging.getLogger(__name__)

_PLUGIN_STEMS = ('scenechange', 'temporalsoften2')
"""Base names of the bundled VapourSynth plugin libraries."""
_PLUGIN_SUFFIXES = ('.so', '.dylib', '.dll')
"""Shared-library suffixes searched for across the supported platforms."""


class TemporalSoften:
    """
    Scene-change-aware wrapper around ``focus2.TemporalSoften2``.

    Binds to an active VapourSynth core and exposes a single :py:meth:`soften`
    call that performs scene detection (via ``scd.Detect``) and temporal
    averaging (via ``focus2.TemporalSoften2``). RGB clips are handled
    automatically by routing detection through a temporary GRAY8 copy and
    copying the ``_SceneChangePrev`` and ``_SceneChangeNext`` frame properties
    back onto the original colour frames.

    Parameters
    ----------
    core : vapoursynth.Core
        VapourSynth core with the ``scenechange`` and ``temporalsoften2``
        plugins already loaded. Call :py:meth:`load_plugins` first to load the
        plugins bundled with this package.
    """

    def __init__(self, core: vs.Core) -> None:
        # VapourSynth plugin namespaces are registered dynamically and are not
        # visible to static type checkers, so they are accessed through Any.
        plugins = cast('Any', core)
        self.modframe = plugins.std.ModifyFrame
        """``core.std.ModifyFrame`` bound for the RGB property-copy path."""
        self.resize = plugins.resize.Point
        """``core.resize.Point`` bound for RGB-to-GRAY8 conversion."""
        self.detect = plugins.scd.Detect
        """``core.scd.Detect`` bound for scene change detection."""
        self.tsoften = plugins.focus2.TemporalSoften2
        """``core.focus2.TemporalSoften2`` bound for temporal soften processing."""

    @staticmethod
    def load_plugins(core: vs.Core) -> None:
        """
        Load the bundled ``scenechange`` and ``temporalsoften2`` plugins.

        The compiled VapourSynth plugins are shipped inside this package (for
        example ``site-packages/scenechange/libscenechange.so``). Their location
        is resolved with :py:mod:`importlib.resources`, so this works regardless
        of how the package was installed. Plugins that are already registered on
        the core are skipped, so repeated calls are harmless.

        Parameters
        ----------
        core : vapoursynth.Core
            Core to load the plugins into.
        """
        for entry in resources.files(__package__ or 'scenechange').iterdir():
            name = entry.name
            if not name.endswith(_PLUGIN_SUFFIXES):
                continue
            stem = name.removeprefix('lib')
            if stem.split('.', 1)[0] not in _PLUGIN_STEMS:
                continue
            with resources.as_file(entry) as path:
                try:
                    cast('Any', core).std.LoadPlugin(str(path))
                except vs.Error as e:
                    # The plugin namespace is already registered; nothing to do.
                    log.debug('Skipping already-loaded plugin `%s`: %s.', name, e)

    @staticmethod
    def set_props(n: int, f: Sequence[vs.VideoFrame]) -> vs.VideoFrame:  # ruff:ignore[unused-static-method-argument]
        """
        Copy scene change properties from a reference frame onto a colour frame.

        Used as the ``ModifyFrame`` callback when scene detection is performed
        on a GRAY8 copy of an RGB clip.

        Parameters
        ----------
        n : int
            Frame number. Unused; supplied by VapourSynth.
        f : Sequence[vapoursynth.VideoFrame]
            Two-element sequence ``[colour_frame, scene_detected_frame]``.

        Returns
        -------
        vapoursynth.VideoFrame
            Copy of the colour frame with ``_SceneChangePrev`` and
            ``_SceneChangeNext`` properties carried over from the reference.
        """
        fout = f[0].copy()
        fout.props['_SceneChangePrev'] = f[1].props['_SceneChangePrev']
        fout.props['_SceneChangeNext'] = f[1].props['_SceneChangeNext']
        return fout

    def set_scenechange(
        self,
        clip: vs.VideoNode,
        threshold: int = 15,
        log: str | None = None,  # ruff:ignore[unused-method-argument]
    ) -> vs.VideoNode:
        """
        Run scene change detection on a clip and attach the result properties.

        For RGB clips, detection is performed on a GRAY8 copy and the resulting
        ``_SceneChangePrev`` and ``_SceneChangeNext`` properties are merged back
        onto the original frames via ``ModifyFrame``.

        Parameters
        ----------
        clip : vapoursynth.VideoNode
            Input clip in any colour family.
        threshold : int
            Detection threshold passed to ``scd.Detect``. Defaults to ``15``.
        log : str | None
            Path to a log file. Reserved for future use. Currently ignored.

        Returns
        -------
        vapoursynth.VideoNode
            Clip with ``_SceneChangePrev`` and ``_SceneChangeNext`` properties
            attached.
        """
        sc = clip
        cf = clip.format.color_family
        if cf == vs.RGB:
            sc = self.resize(format=vs.GRAY8)
        sc = self.detect(sc, threshold)
        if cf == vs.RGB:
            sc = self.modframe([clip, sc], self.set_props)
        return sc

    def soften(
        self,
        clip: vs.VideoNode,
        radius: int = 4,
        luma_threshold: int = 4,
        chroma_threshold: int = 8,
        scenechange: int = 15,
        mode: int | None = None,  # ruff:ignore[unused-method-argument]
        log: str | None = None,  # ruff:ignore[unused-method-argument]
    ) -> vs.VideoNode:
        """
        Apply temporal soften, optionally preceded by scene change detection.

        Parameters
        ----------
        clip : vapoursynth.VideoNode
            Input clip.
        radius : int
            Temporal radius forwarded to ``focus2.TemporalSoften2``. Range 1 to
            7. Defaults to ``4``.
        luma_threshold : int
            Luma threshold (0 to 255). Applied to all planes for RGB clips.
            Defaults to ``4``.
        chroma_threshold : int
            Chroma threshold (0 to 255). Ignored for RGB and Gray clips.
            Defaults to ``8``.
        scenechange : int
            Detection threshold passed to :py:meth:`set_scenechange`. When
            ``0``, the detection pass is skipped and any ``_SceneChange*``
            properties already on the clip are honoured directly by the soften
            filter. Defaults to ``15``.
        mode : int | None
            Mode parameter forwarded to ``focus2.TemporalSoften2``. Defaults to
            ``None``.
        log : str | None
            Path to a log file. Reserved for future use. Currently ignored.

        Returns
        -------
        vapoursynth.VideoNode
            Temporally softened clip.
        """
        if scenechange:
            clip = self.set_scenechange(clip, scenechange)
        return self.tsoften(clip, radius, luma_threshold, chroma_threshold, scenechange)
