#!/usr/bin/env python3
"""Reject a compositor-clamped gameplay workload rather than labeling it4K."""
import importlib.util
import os
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
module=Path(os.environ.get('HALO2_BENCHMARK_MODULE',ROOT/'tools/benchmark_framerate.py'))
spec=importlib.util.spec_from_file_location('benchmark',module)
benchmark=importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)

RUNTIME='''#!/usr/bin/env python3
import csv,os,sys
width,height=map(int,os.environ['HALO2_TEST_RENDER_SIZE'].split(','))
with open(os.environ['HALO2_RENDER_TIMINGS'],'w') as stream:
    writer=csv.DictWriter(stream,delimiter='\\t',fieldnames=['interval_ms','render_width','render_height','present_width','present_height'])
    writer.writeheader()
    for _ in range(int(sys.argv[sys.argv.index('--frames')+1])):
        writer.writerow(dict(interval_ms=5,render_width=width,render_height=height,present_width=1280,present_height=720))
'''

class Dimensions(unittest.TestCase):
    def setUp(self):
        self.folder=tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.folder.cleanup)
        directory=Path(self.folder.name)
        runtime=directory/'runtime';runtime.write_text(RUNTIME);runtime.chmod(0o700)
        self.args=SimpleNamespace(runtime=runtime,output=directory,width=1280,height=720,
            render_size=None,frames=6,warmup=1,timeout=10,max_frame_ms=8.333333,
            map=directory/'actual.map',shared_maps=None,starting_weapon=0,local_players=1,
            ui_assets=None,gameplay_xbe=None,profile_multiplayer=False,audio=False,hidden=False)

    def measure(self,size):
        with patch.dict(os.environ,HALO2_TEST_RENDER_SIZE=size):
            return benchmark.measure(self.args,'match',0)

    def test_clamped_window_cannot_pass_as_requested_4k(self):
        self.args.width,self.args.height=3840,2160
        with self.assertRaisesRegex(RuntimeError,'internal render size differs'):
            self.measure('2544,1002')

    def test_internal_4k_uses_small_presentation_window(self):
        self.args.render_size=[3840,2160]
        result=self.measure('3840,2160')
        self.assertEqual(result['dimensions'],{'render':(3840,2160),'present':(1280,720)})
        command=result['command'];index=command.index('--render-size')
        self.assertEqual(command[index+1:index+3],['3840','2160'])
        self.assertEqual(result['frames'],5)
        self.assertTrue(result['frame_budget']['passed'])

    def test_explicit_internal_size_is_also_checked(self):
        self.args.render_size=[3840,2160]
        with self.assertRaisesRegex(RuntimeError,'internal render size differs'):
            self.measure('1280,720')

if __name__=='__main__':
    unittest.main()
