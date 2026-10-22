#!/usr/bin/env python3
"""Promote the reviewed Deserted package with exact old/new artifact guards.

Run through run_test.py. Superseded owned output is removed by that owner's
lifecycle. The requested interactive match is launched separately afterward.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import time


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def install(root,candidate,old_cache_sha,old_model_sha):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    cache=Path('content/halo2/Mods/deserted/deserted.map')
    digest=sha(candidate/cache)
    for name in ('geometry-final','routes-final','atlas-seams-final','collision-cache-final',
            'compiled-faces-final','native-fixed-crossings','native-runtime-review'):
        report=json.loads((candidate/'evidence'/f'{name}.json').read_text())
        assert report['passed'],name
        if 'cache_sha256' in report:assert report['cache_sha256']==digest,name
    crossing=json.loads((candidate/'evidence/native-fixed-crossings.json').read_text())
    poses={c['name']:c['position'] for c in crossing['cases']}
    assert poses['t-spawn'][1]<-6.7 and poses['t-spawn'][2]>.95,poses
    assert poses['upper-fork-to-t'][1]<-3 and poses['upper-fork-to-t'][2]>.9,poses
    assert poses['lower-fork-to-mid'][0]>-4.3,poses
    assert sha(root/cache)==old_cache_sha,'The current published cache changed; preserve it'
    assert sha(root/'model/dust2_mombasa.glb')==old_model_sha,'The current authored model changed; preserve it'

    stopped=[]
    target=(root/cache).resolve()
    for proc in Path('/proc').iterdir():
        if not proc.name.isdigit():continue
        try:
            if (proc/'comm').read_text().strip()!='halo2_native':continue
            argv=(proc/'cmdline').read_bytes().split(bytes([0]))
            if b'--map' not in argv:continue
            i=argv.index(b'--map');path=Path(os.fsdecode(argv[i+1]))
            if not path.is_absolute():path=(proc/'cwd').resolve()/path
            if path.resolve()!=target:continue
            pid=int(proc.name);os.kill(pid,signal.SIGTERM);stopped.append(pid)
        except (FileNotFoundError,ProcessLookupError,PermissionError):continue
    for pid in stopped:
        deadline=time.monotonic()+10
        while Path(f'/proc/{pid}').exists() and time.monotonic()<deadline:time.sleep(.1)
        if Path(f'/proc/{pid}').exists():
            argv=Path(f'/proc/{pid}/cmdline').read_bytes().split(bytes([0]))
            assert os.fsencode(str(target)) in argv
            os.kill(pid,signal.SIGKILL)

    evidence=root/'evidence'
    shutil.copyfile(root/'model/architecture-solid.npz',evidence/'architecture-before-streets.npz')
    if (evidence/'native-runtime-review.json').exists():
        shutil.copyfile(evidence/'native-runtime-review.json',evidence/'native-runtime-review-before-streets.json')
    moved=[]
    try:
        for leaf in ('model','content','h2ek-tags'):
            (root/leaf).rename(scratch/('previous-owned-'+leaf))
            (candidate/leaf).rename(root/leaf);moved.append(leaf)
        assert sha(root/cache)==digest
    except BaseException:
        for leaf in reversed(moved):(root/leaf).rename(candidate/leaf)
        for leaf in ('model','content','h2ek-tags'):
            prior=scratch/('previous-owned-'+leaf)
            if prior.exists():prior.rename(root/leaf)
        raise
    shutil.copytree(candidate/'evidence',evidence,dirs_exist_ok=True)
    for leaf in ('README.md','build-progress.json'):
        shutil.copyfile(candidate/leaf,root/leaf)
    development=root.parent/'deserted-streets'
    assert json.loads((development/'model/redesign.json').read_text())['name']=='Deserted'
    shutil.copytree(development/'evidence',evidence/'street-collision-development',dirs_exist_ok=True)
    baked=candidate/'baked-inputs'
    pins=json.loads((baked/'retention.json').read_text())
    for leaf,row in pins['files'].items():
        if leaf.startswith('tags/'):
            assert sha(root/'h2ek-tags'/leaf.removeprefix('tags/'))==row['sha256']
    (candidate).rename(scratch/'reviewed-staging-package')
    development.rename(scratch/'superseded-street-prototypes')
    result=dict(passed=True,cache_sha256=digest,previous_cache_sha256=old_cache_sha,
        previous_model_sha256=old_model_sha,stopped_only_matching_native_processes=stopped,
        scope='Reviewed cache/model/tags promoted byte-identically; staging paths in execution receipts are historical',
        retired='Superseded cache/model/tags, temporary street prototypes and duplicate baked inputs removed by owned cleanup')
    (evidence/'installation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result),flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('root','candidate'):p.add_argument('--'+name,required=True,type=Path)
    p.add_argument('--old-cache-sha',required=True)
    p.add_argument('--old-model-sha',required=True)
    install(**vars(p.parse_args()))
