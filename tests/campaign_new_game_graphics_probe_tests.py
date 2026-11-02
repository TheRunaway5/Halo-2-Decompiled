#!/usr/bin/env python3
"""Synthetic log/report tests only; no Vulkan device or engine run is qualified."""
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import test_campaign_new_game_graphics as graphics
from test_campaign_new_game_runtime import INTRODUCTION,ARMORY


class GraphicsProbeTests(unittest.TestCase):
    def long_log(self):
        return (f'Vulkan device: Synthetic parser GPU\n'
                f'Campaign scenario started: {INTRODUCTION} difficulty=1 tick=0 bsp=0 actors=0 scripts=4 script_faults=0 level_finished=0\n'
                'Rendered 2 frames\n'
                +'renderer diagnostic padding\n'*2100
                +'tick=6432 bsp=0 actors=0 scripts=3 script_faults=0 level_finished=1\n'
                +f'Campaign next scenario: {ARMORY} difficulty=1\n'
                +f'Campaign scenario started: {ARMORY} difficulty=1 tick=0 bsp=0 actors=0 scripts=9 script_faults=0 level_finished=0\n'
                +'tick=3 bsp=0 actors=0 scripts=9 script_faults=0 level_finished=0\n'
                +'Rendered 4 frames\n')

    def observe(self,text,case,acceptance='graphics_startup',returncode=0):
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])/self._testMethodName/case
        scratch.mkdir(parents=True)
        runtime=scratch/'runtime';runtime.write_bytes(b'synthetic parser-only runtime input\n')
        cache=scratch/'mcc';maps=cache/'maps';maps.mkdir(parents=True)
        intro=maps/'00a_introduction.map';intro.write_bytes(b'synthetic parser-only map input\n')
        output=scratch/'report.json'
        def run(invocation,**kwargs):
            kwargs['stdout'].write(text)
            trace=Path(invocation[invocation.index('-o')+1])
            trace.write_text(f'openat(AT_FDCWD, "{intro}", O_RDONLY) = 3\n')
            return subprocess.CompletedProcess(invocation,returncode)
        args=['graphics-probe','--runtime',str(runtime),'--mcc-root',str(cache),
              '--frames','6','--timeout','1','--acceptance',acceptance,'--report-output',str(output)]
        printed=io.StringIO()
        with patch.dict(os.environ,{'HALO2_TEST_SCRATCH':str(scratch),'DISPLAY':':parser-test'}),\
             patch.object(sys,'argv',args),\
             patch.object(graphics.shutil,'which',return_value='/parser-only/strace'),\
             patch.object(graphics,'runtime_cache',return_value=(maps,[])),\
             patch.object(graphics,'runtime_map',return_value=intro),\
             patch.object(graphics.subprocess,'run',side_effect=run),\
             contextlib.redirect_stdout(printed):
            status=graphics.main()
        return status,json.loads(output.read_text()),json.loads(printed.getvalue())

    def test_full_log_observations_survive_bounded_diagnostic_tail(self):
        text=self.long_log()
        self.assertGreater(len(text.encode()),56101)
        self.assertNotIn('Vulkan device:',text[-32768:])
        self.assertNotIn('Rendered 2 frames',text[-32768:])
        for mode in ('graphics_startup','opening_handoff'):
            with self.subTest(acceptance=mode):
                status,report,printed=self.observe(text,mode,mode)
                self.assertEqual(report['vulkan_devices'],['Synthetic parser GPU'])
                self.assertEqual(report['rendered_frames'],[2,4])
                self.assertEqual(status,0);self.assertTrue(report['passed'])
                self.assertEqual(report['log_bytes'],len(text.encode()))
                self.assertEqual(report['log_sha256'],hashlib.sha256(text.encode()).hexdigest())
                self.assertEqual(report['log_tail'],text[-32768:])
                self.assertEqual(printed['log_tail'],text[-2200:])
                self.assertFalse(report['native_only_engine_qualified'])
                self.assertFalse(report['framebuffer_parity_qualified'])

    def test_missing_marker_wrong_frame_total_and_actual_failure_still_reject(self):
        text=self.long_log()
        controls=[('device',text.replace('Vulkan device: Synthetic parser GPU\n',''),0),
                  ('frames',text.replace('Rendered 2 frames\n',''),0),
                  ('process',text,1),
                  ('fault','Script fault: synthetic unsupported owner\n'+text,0)]
        for name,changed,returncode in controls:
            with self.subTest(control=name):
                status,report,_=self.observe(changed,name,returncode=returncode)
                self.assertEqual(status,1);self.assertFalse(report['passed'])

    def test_line_parser_preserves_order_duplicates_and_exact_marker_shape(self):
        text=('Vulkan device: first\r\nRendered 2 frames\r\n'
              ' Vulkan device: indented\nlog: Vulkan device: prefixed\n'
              'Rendered -1 frames\nRendered 2.5 frames\nRendered 7 frames trailing\n'
              ' Rendered 8 frames\nrendered 9 frames\n'
              'Vulkan device: first\nVulkan device: second\nRendered 4 frames')
        self.assertEqual(graphics.parse_graphics_lines(io.StringIO(text)),
                         dict(vulkan_devices=['first','first','second'],rendered_frames=[2,4]))
        self.assertEqual(graphics.parse_graphics_lines([]),dict(vulkan_devices=[],rendered_frames=[]))


if __name__=='__main__':
    unittest.main()
