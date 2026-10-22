#!/usr/bin/env python3
"""Create an owned, writable Dire Valley compiler overlay over a stock H2EK.

Run through tools/run_test.py. Only the new map's tags/data and shader collection
file are writable. The installed editing kit is a read-only asset reference.
"""
import argparse
import os
from pathlib import Path
import shutil


def stage(installed,package,output):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output=output.resolve()
    if not output.is_relative_to(scratch) or output.exists():
        raise ValueError('Use a fresh destination inside owned test scratch')
    output.mkdir()
    def overlay(src,dst,private):
        dst.mkdir()
        for child in src.iterdir():
            if child.name in private:overlay(child,dst/child.name,private[child.name])
            else:(dst/child.name).symlink_to(child.resolve(),target_is_directory=child.is_dir())
    overlay(installed/'tags',output/'tags',{'scenarios':{'multi':{},'shaders':{}}})
    (output/'tags/scenarios/multi/dire_valley').mkdir()
    collections=output/'tags/scenarios/shaders/shader_collections.shader_collections'
    text=collections.read_text();collections.unlink();collections.write_text(text)
    overlay(installed/'data',output/'data',{'scenarios':{'multi':{}}})
    shutil.copytree(package/'data/scenarios/multi/dire_valley',output/'data/scenarios/multi/dire_valley')
    for name in ['tool_fast.exe','libfbxsdk.dll']:(output/name).symlink_to((installed/name).resolve())


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['installed','package','output']:p.add_argument('--'+flag,required=True,type=Path)
    stage(**vars(p.parse_args()))
