#!/usr/bin/env python3
"""Require the staged handoff-aware generator to reproduce its qualified C++ exactly."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();source=args.source.resolve();output=args.output.resolve()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();output.mkdir(parents=True,exist_ok=False)
    generator=source/'tools/generate_phantom_force_tu15_20261011.py'
    template=source/'tools/native_lift_handoff_template_tu15.py'
    expected=source/'src/physics/physics_phantom_force_tu15_generated.cpp'
    research=ROOT/'analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011/research-verified/report.json'
    paths=(generator,template,expected,research,Path(__file__))
    pins={str(p):sha(p)for p in paths}
    report=dict(passed=False,scope=__doc__,source_sha256=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        actual=scratch/'force.cpp';manifest=scratch/'generated.json'
        command=[sys.executable,str(generator),'--research',str(research),'--output',str(actual),'--manifest',str(manifest)]
        environment=dict(os.environ,PYTHONPATH=str(ROOT/'tools'))
        p=subprocess.run(command,capture_output=True,text=True,env=environment)
        (output/'generator.log').write_text(p.stdout+p.stderr)
        assert p.returncode==0,p.stderr[-5000:]
        report.update(command=command,generated_sha256=sha(actual),expected_sha256=sha(expected))
        if actual.read_bytes()!=expected.read_bytes():
            diff=''.join(difflib.unified_diff(expected.read_text().splitlines(keepends=True),actual.read_text().splitlines(keepends=True),fromfile='qualified.cpp',tofile='regenerated.cpp'))
            (output/'difference.diff').write_text(diff[:64000])
            raise RuntimeError('Regenerated native handoffs differ; inspect difference.diff')
        report['generated_manifest']=json.loads(manifest.read_text())
        # Retain the full manifest only under the disposable owner; its complete
        # instruction map is existing research evidence, not a new artifact.
        report['generated_manifest_sha256']=sha(manifest)
        report.pop('generated_manifest')
        assert all(sha(Path(name))==value for name,value in pins.items())
        report['passed']=True
    except BaseException as error:
        report['error']=str(error)or repr(error);raise
    finally:
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
