#!/usr/bin/env python3
"""Run unchanged retail radar visibility and sample instructions.

Object velocity, estimate mode, object lookup, local ownership, center and
type/size are explicit service inputs. No visibility, range, rotation,
quantization or sample-capacity result is replaced. This does not execute the
world producers, graphics APIs or physical Xbox pixels. History-render fixtures
execute the original ten-sample renderer through its graphics submission ABI.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
MEM, STACK, STOP = 0x1000000, 0x200ff00, 0x3000000
POOL, HEADERS, BODY, HUD, GLOBALS, PLAYER_INFORMATION, OPTIONS, SENSOR, PLAYERS, LOCAL = [MEM+i*0x1000 for i in range(10)]
CLOCK, ORIGIN = MEM+0xa000, MEM+0xb000


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/hud-functionality')
    args = ap.parse_args()
    x = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(x.data).hexdigest() == PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x600000)
    for s in x.sections:g.mem_write(s.va, x.section_bytes(s))
    g.mem_map(MEM, 0x100000);g.mem_map(0x2000000, 0x10000);g.mem_map(STOP, 0x1000)
    def w(a, n):g.mem_write(a, struct.pack('<I', n & 0xffffffff))
    def r(a):return struct.unpack('<I', g.mem_read(a, 4))[0]
    def floats(a, v):g.mem_write(a, struct.pack('<'+'f'*len(v), *v))
    def ret(value=0, popped=0):
        sp=g.reg_read(UC_X86_REG_ESP)
        g.reg_write(UC_X86_REG_EIP,r(sp));g.reg_write(UC_X86_REG_ESP,sp+4+popped)
        g.reg_write(UC_X86_REG_EAX,value)
    inputs=dict(velocity=[0,0,0], estimate=0, present=True, visible=True,
                origin=[0,0,0], unit=True, type=2, size=0)
    service_counts={};draws=[];composites=[]
    def hook(uc, pc, size, unused):
        if pc not in (0xba1d0,0xe70e0,0xbadc0,0x14ddc0,0x14de70,0xcafc0,0x1a1faf,
                      0x155760,0x254200,0x254490,0x2548f0):return
        service_counts[f'{pc:08x}']=service_counts.get(f'{pc:08x}',0)+1
        if pc==0xba1d0:
            floats(uc.reg_read(UC_X86_REG_ECX),inputs['velocity']);ret(popped=4)
        elif pc==0xe70e0:ret(inputs['estimate'])
        elif pc==0xbadc0:ret(BODY if inputs['present'] else 0,popped=4)
        elif pc==0x14ddc0:ret(int(inputs['unit']))
        elif pc==0x14de70:ret(0)
        elif pc==0xcafc0:floats(uc.reg_read(UC_X86_REG_EAX),inputs['origin']);ret()
        elif pc==0x1a1faf:
            # Classification/size has a separate original object/team/definition
            # owner. Its literal supplied bytes are retained, not reconstructed.
            p=r(uc.reg_read(UC_X86_REG_ESP)+4)
            g.mem_write(p+2,bytes([inputs['type'],inputs['size']]))
            ret(popped=4)
        elif pc==0x155760:ret(inputs['camera_mode'])
        elif pc==0x254200:ret()
        elif pc==0x254490:
            sp=uc.reg_read(UC_X86_REG_ESP)
            draws.append(bytes(uc.mem_read(r(sp+4),8))+bytes(uc.mem_read(sp+8,8))+
                         bytes(uc.mem_read(r(sp+16),12))+bytes([r(sp+20)&255]))
            ret(popped=20)
        elif pc==0x2548f0:
            sp=uc.reg_read(UC_X86_REG_ESP)
            composites.append(bytes(uc.mem_read(r(sp+4),8))+bytes(uc.mem_read(sp+8,4)))
            ret(popped=8)
    g.hook_add(UC_HOOK_CODE,hook)
    w(0x4e0300,POOL);w(POOL+0x44,HEADERS);w(HEADERS+12+8,BODY)
    w(0x510c94,HUD);w(0x4e034c,GLOBALS);w(GLOBALS+0x134,PLAYER_INFORMATION)
    w(0x4e6948,OPTIONS);w(0x51e994,SENSOR);w(0x4e8c24,PLAYERS)
    w(PLAYERS+0x44,PLAYERS+0x100);w(PLAYERS+0x100+0x2c,0x12340001)
    w(0x4ed284,LOCAL)
    w(0x510c54,CLOCK)
    def call(a, argv=(), registers=()):
        w(STACK,STOP)
        for i,v in enumerate(argv):w(STACK+4+i*4,v)
        g.reg_write(UC_X86_REG_ESP,STACK)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
        g.reg_write(UC_X86_REG_FPTAG,0xffff)
        for reg,value in registers:g.reg_write(reg,value)
        try:g.emu_start(a,STOP,count=200000)
        except Exception as error:
            raise RuntimeError(f'Original {a:08x} stopped at {g.reg_read(UC_X86_REG_EIP):08x}, '
                f'ESI={g.reg_read(UC_X86_REG_ESI):08x}, EDI={g.reg_read(UC_X86_REG_EDI):08x}, '
                f'ECX={g.reg_read(UC_X86_REG_ECX):08x}, object={r(g.reg_read(UC_X86_REG_EBP)+8):08x}, inputs={inputs}') from error
        assert g.reg_read(UC_X86_REG_EIP)==STOP,hex(g.reg_read(UC_X86_REG_EIP))
        return g.reg_read(UC_X86_REG_EAX)&255
    def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
    def bits(v):return f'{struct.unpack("<I",struct.pack("<f",v))[0]:08x}'
    rows=[]
    velocities=[[0,0,0],[.029,0,0],[.03,0,0],[.031,0,0],[0,0,.09],[0,0,.1],[3,4,0],
                [.9449,0,0],[.945,0,0],[.9451,0,0]]
    for velocity,sneak in itertools.product(velocities,[.04,.9]):
      for typ in (0,1,12):
       for control in (0,1):
        for owner in (0xffffffff,0x77770002):
         for mode in (0,1,3):
          for flags in (0,8,1<<30,(1<<30)|8):
           for action in (0,1,0x20):
            for mp,force in ((0,0),(1,0),(0,1)):
             inputs.update(velocity=list(map(f32,velocity)),estimate=mode)
             g.mem_write(BODY,bytes(0x200));g.mem_write(BODY+0xaa,bytes([typ]))
             g.mem_write(BODY+0xc1,bytes([control]));w(BODY+0x13c,owner)
             w(BODY+0x134,flags);g.mem_write(BODY+0x14a,bytes([action]))
             floats(HUD+0x294,[.03]);floats(PLAYER_INFORMATION+0x3c,[sneak]);w(OPTIONS+8,2 if mp else 0)
             g.mem_write(0x51e990,bytes([force]))
             moving=call(0x1a1d55,[0x12340001]);visible=call(0x1a20e6,[0x12340001])
             rows.append('M '+' '.join([*(bits(v)for v in inputs['velocity']),str(typ),str(control),str(action),
                 str(owner),str(flags),str(mode),bits(.03),bits(sneak),str(mp),str(force),str(moving),str(visible)]))
    movement_count=len(rows)
    # For sample arithmetic, original visibility is executed over a moving
    # ordinary biped. Supplied roots cover yaw/range boundaries and capacity.
    g.mem_write(BODY,bytes(0x200));w(BODY+0x13c,-1);g.mem_write(0x51e990,b'\0')
    inputs.update(velocity=[1,0,0],estimate=0,present=True,type=2,size=1)
    rng=random.Random(0x1a264e)
    samples=[]
    for i in range(300):
        origin=list(map(f32,[rng.uniform(-100,100),rng.uniform(-100,100),3]))
        yaw=f32(rng.uniform(-20,20));radius=f32(rng.choice([8,16,32]))
        delta=rng.choice([[0,0],[radius,0],[0,radius],[-radius,0],
                          [rng.uniform(-radius,radius),rng.uniform(-radius,radius)]])
        position=[f32(origin[0]+delta[0]),f32(origin[1]+delta[1]),0]
        count=rng.choice([0,1,15,16]);inputs.update(origin=origin,unit=i%17!=0,present=i%13!=0)
        # Supply up to sixteen references to the same actual object to exercise
        # the original capacity without imposing a native ordering heuristic.
        g.mem_write(SENSOR,bytes(0xc00));w(SENSOR+0xbc0,0)
        for n in range(count):w(SENSOR+0x2a8+n*4,0x12340001)
        w(SENSOR+0x2e8,count);w(SENSOR+0x2ec,0)
        floats(HUD+0x290,[radius,.03]);floats(LOCAL+0x24,[yaw]);floats(BODY+0x30,position)
        call(0x1a264e,[0])
        actual=bytes(g.mem_read(SENSOR,0x44));n=struct.unpack_from('<I',actual,0x40)[0]
        raw=actual[:n*4].hex() or '-'
        rows.append('S '+' '.join([str(int(inputs['unit'])),*(bits(v)for v in origin),bits(yaw),bits(radius),
            *(bits(v)for v in position),str(count),str(int(inputs['present'])),str(n),raw]))
        samples.append(dict(index=i,count=n,bytes=raw))
    # Full unchanged1A2823 ->1A2197: preserve original library pow/fmod/fsin,
    # table reads and draw order. Only graphics API callees are captured.
    render_count=0;pulse_count=0
    # Every30Hz phase for two complete sweep cycles, plus negative and
    # long-running clocks;60Hz inputs retain the original arithmetic checks.
    for tick in [-1,*range(127),1000,1000000]:
      for rate in [f32(1/30),f32(1/60)]:
        w(CLOCK+8,tick);floats(CLOCK+4,[rate]);call(0x1a224e)
        pulse=r(0x51e98c)
        rows.append(f'P {tick} {bits(rate)} {pulse:08x}');pulse_count+=1
    for i in range(50):
        view=i%4;mode=[0,1,2,3,-1][i%5];index=i%10
        tick=[0,15,30,61,62,63,1000,-1,1000000][i%9];rate=f32([1/30,1/60][i%2])
        origin=[rng.randint(-320,320),rng.randint(-240,240)]
        top_left=[rng.randint(-240,240),rng.randint(-320,320)]
        g.mem_write(SENSOR,bytes(0xc00));w(SENSOR+0xbc0,index)
        history=bytearray()
        for j in range(10):
            n=[0,1,8,16][(i+j)%4]
            raw=bytes([q for k in range(16)for q in
                [rng.randint(0,64),rng.randint(0,64),(i+j+k)%9,(i+j+k)%4]])
            history+=raw+struct.pack('<I',n)
        g.mem_write(SENSOR+view*0x2f0,bytes(history))
        g.mem_write(ORIGIN,struct.pack('<hh',*origin));g.mem_write(0x4b9dd0,struct.pack('<hh',*top_left))
        w(CLOCK+8,tick);floats(CLOCK+4,[rate]);call(0x1a224e)
        draws.clear();composites.clear();inputs['camera_mode']=mode
        call(0x1a2823,registers=[(UC_X86_REG_ECX,view),(UC_X86_REG_EAX,ORIGIN)])
        assert len(composites)==int(mode not in (2,3))
        rows.append('R '+' '.join([str(view),str(mode),*(str(q)for q in origin+top_left),
            str(tick),bits(rate),str(index),history.hex(),str(len(draws)),
            b''.join(draws).hex()or'-',composites[0].hex()if composites else'-']))
        render_count+=1
    args.output.mkdir(parents=True,exist_ok=True)
    fixture=args.output/'motion-sensor.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='H2_ORIGINAL_MOTION_SENSOR_V1',source_xbe_sha256=PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        visibility_cases=movement_count,sample_cases=len(samples),pulse_cases=pulse_count,
        history_render_cases=render_count,service_calls=service_counts,
        boundary=__doc__,instruction_patches=[])
    (args.output/'motion-sensor.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
