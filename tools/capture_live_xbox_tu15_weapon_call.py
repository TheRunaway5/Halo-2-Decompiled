#!/usr/bin/env python3
"""Observe a real weapon-update call in an owned original TU1.5 session.

Only normal right-trigger input and hardware read-only stop selection are used.
Run through tools/run_test.py: raw physical RAM and debugger products are private
scratch. Required entry/exit RAM is retained as authenticated shared compression.
"""
import argparse
import contextlib
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
from capture_xbox_tu15_grenade_reference import ROOT,qmp,sha,wait_for,write_json,x11_window

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('session','output'):parser.add_argument('--'+name,type=Path,required=True)
    for name in ('unit','weapon','definition'):parser.add_argument('--'+name,type=lambda v:int(v,0),required=True)
    parser.add_argument('--input',choices=('primary-fire','charge-start','full-charge','none'),required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);out=args.output.absolute();out.mkdir(exist_ok=False)
    launch=json.loads((args.session/'launch.json').read_text());qp=Path(launch['qmp'])
    assert Path('/proc',str(launch['pid'])).exists()and launch['core_limit']==0
    qmp(qp,[dict(execute='stop')])
    memory=qmp(qp,[dict(execute='query-memory-size-summary')])[-1]['return'];assert memory['base-memory']==67108864
    parameters=dict(output=str(scratch),qmp=str(qp),ram_bytes=memory['base-memory'],phase='weapon',
        unit=args.unit,weapon=args.weapon,definition=args.definition,warm_views=0,normal_fire=False,weapon_input=args.input)
    config=scratch/'parameters.json';write_json(config,parameters)
    observer=ROOT/'tools/read_xbox_tu15_first_person_case.py';script=scratch/'capture.gdb'
    script.write_text(f'''set pagination off
set confirm off
set architecture i386
set remotetimeout 20
target remote 127.0.0.1:{launch['gdb_port']}
python
import os,sys,runpy
sys.path.insert(0,{str(ROOT/'tools')!r})
os.environ['HALO2_FP_CASE_CONFIG']={str(config)!r}
runpy.run_path({str(observer)!r},run_name='__main__')
end
delete
detach
quit
''')
    process=None;success=False
    try:
        with (scratch/'capture.log').open('w')as log:
            process=subprocess.Popen(['gdb','-q','-nx','-batch','-x',str(script)],stdout=log,stderr=subprocess.STDOUT)
            if args.input!='none':
                wait_for((scratch/'NORMAL_FIRE_READY').exists,30,[process])
                def complete():assert process.wait(timeout=120)==0
                x11_window(launch['display'],press=True,key='e',duration=complete)
            else:assert process.wait(timeout=120)==0
        if qp.exists():
            with contextlib.suppress(OSError,RuntimeError):qmp(qp,[dict(execute='stop')])
        for path in scratch.iterdir():
            if path.is_file()and(path.suffix=='.json'or path.name.endswith('-body.bin')or path.name.endswith('-first-person-state.bin')):
                shutil.copyfile(path,out/path.name)
        base=ROOT/'analysis/weapons-tu15-goal-20261013';spec=importlib.util.spec_from_file_location('retained_content',base/'archive-content-retention-v1.py')
        content=importlib.util.module_from_spec(spec);spec.loader.exec_module(content);records={};verified=set()
        with (content.STORE/'pack.lock').open('a')as lock:
            fcntl.flock(lock,fcntl.LOCK_EX)
            for label in ('before','after'):
                path=scratch/(label+'-physical.bin');assert path.stat().st_size==memory['base-memory'];digest=sha(path)
                with path.open('rb')as source:content.store_file(source,digest,path.stat().st_size,verified)
                recipe=content.content_path('files',digest,'.json')
                records[label]=dict(raw_bytes=path.stat().st_size,raw_sha256=digest,content_recipe=str(recipe),
                    content_recipe_sha256=sha(recipe),exact_reconstruction_verified=True)
        write_json(out/'physical-memory-retention.json',dict(files=records,reason='Required original live charged-weapon update entry/exit reference inputs'))
        authority=base/'first-person-original-call-v1';pins=json.loads((authority/'x87-tag-authority.json').read_text())
        for name in ('xemu-gdbstub.c','xemu-cpu-dump.c'):
            path=authority/name;assert sha(path)==pins['source_files_sha256'][str(path.relative_to(ROOT))];shutil.copyfile(path,out/name)
        write_json(out/'x87-tag-authority.json',dict(source_authority=str(authority/'x87-tag-authority.json'),source_authority_sha256=sha(authority/'x87-tag-authority.json')))
        success=True
    finally:
        if process and process.poll()is None:process.terminate();process.wait(timeout=5)
        if qp.exists():
            with contextlib.suppress(OSError,RuntimeError):qmp(qp,[dict(execute='stop')])
        for source,name in ((Path(__file__),'capture-source.py'),(observer,'observer-source.py'),(script,'capture.gdb'),(config,'parameters.json')):
            if source.exists():shutil.copyfile(source,out/name)
        if(scratch/'capture.log').exists():(out/'capture.log').write_bytes((scratch/'capture.log').read_bytes()[-65536:])
        write_json(out/'cleanup.json',dict(capture_completed=success,observer_exited=process is None or process.poll()is not None,
            original_session_still_owned_by_live_outer_runner=Path('/proc',str(launch['pid'])).exists(),source_launch_sha256=sha(args.session/'launch.json'),
            source_session=str(args.session.absolute()),runner_scratch=str(scratch),working_files_removed_by_runner=True,
            persistent_raw_memory_dumps=False,full_weapon_parity=False))
    print(json.dumps(dict(passed=success,case=str(out),runner_scratch=str(scratch))),flush=True)

if __name__=='__main__':main()
