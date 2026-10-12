#!/usr/bin/env python3
"""Pair complete component allocator, initializer and raw datum allocator."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as source

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    source.OWNERS=(0x16d2c0,0x1d2aa0,0x1d3340);source.BOUNDARIES=()
    source.run(a.output.resolve())

if __name__=='__main__':main()
