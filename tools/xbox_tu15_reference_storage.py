"""Exact read-only reference disks, restored only into owned test scratch."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
from capture_xbox_tu15_grenade_reference import ROOT,sha

def pins(evidence):
    return dict(evidence['linked_files_sha256'])|evidence.get('compressed_linked_files_sha256',{})

def verify_reference_disk(session,evidence):
    disk=session/'session-hdd.qcow2';expected=pins(evidence)[str(disk)]
    if disk.exists():
        assert not disk.stat().st_mode&0o222 and sha(disk)==expected
    else:
        manifest=session/'reference-disk-retention.json'
        assert sha(manifest)==evidence['linked_files_sha256'][str(manifest)]
        data=json.loads(manifest.read_text());assert data['raw_sha256']==expected and data['original_mode']==0o444
        recipe=Path(data['content_recipe']);assert sha(recipe)==data['content_recipe_sha256']
    return expected

def prepare_private_disk(session,working,evidence):
    expected=verify_reference_disk(session,evidence);disk=session/'session-hdd.qcow2';target=working/disk.name
    assert not target.exists()
    if disk.exists():subprocess.run(['cp','--reflink=always','--',str(disk),str(target)],check=True)
    else:
        manifest=json.loads((session/'reference-disk-retention.json').read_text())
        base=ROOT/'analysis/weapons-tu15-goal-20261013';spec=importlib.util.spec_from_file_location('retained_reference',base/'archive-content-retention-v1.py')
        content=importlib.util.module_from_spec(spec);spec.loader.exec_module(content)
        with target.open('wb')as output:assert content.reconstruct(expected,output)==manifest['raw_bytes']
    target.chmod(0o600);assert sha(target)==expected
    return expected
