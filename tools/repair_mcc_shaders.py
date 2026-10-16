"""Prepare retained port materials in the selected MCC cache before preview."""
import argparse
from pathlib import Path
import sys

from mcc_onboarding import repair_shaders


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    arguments = parser.parse_args()
    try:
        repair_shaders(arguments.root)
    except (OSError, ValueError, KeyError) as error:
        print('Native material preparation: ' + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
