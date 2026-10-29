"""Capture-policy fixtures; synthetic pixels do not qualify a GPU/runtime."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import qualify_frontend_runtime as qualification


def witness(**changes):
    row = dict(schema='H2_FRONTEND_INTRO_CLEAR_CAPTURE_V1', clear_observed=True,
        warmup_frames=137, qualified_frames=4, first_clear_ui_clock_ms=4197,
        last_clear_ui_clock_ms=4251, waited_ms=4205, wait_budget_ms=30000)
    row.update(changes)
    return qualification.INTRO_CAPTURE_PREFIX + json.dumps(row) + '\n'


def frame(path, pixels=None):
    path.write_bytes(b'P6\n2 2\n255\n' + (pixels if pixels is not None else
        bytes((0, 0, 0, 240, 240, 240, 40, 100, 160, 20, 30, 40))))


class CaptureWitnessTests(unittest.TestCase):
    def test_only_complete_native_clear_witness_is_accepted(self):
        row = qualification.intro_capture_witness(witness(), 4)
        self.assertEqual(row['warmup_frames'], 137)
        for text in ('Rendered 4 frontend frames\n', witness() + witness(),
                     qualification.INTRO_CAPTURE_PREFIX + '{\n',
                     qualification.INTRO_CAPTURE_PREFIX + '[]\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                qualification.intro_capture_witness(text, 4)

    def test_missing_clear_wrong_frames_and_unbounded_times_reject(self):
        for change in (dict(clear_observed=False), dict(qualified_frames=3),
                       dict(qualified_frames=True), dict(warmup_frames=-1),
                       dict(first_clear_ui_clock_ms=-1), dict(last_clear_ui_clock_ms=2**32),
                       dict(waited_ms=float('nan')), dict(waited_ms=30000), dict(waited_ms=30001),
                       dict(waited_ms=True), dict(wait_budget_ms=0)):
            with self.subTest(change=change), self.assertRaises(ValueError):
                qualification.intro_capture_witness(witness(**change), 4)

    def test_steady_wait_is_not_inferred_from_absolute_source_clock(self):
        row = qualification.intro_capture_witness(witness(first_clear_ui_clock_ms=2**32 - 8,
            last_clear_ui_clock_ms=24, waited_ms=0, warmup_frames=0), 4)
        self.assertEqual(row['last_clear_ui_clock_ms'], 24)

    def test_black_uniform_and_malformed_images_reject(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as owner:
            image = Path(owner) / 'frame.ppm'
            for data in (bytes(12), bytes((255, 0, 0)) * 4):
                frame(image, data)
                with self.assertRaisesRegex(ValueError, 'black or uniform'):
                    qualification.visible_menu_capture(image)
            image.write_bytes(b'not a PPM')
            with self.assertRaises((ValueError, OSError)):
                qualification.visible_menu_capture(image)
            frame(image)
            self.assertEqual(qualification.visible_menu_capture(image)['size'], [2, 2])


class QualificationTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='frontend-qualification-',
            dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        self.runtime = self.root / 'bin/halo2_native'
        self.cache = self.root / 'assets/imported/mcc-fixture'
        self.output = self.root / 'reports'
        self.scratch = self.root / 'capture'
        self.scratch.mkdir()
        self.runtime.parent.mkdir()
        self.runtime.write_bytes(b'capture command fixture; not an executable')
        for name in ('halo2/h2_maps_win64_dx11/mainmenu.map',
                     'halo2/h2_maps_win64_dx11/shared.map',
                     'halo2/h2_maps_win64_dx11/textures.dat',
                     'halo2/h2_maps_win64_dx11/sounds_neutral.dat',
                     'halo2/h2_fonts/font_table.txt', 'presentation/loading-v3/manifest.json',
                     'presentation/loading-v3/glow-mask.dds', 'presentation/loading-v3/loading.wav'):
            path = self.cache / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'owned fixture input ' + name.encode())
        self.commands = []
        self.menu_log = witness()
        self.menu_pixels = None
        self.timeout = False
        self.loading_log = 'MCC V3 loading: genuine PCM and reconstructed artwork admitted\n'
        self.sidecar = self.runtime.with_name(self.runtime.name + '.frontend-test.json')

    def run_capture(self, command, **kwargs):
        self.commands.append(command)
        menu = '--menu-wait-intro-clear' in command
        if menu and self.timeout:
            raise subprocess.TimeoutExpired(command, 180, output=b'capture did not finish\n')
        target = Path(command[command.index('--screenshot') + 1])
        frame(target, self.menu_pixels if menu else None)
        output = ('Frontend movie: fixture frame\n' if not menu else self.menu_log)
        return subprocess.CompletedProcess(command, 0, output + self.loading_log + 'Rendered 4 frontend frames\n')

    def qualify(self):
        with patch.object(qualification.subprocess, 'run', side_effect=self.run_capture), \
             patch('native_renderer_shaders.shader_capability', return_value=dict(files={})), \
             patch.object(qualification, '_readmit_frontend_owner') as readmit:
            result = qualification.qualify(self.root, self.runtime, self.cache,
                                           self.output, self.scratch)
        readmit.assert_called_once()
        return result

    def test_menus_wait_for_actual_clear_before_four_visible_frames(self):
        self.qualify()
        self.assertEqual(len(self.commands), 4)
        self.assertNotIn('--menu-wait-intro-clear', self.commands[0])
        for command in self.commands[1:]:
            self.assertIn('--skip-intro', command)
            self.assertIn('--menu-wait-intro-clear', command)
            self.assertNotIn('--menu-ui-ms', command)
            self.assertEqual(command[command.index('--frames') + 1], '4')
        report = json.loads((self.output / 'report.json').read_bytes())
        self.assertTrue(report['passed'])
        for case in report['cases'][1:]:
            self.assertEqual(case['intro_clear']['qualified_frames'], 4)
            self.assertEqual(case['visible_frame']['size'], [2, 2])
        capability = json.loads(self.sidecar.read_bytes())
        self.assertEqual(capability['validation']['frontend_capture_policy'], 'source-map-intro-clear-v1')

    def test_early_frame_log_without_clear_witness_cannot_publish(self):
        self.menu_log = ''
        with self.assertRaisesRegex(RuntimeError, 'title'):
            self.qualify()
        self.assertFalse(self.sidecar.exists())
        report = json.loads((self.output / 'report.json').read_bytes())
        self.assertFalse(report['passed'])
        self.assertIn('witness', report['cases'][-1]['error'])

    def test_wrong_loading_bank_cannot_publish_a_frontend_capability(self):
        self.loading_log = 'An unrelated loading track was selected\n'
        with self.assertRaisesRegex(RuntimeError, 'intro'):
            self.qualify()
        self.assertFalse(self.sidecar.exists())
        self.assertIn('loading presentation', json.loads(
            (self.output / 'report.json').read_bytes())['cases'][-1]['error'])

    def test_clear_witness_cannot_hide_black_frame(self):
        self.menu_pixels = bytes(12)
        with self.assertRaisesRegex(RuntimeError, 'title'):
            self.qualify()
        self.assertFalse(self.sidecar.exists())
        self.assertIn('black or uniform', json.loads(
            (self.output / 'report.json').read_bytes())['cases'][-1]['error'])

    def test_process_timeout_preserves_failure_report_and_no_capability(self):
        self.timeout = True
        with self.assertRaisesRegex(RuntimeError, 'title'):
            self.qualify()
        report = json.loads((self.output / 'report.json').read_bytes())
        self.assertFalse(report['cases'][-1]['passed'])
        self.assertIsNone(report['cases'][-1]['exit_code'])
        self.assertIn('180-second', report['cases'][-1]['error'])
        self.assertFalse(self.sidecar.exists())


if __name__ == '__main__':
    unittest.main()
