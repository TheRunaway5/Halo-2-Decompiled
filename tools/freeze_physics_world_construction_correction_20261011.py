#!/usr/bin/env python3
"""Seal the evidence-address correction without changing tested code or CMake."""
import hashlib
import json
import shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/physics-engine-20261010'
PARENT_PIN='3e467afc777194559c89cbdb405803d8116c285989301489f1648c07b27f49b7'
APPEND=''
OWNED=('docs/PHYSICS_WORLD_CONSTRUCTION_RUNTIME_20261011.md',
       'tools/validate_physics_world_construction_20261011.py',
       'tools/freeze_physics_world_construction_correction_20261011.py')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parent_path=BASE/'source-manifest-v15.json';assert sha(parent_path)==PARENT_PIN
    parent=json.loads(parent_path.read_text());source=BASE/'source-v15a';path=BASE/'source-manifest-v15a.json'
    for name,pin in parent['files'].items():assert sha(BASE/'source-v15'/name)==pin,name
    if path.exists():
        report=json.loads(path.read_text());assert report['parent_manifest_sha256']==PARENT_PIN
        for name,pin in report['files'].items():assert sha(source/name)==pin,name
        for name in OWNED:assert sha(ROOT/name)==report['files'][name],name
        print(json.dumps(dict(files=len(report['files']),sha256=sha(path),audit='passed')));return
    assert not source.exists();source.mkdir();files={};pins={name:sha(ROOT/name) for name in OWNED}
    for name in sorted(set(parent['files'])|set(OWNED)):
        origin=ROOT/name if name in OWNED else BASE/'source-v15'/name
        target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(origin,target)
        files[name]=sha(target);assert files[name]==(pins[name] if name in OWNED else parent['files'][name]),name
    for name,pin in pins.items():assert sha(ROOT/name)==pin,name
    (source/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    assert all(files[name]==pin for name,pin in parent['files'].items() if name not in OWNED)
    report=dict(schema=1,parent_manifest_sha256=PARENT_PIN,owned_overlays=list(OWNED),files=files,
        scope='Metadata correction only: the core world pointer store is1C6867, not1C68A7. Original frozen v15 remains immutable. All code, headers, tests and CMake are byte-identical to the compiled/tested v15 cohort; existing Release/sanitizer results cover this same executable code.',
        compiled_source_manifest_sha256=PARENT_PIN,code_tests_headers_cmake_unchanged=True,
        correction=dict(incorrect_instruction='001c68a7',verified_instruction='001c6867',source_store_address='00526234'))
    path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(files=len(files),sha256=sha(path))))
if __name__=='__main__':main()
