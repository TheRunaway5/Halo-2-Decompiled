#!/usr/bin/env python3
"""Captured modern account observer versus the exact native Rust/CXX adapter.

Worker/identity/device/population providers are controlled. Complete cached
state and callback-boundary snapshots, retained frame focus, native failures,
and actual Rust state reentry are compared. This is not original Live recovery
or installed-client/provider qualification.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess


def corpus():
    rng=random.Random(0x18fe9e);out=[];ids=[[255,0],'A','B']
    templates=[dict(screen=26,context=0),dict(screen=26,context=1),dict(screen=26,context=2),dict(screen=31,context=46),dict(screen=217),dict(screen=31,context=42),dict(screen=33,context=41),dict(screen=0xffffffff)]
    for old_auth,old_party,new_auth,new_party in itertools.product([False,True],repeat=4):
        for bits in range(64):
            revision=rng.choice([0,1,0x10000000001,0xffffffffffffffff]);next_revision=revision if bits&32 else(revision+1)&0xffffffffffffffff
            old=dict(revision=revision,authenticated=old_auth,party=old_party,busy=bool(bits&1),members=[dict(id=[255,0],locals=2),dict(id='old',locals=0)],friends=ids,clan=ids,invitations=['old'],serial=0xffffffffffffffff,successful=None)
            new=dict(revision=next_revision,authenticated=new_auth,party=new_party,members=[dict(id=[255,0],locals=3),dict(id='new',locals=0)],friends=list(reversed(ids)),clan=list(reversed(ids))if bits&2 else None,invitations=[[255,0]],message=[255,0]if bits&4 else'',search=bool(bits&8),serial=0xffffffffffffffff,successful=False)
            top=templates[bits%len(templates)]|dict(selected=rng.choice([0,1,4,0xffffffff]),pcr=bool(bits&16),entries=[dict(label='same',destination=31,value=0),dict(label='same',destination=31,value=1)])
            frames=[dict(screen=6),dict(screen=186),top]
            if bits%7==0:frames.insert(1,dict(screen=14))
            group=bool(bits&2)
            out.append(dict(before=old,after=new,voice_party=group,voice_wanted=bool(bits&1),voice_client=group or bool(bits&4),queue_client=bool(bits&8),launch=bool(bits&16),frames=frames,
                voice=dict(revision=rng.choice([0,1,0xffffffffffffffff]),joined=True),next_voice=dict(revision=0,joined=True),native=bool(bits&4),sounds=rng.choice([0,15,16,17]),
                signed_in=[128,0]if bits&8 else'',username=[255,0],tags=[dict(key=[128,0],value=[255,0])],appearance=[255,128,0,127,1,2,3,4,5],
                population=[dict(label='same',destination=31,value=1),dict(label=[255,0],destination=31,value=-1),dict(label='same',destination=-82,value=2),dict(label='same',destination=31,value=0)],join_serial=0xffffffffffffffff if bits&16 else 0))
    for serial,reply,busy,party,auth,screen,context in itertools.product([0,1,0x100000001,0xffffffffffffffff],[None,False,True],[False,True],[False,True],[False,True],[31,26],[43,46]):
        out.append(dict(continuation=True,before=dict(authenticated=auth,party=party,busy=busy,serial=serial,successful=reply,message=[255,0]),join_serial=serial,frames=[dict(screen=screen,context=context)],account_client=False))
    for name in ['connect','account-snapshot','voice-snapshot','native-friends']:
        out.append(dict(before=dict(authenticated=True,serial=7,successful=False),after=dict(revision=2,authenticated=True),join_serial=7,reenter_on=name,frames=[dict(screen=31,context=43)],voice_client=True,native=True))
    for account_client,login,identity in itertools.product([False,True],[False,True],[False,True]):
        out.append(dict(account_client=account_client,login_client=login,identity=identity,before=dict(revision=3),after=dict(revision=3),username='',frames=[]))
    for screen,context,native in itertools.product([26,31,33,216,217,236,0xffffffff],[0,1,2,41,42,43,44,45,46],[False,True]):
        out.append(dict(before=dict(authenticated=True,revision=1,friends=ids,clan=ids),after=dict(authenticated=True,revision=2,friends=list(reversed(ids)),clan=list(reversed(ids)),search=True),frames=[dict(screen=screen,context=context,selected=1,entries=[dict(label=[255,0],destination=31,value=0),dict(label='same',destination=31,value=1)])],native=native,population=[dict(label='same',destination=31,value=-1),dict(label='same',destination=31,value=2)]))
    out.append(dict(before=dict(authenticated=True,revision=1),after=dict(authenticated=True,revision=2),frames=[dict(screen=26,context=2,selected=99)],population=[dict(label='partial')],partial_population=True))
    return out


def run(native,cases):
    p=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n'for c in cases),text=True,capture_output=True)
    lines=p.stdout.splitlines()
    if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
    if p.returncode or len(lines)!=len(cases):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(cases),stderr=p.stderr[-3000:],stdout=p.stdout[:1500])))
    return [json.loads(line)for line in lines]


def batches(native,cases):
    for n in range(0,len(cases),16):
        group=cases[n:n+16];yield from zip(group,run(native,group))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    cases=corpus();failures=[];reads=reentries=0
    for index,(case,value)in enumerate(batches(a.native,cases)):
        reads+=len(value['before']['calls']);reentries+=value['rust']['reentries']
        if value['before']!=value['rust']and len(failures)<5:failures.append(dict(index=index,case=case,actual=value))
    selected=[dict(before=dict(authenticated=True,revision=1),after=dict(authenticated=True,revision=2,party=True),voice_wanted=True,voice_client=True,frames=[dict(screen=186)]),
        dict(before=dict(authenticated=True,revision=1,party=True),after=dict(authenticated=True,revision=2),voice_party=True,voice_client=True,voice_wanted=True,frames=[dict(screen=6),dict(screen=14),dict(screen=185,pcr=True)]),
        dict(before=dict(authenticated=True,revision=1),after=dict(authenticated=True,revision=2),signed_in=[255,0],tags=[dict(key=[255,0],value=[128,0])],voice_client=True,native=True,frames=[dict(screen=26,context=0)],population=[dict(label='same')]),
        dict(continuation=True,join_serial=7,before=dict(authenticated=True,serial=7,successful=False),frames=[dict(screen=31,context=43)])]
    faults=[]
    for case,value in zip(selected,run(a.native,selected)):
        for index in range(len(value['before']['calls'])):
            for nonstd in [False,True]:faults.append(case|dict(fail=index,nonstd=nonstd))
    identity=0
    for case,value in batches(a.native,faults):
        identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int'and value['rust'].get('integer')==73))
        if value['before']!=value['rust']and len(failures)<5:failures.append(dict(case=case,actual=value))
    report=dict(passed=not failures,cases=len(cases),native_io_observations=reads,rust_state_reentries=reentries,
        fault_controls=len(faults),preserved_native_exception_identity=identity,differences=failures,native_sha256=hashlib.sha256(a.native.read_bytes()).hexdigest(),scope=__doc__)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
    if failures:print(json.dumps(failures[0])[:7000])
    raise SystemExit(0 if report['passed']else 1)


if __name__=='__main__':main()
