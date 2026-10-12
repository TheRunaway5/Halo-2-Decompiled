#!/usr/bin/env python3
"""Authenticate complete generic object-request and salted lookup owners."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as authority


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    authority.OWNERS = (0xb8820, 0xbbcb0)
    authority.BOUNDARIES = ()
    authority.run(args.output)


if __name__ == '__main__':
    main()
