#!/usr/bin/env python3
"""Unmodified campaign cooperative respawn scheduler component experiments.

Raw bank, cinematic, difficulty, source override, clock and action/AI records
are declared API inputs. Every nested original safety predicate runs; no
instruction patches or successful dependency returns are supplied. Player
birth/placement is deliberately outside these cases and remains a live owner
requirement. Native comparison treats named safety calls as explicit services.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, PIN
from oracle_mp_death_camera_lifecycle_1_1 import (
    BASE, SIZE, STACK, SP, STOP, POOL, HEADERS, UNIT, VEHICLE, PLAYERS, ROWS, USERS, GAME, CLOCK)
from export_mp_ragdoll_owners import spans


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(args.source);assert hashlib.sha256(image.data).hexdigest()==PIN
    specs=[]
    for mode in (1,2):
        for difficulty in range(4):
            for override in (0,1):
                specs.append(dict(name=f'allow_{mode}_{difficulty}_{override}',entry=0x13a010,
                                  game_type=mode,difficulty=difficulty,override=override))
    for pending in (0,1):
        for rate in (30,60):
            for rounding in range(4):
                specs.append(dict(name=f'start_{pending}_{rate}_{rounding}',entry=0x14e4c0,
                                  pending=pending,rate=rate,rounding=rounding))
    for name,extra in [('inactive',dict(pending=0,timer=2)),('delay',dict(timer=2)),
                       ('last_delay_tick',dict(timer=1)),('cinematic',dict(cinematic=1,timer=2)),
                       ('reverting',dict(reverting=1)),('action_unsafe',dict(action_unsafe=True)),
                       ('combat_unsafe',dict(combat_unsafe=True)),('biped_unsafe',dict(biped_unsafe=True)),
                       ('vehicle_unsafe',dict(vehicle_unsafe=True)),('no_survivor',dict(no_survivor=True)),
                       ('bypass',dict(bypass=1,reverting=1,timer=2,action_unsafe=True)),
                       ('safe',dict()),('retired_survivor',dict(retired=True))]:
        specs.append(dict(name=name,entry=0x14e510,**extra))
    cases=[]
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,SIZE);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
        w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
        h=lambda p,v:g.mem_write(p,struct.pack('<H',v&0xffff))
        b=lambda p,v:g.mem_write(p,bytes([v&255]))
        fv=lambda p,v:g.mem_write(p,struct.pack('<f',v))
        cinematic=BASE+0x14000;ai_pool=BASE+0x15000;ai_rows=BASE+0x15100
        ai_links=BASE+0x16000;ai_link_rows=BASE+0x16100;actors=BASE+0x17000;actor_rows=BASE+0x17100
        w(0x4f01f0,USERS);w(0x4edf18,GAME);w(GAME+8,spec.get('game_type',1))
        h(GAME+0x132,spec.get('difficulty',1));b(0x4fcdbf,spec.get('override',0))
        b(GAME+0x1121,spec.get('reverting',0));w(0x518444,CLOCK);h(CLOCK+2,spec.get('rate',30))
        w(0x518440,cinematic);b(cinematic+5,spec.get('cinematic',0))
        h(USERS+0x9c,0x7777);b(USERS+0x9e,spec.get('pending',1))
        b(USERS+0x9f,spec.get('bypass',0));w(USERS+0xa0,spec.get('timer',0))
        w(0x4e78d0,POOL);w(POOL+0x24,12);w(POOL+0x38,2);w(POOL+0x44,HEADERS)
        for i,body in enumerate((UNIT,VEHICLE)):
            h(HEADERS+i*12,1);b(HEADERS+i*12+2,1);b(HEADERS+i*12+3,i);w(HEADERS+i*12+8,body)
            w(body+0x14,-1);b(body+0xaa,i);h(body+0x12a,0x500);h(body+0x346,0x600)
            w(body+0x500,-1);w(body+0x568,-1);h(body+0x506,-1)
            for k,value in enumerate((float(i+1),float(i+2),float(i+3))):fv(body+0x30+k*4,value)
        if spec.get('action_unsafe'):w(UNIT+0x604,1<<22)
        if spec.get('biped_unsafe'):
            b(UNIT+0x348,1);b(UNIT+0x399,30);b(UNIT+0x3dc,1)
        if spec.get('vehicle_unsafe'):w(UNIT+0x14,0x10001);b(VEHICLE+0x34c,1)
        if spec.get('retired'):h(HEADERS,0)
        w(0x4f01f4,PLAYERS);w(PLAYERS+0x24,0x21c);w(PLAYERS+0x38,1)
        w(PLAYERS+0x44,ROWS);w(PLAYERS+0x48,PLAYERS+0x100);w(PLAYERS+0x100,1)
        h(ROWS,1);h(ROWS+2,1);w(ROWS+0x2c,-1 if spec.get('no_survivor') else 0x10000)
        # Empty genuine AI iterator, or one declared live close threat. BACC0
        # consumes the actual object's parent-free center; no AI return is set.
        w(0x509bfc,ai_pool);w(ai_pool+0x24,0x34);w(ai_pool+0x38,int(bool(spec.get('combat_unsafe'))))
        w(ai_pool+0x44,ai_rows);w(ai_pool+0x48,ai_pool+0x80);w(ai_pool+0x80,1)
        if spec.get('combat_unsafe'):
            w(0x509c00,ai_links);w(ai_links+0x44,ai_link_rows)
            w(0x4fcdc8,actors);w(actors+0x44,actor_rows)
            h(ai_rows,1);w(ai_rows+4,0x10000);w(ai_rows+8,0x10000);w(ai_rows+0x14,-1)
            b(ai_link_rows+0x25,1);b(ai_link_rows+0x23,1);w(ai_link_rows+8,0x10000)
            w(UNIT+0x13c,0x10000);b(actor_rows+9,1);b(actor_rows+7,1);w(actor_rows+0x338,-1)
        w(SP,STOP)
        rounding=spec.get('rounding',0);fp=(0x27f+(rounding<<10),0x1f80+(rounding<<13))
        for register,value in ((R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_FPCW,fp[0]),
                               (R.UC_X86_REG_MXCSR,fp[1]),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)):
            g.reg_write(register,value)
        initial,current,writes,calls={},{},[],[];watch=[];last=0
        def touch(p,n):
            for i,v in enumerate(bytes(g.mem_read(p,n))):
                if p+i not in current:initial[p+i]=v;current[p+i]=v
                else:assert current[p+i]==v
        def read(_g,_access,p,n,_value,_data):touch(p,n)
        def write(_g,_access,p,n,value,_data):
            touch(p,n);raw=(value&((1<<(n*8))-1)).to_bytes(n,'little')
            if not STACK<=p<STACK+0x10000:
                writes.append(dict(instruction=last,address=p,before=bytes(g.mem_read(p,n)).hex(),written=raw.hex()))
            current.update({p+i:v for i,v in enumerate(raw)})
        def code(_g,p,_size,_data):
            nonlocal last
            last=p
            for ret,row in watch[:]:
                if p==ret:row['result']=bool(g.reg_read(R.UC_X86_REG_EAX)&255);watch.remove((ret,row))
            if p in (0xcd5c0,0x1ce060,0xe5680):
                sp=g.reg_read(R.UC_X86_REG_ESP);row=dict(entry=p,unit=g.reg_read(R.UC_X86_REG_EAX) if p==0xe5680 else None)
                calls.append(row);watch.append((int.from_bytes(g.mem_read(sp,4),'little'),row))
            assert p not in (0x1509c0,0x1500b0),'Birth/placement is not part of this component fixture'
        g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_CODE,code)
        g.emu_start(spec['entry'],STOP,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        assert not watch
        cases.append(dict(spec=spec,context=[USERS,GAME,CLOCK,PLAYERS,POOL,cinematic],fp=fp,
                          delay_bits=int.from_bytes(image.read(0x465044,4),'little'),
                          initial=[dict(address=p,bytes=raw.hex()) for p,raw in spans(initial)],
                          writes=writes,calls=calls,result=bool(g.reg_read(R.UC_X86_REG_EAX)&255)))
    fixture=args.output/'original.json'
    fixture.write_text(json.dumps(dict(source_sha256=PIN,cases=cases),indent=2)+'\n')
    report=dict(source_sha256=PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),cases=len(cases),
                stores=sum(len(c['writes']) for c in cases),instruction_patches=[],supplied_dependency_returns=[],
                boundaries=__doc__,live_cooperative_campaign=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
