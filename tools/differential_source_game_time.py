#!/usr/bin/env python3
"""Compare original time governors, tick admission and the world-clock store.

680C0, 12BE90 and 137FE0 are explicit caller-service controls. This does not
claim original networking, tick-body execution or all of 137ED0's world updates.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_XMM1,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
TIME,OTHER,OPTIONS,FLAGS,OUTPUT,STACK,STOP=0x2100000,0x2100100,0x2101000,0x2103000,0x2104000,0x21fff00,0x3000000
def f(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def d(value):return struct.unpack('<Q',struct.pack('<d',value))[0]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=ROOT/'build-map-world-20261011/source_game_time_tests')
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/map-world-features/game-time-differential.json')
    parser.add_argument('--write-fixture',type=Path)
    args=parser.parse_args();image=Xbe(ROOT/'assets/disc/default.xbe')
    if hashlib.sha256(image.data).hexdigest()!=PIN:raise ValueError('unsupported original executable')
    guest=Uc(UC_ARCH_X86,UC_MODE_32);guest.mem_map(0x10000,0x600000)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    guest.mem_map(TIME,0x100000);guest.mem_map(STOP,0x1000)
    def put(at,value):guest.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',guest.mem_read(at,4))[0]
    controls={}
    def service(uc,at,size,data):
        if at==0x680c0:
            controls['network']+=1;row=controls['row']
            guest.mem_write(guest.reg_read(UC_X86_REG_EDX),bytes([bool(row[28])]))
            put(0x510c54,OTHER if row[29] else TIME);guest.reg_write(UC_X86_REG_EAX,row[27])
        elif at==0x12be90:
            stopped=controls['stop']==controls['row'][30]
            controls['stop']+=1;guest.reg_write(UC_X86_REG_EAX,int(stopped))
        elif at==0x137fe0:
            controls['tick']+=1;owner=word(0x510c54);put(owner+8,word(owner+8)+1)
        else:return
        stack=guest.reg_read(UC_X86_REG_ESP);guest.reg_write(UC_X86_REG_ESP,stack+4)
        guest.reg_write(UC_X86_REG_EIP,word(stack))
    guest.hook_add(UC_HOOK_CODE,service)
    def call(entry,stack_args=(),eax=0,ebx=0,xmm0=0,xmm1=0,end=STOP):
        put(STACK,STOP)
        for i,value in enumerate(stack_args):put(STACK+4+i*4,value)
        guest.reg_write(UC_X86_REG_ESP,STACK);guest.reg_write(UC_X86_REG_EAX,eax)
        guest.reg_write(UC_X86_REG_EBX,ebx);guest.reg_write(UC_X86_REG_XMM0,xmm0)
        guest.reg_write(UC_X86_REG_XMM1,xmm1)
        guest.emu_start(entry,end,count=100000)
        if guest.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError(f'original {entry:08x} did not finish')
    def original(row):
        controls.update(row=row,network=0,stop=0,tick=0)
        guest.mem_write(TIME,struct.pack('<9I',*row[1:10]));guest.mem_write(OTHER,struct.pack('<9I',*row[10:19]))
        put(0x510c54,TIME);put(0x4e6948,OPTIONS if row[19] else 0)
        guest.mem_write(OPTIONS+0x1120,bytes([bool(row[20])]))
        guest.mem_write(OPTIONS+0xc,bytes([row[21]&255]))
        guest.mem_write(OPTIONS+0xe,struct.pack('<H',row[22]&65535))
        put(0x510c50,FLAGS);guest.mem_write(FLAGS+5,bytes([bool(row[23])]))
        put(OUTPUT,row[24]);put(OUTPUT+4,row[25]);guest.mem_write(0x4ba040,struct.pack('<Q',row[31]))
        guest.reg_write(UC_X86_REG_FPCW,0x027f);guest.reg_write(UC_X86_REG_MXCSR,0x1f80)
        op=row[0]
        if op==0:call(0x1465b0)
        elif op==1:call(0x146980,xmm0=row[24])
        elif op==2:call(0x146860,[row[26]],xmm0=row[25],xmm1=row[24])
        if op in (3,6):call(0x146660,[row[24],OUTPUT,OUTPUT+4])
        if op in (4,6):call(0x137f70,eax=word(OUTPUT+4),ebx=OUTPUT)
        if op in (5,6):
            # Execute the untouched final FLD/FADD/FSTP only. Earlier world
            # services must be real before a native caller schedules this store.
            call(0x137f4d,[0,word(OUTPUT)],end=0x137f5e)
        return [*struct.unpack('<9I',guest.mem_read(TIME,36)),*struct.unpack('<9I',guest.mem_read(OTHER,36)),
            word(OUTPUT),word(OUTPUT+4),controls['tick'],controls['network'],controls['stop'],controls['tick'],
            *struct.unpack('<2I',guest.mem_read(0x4ba040,8))]
    rng=random.Random(0x146660);rows=[]
    def bank():
        rate=rng.choice([1,6,15,30,60,150,3000,32767,-32768,-30,0])
        return [(rng.randrange(256)|(rng.choice([0,0,0,1,2,255])<<8)|((rate&65535)<<16)),
            f(rng.choice([0.,1./30.,1./60.,.1,-.25])),rng.getrandbits(32),
            f(rng.choice([0.,.01,.2,.3333333,1.,1.5,4.999,5.,6.,-1.])),f(rng.uniform(-.999,1.999)),
            f(rng.choice([0.,.01,.1,1.,5.])),f(rng.choice([0.,0.,0.,.01,1.,5.])),
            f(rng.uniform(-1.,6.)),f(rng.uniform(-1.,6.))]
    for op in range(7):
        for i in range(1536):
            first,second=bank(),bank()
            seconds=rng.choice([0.,-0.,1./240.,1./60.,1./30.,.1,.25,1.,5.,-.1])
            row=[op,*first,*second,int(op<3 or i%7!=0),int(i%5!=0),rng.choice([0,1,2,3,4,5,6,255]),
                rng.choice([30,60,1,32767,32768,65506,0]),int(i%4==0),f(seconds),
                f(rng.uniform(-1.,6.)),f(rng.choice([0.,-1.,.01,1.,5.])),
                rng.choice([0,1,2,5,7,30,0x7fffffff,0xffffffff,0x80000000]),int(i%3==0),int(i%4==1),
                rng.choice([0xffffffff,0,1,2,4,7]),d(rng.choice([0.,.125,1.,300.125,2.**40,-100.]))]
            if op==4:row[25]=rng.choice([0xffffffff,0,1,2,5,9])
            if op==6:row[30]=rng.choice([0,1,2,4,7])
            rows.append(row)
    # Correlated complete governor -> tick-consumer -> double-clock frames.
    # Includes paused ramps, network bank replacement and actual abort counts.
    state=[(30<<16)|1,f(1./30.),0,f(1.),0,0,0,0,0];clock=d(0.)
    for frame in range(960):
        state[0]=(state[0]&0xffff00ff)|((frame//120%4==1)<<8)
        if frame%160==0:state[5:9]=[0,f(.5),f(.2),f(5.)]
        row=[6,*state,*state,1,1,2 if frame%5==0 else 0,30,0,
            f([1./240.,1./60.,1./30.,.1,1.][frame%5]),0,0,2,int(frame%15==0),int(frame%25==0),
            [0xffffffff,0,1][frame//60%3],clock]
        rows.append(row);result=original(row)
        state=result[9:18] if row[29] and result[21] else result[:9]
        clock=result[24]|(result[25]<<32)
    expected=[original(row) for row in rows]
    encoded=[' '.join(f'{word:x}' for word in row) for row in rows]
    native=subprocess.run([str(args.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
    actual=[[int(word,16) for word in line.split()] for line in native.stdout.splitlines()]
    if len(actual)!=len(rows):raise RuntimeError('native game-time probe lost rows')
    failures=[{'case':i,'input':row,'source':source,'native':native}
        for i,(row,source,native) in enumerate(zip(rows,expected,actual)) if source!=native]
    if args.write_fixture and not failures:
        selected=list(range(0,7*1536,32))+list(range(7*1536,len(rows),8))
        lines=['# Original game-time executable '+PIN,
            '# 31 hexadecimal input words plus one 64-bit clock, then 26 output words.',
            '# Network budget, stop query and tick body are explicit test services.']
        lines.extend(encoded[i]+' '+' '.join(f'{v:08x}' for v in expected[i]) for i in selected)
        args.write_fixture.write_text('\n'.join(lines)+'\n')
    report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'entries':['001465B0','00146660','00146860','00146980','00137F70','00137F4D..00137F5E'],
        'cases':len(rows),'bit_exact_cases':len(rows)-len(failures),'compared_words':len(rows)*26,
        'caller_services':['680C0: explicit tick budget, synchronous flag and bank replacement',
            '12BE90: explicit stop result','137FE0: explicit test tick counter'],
        'floating_point_environment':{'x87_cw':'027F','mxcsr':'00001F80'},
        'failures':failures[:12],
        'scope':'Original governor/speed/ramp, admitted tick loop and final world-clock store. Platform elapsed, networking, tick bodies, world-update services and live snow remain separate.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
