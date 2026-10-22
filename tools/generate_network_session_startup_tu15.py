#!/usr/bin/env python3
"""Compile the authenticated TU1.5 network session constructor call graph.

Existing source modules and platform APIs retain their own dispatch/bank
ownership. No code/data section, live snapshot or supplied constructor result
is a native runtime input. Unowned indirect callbacks remain required owners.
"""
import argparse,hashlib,json,re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
 assert digest(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 starts={0x6e830,0x673b0};code.md.skipdata=True
 for i in code.md.disasm(code.bytes,code.lo):
  if i.mnemonic=='call' and i.operands[0].type==2 and code.inside(i.operands[0].imm):starts.add(i.operands[0].imm)
 code.md.skipdata=False
 support=set(int(p,16)for p in re.findall(r'static void run_([a-f0-9]+)\(', (ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()))
 existing=set(int(p,16)for p in re.findall(r'\{0x([a-f0-9]+)u,0x[a-f0-9]+u,Module::\w+\}', (ROOT/'src/physics/physics_live_entries_tu15.hpp').read_text()))
 assert int.from_bytes(image.read(0x483808,4),"little")==0x4736ac
 assert int.from_bytes(image.read(0x4736bc,4),"little")==0x673b0
 pending=[0x6e830,0x673b0];seen=set();bodies={};external=set()
 while pending:
  p=pending.pop(0)
  if p in seen:continue
  seen.add(p)
  if p!=0x6e830 and (p in support or p in existing or p==0x2e0b1e or not code.inside(p)):
   external.add(p);continue
  assert len(bodies)<1000,'Unexpected graph expansion requires a fresh ownership audit'
  body=_trace(code,p,starts);bodies[p]=body
  pending.extend(body.calls);pending.extend(body.tail_jumps)
 entries={f'{p:08x}':p for p in sorted(bodies)};sites={};instructions={};addresses={};routines=[]
 for p,body in sorted(bodies.items()):
  sites[f'{p:08x}']=sorted(body.code)
  routines.append(dict(entry=p,tables=[dict(disc=q,count=n)for q,w,n in body.tables if w==4],
   byte_tables=[dict(address=q,width=w,count=n,raw_hex=image.read(q,w*n).hex())for q,w,n in body.tables if w==1]))
  assert all(w in (1,4)for _,w,_ in body.tables)
  for q in sorted(body.code):
   i=code.at(q);assert bytes(i.bytes)==image.read(q,i.size)
   instructions[f'{q:08x}']=q;addresses[q]=q;addresses[q+i.size]=q+i.size
   for o in i.operands:
    if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
    if o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
 paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries=entries,body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},routines=routines)
 a.correspondence=a.manifest.with_name('network-session-bodies.json');a.correspondence.parent.mkdir(parents=True,exist_ok=True)
 a.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');a.additional_entries=Path(__file__)
 generate(a,paired,complete_listings=True)
 source=a.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_network_session_startup_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_network_session_startup_tu15').replace('source_physics_live_support_tu15_owns_entry','source_network_session_startup_tu15_owns_entry')
 a.output.write_text(source)
 # Compile only referenced immutable scalar/vector lanes and explicit branch
 # identity tables. Mutable records have separate constructor-owned extents.
 parameters={}
 for body in bodies.values():
  for q in sorted(body.code):
   for o in code.at(q).operands:
    if o.type!=3 or o.mem.base or o.mem.index or o.mem.segment or o.size not in (1,2,4,8,16):continue
    p=o.mem.disp&0xffffffff
    sections=[s for s in image.sections if s.va<=p<p+o.size<=s.va+s.vsize]
    if len(sections)==1 and sections[0].name=='.rdata':
     for k,value in enumerate(image.read(p,o.size)):parameters[p+k]=value
  for p,width,count in body.tables:
   assert width in (1,4)
   for k,value in enumerate(image.read(p,width*count)):parameters[p+k]=value
 rows=[]
 while parameters:
  p=min(parameters);n=4 if all(p+k in parameters for k in range(4)) else 2 if p+1 in parameters else 1
  value=sum(parameters.pop(p+k)<<(8*k)for k in range(n));rows.append((p,n,value))
 operand_file=a.output.with_name('native_tu15_network_session_parameters.inc')
 operand_file.write_text('// Typed scalar/vector lanes and branch identities from authenticated constructor instructions.\n'+''.join(f'{{0x{p:x}u,{n},0x{v:x}u}},\n'for p,n,v in rows))
 report=json.loads(a.manifest.read_text());report.update(typed_parameter_file_sha256=digest(operand_file),typed_parameter_fields=len(rows))
 report.update(network_session_generator_sha256=digest(__file__),generated_cpp_sha256=digest(a.output),source_entry='0006e830',external_entry_identities=[f'{p:08x}'for p in sorted(external)],existing_support_source_sha256=digest(ROOT/'src/physics/physics_live_support_tu15_generated.cpp'),existing_entry_metadata_sha256=digest(ROOT/'src/physics/physics_live_entries_tu15.hpp'))
 a.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(complete_source_bodies=len(bodies),instructions=len(instructions),external_entries=len(external))))
if __name__=='__main__':main()
