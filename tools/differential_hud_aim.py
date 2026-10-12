#!/usr/bin/env python3
"""Execute retail 171D90/2FD90/1396C7 and compare native observer/HUD helpers."""
from pathlib import Path
import argparse, hashlib, json, math, random, struct, subprocess, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--probe', type=Path, required=True)
parser.add_argument('--fixtures', type=Path, default=ROOT/'tests/fixtures/retail_hud_aim.tsv')
parser.add_argument('--report', type=Path, default=ROOT/'analysis/multiplayer-shading/hud-aim-differential.json')
args = parser.parse_args()
xbe = Xbe(ROOT/'assets/disc/default.xbe')
sha = hashlib.sha256(xbe.data).hexdigest()
assert sha == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g = Uc(UC_ARCH_X86, UC_MODE_32)
g.mem_map(0x10000, 0x600000)
for section in xbe.sections:g.mem_write(section.va, xbe.section_bytes(section))
STATE, STACK, STOP = 0x1000000, 0x200ff00, 0x3000000
g.mem_map(STATE, 0x10000);g.mem_map(0x2000000, 0x10000);g.mem_map(STOP, 0x1000)
def words(a, v):g.mem_write(a, struct.pack('<'+'I'*len(v), *v))
def floats(a, v):g.mem_write(a, struct.pack('<'+'f'*len(v), *v))
def read(a, n):return struct.unpack('<'+'f'*n, g.mem_read(a, n*4))
def f32(v):return struct.unpack('<f', struct.pack('<f', v))[0]
def reset():
    g.reg_write(UC_X86_REG_ESP, STACK);g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    g.reg_write(UC_X86_REG_FPCW, 0x027f);g.reg_write(UC_X86_REG_FPTAG, 0xffff)
def execute(address):
    words(STACK, [STOP]);reset();g.emu_start(address, STOP, count=20000)
    assert g.reg_read(UC_X86_REG_EIP)==STOP
active = True
def services(uc, address, size, user):
    # Context queries only: the complete observer rotation, view/projection,
    # 142570 transform, 30710 admission and 2B460 rounding run unchanged.
    if address not in (0x14ddc0, 0x14de70, 0x155760):return
    result = (1 if active else 0) if address==0x14ddc0 else 0
    sp=uc.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',uc.mem_read(sp,4))[0]
    uc.reg_write(UC_X86_REG_EAX,result);uc.reg_write(UC_X86_REG_ESP,sp+4)
    uc.reg_write(UC_X86_REG_EIP,ret)
g.hook_add(UC_HOOK_CODE,services)
words(0x4e8c24,[STATE+0x2000]);words(STATE+0x2044,[STATE+0x3000]);words(STATE+0x302c,[1])
words(0x4b9ed8,[0]);g.mem_write(0x4686c4,struct.pack('<h',0))
words(0x4687b0,[STATE+0x1000]);floats(STATE+0x1000,[0,0,1])
words(0x4e6948,[0]);words(0x510c50,[0])
rng=random.Random(0x1396c7);fixtures=[];expected=[]
for extent,outer,safe in [
    ((640,480),(0,0,640,480),(48,36,544,408)),
    ((640,480),(0,0,640,240),(48,36,544,200)),
    ((640,480),(320,240,320,240),(324,244,268,200)),
    ((1280,720),(0,0,640,720),(96,54,536,612)),
    ((3840,2160),(1920,1080,1920,1080),(1924,1084,1628,914)),
    ((640,480),(0,0,640,480),(35,27,570,420))]:
  for wide in (False,True):
    for horizontal in (1.2217304706573486, .7, 1.57):
      for offset in ((0,.165),(0,0),(.2,-.3),(1.1,1.1)):
        for activated in (False,True):
          for yaw,pitch in ((0,0),(.3,.2),(1.9,-.7)):
            active=activated
            yaw,pitch=f32(yaw),f32(pitch)
            forward=tuple(map(f32,(math.cos(pitch)*math.cos(yaw),math.cos(pitch)*math.sin(yaw),math.sin(pitch))))
            up=tuple(map(f32,(-math.sin(pitch)*math.cos(yaw),-math.sin(pitch)*math.sin(yaw),math.cos(pitch))))
            horizontal=f32(horizontal);aspect=f32(4/3);offset=tuple(map(f32,offset))
            position=tuple(map(f32,(-5.106619358,-5.505378723,.629999757)))
            projection=f32(2*math.atan(math.tan(horizontal*.5)/aspect)*.7850000262260437)
            row=list(position+forward+up+(horizontal,aspect)+offset+(projection,float(wide))+outer+safe+(float(active),))
            assert len(row)==24
            g.mem_write(STATE,bytes(0x100));floats(STATE+0x20,forward+up)
            floats(STATE+0x38,(horizontal,aspect)+offset)
            g.reg_write(UC_X86_REG_ESI,STATE);execute(0x171d90)
            aim=read(STATE+0x20,6)+read(STATE+0x48,1)
            # The render camera has the already rotated observer vectors.
            g.mem_write(0x4b9da0,bytes(0x74));floats(0x4b9da0,position+aim[:6])
            floats(0x4b9dc8,(projection,.7850000262260437))
            left,top,width,height=outer
            g.mem_write(0x4b9dd0,struct.pack('<4h',int(top),int(left),int(top+height),int(left+width)))
            left,top,width,height=safe
            g.mem_write(0x4b9dd8,struct.pack('<4h',int(top),int(left),int(top+height),int(left+width)))
            floats(0x4b9de0,(.015625,1024));g.mem_write(0x485ac2,bytes((int(wide),)))
            words(0x4e0350,[0]);g.reg_write(UC_X86_REG_EAX,0x4b9da0)
            g.reg_write(UC_X86_REG_ESI,STATE+0x400);execute(0x2f970)
            frustum=read(STATE+0x400,4)
            half_x=f32(f32(frustum[1]-frustum[0])*.5);half_y=f32(f32(frustum[3]-frustum[2])*.5)
            centers=tuple(f32(f32(f32(frustum[2*i+1]+frustum[2*i])/v)*-.5) for i,v in enumerate((half_x,half_y)))
            clip=(f32(1/f32(half_x*(f32(4/3) if wide else 1))),f32(1/half_y))+centers
            g.reg_write(UC_X86_REG_ESI,0x4b9da0);g.reg_write(UC_X86_REG_EBX,0x4b9e14)
            g.reg_write(UC_X86_REG_EDX,STATE+0x400);execute(0x2fd90)
            floats(0x4e9c8c+0x48,(aim[6],))
            g.reg_write(UC_X86_REG_EAX,4);g.reg_write(UC_X86_REG_ESI,STATE+0x1800)
            execute(0x1396c7)
            fixtures.append(row);expected.append(aim+read(STATE+0x1800,2)+frustum+clip)
native=subprocess.run([str(args.probe),'--probe'],input=''.join(' '.join(map(str,a))+'\n' for a in fixtures),
    text=True,capture_output=True,check=True)
actual=[list(map(float,line.split())) for line in native.stdout.splitlines()]
assert len(actual)==len(expected)
errors=[];maximum=0.;anchor_mismatches=0
for n,(row,want,got) in enumerate(zip(fixtures,expected,actual)):
  for i,(a,b) in enumerate(zip(want,got)):
    maximum=max(maximum,abs(a-b))
    if 7<=i<=8:anchor_mismatches+=a!=b
    if (7<=i<=8 and a!=b) or ((i<7 or i>8) and abs(a-b)>4e-7*max(1,abs(a))):
      errors.append(dict(fixture=n,input=row,component=i,original=a,native=b))
args.fixtures.write_text('# Retail 171D90/2FD90/1396C7, executable SHA256 '+sha+'\n'+
    '# position3 forward3 up3 horizontalFov aspect screenOffset2 projectionAngle widescreen outer4 safe4 active; original rotatedForward3 up3 displacement anchor2 frustum4 clipScale2 clipOffset2\n'+
    ''.join(' '.join(format(v,'.9g') for v in row+list(want))+'\n' for row,want in zip(fixtures,expected)))
args.report.write_text(json.dumps(dict(executable_sha256=sha,fixtures=len(fixtures),
    max_error=maximum,anchor_mismatches=anchor_mismatches,mismatches=errors),indent=2)+'\n')
print(json.dumps(dict(fixtures=len(fixtures),max_error=maximum,anchor_mismatches=anchor_mismatches,mismatches=len(errors))))
if errors:sys.exit(1)
