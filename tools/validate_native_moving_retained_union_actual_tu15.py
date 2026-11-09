#!/usr/bin/env python3
"""Replay the observed moving helper with the final bounded-span header."""
from pathlib import Path
import inspect
import validate_native_moving_shape_observed_actual_tu15 as parent

ROOT = Path(__file__).resolve().parents[1]
STAGE = ROOT / 'analysis/physics/native-moving-shape-retained-world-stage-v2-20261102/staged'


def main():
    source = inspect.getsource(parent.main)
    marker = ' source=inspect.getsource(parent.main)'
    assert source.count(marker) == 1
    source = source.replace(marker,
        " overrides['include/halo2/native_tu15_contact_candidate_private_data.hpp']=STAGE/'include/halo2/native_tu15_contact_candidate_private_data.hpp'\n" + marker)
    environment = dict(parent.__dict__, STAGE=STAGE, __file__=__file__)
    exec(source, environment)
    return environment['main']()


if __name__ == '__main__':
    raise SystemExit(main())
