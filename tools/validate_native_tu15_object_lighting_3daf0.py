#!/usr/bin/env python3
"""Unchanged physical 3DAF0 object lighting versus the native 3DAF0 tree.

The whole original 3DAF0 runs on physical i386 with its genuine 3DDC0
(16E6A0/16EA60/2C390/3DD00/BCD80/1349E0), 3DFD0 (BC200/1D8C80/24570/3EA20/
BCDF0/3E590/30BD0/32CCD8/3E4D0/1346C0) and D65C0 (D5D30/11A360/13DE70)
children; no instruction is patched and no callee outcome is supplied. The
D3D90 collision/lightmap query owner runs its own unavailable-bank prefix
(the cases decline its gates); its live branches are covered in the game by
HALO2_OBJECT_LIGHTING_ORACLE. Cases cover default/probe banks, root and child
objects, record reuse/allocation/eviction and color publication, the query
cadence for every metric band, near skips, frozen roots, interpolation
(speed bands, rotation and component steps, machine gate) and snaps, the
defined and undefined-variable conversions, authored scale remaps, special
floats and the MXCSR rounding/DAZ/FTZ profiles. Every non-stack byte, EAX
and the x87 CW/SW and MXCSR words are compared.
Use run_test.py; the ELFs and raw banks are scratch-owned.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
SP,STOP=0x62018000,0x63000000
# Original pages: .rdata constants (copied), .data initial values (copied),
# BSS globals and the declared heap/stack.
SECTIONS={'r44a':(0x44a000,0x1000,True),'r450':(0x450000,0x1000,True),'r456':(0x456000,0x1000,True),
    'r46a':(0x46a000,0x1000,True),'d473':(0x473000,0x2000,True),'d48d':(0x48d000,0x1000,True),'units':(0x4c6000,0x2000,False),
    'objects':(0x4ea000,0x3000,False),'tags':(0x4f0000,0x1000,False),'cache':(0x515000,0x1000,False),
    'physics':(0x52b000,0x1000,False),'statics':(0x56e000,0x1000,False),'heap':(0x1000000,0x10000,False),
    'stack':(0x62000000,0x20000,False)}
ENTRIES=(0x3daf0,0xbcfb0,0x3ddc0,0x16e6a0,0x16ea60,0x2c390,0x3dd00,0xbcd80,0x1349e0,0x3dfd0,0xd3d90,0x24570,
    0x3ea20,0xbc200,0x1d8c80,0x1d8d80,0xbcdf0,0x3e590,0x30bd0,0x32ccd8,0x33210c,0x32ccf5,0x332122,0x33212f,
    0x3e4d0,0x1346c0,0xd65c0,0xd5d30,0x11a360,0x13de70)
H=0x1000000
OBJPOOL,OBJROWS,BODIES,TAGROWS,DEFS=H,H+0x100,H+0x1000,H+0x2000,H+0x2400
CACHEPOOL,BITMAP,RECORDS=H+0x3000,H+0x3100,H+0x4000
LIGHTING,FALLBACK,LIGHTDEF,VARIABLES,BLOBS=H+0x6000,H+0x6400,H+0x6800,H+0x6900,H+0x7000
BSP,PROBES,DEFAULTS,PHYSPOOL,PHYSROWS,RIGIDS,RIGID,HAVOK=H+0x8000,H+0x8400,H+0x9000,H+0xa000,H+0xa100,H+0xa800,H+0xb000,H+0xb800
OBJECTS,RECORD_COUNT=8,16
SPECIAL=(0,0x80000000,1,0x80000001,0x7fffff,0x800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345)
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


class Case:
    def __init__(self,image,n,name):
        self.id=n;self.name=name;self.banks={}
        for key,(at,size,copy) in SECTIONS.items():
            self.banks[at]=bytearray(image.read(at,size)) if copy else bytearray(size)
    def put(self,at,value,size=4):
        for base,data in self.banks.items():
            if base<=at and at+size<=base+len(data):
                data[at-base:at-base+size]=int(value&((1<<(8*size))-1)).to_bytes(size,'little');return
        raise ValueError('Unmapped lighting input '+hex(at))
    def get(self,at,size=4):
        for base,data in self.banks.items():
            if base<=at and at+size<=base+len(data):return int.from_bytes(data[at-base:at-base+size],'little')
        raise ValueError('Unmapped lighting read '+hex(at))
    def row(self,object_,metric,mxcsr):
        return dict(id=self.id,name=self.name,entry=0x3daf0,stop_pc=STOP,max_steps=100000,gp=[object_,0,0,0,SP,0,0,0],
            matrix=metric,source=0,before_fp=[0x27f,0,mxcsr],live_x87=[],
            banks=[dict(address=a,before=bytes(d).hex()) for a,d in self.banks.items()])


def vector(rng,special=False,scale=1.0):
    v=[bits(rng.uniform(-scale,scale)) for _ in range(3)]
    if special and rng.random()<.35:v[rng.randrange(3)]=rng.choice(SPECIAL)
    return v


def color(rng):return rng.getrandbits(24)|(rng.getrandbits(8)<<24)


def build(image,rng,n,special):
    c=Case(image,n,f'lighting-{n}');put=c.put
    put(0x4ecbd0,OBJPOOL);put(OBJPOOL+0x24,12);put(OBJPOOL+0x38,OBJECTS);put(OBJPOOL+0x44,OBJROWS)
    put(0x4f0414,TAGROWS);put(0x515f14,CACHEPOOL);frame=rng.randrange(100,5000);put(0x4c6854,frame)
    put(0x4ecc20,LIGHTING);put(0x4ecc1c,FALLBACK);put(0x4ecc18,BSP);put(BSP+0x230,PROBES);put(0x4eabc4,DEFAULTS)
    put(0x52be5c,PHYSPOOL);put(PHYSPOOL+0x44,PHYSROWS)
    put(0x4739c4,1 if rng.random()<.8 else 0,1)
    # Gate the D3D90 query off in one of its four declined ways.
    gate=rng.randrange(4)
    if gate==1:put(0x4ecc14,H+0xc000);put(H+0xc080,0)
    elif gate==2:put(0x4ecc14,H+0xc000);put(H+0xc080,1);put(H+0xc084,H+0xc100);put(H+0xc11c,0xffffffff)
    elif gate==3:put(0x4ecc14,H+0xc000);put(H+0xc080,1);put(H+0xc084,H+0xc100);put(H+0xc11c,0);put(H+0xc104,7);put(BSP+8,8)
    # Object definitions (tag rows 0..7 -> DEFS).
    for d in range(8):
        put(TAGROWS+16*d+8,DEFS+0x40*d);put(DEFS+0x40*d,rng.choice((0,1,2,3,6,7,9,11,12,-1)),2)
        put(DEFS+0x40*d+2,rng.getrandbits(4),2);put(DEFS+0x40*d+0x1c,rng.getrandbits(1),1)
    # Objects: index 1..7, salts, bodies, parents (2 children of 1 at most).
    for i in range(OBJECTS):
        body=BODIES+0x200*i;salt=0xe000+i
        put(OBJROWS+12*i,salt,2);put(OBJROWS+12*i+3,rng.choice((0,1,2,6,7,7,9)),1);put(OBJROWS+12*i+8,body)
        put(body,rng.randrange(8));put(body+4,rng.getrandbits(32)&0x2000)
        put(body+7,1 if rng.random()<.04 else 0,1);put(body+0x14,0xffffffff)
        for k,v in enumerate(vector(rng,special,40)):put(body+0x30+4*k,v)
        for k,v in enumerate(vector(rng,special,8)):put(body+0x88+4*k,v)
        put(body+0xb4,0xffffffff);put(body+0xc1,1 if rng.random()<.08 else 0,1)
        put(body+0xcc,0xffffffff);put(body+0xd0,0xffff if rng.random()>.04 else rng.randrange(4),2)
        put(body+0x118,rng.choice((0,10,40,95,160)),2);put(body+0x11a,0x140,2)
        for k in range(16):put(body+0x140+k,rng.getrandbits(8),1)
        put(body+0x124,rng.choice((0,24,48,96)),2);put(body+0x126,0x160,2)
        for k in range(24):put(body+0x160+4*k,bits(rng.uniform(-.2,1.3)) if rng.random()>.05 else rng.choice(SPECIAL))
    for p in range(4):
        for k in range(23):put(PROBES+0x5c*p+4*k,rng.getrandbits(32))
    for k in range(21):put(DEFAULTS+0x1c+4*k,rng.getrandbits(32))
    # Parents: object 2 (and maybe 3) attached to 1; occasionally a chain.
    if rng.random()<.5:put(BODIES+0x200*2+0x14,(0xe000+1)<<16|1)
    if rng.random()<.2:put(BODIES+0x200*3+0x14,(0xe000+2)<<16|2)
    # Physics: object 1 sometimes owns a rigid body.
    if rng.random()<.3:
        put(BODIES+0x200+0xb4,0);put(PHYSROWS+0x18,rng.choice((0,1,-1,5)),1);put(PHYSROWS+0x74,2);put(PHYSROWS+0x70,RIGIDS)
        for r in range(2):
            put(RIGIDS+96*r+0x40,RIGID+0x80*r);put(RIGID+0x80*r+0x40,rng.choice((0,0,1)),1);put(RIGID+0x80*r+0x3c,HAVOK+0x80*r)
            for k,v in enumerate(vector(rng,special,8)):put(HAVOK+0x80*r+0x40+4*k,v)
    put(0x474d04,0x44abec);put(0x474d10,0x44ac40)
    # Render cache pool: 16 records of 256 bytes.
    capacity=rng.choice((RECORD_COUNT,RECORD_COUNT,3));put(CACHEPOOL+0x20,capacity);put(CACHEPOOL+0x24,256)
    put(CACHEPOOL+0x44,RECORDS);put(CACHEPOOL+0x48,BITMAP);put(CACHEPOOL+0x40,rng.choice((0x8001,0xfffe,0x1234)),2)
    used=0;extent=0;occupancy=0
    full=rng.random()<.3
    for r in range(capacity):
        if full or rng.random()<.5:
            occupancy|=1<<r;used+=1;extent=r+1
            rec=RECORDS+256*r;put(rec,0x9000+r,2)
            owner=rng.randrange(OBJECTS);put(rec+4,(0xe000+owner)<<16|owner if rng.random()<.8 else rng.getrandbits(32))
            put(rec+2,rng.getrandbits(1),1);put(rec+3,rng.getrandbits(1),1)
            put(rec+8,rng.choice((0xffffffff,frame,frame-1,frame-4,frame-7,frame-10,frame-16,frame-46,frame-61,frame+3)))
            put(rec+12,rng.choice((0xffffffff,frame,frame-1,frame-2,frame-900,frame+5)))
            for k in range(3):put(rec+0x10+4*k,bits(rng.uniform(-40,40)))
            for base in (0x1c,0x30):
                for k,v in enumerate(vector(rng,special,1.5)):put(rec+base+4*k,v)
                put(rec+base+12,color(rng));put(rec+base+16,color(rng))
            for k in range(0x54,0xa8,4):put(rec+k,rng.getrandbits(32))
    put(BITMAP,occupancy);put(CACHEPOOL+0x38,extent if not full else capacity);put(CACHEPOOL+0x3c,used)
    put(CACHEPOOL+0x34,rng.randrange(0,max(extent,1)+1))
    # Objects' cached tokens: valid, stale or absent.
    for i in range(1,OBJECTS):
        r=rng.randrange(capacity)
        if occupancy>>r&1 and rng.random()<.6:
            body=BODIES+0x200*i;put(body+0xcc,(0x9000+r)<<16|r);rec=RECORDS+256*r
            if rng.random()<.8:put(rec+4,(0xe000+i)<<16|i)
            if rng.random()<.7:
                # A current record: interpolation, near skips and speed bands.
                put(rec+2,1,1);put(rec+3,1,1);put(rec+12,rng.choice((frame,frame-1,frame-1,frame-2)))
                if rng.random()<.3:
                    for k in range(3):put(rec+0x10+4*k,bits(struct.unpack('<f',struct.pack('<I',c.get(body+0x30+4*k)))[0]+rng.uniform(-.03,.03)))
                speed=rng.choice((.01,.1,1,8,40,0))
                for k,v in enumerate(vector(rng,special,speed)):put(body+0x88+4*k,v)
                if rng.random()<.3:
                    for k in range(3):put(rec+0x30+4*k,c.get(rec+0x1c+4*k) ^ rng.getrandbits(3))
    # Lighting definition: variables, functions, the fallback handle.
    handle=rng.choice((8,8,8,0xffffffff,9))
    put(LIGHTING+0x33c,handle);put(FALLBACK+0x184,8 if rng.random()<.7 else 10)
    put(TAGROWS+16*8+8,LIGHTDEF if rng.random()>.08 else 0);put(TAGROWS+16*9+8,LIGHTDEF);put(TAGROWS+16*10+8,0)
    count=rng.choice((0,1,2,4,4,-1));put(LIGHTDEF,count);put(LIGHTDEF+4,VARIABLES)
    for v in range(4):
        row=VARIABLES+0x90*v
        put(row,rng.choice((1,1<<rng.randrange(1,14),rng.getrandbits(16),0)))
        put(row+4,bits(rng.uniform(-.3,.3)))
        for lo,hi in ((8,0x14),(0x2c,0x38),(0x44,0x50),(0x68,0x74)):
            for k in range(3):
                a,b=sorted((rng.uniform(-.2,1),rng.uniform(0,1.6)));put(row+lo+4*k,bits(a));put(row+hi+4*k,bits(b))
        put(row+0x20,bits(rng.choice((0,15,30,45,60,80,90))));put(row+0x5c,bits(rng.uniform(-180,180)))
        for fn in (0x24,0x60,0x80,0x88):
            blob=BLOBS+0x40*(4*v+(0x24,0x60,0x80,0x88).index(fn));put(row+fn,rng.choice((1,1,0,-1)));put(row+fn+4,blob)
            put(blob,rng.choice((0,0,1)),1);put(blob+1,rng.choice((0,1,0x10,0x11)),1)
            put(blob+4,bits(rng.uniform(-.5,.8)));put(blob+8,bits(rng.uniform(.5,2)))
    return c,frame


def cases(image):
    rng=random.Random(0x3daf0);n=0
    controls=(0x1f80,0x1f80,0x1f80,0x3f80,0x5f80,0x7f80,0x1fc0,0x9f80)
    for k in range(520):
        special=k%4==3
        c,frame=build(image,rng,n,special)
        object_=rng.randrange(1,OBJECTS);full=(0xe000+object_)<<16|object_
        metric=rng.choice((bits(rng.uniform(0,600)),bits(500),bits(400),bits(250),bits(200),bits(150),bits(100),
            bits(60),bits(50),bits(20),bits(10),bits(3),bits(0),*((SPECIAL) if special else ())))
        yield c.row(full,metric,rng.choice(controls));n+=1
    # Interpolating records with NaN payloads in the same current/target
    # lanes: the component step keeps each original operand order.
    for k in range(48):
        c,frame=build(image,rng,n,True);c.name=f'lighting-nan-step-{n}'
        body=BODIES+0x200;r=rng.randrange(c.get(CACHEPOOL+0x20));rec=RECORDS+256*r
        c.put(BITMAP,c.get(BITMAP)|1<<r);c.put(rec,0x9000+r,2);c.put(body+0xcc,(0x9000+r)<<16|r);c.put(rec+4,(0xe000+1)<<16|1)
        c.put(body+0x14,0xffffffff);c.put(body+0x7,0,1);c.put(body+0xd0,0xffff,2);c.put(body+0xc1,0,1);c.put(0x4739c4,1,1)
        c.put(rec+2,1,1);c.put(rec+3,1,1);c.put(rec+8,frame-1);c.put(rec+12,frame)
        lanes=[rng.randrange(3) for _ in range(rng.randrange(1,4))]
        for lane in range(3):
            if lane in lanes:
                c.put(rec+0x1c+4*lane,rng.choice((0x7fc12345,0xffc54321,0x7f812345,0x7fa00001)))
                c.put(rec+0x30+4*lane,rng.choice((0x7fc0beef,0xff800777,0x7f900002,0xffc00001)))
        for k2,v in enumerate(vector(rng,False,rng.choice((1,8,40)))):c.put(body+0x88+4*k2,v)
        if rng.random()<.5:c.put(OBJROWS+12+3,7,1)
        yield c.row((0xe000+1)<<16|1,bits(rng.uniform(0,600)),rng.choice(controls));n+=1


def prepare(image,scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=ENTRIES;meta=common.metadata(image);meta['stop']=STOP
    meta['sites']=list({s['instruction']:s for s in meta['sites']}.values())
    section=image.section('.text');(scratch/'text.bin').write_bytes(image.section_bytes(section))
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{scratch/"text.bin"}"\n'
    for name,(_,size,_) in SECTIONS.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    (scratch/'reference.s').write_text(assembly)
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '+' '.join(
        f'.{name} 0x{at:x}:{{*(.{name})}}' for name,(at,_,_) in SECTIONS.items())+f' .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n'
    (scratch/'link.ld').write_text(linker)
    for cmd in (['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
        ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(scratch/'observer')]):
        r=subprocess.run(cmd,capture_output=True,text=True)
        if r.returncode:raise ValueError(r.stderr[-2000:])
    return meta


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--limit',type=int,default=0)
    p.add_argument('--work',type=Path,help='owned directory; reuses its physical observation of the same inputs (mutation screens)')
    a=p.parse_args()
    if not a.xbe.is_file():return 77
    if sha(a.xbe)!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);scratch=a.work or Path(os.environ['HALO2_TEST_SCRATCH']);scratch.mkdir(parents=True,exist_ok=True)
    rows=list(cases(image))
    if a.limit:rows=rows[:a.limit]
    inputs=json.dumps(rows,separators=(',',':'))
    if not (a.work and (scratch/'observed.json').is_file() and (scratch/'inputs.json').read_text()==inputs):
        meta=prepare(image,scratch)
        (scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(inputs)
        r=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],
            capture_output=True,text=True,timeout=3000)
        if r.returncode:raise ValueError((r.stdout+r.stderr)[-3000:])
    meta=json.loads((scratch/'metadata.json').read_text())
    proof=json.loads((scratch/'observed.json').read_text());assert not proof['instruction_patches'] and not proof['supplied_original_outcomes']
    stack=SECTIONS['stack'][0];digest=hashlib.sha256();changed=0;coverage=[0]*22;failures=[]
    with (scratch/'native-errors.log').open('w+') as errors:
        native=subprocess.Popen([str(a.native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,actual in zip(rows,proof['cases'],strict=True):
                if not actual['completed']:raise ValueError(row['name']+' original did not complete')
                native.stdin.write(json.dumps(row,separators=(',',':'))+'\n');native.stdin.flush();line=native.stdout.readline()
                if not line:errors.seek(0);raise ValueError(row['name']+': '+errors.read()[-2500:])
                mine=json.loads(line)
                gold=[b for b in actual['banks'] if b['address']!=stack];ours=[b for b in mine['banks'] if b['address']!=stack]
                problem=None
                for g,o in zip(gold,ours,strict=True):
                    if g!=o:
                        x,y=bytes.fromhex(g['after']),bytes.fromhex(o['after'])
                        first=next(i for i in range(len(x)) if x[i]!=y[i])
                        problem=f"bank {g['address']+first:#x}: original {x[first:first+8].hex()} native {y[first:first+8].hex()}";break
                if problem is None and mine['eax']!=actual['boundary_gp'][0]:problem=f"EAX original {actual['boundary_gp'][0]:#x} native {mine['eax']:#x}"
                if problem is None and mine['fp']!=actual['after_fp']:problem=f"FP original {actual['after_fp']} native {mine['fp']}"
                if problem:failures.append(row['name']+' '+problem);continue
                before={b['address']:b['before'] for b in row['banks']}
                changed+=any(before[b['address']]!=b['after'] for b in gold)
                for i,value in enumerate(mine.get('coverage',[])):coverage[i]+=value
                digest.update(line.encode())
            native.stdin.close()
            if native.wait(timeout=60):errors.seek(0);raise ValueError(errors.read()[-2500:])
        finally:
            if native.poll() is None:native.terminate();native.wait(timeout=5)
    names=('calls','default_banks','probe_banks','roots','children','uncached_children','acquisitions','allocations',
        'evictions','updates','unchanged','frozen','due','near_skips','queries','query_hits','interpolations','rotations',
        'snaps','conversions','conversion_fallbacks','color_publications')
    report=dict(passed=not failures,physical_cases=len(rows),failures=failures[:40],failure_count=len(failures),
        cases_with_persistent_stores=changed,native_coverage=dict(zip(names,coverage)),original_xbe_sha256=PIN,
        owners=meta['owners'],original_instructions=proof['original_instructions'],comparison_sha256=digest.hexdigest(),
        instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],
        scope='Whole 3DAF0 with genuine 3DDC0/3DFD0/D65C0 trees; D3D90 declines at its unavailable-bank prefix. Every non-stack byte, EAX and the x87/MXCSR words.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if not failures else 'FAIL',len(rows),'physical 3DAF0 cases,',len(failures),'failures,',changed,'with persistent stores',dict(zip(names,coverage)))
    for f in failures[:12]:print(' ',f)
    return 0 if not failures else 1


if __name__=='__main__':raise SystemExit(main())
