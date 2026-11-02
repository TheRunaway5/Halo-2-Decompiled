#!/usr/bin/env python3
"""Finite owned frozen renderer build/capture session; use tools/run_test.py.

JSON lines: {"refresh": [owned relative source paths]} rebuilds the cohort;
{"argv": [...], "env": {...}} runs a finite command; {"quit": true} closes it.
@SOURCE@, @BUILD@, @SCRATCH@ and @RUNTIME@ expand only as argument data.
No installed runtime, source reference, cache or launcher selection is written.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import shutil
import subprocess
import sys
import time

from build_launcher_engine import freeze_source,source_hashes

ROOT=Path(__file__).resolve().parents[1]
LIMIT=4*1024*1024


def inventory(pins):return hashlib.sha256(json.dumps(pins,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def prepare_refresh(root,source,names):
    """Validate the entire request before touching owned source or references."""
    if not isinstance(names,list)or not names or not all(isinstance(name,str)for name in names):
        raise ValueError('Refresh requires a nonempty list of source paths')
    if len(set(names))!=len(names):raise ValueError('Duplicate renderer refresh path')
    plan=[]
    for name in names:
        relative=Path(name)
        if relative.is_absolute()or'..'in relative.parts or not relative.parts:
            raise ValueError('Refresh path escapes the owned source: '+name)
        destination=source/relative
        if not destination.resolve().is_relative_to(source.resolve()):
            raise ValueError('Refresh path is a borrowed reference, not owned source: '+name)
        data=(root/relative).read_bytes()
        before=hashlib.sha256(destination.read_bytes()).hexdigest()if destination.is_file()else None
        plan.append((name,destination,data,before))
    return plan


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--target',action='append',required=True);p.add_argument('--filter',required=True)
    p.add_argument('--jobs',type=int,default=4);p.add_argument('--idle-seconds',type=int,default=1200)
    a=p.parse_args();assert 30<=a.idle_seconds<=3600
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
    pins=freeze_source(ROOT,source)
    report=dict(status='prepared',passed=False,validation_not_run=True,owner=str(owner),source=str(source),build=str(build),
        initial_inventory_sha256=inventory(pins),source_files=len(pins),refreshes=[],commands=[],large_artifacts_retained=[])
    def save():
        text=json.dumps(report,indent=2)+'\n';assert len(text.encode())<=1024*1024
        (a.output/'session.json').write_text(text)
    save();print(json.dumps(dict(prepared=True,owner=str(owner),source_files=len(pins))),flush=True)
    def expand(value):
        for name,path in {'SOURCE':source,'BUILD':build,'SCRATCH':owner,'RUNTIME':build/'halo2_native'}.items():value=value.replace('@'+name+'@',str(path))
        return value
    def command(argv,env=None,timeout=None):
        actual=[expand(arg)for arg in argv];record=dict(argv=actual,started=time.time(),exit_code=None)
        report['commands'].append(record);save();log=a.output/('command-'+str(len(report['commands']))+'.log')
        environment={**os.environ,**{k:expand(v)for k,v in (env or {}).items()}}
        with log.open('w+b')as output:
            process=subprocess.Popen(actual,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=environment,cwd=source)
            record.update(pid=process.pid,log=str(log));save()
            try:
                # Nonblocking reads permit a bounded timeout without buffering
                # compiler output in memory or allowing a huge warning log.
                os.set_blocking(process.stdout.fileno(),False);last=time.monotonic()
                while process.poll()is None:
                    if timeout and time.monotonic()-last>timeout:
                        process.terminate();record['timed_out']=True;break
                    select.select([process.stdout],[],[],.25)
                    data=process.stdout.read(65536)
                    if data:
                        output.write(data)
                        if output.tell()>2*LIMIT:
                            output.seek(-LIMIT,os.SEEK_END);tail=output.read();output.seek(0);output.truncate();output.write(tail)
                        output.flush()
                try:record['exit_code']=process.wait(timeout=10)
                except subprocess.TimeoutExpired:process.kill();record['exit_code']=process.wait()
                while data:=process.stdout.read(65536):
                    output.write(data)
                    if output.tell()>2*LIMIT:
                        output.seek(-LIMIT,os.SEEK_END);tail=output.read();output.seek(0);output.truncate();output.write(tail)
                output.flush();output.seek(max(0,output.tell()-2400));record['tail']=output.read().decode(errors='replace')
            finally:
                if process.poll()is None:process.terminate();process.wait(timeout=10)
                process.stdout.close()
        record['finished']=time.time();save();print(json.dumps(dict(command_finished=len(report['commands']),exit_code=record['exit_code'],tail=record['tail'])),flush=True)
        return record['exit_code']
    def worker():
        assert source_hashes(source)==pins,'Owned source changed outside a declared refresh'
        argv=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--output',str(a.output/'cmake'),
            '--jobs',str(a.jobs),'--test-jobs','1','--filter',a.filter,
            '--cmake-arg=-DHALO2_STAGE_BUILD_OUTPUT=OFF','--cmake-arg=-DHALO2_BUILD_WINDOWS=OFF',
            '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT='+str(ROOT),
            '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE=/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',
            '--retain-report','native-tu15-normalize-vector.json','--retain-report','Testing/Temporary/LastTest.log']
        for target in a.target:argv+=['--target',target]
        report['status']='building';save();result=command(argv)
        report.update(status='ready',passed=result==0,validation_not_run=False,current_inventory_sha256=inventory(pins))
        if (build/'halo2_native').is_file():report['runtime_sha256']=hashlib.sha256((build/'halo2_native').read_bytes()).hexdigest()
        save();return result
    try:
        worker();pending=b''
        while True:
            while b'\n'not in pending:
                if not select.select([sys.stdin],[],[],a.idle_seconds)[0]:return
                chunk=os.read(sys.stdin.fileno(),65536)
                if not chunk:return
                pending+=chunk
                if len(pending)>1024*1024:raise ValueError('Session command budget exceeded')
            line,pending=pending.split(b'\n',1);request=json.loads(line)
            if request=={'quit':True}:break
            if 'refresh'in request:
                try:plan=prepare_refresh(ROOT,source,request['refresh'])
                except (ValueError,OSError)as error:
                    report.setdefault('rejected_refreshes',[]).append(dict(paths=request['refresh'],error=str(error)))
                    save();print(json.dumps(dict(refresh_rejected=True,error=str(error))),flush=True);continue
                refreshed=[]
                for name,destination,data,before in plan:
                    destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data);os.utime(destination,None)
                    refreshed.append(dict(path=name,before_sha256=before,after_sha256=hashlib.sha256(data).hexdigest()))
                report['refreshes'].append(refreshed);pins=source_hashes(source);save();worker()
            elif 'argv'in request:
                assert isinstance(request['argv'],list)and all(isinstance(s,str)for s in request['argv'])
                command(request['argv'],request.get('env'),request.get('timeout',600))
            else:raise ValueError('Unknown renderer session command')
    finally:
        report['status']='closed';save();print('Renderer session closed; run_test.py removes its owned source/build/captures.',flush=True)


if __name__=='__main__':main()
