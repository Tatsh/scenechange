"""
TemporalSoften2 wrapper module.

Place this Python script into ``Python3.x/Lib/site-packages`` as ``temporalsoften2.py``.

Usage:

    from temporalsoften2 import TemporalSoften
    core.std.LoadPlugin('/path/to/scenechange.dll')
    core.std.LoadPlugin('/path/to/temporalsoften2.dll')
    clip = something
    clip = TemporalSoften(core).soften(clip, luma_threshold=4, ...)
"""
from __future__ import annotations

from collections.abc import Sequence

import vapoursynth as vs  # type: ignore[import-untyped]

__all__ = ('TemporalSoften',)
__version__ = 'v0.2.0'


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
        plugins already loaded.
    """
    def __init__(self, core: vs.Core) -> None:
        self.modframe = core.std.ModifyFrame
        """``core.std.ModifyFrame`` bound for the RGB property-copy path."""
        self.resize = core.resize.Point
        """``core.resize.Point`` bound for RGB-to-GRAY8 conversion."""
        self.detect = core.scd.Detect
        """``core.scd.Detect`` bound for scene change detection."""
        self.tsoften = core.focus2.TemporalSoften2
        """``core.focus2.TemporalSoften2`` bound for temporal soften processing."""

    def set_props(self, n: int, f: Sequence[vs.VideoFrame]) -> vs.VideoFrame:
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
        fout.props._SceneChangePrev = f[1].props._SceneChangePrev
        fout.props._SceneChangeNext = f[1].props._SceneChangeNext
        return fout

    def set_scenechange(self,
                        clip: vs.VideoNode,
                        threshold: int = 15,
                        log: str | None = None) -> vs.VideoNode:
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

    def soften(self,
               clip: vs.VideoNode,
               radius: int = 4,
               luma_threshold: int = 4,
               chroma_threshold: int = 8,
               scenechange: int = 15,
               mode: int | None = None,
               log: str | None = None) -> vs.VideoNode:
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
