#!/usr/bin/env python3
"""Replay untouched12FA0 and foreground12D50 into complete c0..11 banks."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE = 0x2100000
CAMERA, BANK, CONSTANTS, DEVICE, SURFACE, PUSH = [BASE+i*0x1000 for i in range(6)]
STACK, STOP = BASE+0xfff00, 0x3000000

def words(raw):
    return list(struct.unpack('<'+'I'*(len(raw)//4), raw))

def packed(values):
    return struct.pack('<'+'I'*len(values), *values)

def bits(value):
    return words(struct.pack('<f', value))[0]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--output', type=Path,
                        default=ROOT/'analysis/multiplayer-shading/source-view-shader-bank-original.json')
    args = parser.parse_args()
    xbe = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(xbe.data).hexdigest() == PIN
    assert words(xbe.read(0x45e460, 4)) == [0x443ff400]
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    for section in xbe.sections:
        guest.mem_write(section.va, xbe.section_bytes(section))
    guest.mem_map(BASE, 0x100000)
    guest.mem_map(STOP, 4096)
    executed = {}
    guest.hook_add(UC_HOOK_CODE, lambda u, pc, size, _: executed.setdefault(pc, bytes(u.mem_read(pc,size))))
    def put(address, value):
        guest.mem_write(address, packed([value]))
    def run(address, registers, arguments=(), stop=STOP):
        for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
                    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):
            guest.reg_write(reg,0)
        for reg,value in registers.items():
            guest.reg_write(reg,value)
        for reg,value in ((UC_X86_REG_FPCW,0x27f),(UC_X86_REG_MXCSR,0x1f80),
                          (UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPTAG,0xffff)):
            guest.reg_write(reg,value)
        guest.reg_write(UC_X86_REG_ESP,STACK)
        guest.mem_write(STACK,packed([STOP,*arguments]))
        guest.emu_start(address,stop,count=10000)
        assert guest.reg_read(UC_X86_REG_EIP)==stop
    rows = [list(map(int,line.split())) for line in
            (ROOT/'tests/fixtures/retail_source_view_projection.tsv').read_text().splitlines()]
    rng = random.Random(0x12fa012d50)
    viewports = [(0,0,640,480),(0,0,1280,720),(0,0,1280,800),
                 (0,0,1920,1080),(0,0,3840,2160),(0,0,853,479),
                 (0,240,640,240),(320,240,320,240),(-8,12,853,479)]
    cases = []
    # The first row is the retained Zanzibar view, whose original GPU bank
    # corroborates the complete ordinary submitting-camera result below.
    controls = [(0,0,640,480,1,0,0)]
    controls += [(*view,scale,flags,mode) for view in viewports
                 for scale in (.01,.0625,.5,1,2) for flags in range(4) for mode in (0,2)]
    for index,control in enumerate(controls):
        row = rows[0] if index==0 else rng.choice(rows)
        # Frozen input32, output76: the first48 output words are the entire
        # actual2FD90 bank. No reconstruction supplies source12FA0 matrices.
        assert len(row)==109
        raw_bank=row[33:81]
        eye=row[1:4]
        distance=row[32]
        left,top,width,height,scale,flags,mode=control
        guest.mem_write(CAMERA,bytes(0x74))
        guest.mem_write(CAMERA,packed(eye))
        guest.mem_write(CAMERA+0x30,struct.pack('<4h',top,left,top+height,left+width))
        guest.mem_write(BANK,packed(raw_bank))
        guest.mem_write(DEVICE,bytes(0x2500))
        put(0x407488,DEVICE);put(DEVICE,PUSH);put(DEVICE+4,PUSH+0xf00)
        put(DEVICE+0x794,SURFACE);put(SURFACE+4,0)
        for offset in (0x93c,0x944,0x948,0x95c,0x960):
            put(DEVICE+offset,bits(1))
        put(0x48565c,distance);put(0x4670c8,bits(scale))
        guest.mem_write(0x5093fc,bytes([bool(flags&2)]))
        guest.mem_write(0x404fe8,b'\0')
        run(0x12fa0,{UC_X86_REG_EAX:BANK,UC_X86_REG_ECX:CAMERA},
            [int(bool(flags&1)),mode],stop=0x132b3)
        ordinary=words(guest.mem_read(0x407738,192))
        for foreground in (False,True):
            expected=ordinary.copy()
            if foreground:
                run(0x12d50,{UC_X86_REG_EAX:BANK,UC_X86_REG_EBX:1},[CONSTANTS])
                expected[:16]=words(guest.mem_read(CONSTANTS,64))
            inputs=[int(foreground),*raw_bank,distance,*eye,
                    *map(bits,(left,top,width,height,scale)),flags,mode]
            assert len(inputs)==60
            cases.append((inputs,expected))
    for pc,code in executed.items():
        assert xbe.read(pc,len(code))==code,hex(pc)
    native=subprocess.run([str(args.probe.resolve()),'--probe'],
        input=''.join(' '.join(map(str,i))+'\n' for i,_ in cases),
        text=True,capture_output=True,check=True)
    outputs=[list(map(int,line.split())) for line in native.stdout.splitlines()]
    assert len(outputs)==len(cases)
    failures=[index for index,((_,expected),actual) in enumerate(zip(cases,outputs)) if expected!=actual]
    gpu_fixture=ROOT/'tests/fixtures/retail_shadow_vertex.tsv'
    gpu_camera={}
    for line in gpu_fixture.read_text().splitlines():
        fields=line.split()
        if fields and fields[0]=='C' and int(fields[2])<12:
            gpu_camera.setdefault(int(fields[1]),[]).extend(int(v,16) for v in fields[3:])
    assert len(gpu_camera)==11 and all(len(bank)==48 for bank in gpu_camera.values())
    assert all(bank==cases[0][1] for bank in gpu_camera.values()),'actual retained shadow camera bank differs'
    fixture=ROOT/'tests/fixtures/retail_source_view_shader_bank.tsv'
    if not failures:
        fixture.write_text(''.join(' '.join(map(str,[*i,*e]))+'\n' for i,e in cases))
    report={'executable_sha256':PIN,'source_entries':['12FA0 until132B3','12D50','3F6190'],
            'native_probe_sha256':hashlib.sha256(args.probe.read_bytes()).hexdigest(),
            'service_hooks':[],'executed_instruction_addresses':len(executed),
            'original_instructions_verified':True,'cases':len(cases),
            'compared_words':len(cases)*48,'failures':failures,
            'eye_w_original_word':words(xbe.read(0x45e460,4))[0],
            'actual_original_gpu_camera_banks':{'draws':len(gpu_camera),
                'words':len(gpu_camera)*48,'constants':'c0..11','exact':True},
            'scope':'Original fullcamera c0..11, explicit actual2FD90 banks/cameraeye/Wdistance '
                    'and viewport/scale/mode, then original foreground c0..3 replacement. '
                    'Private valid D3D push shell; no source arithmetic hooks or hardware precision claim.',
            'sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in (Path(__file__),fixture,ROOT/'tests/fixtures/retail_source_view_projection.tsv',gpu_fixture,
                                ROOT/'src/source_view_projection.cpp',ROOT/'src/render_view.cpp',
                                ROOT/'include/halo2/source_view_projection.hpp')
                      if p.exists()}}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if failures:
        first=failures[0];print(json.dumps({'first_failure':first,'original':cases[first][1],
                                          'native':outputs[first]},indent=2));return 1
    fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='sha256'},indent=2))
    return 0

if __name__=='__main__':
    raise SystemExit(main())
