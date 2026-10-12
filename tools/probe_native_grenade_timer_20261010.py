#!/usr/bin/env python3
"""Execute unchanged FCF90/FDEB0 timer prefix over real captured grenade bytes.

Only this explicitly bounded ABI kernel gets declared pool/tag/time bindings and
varied scalar controls. It does not create a live Unit or provide world returns.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM,X86_OP_REG
PIN='b459716943cc586b46be8b7fac796aea57349ff8efdb804a9c8f041890b2674d'
IMAGES=[('tu11',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',0xfcf90,0xfd0f8,
'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'),
('u5',Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'),0xfdeb0,0xfe018,
'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd')]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def raw(x,p,n):
 for s in x.sections:
  b=x.section_bytes(s)
  if s.va<=p and p+n<=s.va+len(b):return b[p-s.va:p-s.va+n]
 raise ValueError(hex(p))
def fields(b):return dict(flags=struct.unpack_from('<I',b,0x12c)[0],state=struct.unpack_from('<h',b,0x130)[0],
 timer_bits=struct.unpack_from('<I',b,0x158)[0],timer_step_bits=struct.unpack_from('<I',b,0x15c)[0],
 arming_bits=struct.unpack_from('<I',b,0x160)[0],arming_step_bits=struct.unpack_from('<I',b,0x164)[0],
 age=struct.unpack_from('<h',b,0x1aa)[0])
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 prior=ROOT/'analysis/multiplayer/integration/live-grenade-20261010/release-probe/report.json';assert digest(prior)==PIN
 source=json.loads(prior.read_text());obj=next(o for o in source['states'][183]['objects']if o['datum']=='e2710002');body=bytes.fromhex(obj['body_hex']);assert len(body)==522
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');tag=cache.tags[int(obj['definition'],16)];definition=bytes(cache.data[tag['offset']:tag['offset']+tag['size']])
 cases=[]
 for start,flags,timer,state,age in itertools.product((0,1,2,3,-1),(2,6,18,0x802,10,34), (0.,14/15,1.),(0,1,2),(0,32767)):
  b=bytearray(body);struct.pack_into('<Ih',b,0x12c,flags,state);struct.pack_into('<f',b,0x158,timer);struct.pack_into('<h',b,0x1aa,age)
  cases.append(dict(start=start,before=bytes(b)))
 report=dict(schema='grenade-timer-prefix-20261010-v1',prior=dict(path=str(prior.relative_to(ROOT)),sha256=PIN,state=183,datum=obj['datum']),
 tool_sha256=digest(Path(__file__)),boundaries=['Actual captured source projectile522 bytes. Explicit low16 pool/header/tag/time locator ABI, not allocator/world initialization.',
 'Varied timer-start/contact flags/state/age/timer are declared kernel controls only. Flag1 remains set so initial-effect-owner branch is not supplied.',
 'Stop before FD0F8/FE018 and all world/contact/detonation services. Default masked x87/MXCSR; no Xbox hardware FP-status claim.'],images=[])
 signatures=[];canonical=[]
 for name,path,entry,end,pin in IMAGES:
  assert digest(path)==pin;x=Xbe(path);cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True;ins=list(cs.disasm(raw(x,entry,end-entry),entry));assert ins[-1].address+ins[-1].size==end
  absrefs=[];signature=[];calltargets=[]
  for i in ins:
   ops=[]
   for o in i.operands:
    if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index:
     if o.mem.disp not in absrefs:absrefs.append(o.mem.disp)
     ops.append(('absolute',absrefs.index(o.mem.disp),o.size))
    elif o.type==X86_OP_IMM and i.group(1):ops.append(('branch',o.imm-entry))
    elif o.type==X86_OP_IMM and i.group(2):
     calltargets.append(f'{o.imm:08x}');ops.append(('external_unexecuted_call',len(calltargets)-1))
    elif o.type==X86_OP_REG:ops.append(('register',o.reg,o.size))
    elif o.type==X86_OP_MEM:ops.append(('memory',o.mem.base,o.mem.index,o.mem.scale,o.mem.disp,o.size))
    elif o.type==X86_OP_IMM:ops.append(('immediate',o.imm,o.size))
    else:raise AssertionError((i.mnemonic,i.op_str,o.type))
   signature.append((i.mnemonic,ops))
  signatures.append(signature);assert len(absrefs)==5,absrefs
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x900000)
  for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
  g.mem_map(0x1000000,0x200000);g.mem_map(0x2000000,0x10000)
  pool,headers,table,baddr,daddr,time=0x1000000,0x1001000,0x1010000,0x1110000,0x1120000,0x1130000
  w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
  for p,v in zip(absrefs[:3],(pool,table,time)):w(p,v)
  w(pool+0x44,headers);w(headers+2*12+8,baddr);w(table+(int(obj['definition'],16)&65535)*16+8,daddr)
  g.mem_write(daddr,definition);g.mem_write(time,struct.pack('<IfI',0,1/30,0));stack=0x200f000
  stores=[]
  def hook(uc,access,address,size,value,user):
   if baddr<=address<baddr+len(body):stores.append(dict(instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}',offset=address-baddr,bytes=size,value=value&((1<<(size*8))-1)))
  g.hook_add(UC_HOOK_MEM_WRITE,hook);rows=[];listing='\n'.join(f'{i.address:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}' for i in ins)+'\n'
  for index,c in enumerate(cases):
   g.mem_write(baddr,c['before']);g.mem_write(daddr+0xc0,struct.pack('<h',c['start']));w(stack,0x3000000);w(stack+4,int(obj['datum'],16));g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80);stores.clear()
   g.emu_start(entry,end,count=1000);assert g.reg_read(UC_X86_REG_EIP)==end
   after=bytes(g.mem_read(baddr,len(body)));rows.append(dict(id=index,start=c['start'],before_hex=c['before'].hex(),after_hex=after.hex(),before=fields(c['before']),after=fields(after),stores=stores.copy()))
   if name=='tu11':canonical.append((c['start'],c['before'],after))
   else:assert after==canonical[index][2],index
  report['images'].append(dict(name=name,path=str(path),sha256=pin,entry=f'{entry:08x}',stop=f'{end:08x}',absolute_refs=[f'{v:08x}'for v in absrefs],one_hex=raw(x,absrefs[-1],4).hex(),unexecuted_call_targets=calltargets,cases=rows))
  a.output.mkdir(parents=True,exist_ok=True);(a.output/(name+'.asm')).write_text(listing)
 assert signatures[0]==signatures[1],next((i,a,b)for i,(a,b)in enumerate(zip(*signatures))if a!=b)
 fixture=b'GRT1'+struct.pack('<I',len(canonical))+b''.join(struct.pack('<h',s)+b+a for s,b,a in canonical)
 (a.output/'fixtures.bin').write_bytes(fixture);report['fixture_sha256']=hashlib.sha256(fixture).hexdigest()
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n');print(json.dumps(dict(cases=len(canonical),report_sha256=digest(a.output/'report.json'),fixture_sha256=report['fixture_sha256'])))
if __name__=='__main__':main()
