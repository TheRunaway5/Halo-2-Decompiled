#!/usr/bin/env python3
"""Compact one completed owned reference VM, proving exact bytes and restoration.

Required VM contents remain authenticated shared chunks. A private compressed-
only view must boot the saved snapshot and expose the same original weapon
identity before the owned raw disk is retired. All restored disks use scratch.
"""
import argparse
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from capture_xbox_tu15_grenade_reference import ROOT,TEXT_SHA,sha,write_json,wait_for

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True);parser.add_argument('--census',type=Path,required=True)
    parser.add_argument('--smoke-output',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);session=args.session.absolute();disk=session/'session-hdd.qcow2'
    cleanup=json.loads((session/'cleanup.json').read_text());assert cleanup['completed']and cleanup['all_processes_exited']
    launch=json.loads((session/'launch.json').read_text())
    for key in ('pid','xvfb_pid'):assert not Path('/proc',str(launch[key])).exists()
    evidence_path=session/'reference-evidence.json';evidence=json.loads(evidence_path.read_text());evidence_sha=sha(evidence_path)
    digest=sha(disk);assert not disk.stat().st_mode&0o222 and digest==evidence['linked_files_sha256'][str(disk)]
    raw_bytes=disk.stat().st_size
    base=ROOT/'analysis/weapons-tu15-goal-20261013';spec=importlib.util.spec_from_file_location('reference_content',base/'archive-content-retention-v1.py')
    content=importlib.util.module_from_spec(spec);spec.loader.exec_module(content)
    with (content.STORE/'pack.lock').open('a')as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        with disk.open('rb')as source:content.store_file(source,digest,raw_bytes,set())
    recipe=content.content_path('files',digest,'.json');records=json.loads(recipe.read_text())['chunks']
    chunk_paths={content.content_path('chunks',r['sha256'],'.z')for r in records}
    restored=scratch/'exact-restoration.qcow2'
    with restored.open('wb')as output:assert content.reconstruct(digest,output)==raw_bytes
    assert sha(restored)==digest;restored.unlink()
    original_census=json.loads(args.census.read_text());unit=original_census['players'][0]['current_unit']
    expected=next(o for o in original_census['objects']if o['datum']==0xe2830014)
    manifest=dict(raw_sha256=digest,raw_bytes=raw_bytes,original_mode=0o444,original_path=str(disk),
        content_recipe=str(recipe),content_recipe_sha256=sha(recipe),snapshot=evidence['snapshot'],
        referenced_unique_compressed_chunk_bytes=sum(p.stat().st_size for p in chunk_paths),
        shared_chunks=[dict(path=str(p),compressed_bytes=p.stat().st_size)for p in sorted(chunk_paths)],
        exact_reconstruction_verified=True,reason='Required normally played original Lockout weapon reference, including complete VM state',
        observed_unit=unit,observed_weapon=expected['datum'],observed_weapon_definition=expected['definition'],
        observed_weapon_name=expected['name'],source_census_sha256=sha(args.census))
    # The smoke source has no raw disk. It exercises the compressed restore
    # route while the original required reference is still safely available.
    view=scratch/'compressed-only-reference';view.mkdir();(scratch/'smoke').mkdir()
    manifest_path=view/'reference-disk-retention.json';write_json(manifest_path,manifest)
    shutil.copyfile(session/'eeprom.bin',view/'eeprom.bin')
    (view/'xemu.toml').write_text((session/'xemu.toml').read_text().replace(str(session),str(view)))
    smoke_evidence=dict(evidence);smoke_evidence['linked_files_sha256']={p:d for p,d in evidence['linked_files_sha256'].items()if not Path(p).is_relative_to(session)}
    smoke_evidence['linked_files_sha256'].update({str(p):sha(p)for p in (manifest_path,view/'eeprom.bin',view/'xemu.toml')})
    smoke_evidence['compressed_linked_files_sha256']={str(view/'session-hdd.qcow2'):digest}
    write_json(view/'reference-evidence.json',smoke_evidence)
    helper=ROOT/'tools/run_xbox_tu15_reference_session.py';command=[sys.executable,str(helper),'--session',str(view),
        '--evidence',str(view/'reference-evidence.json'),'--snapshot',evidence['snapshot'],'--max-seconds','300','--output',str(args.smoke_output.absolute())]
    env=dict(os.environ,HALO2_TEST_SCRATCH=str(scratch/'smoke'))
    driver=session/'compressed-restore-smoke-driver.log';process=None
    try:
        with driver.open('w')as log:process=subprocess.Popen(command,env=env,stdout=log,stderr=subprocess.STDOUT,cwd=ROOT)
        wait_for((args.smoke_output/'launch.json').exists,90,[process])
        smoke_launch=json.loads((args.smoke_output/'launch.json').read_text())
        # A saved API-return point may resume inside an object update. Observe
        # actual completed scene entries and keep GDB stopped during the census.
        observer=scratch/'restore-observer.gdb'
        census_command=[sys.executable,str(ROOT/'tools/capture_xbox_tu15_weapon_census.py'),'--qmp',smoke_launch['qmp'],
            '--map',str(ROOT/'assets/disc/maps/lockout.map'),'--output',str((args.smoke_output/'census').absolute())]
        observer.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 30
target remote 127.0.0.1:{smoke_launch['gdb_port']}
python
import gdb,hashlib,json,pathlib,subprocess,struct
inf=gdb.selected_inferior()
assert hashlib.sha256(bytes(inf.read_memory(0x12000,3611948))).hexdigest()=={TEXT_SHA!r}
b=gdb.Breakpoint('*0x132b0',gdb.BP_HARDWARE_BREAKPOINT,internal=True)
frames=[]
for i in range(4):
 gdb.execute('continue')
 assert int(gdb.parse_and_eval('$eip'))==0x132b0
 clock=struct.unpack('<I',inf.read_memory(0x51df54,4))[0]
 frames.append(dict(entry=0x132b0,tick=struct.unpack('<I',inf.read_memory(clock+8,4))[0]))
b.delete()
pathlib.Path({str((args.smoke_output/'observed-frames.json').absolute())!r}).write_text(json.dumps(frames,indent=2)+'\\n')
result=subprocess.run({census_command!r},capture_output=True,text=True,timeout=30)
pathlib.Path({str((args.smoke_output/'census-driver.log').absolute())!r}).write_text((result.stdout+result.stderr)[-65536:])
assert result.returncode==0,result.stderr[-2000:]
end
detach
quit
''')
        result=subprocess.run(['gdb','-q','-nx','-batch','-x',str(observer)],capture_output=True,text=True,timeout=90)
        (args.smoke_output/'observer.log').write_text((result.stdout+result.stderr)[-65536:])
        shutil.copyfile(observer,args.smoke_output/'restore-observer.gdb')
        assert result.returncode==0,result.stderr[-2000:]
        observed=json.loads((args.smoke_output/'census/report.json').read_text())
        assert observed['players'][0]['current_unit']==unit
        weapon=next(o for o in observed['objects']if o['datum']==expected['datum'])
        assert weapon['definition']==expected['definition']and weapon['name']==expected['name']
        (args.smoke_output/'STOP').write_text('Compressed-only original reference restored and live weapon authenticated\n')
        assert process.wait(timeout=30)==0
        smoke_cleanup=json.loads((args.smoke_output/'cleanup.json').read_text());assert smoke_cleanup['completed']and smoke_cleanup['all_processes_exited']
    finally:
        if process and process.poll()is None:
            if args.smoke_output.exists():(args.smoke_output/'STOP').write_text('Terminate owned smoke session\n')
            try:process.wait(timeout=10)
            except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=5)
    assert sha(disk)==digest and sha(evidence_path)==evidence_sha
    manifest.update(raw_disk_intentionally_retired=True,compressed_only_VM_restore_verified=True,
        restore_smoke_report=str((args.smoke_output/'census/report.json').absolute()),
        restore_smoke_report_sha256=sha(args.smoke_output/'census/report.json'),runner_scratch=str(scratch),
        new_persistent_raw_disk_copies=False,tool_sha256=sha(Path(__file__)))
    final_manifest=session/'reference-disk-retention.json';write_json(final_manifest,manifest)
    updated=dict(evidence);updated['linked_files_sha256']=dict(evidence['linked_files_sha256']);del updated['linked_files_sha256'][str(disk)]
    updated['linked_files_sha256'][str(final_manifest)]=sha(final_manifest);updated['compressed_linked_files_sha256']={str(disk):digest}
    updated['retained_large_artifacts']=[dict(kind='shared compressed original VM disk',manifest=str(final_manifest),
        raw_logical_bytes=raw_bytes,unique_referenced_compressed_chunk_bytes=manifest['referenced_unique_compressed_chunk_bytes'],reason=manifest['reason'])]
    updated['observed_weapon_name']=expected['name'];updated['compressed_restore_verified']=True;write_json(evidence_path,updated)
    disk.unlink();assert not disk.exists();assert content.reconstruct(digest)==raw_bytes
    shutil.copyfile(Path(__file__),session/'disk-compaction-source.py')
    print(json.dumps(dict(passed=True,raw_disk_bytes_retired=raw_bytes,exact_VM_contents_retained=True,
        unique_referenced_compressed_chunk_bytes=manifest['referenced_unique_compressed_chunk_bytes'],
        compressed_only_VM_restore_verified=True,runner_scratch=str(scratch))),flush=True)

if __name__=='__main__':main()
