#!/usr/bin/env python3
"""Freeze test sources newly registered by the joined qualified CMake modules."""
import argparse
import json
import os
from pathlib import Path
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
NAMES=('tests/cloth_shader_tu15_tests.cpp','tests/objective_goal_renderer_tests.cpp',
       'tests/native_lens_presentation_tu15_tests.cpp')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source';prepared=json.loads(args.prepared.read_text());pins=dict(prepared['source_hashes'])
    if Path(prepared['owner'])!=owner or any(sha(source/name)!=pin for name,pin in pins.items()):
        raise ValueError('Use the same unchanged joined managed source')
    replacements={}
    for name in NAMES:
        if (source/name).exists() or name in pins:raise ValueError('Do not replace an existing test input')
        path=ROOT/name;pin=sha(path);raw=path.read_bytes()
        if sha(path)!=pin:raise ValueError('A newly registered test input changed')
        replacements[name]=(raw,pin)
    args.output.mkdir(parents=True,exist_ok=False)
    for name,(raw,pin) in replacements.items():
        (source/name).write_bytes(raw);pins[name]=pin
        if sha(source/name)!=pin:raise ValueError('Frozen test input differs')
    report=dict(prepared,source_hashes=pins,previous_prepared_sha256=sha(args.prepared),
        private_test_dependencies=[dict(path=name,sha256=pin)for name,(_,pin)in replacements.items()],
        production_bodies_changed=False,tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',test_inputs=len(replacements))),flush=True)


if __name__=='__main__':main()
