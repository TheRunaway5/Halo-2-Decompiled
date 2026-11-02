#!/usr/bin/env python3
"""Captured modern C++ lobby kernels versus actual Rust/CXX policy and IO.

Native cache/query facts are controlled. Original executable cases have their
own unchanged fixture comparators. This does not certify the installed client,
historical Live producers or full OG framebuffer parity.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess


def cases():
    rng=random.Random(0x24fd74)
    limits=[-0x80000000,-65536,-1,0,1,2,3,4,5,6,7,8,9,10,16,17,255,65536,0x7fffffff]
    out=[]
    for session,previous,mask in itertools.product(range(-1,8),[-0x80000000,-1,0,1,2,3,0x7fffffff],range(32)):
        out.append(dict(session=session,previous=previous,host=bool(mask&1),teams=bool(mask&2),local_team=bool(mask&4),
            countdown=bool(mask&8),permission=bool(mask&16),guard=bool(rng.randrange(2)),status=rng.choice([*range(-1,28),*limits]),
            delayed=rng.choice(limits),cancelled=bool(rng.randrange(2)),loading=bool(rng.randrange(2)),editing=bool(rng.randrange(2)),
            team_count=rng.choice(limits),players=rng.choice(limits),maximum=rng.choice(limits),overflow=rng.getrandbits(32),
            privacy=rng.choice(limits),stats=rng.choice(limits)))
    for cls,sub,campaign,variant,valid in itertools.product([-0x80000000,-1,0,1,2,3,0x7fffffff],[-1,0,1,2,3],[-1,0,1],
                                                           [None,-0x80000000,-1,0,1,4,5,6,7,9,10,0x7fffffff],[False,True]):
        out.append(dict(kind='session',**{'class':cls},subtype=sub,campaign=campaign,variant=variant,valid=valid))
    for _ in range(1000):
        out.append(dict(kind='scalars',**dict(zip('abcde',[rng.choice(limits) for _ in range(5)])),
            u=rng.choice([*range(9),0xffffffff]),difficulty=rng.choice([-32768,-1,0,1,2,3,4,32767])))
    for engine in [*range(8),0xffffffff]:
        for parameter in [0,5,91,111,112,256,0xffffffff]:out.append(dict(kind='default',engine=engine,parameter=parameter))
    for engine,count,scenario in itertools.product([*range(8),0xffffffff],[0,5,6],range(12)):
        categories=[dict(id=id,rows=[10,404,11,12,10,13,14]) for id in [22,23,24,25,28,29,30]]
        p=[dict(tag=10,parameter=5,title=[0xff,0,0x80],choices=[dict(label=[0xfe,0],value=-1),dict(label='second duplicate',value=-1)]),
           dict(tag=11,parameter=112,title=''),dict(tag=12,parameter=112,title='out of range',choices=[dict(label='override',value=73)]),
           dict(tag=13,parameter=0xffffffff,title='wide',choices=[dict(label='override',value=73)]),
           dict(tag=14,parameter=5,title='seventh')]
        values=[]
        if scenario&1:values+=[dict(id=5,value=rng.choice([-0x80000000,-1,0,1,0x7fffffff]))]
        if scenario&2:values+=[dict(id=112,value=73),dict(id=0xffffffff,value=73)]
        if scenario&4:categories=[]
        if scenario&8:p=[]
        out.append(dict(kind='quick',engine=engine,count=count,categories=categories,parameters=p,values=values,
                        separator=rng.choice([': ','',[0xff,0,0x80]])))
    # Defaults may be absent even when their guard passes; don't bind a row.
    for default in [None,-1,0,0x7fffffff]:
        out.append(dict(kind='quick',engine=0,count=6,categories=[dict(id=23,rows=[10,10])],
            parameters=[dict(tag=10,parameter=5,title='title',choices=[dict(label='first',value=-1),dict(label='second',value=-1)])],
            default=default))
    return out


def run(native,corpus):
    p=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n' for c in corpus),text=True,capture_output=True)
    lines=p.stdout.splitlines()
    if lines[:1]==['Shutdown handler: initialize.'] and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
    if p.returncode or len(lines)!=len(corpus):
        raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(corpus),stderr=p.stderr[-2000:],stdout=p.stdout[:1000])))
    return [json.loads(line) for line in lines]


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('native',type=Path);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();corpus=cases();answers=run(args.native,corpus);failures=[];observations=production=0
    for index,(case,value) in enumerate(zip(corpus,answers)):
        if isinstance(value['before'],dict):observations+=len(value['before'].get('calls',[]))
        if 'production'in value:
            production+=1
            if value['before'].get('value')!=value['production'] and len(failures)<8:failures.append(dict(index=index,case=case,actual=value))
        if value['before']!=value['rust'] and len(failures)<8:failures.append(dict(index=index,case=case,actual=value))
    selected=[next(c for c in corpus if c.get('kind')=='quick' and c.get('default')==-1)]
    faults=[]
    for case,value in zip(selected,run(args.native,selected)):
        for fail in range(len(value['before']['calls'])):
            for nonstd in [False,True]:faults.append(case|dict(fail=fail,nonstd=nonstd))
    identity=0
    for case,value in zip(faults,run(args.native,faults)):
        if value['before']!=value['rust'] and len(failures)<8:failures.append(dict(case=case,actual=value))
        identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int' and value['rust'].get('integer')==73))
    report=dict(passed=not failures,cases=len(corpus),native_io_observations=observations,production_adapter_comparisons=production,
        fault_controls=len(faults),preserved_native_exception_identity=identity,differences=failures,
        native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),scope=__doc__)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'}))
    if failures:print(json.dumps(failures[0])[:5000])
    raise SystemExit(0 if report['passed'] else 1)


if __name__=='__main__':main()
