#!/usr/bin/env python3
"""Preserve v1; correct only the validator's CTest summary format handling."""
import hashlib
import json
import shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012'
OVERLAYS=('tools/validate_physics_pool_startup_tu15_20261012.py',
          'tools/freeze_physics_pool_audit_correction_20261012.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    parent_path=E/'source-manifest-v1.json'
    assert sha(parent_path)=='3c6d29bc88b35b1681623dc7da32e3de09b1c381973cb2b4d3f2ebe0583c0903'
    parent=json.loads(parent_path.read_text());source=E/'source-v1a'
    assert not source.exists(),'Preserve existing correction cohort'
    source.mkdir();files={}
    for relative in sorted(set(parent['files'])|set(OVERLAYS)):
        if relative in parent['files']:assert sha(E/'source-v1'/relative)==parent['files'][relative],relative
        origin=ROOT/relative if relative in OVERLAYS else E/'source-v1'/relative
        destination=source/relative;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(origin,destination);files[relative]=sha(destination)
    (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    report=dict(parent,files=files,owned=sorted(set(parent['owned'])|set(OVERLAYS)),
        parent_manifest_sha256=sha(parent_path),metadata_only_overlays=list(OVERLAYS),
        audit_correction='CTest also reports 100% tests passed out of 1; accept either exact successful summary. No production source, headers, CMake, test corpus, test source or native binaries changed.')
    path=E/'source-manifest-v1a.json';path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(files=len(files),sha256=sha(path))))
if __name__=='__main__':main()
