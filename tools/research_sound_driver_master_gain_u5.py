#!/usr/bin/env python3
"""Authenticate complete master-output gain CPU owner and original CRT helper."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as authority


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    authority.OWNERS=(0x2230e0,0x32909c)
    authority.BOUNDARIES=()
    authority.run(args.output)


if __name__=='__main__':
    main()
