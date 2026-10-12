#!/usr/bin/env python3
"""Pair complete original object/body velocity callers and activity owners."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as source

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    source.OWNERS=(0x1c8430,0x1d5600,0x1d5650,0x1d45f0,0x1d47c0,0x1d4840,0x3121a0,0x3121c0)
    source.BOUNDARIES=(0x312a90,0x3131c0)
    source.run(a.output.resolve())

if __name__=='__main__':main()
