#!/usr/bin/env python3
"""Untouched campaign23FB00/23FF50 with actual declared source banks.

Occupied player/object pools, input edges, clock, node matrices, frame time and
zero-initialized output packet storage
are component inputs. Constructor, target/restriction/fallback/packet owners and
their original dependencies execute without instruction patches or supplied
returns. This does not establish a playable cooperative campaign session.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, PIN
from oracle_mp_death_camera_lifecycle_1_1 import (
    BASE, SIZE, STACK, SP, STOP, POOL, HEADERS, PLAYERS, ROWS, USERS, GAME, CLOCK, RNG, OUT)
from oracle_mp_scope_camera_1_1 import INITIALIZERS


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    image = Xbe(args.source)
    assert hashlib.sha256(image.data).hexdigest() == PIN
    rows, cases = [], []
    specs = [dict(name='automatic_partner', frames=9),
             dict(name='manual_partner', frames=9, cycle=True, seconds=.5),
             dict(name='paused_clock', frames=9, paused=True),
             dict(name='dead_partner_restricts_enemy', frames=3, dead_partner=True, retire=True),
             dict(name='enemy_only', frames=3, occupied=5, retire=True),
             dict(name='all_dead_fallback', frames=3, all_dead=True, retire=True),
             dict(name='all_dead_no_fallback', frames=3, all_dead=True, retire=True, no_fallback=True)]
    for spec in specs:
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0, 0x810000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, SIZE)
        g.mem_map(STACK, 0x10000)
        g.mem_map(STOP, 0x1000)
        w = lambda p, v: g.mem_write(p, struct.pack('<I', v & 0xffffffff))
        h = lambda p, v: g.mem_write(p, struct.pack('<H', v & 0xffff))
        b = lambda p, v: g.mem_write(p, bytes([v & 255]))
        fv = lambda p, v: g.mem_write(p, struct.pack('<f', v))
        vec = lambda p, v: g.mem_write(p, struct.pack('<3f', *v))
        hx = lambda p: bytes(g.mem_read(p, 4))[::-1].hex()
        def call(entry, arguments=(), registers=()):
            w(SP, STOP)
            for i, value in enumerate(arguments):
                w(SP+4+i*4, value)
            for reg, value in ((R.UC_X86_REG_ESP, SP), (R.UC_X86_REG_FPCW, 0x27f),
                               (R.UC_X86_REG_MXCSR, 0x1f80), (R.UC_X86_REG_FPTAG, 0xffff),
                               (R.UC_X86_REG_FPSW, 0), *registers):
                g.reg_write(reg, value)
            g.emu_start(entry, STOP, count=100000)
            assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        for entry in INITIALIZERS:
            call(entry)
        if not rows:
            constants = [0x465ad0,0x4650c0,0x465080,0x46507c,0x4457bc,0x465640,
                         0x46563c,0x465044,0x465068,0x4656a4,0x465030,0x46502c,
                         0x45ac88,0x4650d8,0x46504c,0x44aff4,0x44aff8,0x44aff8]
            rows.append('K '+' '.join(hx(p) for p in constants))
            scope = [0x465ad0,0x4650c4,0x465540,0x4650c4,0x465540,0x46570c,0x465708,
                     0x465030,0x47885c,0x4653b8,0x4701c8,0x46502c,0x465044,0x465060,
                     0x465068,0x46506c,0x465070,0x465078,0x46507c,0x465080,0x465084,
                     0x465218,0x44b020]
            rows.append('G '+' '.join(hx(p) for p in scope)+' '+image.read(0x465130,8)[::-1].hex()+' '+
                        image.read(0x4701d0,12).hex()+' '+image.read(0x4701dc,12).hex()+' '+
                        image.read(0x44b010,24).hex()+' '+' '.join(hx(p) for p in
                        (0x46504c,0x4650e0,0x465090,0x54f214,0x54f218,0x465088)))
            rows.append('H '+' '.join(hx(p) for p in (0x452044,0x5563b0,0x465474))+' 3ca3d70a '+
                        ' '.join(hx(p) for p in (0x46508c,0x44b004))+' '+image.read(0x465968,8)[::-1].hex()+' '+
                        ' '.join(bytes(g.mem_read(int.from_bytes(g.mem_read(p,4),'little'),12)).hex()
                                 for p in (0x46fc48,0x46fc50,0x46fc44)))
        w(0x4e78d0,POOL);w(POOL+0x24,12);w(POOL+0x38,4);w(POOL+0x44,HEADERS)
        targets = []
        for i in range(4):
            body=BASE+0x15000+i*0x1000;typ=6 if i>=2 else 0
            h(HEADERS+i*12,1);b(HEADERS+i*12+3,typ);w(HEADERS+i*12+8,body)
            w(body+0x14,-1);h(body+0x2c,0);b(body+0xaa,typ);h(body+0x116,0x800)
            vec(body+0x828,(float(i*4+1),float(i*2-3),float(i+2)))
            targets.append((0x10000+i,bytes(g.mem_read(body+0x828,12)).hex(),typ,0))
        if spec.get('retire'):
            h(HEADERS,0);targets=targets[1:]
        if spec.get('no_fallback'):
            h(HEADERS+24,0);h(HEADERS+36,0);targets=[t for t in targets if t[0]<0x10002]
        w(0x4f01f4,PLAYERS);w(PLAYERS+0x24,0x21c);w(PLAYERS+0x38,3)
        w(PLAYERS+0x44,ROWS);w(PLAYERS+0x48,PLAYERS+0x100)
        occupied=spec.get('occupied',7);w(PLAYERS+0x100,occupied)
        bank=[]
        for i in range(3):
            current=-1 if i==0 or spec.get('all_dead') or (i==1 and spec.get('dead_partner')) else 0x10001
            previous=0x10000 if i==0 else -1;team=0 if i<2 else 1;row=ROWS+i*0x21c
            h(row,i+1);w(row+0x24,0 if i==0 else -1);w(row+0x2c,current);w(row+0x30,previous);b(row+0xc0,team)
            if occupied&(1<<i):bank.append(((i+1)<<16|i,current&0xffffffff,previous&0xffffffff,team))
        w(0x4f01f0,USERS);w(USERS+12,0x10000);w(0x4edf18,GAME);w(GAME+8,1)
        w(0x518444,CLOCK);b(CLOCK,1);b(CLOCK+1,int(spec.get('paused',False)));h(CLOCK+2,30)
        w(0x4ee9d8,RNG);w(RNG+4,0x12345678);h(0x46fb64,0)
        vec(0x4f125c,(3.,-2.,4.));vec(0x4f125c+0x2c,(0.,0.,1.));fv(0x5563a4,1.2217305)
        camera=BASE+0x14000;command=BASE+0x14100
        call(0x23fb00,(-1,),((R.UC_X86_REG_EAX,camera),(R.UC_X86_REG_EDX,0)))
        seconds=spec.get('seconds',1.);fv(command+8,seconds)
        for frame in range(spec['frames']):
            cycle=bool(spec.get('cycle') and frame==2)
            w(0x4ed79c,1);b(0x4ed789,0);b(0x4ed7ac+0x10,int(cycle))
            before=bytes(g.mem_read(camera,64)).hex()
            g.mem_write(OUT,bytes(0xac))
            call(0x23ff50,(camera,command,OUT))
            after=bytes(g.mem_read(camera,64)).hex();packet=bytes(g.mem_read(OUT,0xac)).hex()
            row=f'P {spec["name"]} {frame} {hx(command+8)} 30 '+bytes(g.mem_read(0x4f125c,12)).hex()+' '+bytes(g.mem_read(0x4f125c+0x2c,12)).hex()
            row+=f' {before} {after} {packet} {int(cycle)} 1 {int(spec.get("paused",False))} '
            # The fallback is read independently by the native BBDA0 binding
            # in the lifecycle fixture. Here its declared output is an API input.
            fallback=0xffffffff if spec.get('no_fallback') else 0x10003
            row+=f'{fallback:08x} {len(bank)} '+ ' '.join(f'{d:08x} {c:08x} {p:08x} {t}' for d,c,p,t in bank)
            row+=f' {len(targets)} '+' '.join(f'{d:08x} {n} {t} {f}' for d,n,t,f in targets)
            rows.append(row)
        cases.append(spec)
    fixture=args.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(source_sha256=PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),cases=cases,
                original_initializers=INITIALIZERS,instruction_patches=[],supplied_dependency_returns=[],
                boundaries=__doc__,live_campaign_proof=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),frames=sum(c['frames'] for c in cases))))

if __name__=='__main__':main()
