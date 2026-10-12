#!/usr/bin/env python3
"""Freeze the frontend owner/child geometry caller and actual menu LOD banks.

1337DF executes through the real 4B5D0 stack allocator to 4BCC0. The sole
visibility seam is 4BAF0 (ret8); no distance/coverage argument is substituted.
133E30 independently copies that produced owner collector to the child call.
Literal unit/hlmt/region metadata and immutable captured object headers drive
4BCC0's metric/force-highest branches, 4C450 and 4C2B0 section selection.
Resource readiness is explicit true, as in the captured resident collectors.
"""
import hashlib,json,struct
from pathlib import Path
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail,ROOT,STACK,STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

BASE=ROOT/'analysis/frontend/customization'
LIVE=ROOT/'analysis/frontend/profile-regression/original-model-menu-held-weapon-ik.json'
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def reset(r):
 r.g.reg_write(UC_X86_REG_FPSW,0);r.g.reg_write(UC_X86_REG_FPTAG,0xffff)
def main():
 c=Cache(ROOT/'assets/disc/maps/mainmenu.map');r=Retail()
 r.g.mem_map(0x2010000,0x1000);r.g.mem_map(0x80000000,0x4000000)
 def hook(u,a,n,data):
  if a==0x4baf0:
   sp=u.reg_read(UC_X86_REG_ESP);r.ret(1);u.reg_write(UC_X86_REG_ESP,sp+12)
  elif a==0x12de70:r.ret(1)
 r.g.hook_add(UC_HOOK_CODE,hook)
 scene=r.alloc(bytes(0x2a80));record=r.alloc(bytes(24))
 r.w(scene+0x14,record);r.w(record+8,r.alloc(bytes(4)));r.w(record+20,r.alloc(bytes.fromhex('ffffffff')))
 for i in range(3):r.w(0x4b9da0+i*4,0)
 r.w(0x4b9dcc,word(1));r.w(0x4b9ed0,0x43e480f1);r.w(0x4ba028,word(1));r.w(0x4b9ef0,0)
 for off,v in [(0x20,word(-1)),(0x2c,0),(0x38,0),(0x44,0)]:r.w(0x4b9e00+off,v)
 object_pool=r.alloc(bytes(0x80));descriptors=r.alloc(bytes(0x1000));r.w(object_pool+0x44,descriptors);r.w(0x4e0300,object_pool)
 cache_pool=r.alloc(bytes(0x80));cache_record=r.alloc(bytes(0xc0));r.w(cache_pool+0x44,cache_record);r.w(0x509434,cache_pool)
 context=r.alloc(bytes(0x200));result=r.alloc(bytes(16));geometry=r.alloc(bytes(4))
 r.run(0x2c490,registers=[(UC_X86_REG_EAX,1)])
 copied=set()
 def transplant(datum,group):
  tag,p=c.tag(datum,group)
  if datum not in copied:
   r.g.mem_write(tag['address'],bytes(c.data[p:p+tag['size']]))
   r.g.mem_write(r.table+(datum&65535)*16,struct.pack('<4I',int.from_bytes(group.encode(),'big'),datum,tag['address'],tag['size']))
   copied.add(datum)
  return tag,p
 def owner_arguments(obj,handle):
  reset(r);r.run(0x1337df,stop=0x4bcc0,registers=[(UC_X86_REG_EAX,obj),(UC_X86_REG_ESI,scene),(UC_X86_REG_EBP,0),(UC_X86_REG_EBX,handle),(UC_X86_REG_EDI,0)])
  sp=r.g.reg_read(UC_X86_REG_ESP)
  argv=[r.r(sp+i)for i in range(4,28,4)]
  collector=bytes(r.g.mem_read(STACK+0x10,0x1e4))
  assert struct.unpack_from('<I',collector,0x1b8)[0]==argv[1]
  return argv,collector
 def child_arguments(parent,collector,child_handle):
  # The actual produced owner record includes its handle prefix, before the
  # pointer passed to4B5D0. Its distance survives the original REP MOVSD.
  source=r.alloc(collector);child_record=r.alloc(bytes(24));r.w(child_record+20,0)
  r.w(STACK+0x10,scene)
  reset(r);r.run(0x133e30,stop=0x4bcc0,registers=[(UC_X86_REG_EBX,source),(UC_X86_REG_EAX,child_handle),(UC_X86_REG_EBP,child_record),(UC_X86_REG_EDX,0)])
  sp=r.g.reg_read(UC_X86_REG_ESP);out=[r.r(sp+i)for i in range(4,28,4)]
  assert out[0]==child_handle and out[1]==parent[1],(parent,out)
  return out
 def selection(obj,definition,flags,modifier,aux,previous):
  r.w(obj+4,flags);r.g.mem_write(obj+0x10a,bytes([aux]))
  unit,p=transplant(definition,'bipd'if definition in(0xe23400bf,0xe3110154)else'weap')
  hlmt,mp=transplant(c.u32(p+0x38),'hlmt');render=c.u32(mp+4);transplant(render,'mode')
  bank=list(struct.unpack_from('<9I',c.data,mp+0x28))
  r.w(obj+0xcc,0x12340000);r.w(cache_record+4,0x12340000)
  r.g.mem_write(cache_record+0xbc,bytes([previous&255])*4);r.w(0x4ba050,0);r.g.mem_write(0x4ba019,bytes([modifier]))
  r.w(STACK+0x10,obj);r.w(STACK+0x1c,0x12340000);r.w(STACK+0x20,arguments[1]);r.w(STACK+0x30,geometry)
  r.g.mem_write(geometry,bytes(4));r.w(context+0x16c,0)
  reset(r);r.run(0x4bde5,stop=0x4be90,registers=[(UC_X86_REG_EAX,flags),(UC_X86_REG_EDX,definition),(UC_X86_REG_ESI,r.table),(UC_X86_REG_ECX,geometry),(UC_X86_REG_EDI,context)])
  level=r.g.mem_read(geometry,1)[0];metric=r.g.reg_read(UC_X86_REG_XMM4)&0xffffffff
  wanted=r.alloc(bytes(16));current=r.alloc(bytes(16));sections=r.alloc(bytes(16));fallback=r.alloc(bytes(4))
  r.run(0x4c2b0,argv=[wanted,current,level,0,sections,fallback],registers=[(UC_X86_REG_EAX,render)])
  _,mode=c.tag(render,'mode');count=c.u32(mode+0x1c)
  selected=list(r.g.mem_read(sections,count));assert r.g.reg_read(UC_X86_REG_EAX)&255
  return bank,metric,level,selected,render
 objects=json.loads(LIVE.read_text())['objects'];rows=[];lines=['# Unchanged1337DF/133E30 ABI;4BCC0/4C450/4C2B0 literal menu banks; XBE '+PIN,'# owner-center3,distance,definition,objectflags,aux,scenarioType,previous,bank9,metric,LOD,render,regionCount,sections... hexadecimal.']
 abi=[]
 for index,o in enumerate(objects):
  header=bytes.fromhex(o['object_header_hex']);obj=r.alloc(header)
  handle=0x12340000;arguments,collector=owner_arguments(obj,handle)
  inherited=child_arguments(arguments,collector,0x12340001)
  expected_distance=word((sum(v*v for v in struct.unpack_from('<3f',header,0x30)))**.5)
  assert arguments[1]==expected_distance # Separate check of argument identity.
  abi.append(dict(owner=o['handle'],owner_4bcc0_arguments=[f'{v:08x}'for v in arguments],child_4bcc0_arguments=[f'{v:08x}'for v in inherited],distance_float=struct.unpack('<f',struct.pack('<I',arguments[1]))[0]))
  for kind,current in [('body',o),('weapon',o['held_weapon'])]:
   raw=bytes.fromhex(current['object_header_hex']);live_flags=struct.unpack_from('<I',raw,4)[0]
   target=r.alloc(raw);r.w(descriptors+8,target)
   if kind=='body':
    # Actual2B0C08→BBF40(true), executed on each requested visible body.
    r.run(0xbbf40,argv=[1],registers=[(UC_X86_REG_EAX,handle)])
   flags=r.r(target+4);datum=int(current['definition'],16);aux=raw[0x10a]
   bank,metric,lod,sections,render=selection(target,datum,flags,1,aux,-1)
   for previous in [-1,lod]:
    bank,metric,level,sections,render=selection(target,datum,flags,1,aux,previous)
    values=[*struct.unpack_from('<3I',header,0x30),arguments[1],datum,flags,aux,2,previous&255,*bank,metric,level,render,len(sections),*sections]
    lines.append(' '.join(f'{v:08x}'for v in values))
    rows.append(dict(object_index=index,kind=kind,owner_handle=o['handle'],definition=current['definition'],captured_flags=f'{live_flags:08x}',visible_flags=f'{flags:08x}',previous_lod=previous,distance_word=f'{arguments[1]:08x}',metric_word=f'{metric:08x}',geometry_lod=level,render_model=f'{render:08x}',sections=sections))
 assert all(v['geometry_lod']==(5 if v['kind']=='body'else 0)for v in rows)
 boundary=[]
 for weapon in [objects[0]['held_weapon'],objects[4]['held_weapon']]:
  datum=int(weapon['definition'],16)
  for scenario in [0,2]:
   for aux in [0,4]:
    for flags in [0,0x80000]:
     raw=bytearray(bytes.fromhex(weapon['object_header_hex']));struct.pack_into('<3f',raw,0x30,6,0,0)
     target=r.alloc(raw);arguments,_=owner_arguments(target,0x12340000)
     bank,metric,level,sections,render=selection(target,datum,flags,int(scenario==2),aux,-1)
     values=[*struct.unpack_from('<3I',raw,0x30),arguments[1],datum,flags,aux,scenario,255,*bank,metric,level,render,len(sections),*sections]
     lines.append(' '.join(f'{v:08x}'for v in values))
     boundary.append(dict(definition=weapon['definition'],scenario_type=scenario,auxiliary_byte=aux,object_flags=f'{flags:08x}',metric_word=f'{metric:08x}',geometry_lod=level,sections=sections))
 (BASE/'frontend-model-geometry-original.tsv').write_text('\n'.join(lines)+'\n')
 report=dict(method=__doc__.splitlines()[2:],original_xbe_sha256=PIN,mainmenu_sha256=hashlib.sha256(c.data).hexdigest(),captured_input_sha256=hashlib.sha256(LIVE.read_bytes()).hexdigest(),caller_abi_cases=abi,geometry_cases=rows,explicit_branch_cases=boundary,boundary='All ten immutable owner headers and both weapon definitions at initial/retained source LODs. Body bit19 is executed via BBF40 for visible widget objects. Children inherit whole-owner distance. Literal mainmenu scenario2 enables the original .3 modifier; auxiliary bit2 is absent in actual ten captured objects. Sixteen explicit cases separately exercise source .3/1.7 scales and force-highest ordering. Visibility/resources are explicit seams; source center production and all-frame native stability require separate tests.')
 (BASE/'frontend-model-geometry-original.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()
