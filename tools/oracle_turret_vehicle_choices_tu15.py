#!/usr/bin/env python3
"""Execute unchanged English TU1.5 15CDF0 over owned Xbox map references.

This is a bounded cold-template selector oracle. Globals/tag-table/stack are
explicit fixture bindings, not a retail game capture or physics trajectory.
No original instruction/callee/return result is replaced. Random5 is excluded.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools/reference/halo2-decompiled/tools'),str(ROOT/'tools')]
from xbe import Xbe
from audit_rendering import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ECX
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
HEADER,STACK,OUTPUT,STOP=0x61000000,0x62000000,0x63000000,0x64000000

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 result=dict(reference='English TU1.5 only',xbe_path=str(a.xbe.resolve()),xbe_sha256=PIN,entry='0015cdf0',shared_maps=str((ROOT/'assets/disc/maps').resolve()),full_physics_parity=False,boundary=__doc__,maps=[])
 for path in sorted((ROOT/'assets/disc/maps').glob('*.map'))+sorted((ROOT/'assets/map-pack/maps').glob('*.map')):
  if path.name in ('mainmenu.map','shared.map','single_player_shared.map') or path.name[0].isdigit():continue
  c=Cache(path)
  try:
   _,s=c.tag(c.scenario,'scnr');rows=[(i,q)for i,q in enumerate(c.block(s+0x120,0x90))if c.data[q+0x14]in(6,7)]
   if not rows:continue
   u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
   for sec in x.sections:u.mem_write(sec.va,x.section_bytes(sec))
   for at,size in ((HEADER,0x100000),(STACK,0x10000),(OUTPUT,0x1000),(STOP,0x1000)):u.mem_map(at,size)
   mapped=set()
   for group in ('matg','mulg'):
    datum=next(d for d,t in c.tags.items()if t['group']==group);tag,q=c.tag(datum,group);base=tag['address']
    for page in range(base&~4095,(base+tag['size']+4095)&~4095,4096):
     if page not in mapped:u.mem_map(page,4096);mapped.add(page)
    u.mem_write(base,bytes(c.data[q:q+tag['size']]));u.mem_write(HEADER+(datum&65535)*16+8,struct.pack('<I',base))
    if group=='matg':u.mem_write(0x4ecc1c,struct.pack('<I',base))
   u.mem_write(0x4f0414,struct.pack('<I',HEADER))
   calls=[];pcs=[]
   u.hook_add(UC_HOOK_CODE,lambda uc,pc,size,data:pcs.append((pc,size)))
   for row,q in rows:
    default=0xffffffff;variant=0;collection=c.tags.get(c.u32(q+0x5c))
    if collection:
     _,v=c.tag(collection['datum'],'vehc');items=c.block(v,0x10);assert len(items)==1
     default=c.u32(items[0]+8);variant=c.u32(items[0]+12)
    for mode in (0,1,2,3,4,6):
     pcs.clear();u.mem_write(OUTPUT,struct.pack('<I',variant));u.mem_write(STACK+0x8000,struct.pack('<4I',STOP,default,mode,OUTPUT))
     u.reg_write(UC_X86_REG_ESP,STACK+0x8000);u.reg_write(UC_X86_REG_ECX,0)
     u.emu_start(0x15cdf0,STOP,count=500)
     assert u.reg_read(UC_X86_REG_EIP)==STOP
     calls.append(dict(parameter=81+c.data[q+0x14]-6,mode=mode,row=row,definition=u.reg_read(UC_X86_REG_EAX),variant=struct.unpack('<I',u.mem_read(OUTPUT,4))[0],instruction_count=len(pcs),executed_instruction_sha256=hashlib.sha256(b''.join(struct.pack('<II',pc,size)+x.read(pc,size)for pc,size in pcs)).hexdigest()))
   result['maps'].append(dict(cache_path=str(path.resolve()),cache_sha256=hashlib.file_digest(path.open('rb'),'sha256').hexdigest(),cases=calls))
  finally:c.close()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
 print('PASS unchanged original selectors',sum(len(m['cases'])for m in result['maps']),'cases',len(result['maps']),'maps',sum(c['instruction_count']for m in result['maps']for c in m['cases']),'instructions')
if __name__=='__main__':main()
