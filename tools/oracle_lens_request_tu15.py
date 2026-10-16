#!/usr/bin/env python3
"""Execute whole untouched TU1.5 2DB80 request admission and actual callees.

Actual lens definitions come from every surveyed direct attachment and effe
lens part. View, scale, orientation and retained request-pool state are declared
caller inputs. This does not establish pool construction or GPU occlusion.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from audit_rendering import Cache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();assert not a.fixture.exists() and not a.report.exists()
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 original=bytes(u.mem_read(0x10000,0x800000));u.mem_map(0x2400000,0x300000);u.mem_map(0x2800000,0x1000)
 regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
 trace=[];reads=set()
 def code(cpu,pc,n,_):
  assert bytes(cpu.mem_read(pc,n))==x.read(pc,n)
  trace.append(pc)
 u.hook_add(UC_HOOK_CODE,code)
 u.hook_add(UC_HOOK_MEM_READ,lambda g,t,p,n,v,_:reads.update(range(p&~4095,(p+n+4095)&~4095,4096)) if p<0x800000 else None)
 inventory=json.loads(Path('analysis/map-world-features/inventory.json').read_text());definitions=[];maps={};seen=set()
 for m in inventory['maps']:
  path=Path(m['cache']);assert hashlib.sha256(path.read_bytes()).hexdigest()==m['cache_sha256'];maps[m['map']]=m['cache_sha256'];c=Cache(path)
  ids=[]
  for obj in m['placements']:
   for attachment in obj['attachments']:
    t=attachment['definition']
    if not t:continue
    if t['group']=='lens':ids.append(int(t['datum'],16))
    if t['group']=='effe':
     _,at=c.tag(int(t['datum'],16),'effe')
     for event in c.block(at+0x14,0x38):
      for part in c.block(event+0x18,0x38):
       if c.u32(part+8)==0x6c656e73 and c.u32(part+0x10)!=0xffffffff:ids.append(c.u32(part+0x10))
  for datum in ids:
   if (m['map'],datum)in seen:continue
   seen.add((m['map'],datum));tag,at=c.tag(datum,'lens');definitions.append((m['map'],tag,bytes(c.data[at:at+tag['size']])))
  c.close()
 rows=[]
 for mapname,tag,definition in definitions:
  for variant in ['accepted','owner-view','no-view','no-tag','zero-scale','zero-fade','disabled-pool','full-pool']:
   u.mem_write(0x10000,original)
   table,root,position,axis,stack=0x2400000+(tag['datum']&65535)*16,0x2500000,0x2501000,0x2501020,0x2600000
   globals=bytearray(0x1000);struct.pack_into('<3f',globals,0x5c0,0,0,0);struct.pack_into('<3f',globals,0x5cc,1,0,0);struct.pack_into('<I',globals,0x6f4,0xffffffff if variant=='no-view' else 0)
   pool=bytearray(0x6000);pool[0xfd8]=0 if variant=='disabled-pool' else 1;struct.pack_into('<I',pool,0x5094,0)
   if variant=='full-pool':
    struct.pack_into('<I',pool,0x4fbc,511)
    for n in range(511):struct.pack_into('<II',pool,0xfdc+n*32,0x00ffffff,1)
   rawstack=bytearray([0xcd])*0x8000
   args=[int(variant=='owner-view'),1,0,0,position,axis,0 if variant=='zero-scale' else 0x3f800000,0 if variant=='zero-fade' else 0x3f800000,0x3f800000]
   struct.pack_into('<10I',rawstack,0x4000,0x2800000,*args)
   banks=[(0x4c6000,bytes(globals)),(0x4f0414,struct.pack('<I',0x2400000)),(0x52d000,bytes(pool)),(table,struct.pack('<4I',tag['datum'],0,root,len(definition))),(root,definition),(position,struct.pack('<3f',1,0,0)),(axis,struct.pack('<3f',0,0,1)),(stack,bytes(rawstack))]
   for address,data in banks:u.mem_write(address,data)
   gp=[0x72000000+n*0x1000 for n in range(8)];gp[1]=0xffffffff if variant=='no-tag' else tag['datum'];gp[4]=stack+0x4000;gp[7]=axis
   for reg,value in zip(regs,gp):u.reg_write(reg,value)
   for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
   for reg,value in [(R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)]:u.reg_write(reg,value)
   trace.clear();reads.clear();u.emu_start(0x2db80,0x2800000,count=100000);assert u.reg_read(R.UC_X86_REG_EIP)==0x2800000, (mapname,variant,hex(u.reg_read(R.UC_X86_REG_EIP)),len(trace))
   low=[]
   for page in sorted(reads):
    data=bytearray(u.mem_read(page,4096))
    for address,value in banks:
     lo,hi=max(address,page),min(address+len(value),page+4096)
     if lo<hi:data[lo-page:hi-page]=value[lo-address:hi-address]
    low.append((page,bytes(data)))
   low=[(address,data)for address,data in low if not 0x52d000<=address<0x533000]
   low.append((0x52d000,bytes(pool)))
   high=[(address,data)for address,data in banks if address>=0x800000]
   final=[(0x52d000,bytes(u.mem_read(0x52d000,0x6000))),(stack+0x4000,bytes(u.mem_read(stack+0x4000,40)))]
   rows.append(dict(map=mapname,tag=tag['name'],variant=variant,entry_gp=gp,exit_gp=[u.reg_read(reg)for reg in regs],eflags=u.reg_read(R.UC_X86_REG_EFLAGS),exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],x87_status=u.reg_read(R.UC_X86_REG_FPSW),mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),instructions=trace.copy(),initial_banks=[dict(address=address,hex=data.hex())for address,data in low+high],final_banks=[dict(address=address,hex=data.hex())for address,data in final],accepted=bool(u.reg_read(R.UC_X86_REG_EAX)&255)))
 assert len(maps)==21 and any(r['accepted']for r in rows) and any(0x2dcc7 in r['instructions']for r in rows)
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.report.parent.mkdir(parents=True,exist_ok=True)
 banks=[];bank_ids={}
 for row in rows:
  for key in ['initial_banks','final_banks']:
   refs=[]
   for bank in row[key]:
    identity=(bank['address'],bank['hex'])
    if identity not in bank_ids:bank_ids[identity]=len(banks);banks.append(bank)
    refs.append(bank_ids[identity])
   row[key]=refs
 a.fixture.write_text(json.dumps(dict(entry=0x2db80,original_sha256=NEW_PIN,pool_capacity=511,banks=banks,cases=rows),separators=(',',':'))+'\n')
 report=dict(passed=True,cases=len(rows),authored_definitions=len(definitions),maps=maps,accepted=sum(r['accepted']for r in rows),original_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[],replaced_services=[],boundary=__doc__,full_goal_complete=False)
 a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS whole original lens requests',len(rows),'cases',report['accepted'],'admitted')
if __name__=='__main__':main()
