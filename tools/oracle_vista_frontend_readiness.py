#!/usr/bin/env python3
"""Execute unchanged616490 and consumed lifecycle/session/player leaves.

External services supply declared query results and owner-held field views.
No runtime session, network readiness, loader completion, audio or UI is made.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_MXCSR
from oracle_vista_lobby_countdown import SOURCE_SHA, BASE, STOP, STACK, make_machine, read32, ret_input, word

SESSION=0x1a00000
VARIANT=0x1a10000
BUILTIN=0x1a11000
PLAYERS=0x1a13000
OUTPUT=0x1a16000

def defaults():
    return dict(active=1,slot=1,current=1,session=5,mode=-1,status=0,progress=0,
                local=0,players=[[0,0,None] for _ in range(16)],variant=None,
                map=[-1,0],builtin=None,customloaded=0,custom=None,
                countdown=-1,host=1,mxcsr=0x1f80,output=True)

def execute(image,q):
    uc=make_machine(image)
    uc.mem_map(SESSION,0x30000)
    esp=STACK+0x8000
    uc.mem_write(esp,struct.pack('<2I',STOP,OUTPUT if q['output'] else 0))
    uc.mem_write(OUTPUT,struct.pack('<I',0xdeadbeef))
    if q['session'] is not None:uc.mem_write(SESSION+0x73a0,struct.pack('<I',word(q['session'])))
    if q['variant'] is not None:
        uc.mem_write(VARIANT+0x44,struct.pack('<I',word(q['variant'][0])))
        uc.mem_write(VARIANT+0x48,bytes([q['variant'][1]]))
    if q['builtin'] is not None:uc.mem_write(BUILTIN+0xc54,bytes(q['builtin']))
    for i,p in enumerate(q['players']):
        if p[2] is not None:uc.mem_write(PLAYERS+i*0x128+0x7c,bytes([p[2]&255]))
    uc.reg_write(UC_X86_REG_ESP,esp)
    uc.reg_write(UC_X86_REG_MXCSR,q['mxcsr'])
    calls=[]
    names={0x6154d8:'lifecycle_context',0x5ad423:'session_admission'}
    inputs={0x5b5038:('lifecycle_active','active'),0x5ad660:('lifecycle_slot','slot'),
            0x615ea9:('mode','mode'),0x5b13c7:('countdown','countdown'),
            0x5ad940:('host_permission','host')}
    def note(name,arg=0):calls.append([name,word(arg)])
    def hook(m,a,n,_):
        s=m.reg_read(UC_X86_REG_ESP)
        if a==STOP:m.emu_stop()
        elif a in names:note(names[a])
        elif a in inputs:
            name,key=inputs[a];note(name);ret_input(m,q[key])
        elif a==0x5ad736:
            note('current_session')
            if q['current']:
                if q['session'] is None:raise ValueError('successful session query requires actual field view')
                m.mem_write(read32(m,s+4),struct.pack('<I',SESSION))
            ret_input(m,q['current'])
        elif a==0x5b1929:
            note('map_readiness');assert read32(m,s+8)==0
            m.mem_write(read32(m,s+4),struct.pack('<I',word(q['progress'])))
            ret_input(m,q['status'])
        elif a==0x5b1913:
            note('local_map_readiness');m.mem_write(read32(m,s+4),struct.pack('<I',0x12345678))
            ret_input(m,q['local'])
        elif a in (0x6156e1,0x615820,0x6157ca):
            i=read32(m,s+4);assert i<16
            if a==0x6156e1:note('player_present',i);v=q['players'][i][0]
            elif a==0x615820:note('player_eligible',i);v=q['players'][i][1]
            else:note('player_configuration',i);v=0 if q['players'][i][2] is None else PLAYERS+i*0x128
            ret_input(m,v)
        elif a==0x5b1313:
            note('variant');ret_input(m,VARIANT if q['variant'] is not None else 0)
        elif a==0x61564e:
            note('map_selection');assert q['map'] is not None
            for i,v in enumerate((-1,q['map'][0],q['map'][1])):
                m.mem_write(read32(m,s+4+i*4),struct.pack('<I',word(v)))
            ret_input(m,1)
        elif a==0x459a1a:
            note('builtin_map_metadata',read32(m,s+4));ret_input(m,BUILTIN if q['builtin'] is not None else 0)
        elif a==0x687ba9:
            dest,value,count=(read32(m,s+4+i*4) for i in range(3))
            assert value==0 and count==0xc54
            note('custom_metadata_clear',count);m.mem_write(dest,bytes(count));ret_input(m,dest)
        elif a==0x459369:
            note('custom_map_metadata',read32(m,s+4))
            if q['customloaded']:
                assert q['custom'] is not None
                m.mem_write(read32(m,s+8)+0xc48,bytes(q['custom']))
            ret_input(m,q['customloaded'])
        elif a==0x6875d3:ret_input(m,m.reg_read(UC_X86_REG_EAX))
    uc.hook_add(UC_HOOK_CODE,hook)
    uc.emu_start(0x616490,STOP+1,count=20000)
    assert uc.reg_read(UC_X86_REG_EIP)==STOP
    return dict(state=uc.reg_read(UC_X86_REG_EAX),progress=read32(uc,OUTPUT) if q['output'] else None,calls=calls)

def player_leaf(image,leaf,query,mask,datum,local,peers,teams):
    uc=make_machine(image);uc.mem_map(SESSION,0x30000)
    esp=STACK+0x8000
    uc.mem_write(esp,struct.pack('<2I',STOP,word(datum)))
    for i,p in enumerate(peers):uc.mem_write(PLAYERS+i*0x128+8,struct.pack('<I',word(p)))
    for i,t in enumerate(teams):uc.mem_write(PLAYERS+i*0x128+0x118,bytes([t&255]))
    uc.reg_write(UC_X86_REG_ESP,esp)
    def hook(m,a,n,_):
        if a==STOP:m.emu_stop()
        elif a==0x5b1bde:
            s=m.reg_read(UC_X86_REG_ESP)
            if query:
                for arg,value in ((2,local),(8,mask),(9,PLAYERS)):
                    p=read32(m,s+arg*4)
                    if p:m.mem_write(p,struct.pack('<I',word(value)))
            ret_input(m,query)
    uc.hook_add(UC_HOOK_CODE,hook)
    uc.emu_start(leaf,STOP+1,count=20000)
    assert uc.reg_read(UC_X86_REG_EIP)==STOP
    value=uc.reg_read(UC_X86_REG_EAX)
    if leaf==0x6157ca:
        return None if value==0 else struct.unpack('<b',bytes(uc.mem_read(value+0x7c,1)))[0]
    return value&255

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'))
    p.add_argument('--fixture',type=Path,default=Path('tests/fixtures/vista_frontend_readiness_original.json'))
    p.add_argument('--report',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/frontend-readiness/original.json'))
    a=p.parse_args();image=a.exe.read_bytes()
    assert hashlib.sha256(image).hexdigest()==SOURCE_SHA
    queries=[]
    def add(label,**change):
        q=defaults();q.update(change);queries.append(dict(label=label,query=q,result=execute(image,q)))
    for active in (0,1,128,255):
        for slot in (0,1,2,3,4,5,6,7,0xffffffff):add(f'context-{active}-{slot}',active=active,slot=slot)
    for state in (0,1,2,3,4,5,6,7,8,9,10,11,0xffffffff):add(f'session-{state}',session=state)
    add('missing-session',current=0,session=None)
    for status in (0,1,2,3,4,5,6,0xffffffff):
        for local in (-1,0,1,2):
            for output in (False,True):add(f'map-{status}-{local}-{output}',status=status,local=local,output=output)
    for status in (0,2,3,4,5):
        for progress in (-0x80000000,-1,0,1,9,10,33,99,100,101,0x7fffffff):
            for mxcsr in (0x1f80,0x3f80,0x5f80,0x7f80):
                add(f'progress-{status}-{progress}-{mxcsr}',status=status,progress=progress,mxcsr=mxcsr)
    for mode in (-1,0,1,2,3,4,5):
        for n,eligible in ((0,0),(1,1),(2,0),(2,1),(2,2),(3,2),(3,3),(16,16)):
            players=[[1 if i<n else 0,1 if i<eligible else 0,None] for i in range(16)]
            add(f'paired-{mode}-{n}-{eligible}',mode=mode,players=players)
    for status in (0,2,3,4):
        for countdown in (-0x80000000,-1,0,1,10,0x7fffffff):
            for host in (0,1,128,255):add(f'final-{status}-{countdown}-{host}',status=status,countdown=countdown,host=host)
    team_cases=([None],[-128],[-1],[0],[15],[16],[127],[0,0],[0,1],[0,1,2],list(range(16)))
    for custom in (False,True):
        for engine in (-1,0,1,9,10):
            for capacity in (0,1,2,16,255):
                for teams in team_cases:
                    players=[[1,0,t] if i<len(teams) else [0,0,None] for i,t in enumerate(list(teams)+[None]*(16-len(teams)))]
                    metadata=[capacity]*10
                    add(f'teams-{custom}-{engine}-{capacity}-{len(queries)}',variant=[engine,1],map=[17,123 if custom else 0],
                        builtin=None if custom else metadata,customloaded=int(custom),custom=metadata if custom else None,players=players)
    for flags in (0,2,128,255):add(f'variant-flags-{flags}',variant=[1,flags])
    for custom in (False,True):add(f'absent-metadata-{custom}',variant=[1,1],map=[17,123 if custom else 0],players=[[1,0,0]]+[[0,0,None]]*15)
    rng=random.Random(0x616490)
    for i in range(120):
        players=[[rng.choice((0,1,128,255)),rng.choice((0,1,128,255)),rng.choice((None,-128,-1,0,1,2,15,16,127))] for _ in range(16)]
        add(f'mixed-{i}',mode=rng.randrange(-1,6),status=rng.randrange(7),progress=rng.randrange(-100,200),
            countdown=rng.choice((-1,0,1)),host=rng.choice((0,1,128,255)),players=players,
            variant=[rng.randrange(-1,11),rng.choice((0,1,2,128,255))],builtin=[rng.randrange(17) for _ in range(10)])
    leaves=[]
    peers=[i%4 for i in range(64)];teams=[i%32-8 for i in range(64)]
    for query in (0,1,128,255):
        for mask in (0,1,0x8000,0xffff,0xffffffff):
            for datum in (0,1,15,16,31,32,47,63,0x12340000,0x1234000f,0xffffffff):
                # Unsafe huge record reads are excluded only when source would consume them.
                index=datum&0xffff
                leafids=[0x6156e1]+([0x615820,0x6157ca] if index<len(peers) or not query or not(mask&(1<<(index&31))) else [])
                for leaf in leafids:
                    leaves.append(dict(leaf=leaf,query=query,mask=mask,datum=datum,local=2,peers=peers,teams=teams,
                        result=player_leaf(image,leaf,query,mask,datum,2,peers,teams)))
    fixture=dict(version='H2V_FRONTEND_READINESS_ORIGINAL_V1',source_sha256=SOURCE_SHA,cases=queries,player_leaves=leaves)
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
    ranges={'producer':(0x616490,0x61678c),'lifecycle_context':(0x6154d8,0x61553c),
            'session_admission':(0x5ad423,0x5ad457),'player_present':(0x6156e1,0x61573d),
            'player_eligible':(0x615820,0x615884),'player_configuration':(0x6157ca,0x615820)}
    report=dict(version=fixture['version'],source=str(a.exe.resolve()),source_sha256=SOURCE_SHA,
        unchanged_instructions=True,case_count=len(queries),player_leaf_count=len(leaves),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        ranges={k:dict(start=f'{x:08x}',end_exclusive=f'{z:08x}',sha256=hashlib.sha256(image[x-BASE:z-BASE]).hexdigest()) for k,(x,z) in ranges.items()},
        boundaries={'actual_executed':['616490','6154D8','5AD423','6156E1 player controls','615820 player controls','6157CA player controls'],
          'supplied_results':['5B5038','5AD660','5AD736 field view','615EA9','5B1929','5B1913','5B1313 field view','61564E outputs','459A1A metadata','459369 metadata','5B13C7','5AD940','5B1BDE player control bank'],
          'source_joins':'Core player calls receive explicit declared AL/pointer fields; separate controls execute each actual player helper against declared membership masks and banks.',
          'not_claimed':'Actual session ownership, map loading, readiness publication, network delivery, audio, UI rendering, or complete runtime transition.'})
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(queries),player_leaves=len(leaves),fixture=str(a.fixture),fixture_sha256=report['fixture_sha256'])))
if __name__=='__main__':main()
