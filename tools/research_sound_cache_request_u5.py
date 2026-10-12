#!/usr/bin/env python3
"""Pair whole sound-cache request/page CPU owners, without service results."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as a
OWNERS=(0x21a960,0x21ab20,0x13eae0,0x13ea90,0x13efa0,0x16d2c0,0x16d390,0x16d480,0x16d4c0,0x21aab0,0x21aaf0)
BOUNDARIES=(0x19d8c0,0x1a4870,0x121fd0,0x122020,0x1220e0,0x122150,0x122360,0x1a47d0)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);q=p.parse_args();a.OWNERS=OWNERS;a.BOUNDARIES=BOUNDARIES;a.run(q.output)
if __name__=='__main__':main()
