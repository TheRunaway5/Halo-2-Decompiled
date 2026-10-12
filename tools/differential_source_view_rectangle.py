#!/usr/bin/env python3
"""Freeze integer full-surface layout edges and untouched12FA0 c8..11.

The 640x480 layout is the independently transcribed2F600/2F640/2F6B0 bank;
modern drawable scaling is an explicit native extension. Only the unrelated
12D50 matrix upload is neutralized. No source camera/effect math is replaced.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

ROOT=Path(__file__).resolve().parents[1]
XBE_SHA='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]

def edges(width,height,wide,players,view):
    columns=rows=1
    while columns*rows<players:
        if columns<=rows if wide else columns<rows:columns+=1
        else:columns=1;rows+=1
    span_x=span_y=1;offset=0
    if players<columns*rows:
        if not wide:
            if view==0:span_x=2
            else:offset=1
        else:
            if view==0:span_y=2
            else:offset=view//columns
    x,y=(view+offset)%columns,(view+offset)//columns
    cell_w,cell_h=(592-48)//columns,(444-36)//rows
    l=48+x*cell_w if x else 0
    t=36+y*cell_h if y else 0
    r=640 if x+span_x==columns else 48+(x+span_x)*cell_w
    b=480 if y+span_y==rows else 36+(y+span_y)*cell_h
    sx,sy=f32(width/640),f32(height/480)
    left,top=f32(l*sx),f32(t*sy)
    right,bottom=f32(left+f32((r-l)*sx)),f32(top+f32((b-t)*sy))
    return [int(top),int(left),int(bottom),int(right)]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--probe',type=Path,required=True)
    p.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_view_rectangle.tsv')
    p.add_argument('--report',type=Path,default=ROOT/'analysis/multiplayer-shading/source-view-rectangle-original.json')
    a=p.parse_args();image=(ROOT/'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(image).hexdigest()!=XBE_SHA:raise ValueError('unsupported original executable')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    for i in range(count):
        _,address,_,offset,size=struct.unpack_from('<5I',image,table-base+i*56)
        u.mem_write(address,image[offset:offset+size])
    u.mem_map(0x2100000,0x100000)
    u.mem_write(0x12d50,b'\xc2\x04\x00');u.mem_write(0x404fe8,b'\0')
    u.mem_write(0x4670c8,struct.pack('<f',1));u.mem_write(0x5093fc,b'\0')
    rows=subprocess.run([str(a.probe.resolve()),'--probe'],check=True,capture_output=True,text=True).stdout.splitlines()
    fixture=[];even=0;cases=[]
    for line in rows:
        row=list(map(int,line.split()))
        if len(row)!=25:raise ValueError('unexpected viewport probe columns')
        w,h,wide,players,view=row[:5];expected_edges=edges(w,h,bool(wide),players,view)
        if row[5:9]!=expected_edges:raise ValueError('native edges differ from literal integer layout')
        u.mem_write(0x2100030,struct.pack('<4h',*expected_edges))
        u.mem_write(0x21fff00,struct.pack('<3I',0x3000000,0,0))
        u.reg_write(UC_X86_REG_EAX,0x2101000);u.reg_write(UC_X86_REG_ECX,0x2100000)
        u.reg_write(UC_X86_REG_ESP,0x21fff00);u.reg_write(UC_X86_REG_FPCW,0x027f)
        u.reg_write(UC_X86_REG_MXCSR,0x1f80)
        u.emu_start(0x12fa0,0x132b3,count=10000)
        bank=list(struct.unpack('<16I',u.mem_read(0x4077b8,64)))
        if row[9:]!=bank:raise ValueError('native viewport bank differs from untouched12FA0')
        fixture.append(' '.join(map(str,row[:5]+expected_edges+bank)))
        even+=w%2==0 and h%2==0
        cases.append({'extent':[w,h],'wide':bool(wide),'players':players,'view':view,'edges':expected_edges})
    if len(fixture)!=160 or even!=80:raise ValueError('incomplete layout corpus')
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(fixture)+'\n')
    report={'xbe_sha256':XBE_SHA,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),
      'fixture_sha256':hashlib.sha256(a.fixture.read_bytes()).hexdigest(),'cases':160,'words':2560,
      'standard_even_layouts':80,'modern_odd_layouts':80,'original_function':'00012FA0',
      'neutralized_independent_service':'00012D50 matrix upload, unused by c8..11',
      'stop_before':'000132B3 device upload','FPCW':0x027f,'MXCSR':0x1f80,
      'boundary':'Native full-surface scaling feeds literal signed-short edges; physical Xbox odd video modes and higher-resolution support are not asserted.',
      'cases_inputs':cases}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print('160 unchanged12FA0 banks /2560 words exact; 80 standard layouts unchanged')
if __name__=='__main__':main()
