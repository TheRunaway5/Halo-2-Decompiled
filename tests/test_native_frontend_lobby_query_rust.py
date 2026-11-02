#!/usr/bin/env python3
"""Whole captured modern lobby query versus the exact production Rust/CXX IO.

Native cache, gameplay, profile and resource observations are controlled. The
complete view, native map binding, all nine appearance bytes, ordered reads,
real Rust reentry and native exception identity are compared. Original Live
producers, installed input and historical whole-system parity remain separate.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess


def cases():
    rng=random.Random(0x19a2ce)
    appearance=[255,128,0,127,254,1,9,255,73]
    member=lambda i:dict(handle=[255,i%256,0],name=[128,i%256,0,255],self=i%3==0,appearance=appearance)if i%2==0 else dict(handle=[255,i%256,0],name='remote '+str(i),self=i%3==0)
    physical=[dict(handle=[255,0],leader=False),dict(handle=[255,0],leader=True)]
    for bits,count in itertools.product(range(128),[0,1,16,19]):
        yield dict(mode=3,party=bool(bits&1),leader=bool(bits&2),stats=bool(bits&4),voice_party=bool(bits&8),joined=bool(bits&16),transmitting=bool(bits&32),microphone_muted=bool(bits&64),
            physical=physical,requester=[255,0],party_privacy=rng.choice([0,1,2,65536,0xffffffff]),prior_count=rng.choice([0,1,0x80000000,0xffffffff]),seconds=rng.choice([-1,0,9]),
            start_error='blocked'if rng.randrange(2)else'',member_reads=[[member(i)for i in range(count+3)],[member(i)for i in range(count)]],local_reads=[[0,1]],talking=[[255,2,0]],muted=[[255,4,0]])
    for mode,engine,countdown,host in itertools.product([1,2,4,-1,0,5,65539],list(range(7))+[0xffffffff],[None,-1,0],[None,False,True]):
        maps=[dict(scenario='folder/lockout',name=[255,0],counts=[rng.choice([0,1,2,8,16,255])for _ in range(16)])]
        campaign=[dict(scenario=[255,47,0],campaign_id=-1),dict(scenario='levels\\00a_introduction',campaign_id=19),dict(scenario='00a_introduction',campaign_id=20)]
        n=rng.choice([0,1,2,17]);players=[dict(name=[255,i,0],session_index=rng.choice([-1,i,100+i,0x7fffffff]),team=rng.choice([-2,-1,0,1,7,8]),rank=rng.choice([-128,-1,50,0x7fffffff]),member_count=rng.choice([-1,1,4,8]),flag0=bool(i%2),query=bool(i%3),appearance=appearance,appearance_available=bool(i%2))for i in range(n)]
        yield dict(mode=mode,engine=engine,countdown=countdown,system_host=host,machines=rng.choice([0,1,17,0xffffffff]),system_players=players,
            maps=maps,campaign_maps=campaign,level=rng.choice([0,0,0xffffffff]),map=rng.choice([0,0,0xffffffff]),protocol=rng.choice([-1,0,1,2,0x7fffffff]),
            profile=[255,0]if rng.randrange(2)else'',profile_appearance=appearance,privacy=rng.choice([-1,0,2,0x7fffffff]),difficulty=rng.choice([0,3,0xffffffff]),team=rng.choice([-1,0,7,0x7fffffff]),
            local_reads=[[0,1],rng.choice([[],[-1],[0,1],[-1,15,20]])],indices=[-1,0,0x7fffffff,-2147483648],
            edit=dict(valid=False,team=-1),edits=[dict(valid=bool(rng.randrange(2)),team=rng.choice([-32768,-1,0,7,32767]))for _ in range(4)],
            values=[[13,rng.choice([0,1,-1])],[91,rng.choice([0,1,-2147483648])],[90,123]],custom=bool(rng.randrange(2)),defusal=bool(rng.randrange(2)),capacity=-2147483648,separator=[255,0,47])
    for level,maps in itertools.product([0,14,15,0xffffffff],[[],[dict(scenario='not a match')],[dict(scenario='00a_introduction')],[dict(scenario='a/b\\00a_introduction'),dict(scenario='00a_introduction')]]):
        yield dict(mode=4,engine=0,level=level,campaign_maps=maps,local_reads=[[0],[0]],profile='profile')
    for missing,custom,profile in itertools.product([False,True],[False,True],['',[255,0]]):
        yield dict(mode=1,engine=0,missing_screen=missing,custom=custom,profile=profile,local_reads=[[0],[0]],values=[[91,1],[13,1]],maps=[dict(counts=[0,0,2])])
    for mode in [1,2,4]:
        for countdown in [None,-2147483648,-1,0,2147483647]:
            yield dict(mode=mode,countdown=countdown,system_host=True,maps=[{}],campaign_maps=[],local_reads=[[0],[-1]],engine=0,values=[[91,1]])


def run(native,corpus):
    p=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n'for c in corpus),text=True,capture_output=True)
    lines=p.stdout.splitlines()
    if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
    if p.returncode or len(lines)!=len(corpus):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(corpus),stderr=p.stderr[-2500:],stdout=p.stdout[:1000])))
    return [json.loads(line)for line in lines]


def batches(native,corpus):
    for n in range(0,len(corpus),32):
        group=corpus[n:n+32]
        yield from zip(group,run(native,group))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
    corpus=list(cases());failures=[];observations=reentries=0
    for index,(case,value)in enumerate(batches(args.native,corpus)):
        observations+=len(value['before']['calls']);reentries+=value['rust']['reentries']
        if value['before']!=value['rust']and len(failures)<8:failures.append(dict(index=index,case=case,actual=value))
    local=dict(mode=1,profile=[255,0],profile_appearance=[255,0,128,127,1,2,3,4,5],custom=True,capacity=-2147483648,maps=[dict(counts=[0,0,2])],local_reads=[[0],[0]],values=[[91,1],[13,1]])
    selected=[dict(mode=3,party=True,stats=True,prior_count=0xffffffff,member_reads=[[],[]]),dict(mode=3,party=False,member_reads=[[],[]],local_reads=[[0]]),local,
        dict(mode=4,profile='profile',campaign_maps=[dict(scenario='unmatched'),dict(scenario='00a_introduction',campaign_id=19)],local_reads=[[0],[0]]),local|dict(mode=2,system_host=False)]
    faults=[]
    for case,value in zip(selected,run(args.native,selected)):
        for fail in range(len(value['before']['calls'])):
            for nonstd in [False,True]:faults.append(case|dict(fail=fail,nonstd=nonstd))
    identity=0
    for case,value in batches(args.native,faults):
        identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int'and value['rust'].get('integer')==73))
        if value['before']!=value['rust']and len(failures)<8:failures.append(dict(case=case,actual=value))
    fixture_path=Path(__file__).parent/'fixtures/frontend_lobby_signed_team_original.json'
    original=json.loads(fixture_path.read_text());original_cases=[]
    for row in original['cases']:
        original_cases.append(dict(mode=1,rust_only=True,engine=0,maps=[dict(counts=[0,0,row['maximum']])],local_reads=[[row['team']],[row['team']]],values=[[91,1]]))
    for row,(case,value)in zip(original['cases'],batches(args.native,original_cases)):
        actual=value['rust'].get('view',{}).get('input',[None]*9)[8]
        if actual!=row['status']and len(failures)<8:failures.append(dict(original_team=row['team'],expected=row['status'],actual=value))
    report=dict(passed=not failures,cases=len(corpus),native_io_observations=observations,rust_reentry_observations=reentries,
        fault_controls=len(faults),preserved_native_exception_identity=identity,original_signed_team_controls=len(original_cases),original_signed_team_fixture_sha256=hashlib.sha256(fixture_path.read_bytes()).hexdigest(),differences=failures,native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),scope=__doc__,
        undefined_native_shift_inputs_excluded='Shifts of32 or more are excluded only from the undefined captured C++ comparison. Nine separately executed original signed-team controls qualify the Rust bound without invoking that C++ behavior.')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
    if failures:print(json.dumps(failures[0])[:6000])
    raise SystemExit(0 if report['passed']else 1)


if __name__=='__main__':main()
