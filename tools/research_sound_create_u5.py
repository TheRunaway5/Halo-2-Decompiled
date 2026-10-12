#!/usr/bin/env python3
"""Pair complete source factory, its CPU helpers and exact unresolved owners."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as a
OWNERS=(0x127750,0x1269b0,0x126930,0x126a40,0x129570,0x2247c0,0x21ae80,0x21aef0,0x21b700,0x21b900,0x128760,0x223920,0x223960,0x2239b0,0x21b020,0x21b060,0x21b180,0x21b4e0,0x21b420,0x21b3c0,0x21b5d0,0x21b480,0x21b220,0x21b670,0x129e00,0x1280b0,0x1275b0,0x128030,0x16d2c0,0x1479b0,0x32909c)
BOUNDARIES=(0x21a960,0x128380,0x129450,0x18eac0,0x18eba0,0x21bb40,0x21ba00,0x21c070)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);q=p.parse_args();a.OWNERS=OWNERS;a.BOUNDARIES=BOUNDARIES;a.run(q.output)
if __name__=='__main__':main()
