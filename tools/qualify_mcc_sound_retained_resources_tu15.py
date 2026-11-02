#!/usr/bin/env python3
"""Qualify retained PCM/preflight with an owned distinct-inode MCC clone.

The native metadata fixture hides and edits only this disposable clone. This
does not grant a native publication or certify original voice/device output.
Run through tools/run_test.py (or its active owned command session).
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from mcc_prepared_cache_delta import _clone_tree, _snapshot_sha
from prepare_mcc_original_sound_catalog_tu15 import source_snapshot


def sha(path):
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in('binary','source','output'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    binary=args.binary.resolve(strict=True);source=args.source.resolve(strict=True)
    assert binary.is_relative_to(owner)and source==args.source.absolute()and not source.is_relative_to(owner)
    args.output=args.output.absolute();args.output.mkdir(parents=True,exist_ok=False)
    clone=owner/'retained-resource-control';assert not clone.exists();clone.mkdir()
    before=source_snapshot(source);pin=sha(binary)
    report=dict(passed=False,goal_complete=False,source=str(source),binary_sha256=pin,
                native_world_or_source_voice_verified=False,retained_large_artifacts=[])
    try:
        report['clone']=_clone_tree(source,clone,before)
        output=args.output/'component.json';command=[str(binary),str(clone/'halo2/h2_maps_win64_dx11/lockout.map'),
            '--retained-resources',str(clone),str(output)]
        report['command']=command
        with(args.output/'component.log').open('wb')as stream:
            result=subprocess.run(command,stdin=subprocess.DEVNULL,stdout=stream,stderr=subprocess.STDOUT,timeout=600,
                env=dict(os.environ,SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'retained-resource-cache')))
        data=json.loads(output.read_text())if output.is_file()else{}
        report.update(exit_code=result.returncode,component_sha256=sha(output)if output.is_file()else None,
            passed=result.returncode==0 and data.get('passed')is True and all(data.get(k)is True for k in
                ('unavailable_paths_uncached_audio_verified','retained_pcm_byte_equal','actual_changed_dat_preflight_refused',
                 'same_resources_stale_saved_epoch_admitted','foreign_world_resources_refused',
                 'unavailable_paths_new_provider_verified','equal_bytes_distinct_dat_preflight_admitted',
                 'encoded_byte_mutation_isolated','typed_sector_mutation_isolated')))
    finally:
        after=source_snapshot(source);report['source_inode_snapshot_sha256']=_snapshot_sha(before)
        report['source_unchanged']=before==after;report['binary_unchanged']=sha(binary)==pin
        shutil.rmtree(clone);report['private_clone_removed']=not clone.exists()
        report['passed']=report['passed']and report['source_unchanged']and report['binary_unchanged']and report['private_clone_removed']
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in('passed','source_unchanged','binary_unchanged','private_clone_removed')}),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
