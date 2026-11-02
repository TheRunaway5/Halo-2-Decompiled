#!/usr/bin/env python3
"""Captured modern party menus/text versus the actual Rust/CXX view owner.

Real native frame/account/member types and the literal native adapter operate
over controlled cached facts, resource reads, view queries and exceptions.
Frame snapshots at every observable read preserve partial-mutation evidence.
Original Live/locale/identity producers and installed input are separate gates.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import subprocess


def cases():
    rng=random.Random(0x250fb3)
    members=[dict(handle=[0xff,0],name=[0xff,0,0x80],leader=True),dict(handle='remote',name='Same name'),dict(handle='self',name='Same name',self=True)]
    projected=[dict(name='primary',controller=-1),dict(name='signed local',controller=2),dict(name='guest',controller=-2)]
    invites=[dict(id=[0xff,0],name=[0xfe,0]),dict(id=[0xff,0],name='second duplicate')]
    out=[]
    for screen,context,mask,original in itertools.product([14,26,216,31,9,0xffffffff],[0,2,42,45],range(32),[False,True]):
        out.append(dict(screen=screen,context=context,party=bool(mask&1),leader=bool(mask&2),clan=bool(mask&4),
            voice_party=bool(mask&8),voice_client=bool(mask&16),joined=bool(rng.randrange(2)),connecting=bool(rng.randrange(2)),
            microphone_muted=bool(rng.randrange(2)),original=original,role=rng.choice([0,1,2,0x10000,0xffffffff]),
            members=members,projected=projected,outgoing=invites,invitations=invites,target=rng.choice([[0xff,0],'remote','self','missing']),
            kind=rng.randrange(3),tab=rng.randrange(2),muted=[[0xff,0],'remote'],friends=[
                dict(name='not a friend',state='request',presence='online'),dict(name=[0xff,0],state='friend',presence='playing'),
                dict(name='offline',state='friend',presence='offline'),dict(name='unknown',state='friend',presence='OTHER')],
            clan_members=[dict(name=str(p),presence=p,self=p==1)for p in [0,1,2,3,258,0xffffffff]],
            entries=[]if rng.randrange(2)else[dict(label=[0xff,0],destination=-82,value=-1)],friends_message=[0xff,0,0x80]))
    for screen,index,mask in itertools.product([14,26,31,9],list(range(0,12))+[43,44,45,47,48,0xffffffff],range(16)):
        out.append(dict(operation='text',screen=screen,context=rng.choice([0,2,42,45]),index=index,party=bool(mask&1),
            view=bool(mask&2),original=bool(mask&4),stats=bool(mask&8),seconds=rng.choice([-0x80000000,-1,0,3,0x7fffffff]),
            matchmaking=bool(rng.randrange(2)),countdown_supported=bool(rng.randrange(2)),selected=rng.choice(['',[0xff,0]]),
            party_message=rng.choice(['',[0xff,0]]),account_message=[0xff,0,0x80],description=[0xfd,0,0x80],
            protocol=rng.choice([0,1,2,3,0x7fffffff]),machines=rng.choice([-0x80000000,-1,0,1,2,0x7fffffff]),
            session=rng.choice([-1,0,1,2,3,4,5,6,7]),previous=rng.choice([-1,0,1,2,3]),privacy=rng.choice([-1,0,1,2,0x7fffffff]),
            status=rng.choice(list(range(-1,28))),host=bool(rng.randrange(2)),delayed=rng.choice([-1,0,0x7fffffff]),
            cancelled=bool(rng.randrange(2)),loading=bool(rng.randrange(2)),editing=bool(rng.randrange(2)),
            team_count=rng.choice([-1,0,1,2,8,9,0x7fffffff]),players=rng.choice([-1,0,1,16,0x7fffffff]),maximum=rng.choice([0,2,16]),
            overflow=rng.getrandbits(32),members=members,projected=projected,voice_party=True,joined=True,microphone_muted=bool(rng.randrange(2))))
    for original,party,buttons,panes in itertools.product([False,True],[False,True],range(3),range(4)):
        out.append(dict(screen=14,original=original,party=party,buttons=buttons,panes=panes,entries=[dict(label='old',destination=19,value=1)]))
    for screen,operation,index in itertools.product([14,26],['populate','text'],[0,1,43,45,47,48,0xffffffff]):
        out.append(dict(screen=screen,operation=operation,index=index,missing14=True,missing26=True,party=True))
        out.append(dict(screen=screen,operation=operation,index=index,panes=0,party=True))
    for original,index in itertools.product([False,True],[1,3,43,44,48]):
        out.append(dict(operation='text',screen=14,index=index,original=original,texts=1,party=True,members=members))
    for kind,target in itertools.product(range(3),[[0xff,0],'missing','remote','self']):
        out.append(dict(screen=31,party=True,leader=True,kind=kind,target=target,members=members,outgoing=invites,invitations=invites,
                        resources={},entries=[dict(label='retained',destination=19,value=-2147483648)]))
    # First raw token replacement and distinct locale/expansion namespaces.
    for original in [False,True]:
        key='original-text:14:2:43'if original else'locale:14:'+str(0x60000000+43)
        out.append(dict(operation='text',screen=14,index=43,original=original,party=True,members=members,
                        resources={key:[0xee,0x90,0x96,47,0xee,0x90,0x96]}))
    for tab,clan,retained in itertools.product([0,1],[False,True],[False,True]):
        out.append(dict(screen=31,context=45,tab=tab,clan=clan,friends_message=[0xff,0],
            clan_members=[dict(name='offline',presence=1),dict(name='self',presence=2,self=True)],
            entries=[dict(label='old',destination=19,value=1)]if retained else[]))
    for session,previous in [(-1,9),(6,0)]:
        out.append(dict(operation='text',screen=14,index=0,view=True,seconds=0,status=-1,session=session,previous=previous,
            matchmaking=True,party=True,countdown_supported=False,selected='ranked'))
    out += [dict(screen=14,party=True,original=True,missing_original=True),dict(operation='text',screen=14,index=1,original=True,missing_original=True)]
    return out


def run(native,corpus):
    p=subprocess.run([str(native)],input=''.join(json.dumps(c)+'\n'for c in corpus),text=True,capture_output=True)
    lines=p.stdout.splitlines()
    if lines[:1]==['Shutdown handler: initialize.']and lines[-1:]==['Shutdown handler: cleanup.']:lines=lines[1:-1]
    if p.returncode or len(lines)!=len(corpus):raise RuntimeError(json.dumps(dict(exit_code=p.returncode,actual=len(lines),expected=len(corpus),stderr=p.stderr[-3000:],stdout=p.stdout[:1000])))
    return [json.loads(line)for line in lines]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('native',type=Path);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
    corpus=cases();answers=run(args.native,corpus);failures=[];observations=reentries=0
    for index,(case,value)in enumerate(zip(corpus,answers)):
        observations+=len(value['before']['calls']);reentries+=value['rust']['reentries']
        if value['before']!=value['rust']and len(failures)<8:failures.append(dict(index=index,case=case,actual=value))
    selected=[dict(screen=14,party=True,original=o,entries=[dict(label='old',destination=19,value=1)])for o in [False,True]]
    selected += [dict(screen=26,party=True,projected=[dict(name='local',controller=-1)]),dict(operation='text',screen=14,index=0,seconds=0,view=True),
        dict(operation='text',screen=14,index=0,seconds=-1,matchmaking=True,party=True,countdown_supported=False,selected='ranked'),
        dict(operation='text',screen=14,index=43,party=True,members=[dict(name='leader',leader=True)]),
        dict(operation='text',screen=14,index=45,stats=True),dict(operation='text',screen=14,index=47,view=True,protocol=2,machines=1),
        dict(operation='text',screen=26,index=4,party=True,projected=[dict(name='member')])]
    faults=[]
    for case,value in zip(selected,run(args.native,selected)):
        for fail in range(len(value['before']['calls'])):
            for nonstd in [False,True]:faults.append(case|dict(fail=fail,nonstd=nonstd))
    identity=0
    for case,value in zip(faults,run(args.native,faults)):
        identity+=int(value['rust'].get('same_object',value['rust'].get('error')=='int'and value['rust'].get('integer')==73))
        if value['before']!=value['rust']and len(failures)<8:failures.append(dict(case=case,actual=value))
    report=dict(passed=not failures,cases=len(corpus),native_io_observations=observations,frame_snapshots_at_reads=observations,
        same_controller_reentry_observations=reentries,fault_controls=len(faults),preserved_native_exception_identity=identity,differences=failures,
        native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),scope=__doc__)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='differences'}))
    if failures:print(json.dumps(failures[0])[:6500])
    raise SystemExit(0 if report['passed']else 1)


if __name__=='__main__':main()
