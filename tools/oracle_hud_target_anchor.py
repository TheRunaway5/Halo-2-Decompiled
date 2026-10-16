#!/usr/bin/env python3
"""Compare1396C7 mode5, unchanged30710 projection and2B460 rounding."""
import argparse,hashlib,json,math,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--probe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
    pin=hashlib.sha256(x.data).hexdigest()
    assert pin=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    state,stack,stop=0x1000000,0x200ff00,0x3000000
    g.mem_map(state,0x10000);g.mem_map(0x2000000,0x10000);g.mem_map(stop,0x1000)
    def words(at,*values):g.mem_write(at,struct.pack('<'+'I'*len(values),*values))
    def floats(at,values):g.mem_write(at,struct.pack('<'+'f'*len(values),*values))
    def run(entry):
        words(stack,stop);g.reg_write(UC_X86_REG_ESP,stack)
        for reg,value in ((UC_X86_REG_FPCW,0x027f),(UC_X86_REG_MXCSR,0x1f80),
                (UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0)):g.reg_write(reg,value)
        g.emu_start(entry,stop,count=20000);assert g.reg_read(UC_X86_REG_EIP)==stop
    words(0x4e6948,0);words(0x510c50,0);words(0x4e0350,0)
    words(0x510c4c,state+0x6000);words(state+0x61bc,state+0x7000)
    g.mem_write(state+0x70ad,b'\x01\x00')
    sentinel=struct.unpack('<f',x.read(0x45de38,4))[0]
    rows=[];expected=[]
    for outer,safe in (((0,0,640,480),(48,36,544,408)),((0,0,640,240),(48,36,544,200)),
            ((320,240,320,240),(324,244,268,200)),((0,0,1280,720),(96,54,1088,612)),
            ((0,0,640,720),(96,54,536,612)),((640,0,640,720),(648,54,536,612)),
            ((640,360,640,360),(648,366,536,300))):
      for wide in (False,True):
       for yaw,pitch in ((0,0),(.3,.2),(1.9,-.7)):
        position=(-5.106619358,-5.505378723,.629999757)
        forward=(math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),math.sin(pitch))
        up=(-math.sin(pitch)*math.cos(yaw),-math.sin(pitch)*math.sin(yaw),math.cos(pitch))
        for depth,side,height in ((10,0,0),(10,2,1),(10,-2,-1),(10,100,0),(-10,0,0),(0,0,0)):
         point=tuple(position[i]+forward[i]*depth+up[i]*height+(-math.sin(yaw),math.cos(yaw),0)[i]*side for i in range(3))
         row=list(position+forward+up+(.7,float(wide))+outer+safe+point)
         row=list(struct.unpack('<22f',struct.pack('<22f',*row)))
         g.mem_write(0x4b9da0,bytes(0x74));floats(0x4b9da0,row[:9]);floats(0x4b9dc8,(row[9],.7850000262260437))
         for at,r in ((0x4b9dd0,outer),(0x4b9dd8,safe)):
          l,t,w,h=r;g.mem_write(at,struct.pack('<4h',t,l,t+h,l+w))
         floats(0x4b9de0,(.015625,1024));g.mem_write(0x485ac2,bytes((int(wide),)))
         g.reg_write(UC_X86_REG_EAX,0x4b9da0);g.reg_write(UC_X86_REG_ESI,state+0x400);run(0x2f970)
         g.reg_write(UC_X86_REG_ESI,0x4b9da0);g.reg_write(UC_X86_REG_EBX,0x4b9e14)
         g.reg_write(UC_X86_REG_EDX,state+0x400);run(0x2fd90)
         floats(state+0x70b4,row[19:]);g.reg_write(UC_X86_REG_EAX,5)
         g.reg_write(UC_X86_REG_ESI,state+0x1800);run(0x1396c7)
         result=struct.unpack('<2f',g.mem_read(state+0x1800,8));visible=result!=(sentinel,sentinel)
         rows.append(row);expected.append((int(visible),*result) if visible else (0,0,0))
    result=subprocess.run([str(a.probe.resolve()),'--probe'],input=''.join(' '.join(map(str,r))+'\n' for r in rows),text=True,capture_output=True,check=True)
    actual=[tuple(map(float,line.split())) for line in result.stdout.splitlines()]
    errors=[dict(row=n,expected=e,actual=got) for n,(e,got) in enumerate(zip(expected,actual)) if e!=got]
    assert len(actual)==len(rows)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(dict(xbe_sha256=pin,cases=len(rows),mismatches=errors,instruction_patches=[],supplied_services=[]),indent=2)+'\n')
    a.output.with_suffix('.tsv').write_text('# Original1396C7 mode5/30710/2B460\n'+''.join(' '.join(format(v,'.9g') for v in (*r,*e))+'\n' for r,e in zip(rows,expected)))
    print(len(rows),'target anchors;',len(errors),'mismatches')
    if errors:raise SystemExit(1)
if __name__=='__main__':main()
