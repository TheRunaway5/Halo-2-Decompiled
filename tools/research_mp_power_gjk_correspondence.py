#!/usr/bin/env python3
"""Complete GJK retrieve/support/simplex authenticated update correspondence."""
import argparse,json,sys
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM
import research_mp_sandbox_update_functions as base
from oracle_mp_power_gjk_owners_1_1 import ROOT,OUT,verify,sha
from oracle_mp_power_body_prepare_1_1 import OUT as WRAPPER_OUT
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
ENTRIES={0x2e2320,0x3039c0,0x1f1da0,0x2e4320,0x30ae40,0x30eb30,0x309900,0x309d40,0x309ae0,0x30a370}
SHORT=set()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'final/later-correspondence/report.json');a=ap.parse_args();verify()
 base.ENTRIES=tuple(sorted(ENTRIES));sys.argv=[sys.argv[0],'--output',str(a.output)];base.main();r=json.loads(a.output.read_text())
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapped={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']if q.get('normalized_instruction_and_internal_branch_topology_equal')}
 starts=set(mapped.values());md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;sec=new.section('.text')
 for i in md.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:starts.add(i.operands[0].imm)
 while True:
  before=len(mapped)
  for left,right in list(mapped.items()):
   oi=[oc.at(p)for p in sorted(_trace(oc,left,ENTRIES|SHORT).code)];ni=[nc.at(p)for p in sorted(_trace(nc,right,starts).code)]
   assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(left),hex(right))
   for x,y in zip(oi,ni):
    if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==X86_OP_IMM and x.operands[0].imm in ENTRIES|SHORT:
     p,q=x.operands[0].imm,y.operands[0].imm;assert p not in mapped or mapped[p]==q;mapped[p]=q
  if before==len(mapped):break
 assert ENTRIES<=mapped.keys(),[hex(p)for p in ENTRIES-mapped.keys()]
 virtual=[]
 wp=ROOT/'analysis/multiplayer/sandbox-source/power-child-agents/final/later-correspondence/complete-report.json'
 bp=ROOT/'analysis/multiplayer/sandbox-source/power-bsp-children/final/later-correspondence/complete-report.json'
 assert ENTRIES|SHORT==mapped.keys()
 previous={int(q['disc_1_1_entry'],16):q for q in r['routines']};rows=[]
 for p,b in sorted(mapped.items()):
  oi=[oc.at(x)for x in sorted(_trace(oc,p,ENTRIES|SHORT).code)];ni=[nc.at(x)for x in sorted(_trace(nc,b,starts|set(mapped.values())).code)]
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(p),hex(b))
  q=previous.get(p,dict(disc_1_1_entry=f'{p:08x}',mapped_from_original_constructor_or_direct_call=True))
  q.update(later_entry=f'{b:08x}',old_instructions=len(oi),later_instructions=len(ni),normalized_instruction_and_internal_branch_topology_equal=True)
  for label,ins,image,entry in(('disc',oi,old,p),('later',ni,new,b)):
   dst=a.output.parent/'instructions'/f'{p:08x}-{label}-{entry:08x}.asm'
   for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
   frozen_write(dst,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());q[label+'_listing']=dict(file=dst.name,sha256=sha(dst))
  of,nf=_trace(oc,p,ENTRIES|SHORT),_trace(nc,b,starts|set(mapped.values()));assert len(of.tables)==len(nf.tables)
  op={i.address:n for n,i in enumerate(oi)};np={i.address:n for n,i in enumerate(ni)};tables=[]
  for left,right in zip(of.tables,nf.tables):
   at,width,count=left;bt,nwidth,ncount=right;assert(width,count)==(nwidth,ncount)
   ob,nb=old.read(at,width*count),new.read(bt,width*count)
   if width==4:
    keys=lambda raw,positions:[positions.get(int.from_bytes(raw[k:k+4],'little'),'ZERO'if int.from_bytes(raw[k:k+4],'little')==0 else'EXTERNAL')for k in range(0,len(raw),4)]
    assert keys(ob,op)==keys(nb,np)
   else:assert ob==nb
   tables.append(dict(old_address=f'{at:08x}',later_address=f'{bt:08x}',width=width,entries=count,old_hex=ob.hex(),later_hex=nb.hex(),mapped_table_equal=True))
  refs=[];calls=[]
  for x,y in zip(oi,ni):
   for xo,yo in zip(x.operands,y.operands):
    if xo.type==yo.type==3 and not xo.mem.base and not yo.mem.base and base.containing(old,xo.mem.disp)and base.containing(new,yo.mem.disp):
     assert xo.size==yo.size;ob,nb=old.read(xo.mem.disp,xo.size),new.read(yo.mem.disp,yo.size)
     refs.append(dict(old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_address=f'{xo.mem.disp:08x}',later_address=f'{yo.mem.disp:08x}',width=xo.size,old_hex=ob.hex(),later_hex=nb.hex(),bytes_equal=ob==nb))
   if x.mnemonic=='call'and x.operands[0].type==y.operands[0].type==X86_OP_IMM:
    target=x.operands[0].imm;actual=y.operands[0].imm;calls.append(dict(old_site=f'{x.address:08x}',later_site=f'{y.address:08x}',old_target=f'{target:08x}',later_target=f'{actual:08x}',mapped_target_matches=mapped.get(target)==actual if target in mapped else None))
  q.update(switch_tables=tables,switch_table_topology_equal=True,paired_image_memory_operands=refs,paired_direct_calls=calls);rows.append(q)
 r.update(routines=rows,correspondence_tool_sha256=sha(__file__),source_measured_sha256=sha(OUT/'final/measured/report.json'),complete_bodies=len(mapped),original_vtable_owners=virtual,wrapper_prior_report_sha256=sha(wp),bsp_prior_report_sha256=sha(bp),later_gameplay_executed=False)
 frozen_write(a.output.parent/'complete-report.json',encoded(r));print(json.dumps(dict(complete_bodies=len(mapped),report=sha(a.output.parent/'complete-report.json'))))
if __name__=='__main__':main()
