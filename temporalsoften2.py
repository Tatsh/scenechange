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
from typing import Any

import vapoursynth as vs  # type: ignore[import-untyped]

__all__ = ('TemporalSoften',)
__version__ = 'v0.2.0'


class TemporalSoften:
    def __init__(self, core: vs.Core) -> None:
        self.modframe = core.std.ModifyFrame
        self.resize = core.resize.Point
        self.detect = core.scd.Detect
        self.tsoften = core.focus2.TemporalSoften2

    def set_props(self, n: int, f: Sequence[vs.VideoFrame]) -> vs.VideoFrame:
        fout = f[0].copy()
        fout.props._SceneChangePrev = f[1].props._SceneChangePrev
        fout.props._SceneChangeNext = f[1].props._SceneChangeNext
        return fout

    def set_scenechange(self, clip: vs.VideoNode, threshold: int = 15,
                        log: Any = None) -> vs.VideoNode:
        sc = clip
        cf = clip.format.color_family
        if cf == vs.RGB:
            sc = self.resize(format=vs.GRAY8)
        sc = self.detect(sc, threshold)
        if cf == vs.RGB:
            sc = self.modframe([clip, sc], self.set_props)
        return sc

    def soften(self, clip: vs.VideoNode, radius: int = 4, luma_threshold: int = 4,
               chroma_threshold: int = 8, scenechange: int = 15, mode: int | None = None,
               log: Any = None) -> vs.VideoNode:
        if scenechange:
            clip = self.set_scenechange(clip, scenechange)
        return self.tsoften(clip, radius, luma_threshold, chroma_threshold,
                            scenechange)
