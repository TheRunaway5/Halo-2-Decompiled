#!/usr/bin/env python3
"""Captured native party callers versus current native Rust/CXX callers.

The local profile/login/seat observations are declared synthetic IO. This
qualifies projection/caller equivalence, not original Live identity production,
physical controller input or installed-runtime delivery. The enclosing owned
run_tests.py/run_test.py lifecycle removes every generated build product.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess


def cases():
    rng = random.Random(0x194fbd)
    out = []
    def avatar():
        return [rng.randrange(256) for _ in range(9)]
    def player(index, own=False, local_count=0):
        return dict(handle=['a','b','a',[255,0]][index%4], name='Same', self=own,
                    leader=index==0, online=bool(index%2), ready=['','hopper'][index%2],
                    guest=bool(index%3==1), controller=rng.choice([-1,-2,-2147483648]),
                    appearance=avatar() if index%2 else None, postgame=rng.choice([None,False,True]),
                    locals=[dict(name=[254,0,i],guest=bool(i%2),appearance=avatar() if i%2 else None) for i in range(local_count)])
    for count, own, local_count, grants in itertools.product(range(8), (False,True), range(4), range(8)):
        party=[player(i,own and i==count//2,(i+local_count)%4) for i in range(count)]
        out.append(dict(party=party,primary=avatar(),seats=[dict(controller=i,name=[255,0,i],appearance=avatar()) for i in range(local_count+1)],
                        authorizations=[None,*[bool(grants&(1<<i)) if (i+grants)%3 else None for i in range(3)]]))
    out.append(dict(party=None,seats=[dict(controller=1,name='unused')]))
    base=dict(party=[player(0,True,2),player(1,False,2)],seats=[dict(controller=0,name='primary'),dict(controller=1,name='couch')],authorizations=[None,True],primary=avatar())
    for screen,row,column,voice in itertools.product((14,26,0xffffffff),range(7),(0,1,2,0xffffffff),range(8)):
        out.append(base|dict(op='column',screen=screen,row=row,column=column,voice_party=bool(voice&1),joined=bool(voice&2),transmitting=bool(voice&4),
                            muted=['a'] if voice&2 else [],talking=['b'] if voice&4 else []))
    shapes=[None,[],[player(0)], [player(0,local_count=1)], [player(0,local_count=2)], [player(0),player(1)]]
    for old_party,new_party,mask in itertools.product(shapes,shapes,range(16)):
        old=dict(authenticated=bool(mask&1),busy=bool(mask&4),party=old_party,message='error' if mask&8 else '',
                 invitations=[dict(id=[255,0],name='old')])
        new=dict(authenticated=bool(mask&2),busy=bool(mask&8),party=new_party,message='error' if mask&4 else '',
                 invitations=[dict(id='new' if mask&4 else [255,0],name='changed')])
        out.append(dict(op='sounds',before=old,after=new))
    invitations=[dict(id='dismissed',name='first'),dict(id=[255,0],name=[128,0]),dict(id='later',name='third')]
    for flags,status,dismissed in itertools.product(range(32),[0,1,2,3,4,5,6,7,8,9,262,263,264,0xffffffff],([],['dismissed'],['dismissed',[255,0],'later'])):
        out.append(dict(op='notice',authenticated=bool(flags&1),signed_in=[0] if flags&2 else '',busy=bool(flags&4),party=[] if flags&8 else None,
                        launch=bool(flags&16),status=status,invitations=invitations,dismissed=dismissed))
    return out


def run(native, corpus):
    result=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n' for c in corpus),text=True,capture_output=True)
    lines=result.stdout.splitlines()
    if lines[:1]==['Shutdown handler: initialize.'] and lines[-1:]==['Shutdown handler: cleanup.']:
        lines=lines[1:-1]
    if result.returncode or len(lines)!=len(corpus):
        raise RuntimeError(json.dumps(dict(exit_code=result.returncode,lines=len(lines),expected=len(corpus),stderr=result.stderr[-4000:],stdout=result.stdout[:1000])))
    return [json.loads(line) for line in lines]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    corpus=cases(); answers=run(args.native,corpus)
    failures=[]; reads=comparisons=identities=0
    for index,(case,value) in enumerate(zip(corpus,answers)):
        reads+=len(value['before']['calls']);comparisons+=1+len(value['before']['calls'])
        if value['before']!=value['rust'] and len(failures)<8:failures.append(dict(index=index,case=case,actual=value))
    controls=[dict(party=[dict(handle='a',self=True,leader=True)],seats=[dict(controller=i,name='Same') for i in range(4)],authorizations=[None,True,False,None]),
              dict(op='column',screen=0xffffffff,column=0xffffffff,party=[dict(handle='a',self=True)],seats=[dict(controller=1,name='Seat')],authorizations=[None,True])]
    faults=[]
    for case,value in zip(controls,run(args.native,controls)):
        for index in range(len(value['before']['calls'])):
            for nonstd in (False,True):faults.append(case|dict(fail=index,nonstd=nonstd))
    for case,value in zip(faults,run(args.native,faults)):
        comparisons+=1+len(value['before']['calls'])
        identities+=value['rust'].get('same_object',value['rust'].get('error')=='int')
        if value['before']!=value['rust'] and len(failures)<8:failures.append(dict(case=case,actual=value))
    report=dict(passed=not failures,cases=len(corpus),native_observations=reads,comparisons=comparisons,fault_controls=len(faults),
                preserved_native_exception_identity=identities,differences=failures,native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),scope=__doc__)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='differences'}))
    if failures:print(json.dumps(failures[0])[:5000])
    return 1 if failures else 0


if __name__=='__main__':
    raise SystemExit(main())
