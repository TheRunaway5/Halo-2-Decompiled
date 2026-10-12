#!/usr/bin/env python3
"""Freeze the unchanged237C04→235E5E→23675A selection/caret caller ABI.

Find-child, plain-string update, measured font bounds, rasterizer readiness,
and the GPU sink are bounded seams. Bounds are independently verified by the
original font oracle. No original draw, UV/clamp, color or blend instructions
are replaced.
"""
import sys,struct,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_multiplayer_hud_positions import Retail,PIN
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
r=Retail();g=r.g;c=Cache(ROOT/'assets/disc/maps/mainmenu.map')
def puttag(datum):
 tag,p=c.tag(datum,'bitm');qs=c.block(p+68,116);pages=r.alloc(c.data[qs[0]:qs[0]+len(qs)*116]);hdr=bytearray(76);struct.pack_into('<II',hdr,68,len(qs),pages);h=r.alloc(hdr);r.w(r.table+(datum&65535)*16+8,h);return h,pages
selection,pages=puttag(0xe4750255);caret,caretpages=puttag(0xe4d702b7)
interface=r.alloc(bytes(0x130));r.w(interface+0x64,0xe4750255);matg=r.alloc(bytes(0x120));r.w(matg+0x108,1);r.w(matg+0x10c,interface);r.w(0x4e034c,matg)
wigl=r.alloc(bytes(0x200));r.w(wigl+0x114,0xe4d702b7)
field=r.alloc(bytes(0x100));g.mem_write(field+0x42,struct.pack('<H',4));keyboard=r.alloc(bytes(0x1000));string=r.alloc('Halo0001'.encode('utf-16le')+bytes(2));r.w(keyboard+0x818,string);r.w(keyboard+0x81c,string+16);g.mem_write(keyboard+0x828,b'\1');g.mem_write(keyboard+0x64,struct.pack('<f',1.));r.w(0x54d5b8,0)
draws=[];calls=[]
def ret(value=0,pop=0):
 sp=g.reg_read(UC_X86_REG_ESP);r.ret(value);g.reg_write(UC_X86_REG_ESP,sp+4+pop)
def hook(u,a,s,d):
 sp=u.reg_read(UC_X86_REG_ESP)
 if a==0x22edb8:ret(field,12)
 elif a==0x238cba:ret()
 elif a==0x253ab4:g.mem_write(r.r(sp+8),struct.pack('<4h',167,72,189,137));ret(pop=8)
 elif a==0x253a73:g.mem_write(r.r(sp+12),struct.pack('<4h',167,137,187,138));ret(pop=12)
 elif a==0x148350:ret(wigl)
 elif a==0x12360:ret(1)
 elif a==0x235e5e:calls.append(dict(bitmap=hex(u.reg_read(UC_X86_REG_ECX)),uv=list(struct.unpack('<4f',u.mem_read(u.reg_read(UC_X86_REG_EAX),16))),args=[hex(r.r(sp+v))for v in[4,8,12,16]]))
 elif a==0x52040:
  p=r.r(sp+4);v=r.r(sp+8);draws.append(dict(bitmap=hex(r.r(p+12)),mode=struct.unpack('<H',u.mem_read(p+0x94,2))[0],descriptor=u.mem_read(p,0x98).hex(),vertices=u.mem_read(v,80).hex()));ret(pop=8)
g.hook_add(UC_HOOK_CODE,hook);r.run(0x237c04,argv=[r.alloc(struct.pack('<4h',0,0,480,640))],registers=[(UC_X86_REG_ECX,keyboard)])
assert [d['mode']for d in draws]==[0,0]
assert calls[0]['args'][1:]==['0xffffffff','0x1','0x0']
report=dict(xbe_sha256=PIN,calls=calls,draws=draws,
 boundary_stubs=['22EDB8 find-child returns suppliedfield','238CBA plain-string update already supplied',
 '253AB4/253A73 verified font bounds supplied','148350 sharedUIglobals supplied',
 '12360 resource readiness supplied','52040 GPU draw sink captured'],
 scope='Unchanged237C04,235E5E,23675A,236235 and137610 draw/UV/clamp/color/descriptor instructions execute. Fullcaller provesselectionclamp1/blend0 andcaretblend0. NoGPUtexturebinding/pixelresultclaim.')
output=ROOT/'analysis/frontend/profile-regression/keyboard-draw-original.json'
output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
