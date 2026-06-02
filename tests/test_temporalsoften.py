from __future__ import annotations

from contextlib import contextmanager
from pathlib import Path
from typing import TYPE_CHECKING

from scenechange import TemporalSoften
import vapoursynth as vs  # type: ignore[import-untyped]

if TYPE_CHECKING:
    from collections.abc import Iterator

    from pytest_mock import MockerFixture


@contextmanager
def _identity_as_file(entry: Path) -> Iterator[Path]:
    yield entry


def test_init_binds_plugin_namespaces(mocker: MockerFixture) -> None:
    core = mocker.MagicMock()
    ts = TemporalSoften(core)
    assert ts.modframe is core.std.ModifyFrame
    assert ts.resize is core.resize.Point
    assert ts.detect is core.scd.Detect
    assert ts.tsoften is core.focus2.TemporalSoften2


def test_load_plugins_loads_each_bundled_plugin(mocker: MockerFixture) -> None:
    files = mocker.Mock()
    files.iterdir.return_value = [
        Path('libscenechange.so'),
        Path('libtemporalsoften2.so'),
        Path('notes.txt'),
    ]
    mocker.patch('scenechange.temporalsoften.resources.files', return_value=files)
    mocker.patch('scenechange.temporalsoften.resources.as_file', _identity_as_file)
    core = mocker.MagicMock()
    TemporalSoften.load_plugins(core)
    loaded = [call.args[0] for call in core.std.LoadPlugin.call_args_list]
    assert loaded == ['libscenechange.so', 'libtemporalsoften2.so']


def test_load_plugins_ignores_already_loaded(mocker: MockerFixture) -> None:
    files = mocker.Mock()
    files.iterdir.return_value = [Path('libscenechange.so')]
    mocker.patch('scenechange.temporalsoften.resources.files', return_value=files)
    mocker.patch('scenechange.temporalsoften.resources.as_file', _identity_as_file)
    core = mocker.MagicMock()
    core.std.LoadPlugin.side_effect = vs.Error('already loaded')
    TemporalSoften.load_plugins(core)


def test_set_props_returns_copy_with_scene_change_props(mocker: MockerFixture) -> None:
    colour = mocker.MagicMock()
    detected = mocker.MagicMock()
    result = TemporalSoften.set_props(0, [colour, detected])
    assert result is colour.copy.return_value
    detected.props.__getitem__.assert_any_call('_SceneChangePrev')
    detected.props.__getitem__.assert_any_call('_SceneChangeNext')


def test_set_scenechange_detects_directly_for_non_rgb(mocker: MockerFixture) -> None:
    ts = TemporalSoften(mocker.MagicMock())
    clip = mocker.Mock()
    result = ts.set_scenechange(clip, 20)
    ts.detect.assert_called_once_with(clip, 20)
    ts.modframe.assert_not_called()
    assert result is ts.detect.return_value


def test_set_scenechange_copies_props_back_for_rgb(mocker: MockerFixture) -> None:
    ts = TemporalSoften(mocker.MagicMock())
    clip = mocker.Mock()
    clip.format.color_family = vs.RGB
    ts.set_scenechange(clip, 20)
    ts.resize.assert_called_once()
    ts.detect.assert_called_once()
    ts.modframe.assert_called_once()


def test_soften_runs_detection_when_scenechange_nonzero(mocker: MockerFixture) -> None:
    ts = TemporalSoften(mocker.MagicMock())
    set_scenechange = mocker.patch.object(ts, 'set_scenechange')
    clip = mocker.Mock()
    ts.soften(clip, scenechange=15)
    set_scenechange.assert_called_once_with(clip, 15)
    assert ts.tsoften.call_args.args[0] is set_scenechange.return_value


def test_soften_skips_detection_when_scenechange_zero(mocker: MockerFixture) -> None:
    ts = TemporalSoften(mocker.MagicMock())
    set_scenechange = mocker.patch.object(ts, 'set_scenechange')
    ts.soften(mocker.Mock(), scenechange=0)
    set_scenechange.assert_not_called()
    ts.tsoften.assert_called_once()
