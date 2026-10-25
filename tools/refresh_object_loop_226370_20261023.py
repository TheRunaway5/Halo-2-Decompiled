#!/usr/bin/env python3
"""Admit the one recorded original-callee repair into the active owned build."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    out=BASE/'repair-followup-20261023';manifest=out/'source-hashes.json';before=manifest.read_bytes();pins=json.loads(before)
    assert source_hashes(source)==pins
    repair=json.loads((BASE/'repair-scenery-audio/original-callee-226370-source-change.json').read_text())
    relative=repair['path'];assert sha(source/relative)==repair['before_sha256']
    assert sha(ROOT/relative)==repair['after_sha256']
    (out/'source-hashes-before-226370.json').write_bytes(before)
    (source/relative).write_bytes((ROOT/relative).read_bytes())
    tool='tools/generate_object_loop_audio_tu15.py'
    (source/tool).write_bytes((ROOT/tool).read_bytes())
    after=source_hashes(source)
    assert [p for p in pins if pins[p]!=after[p]]==[relative]
    manifest.write_text(json.dumps(after,indent=2,sort_keys=True)+'\n')
    receipt=dict(changed_production_files=[relative],before_manifest_sha256=hashlib.sha256(before).hexdigest(),
        after_manifest_sha256=sha(manifest),before_sha256=repair['before_sha256'],after_sha256=repair['after_sha256'],
        admitted_generator_sha256=sha(source/tool),scope='One complete unchanged original callee and its existing call edge; no native device/voice outcome supplied')
    (out/'source-refresh-226370.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)

if __name__=='__main__':main()
