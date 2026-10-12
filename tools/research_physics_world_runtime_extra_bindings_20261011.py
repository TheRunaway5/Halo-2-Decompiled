#!/usr/bin/env python3
"""Authenticate the world dispatcher and event-listener constructor anchors."""
import argparse,hashlib,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
import research_mp_sandbox_update_functions as base

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--parent',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 parent=json.loads(a.parent.read_text());assert parent['tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_bindings_20261011.py')
 assert parent['disc_sha256']==PIN and parent['later_sha256']==base.NEW_PIN
 oldp=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldp)==PIN and sha(base.NEW_DEFAULT)==base.NEW_PIN
 old,new=Xbe(oldp),Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text');starts=[]
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True
 for image in (old,new):
  section=image.section('.text');found=set()
  for i in md.disasm(image.section_bytes(section),section.va):
   if i.id and i.mnemonic=='call' and i.operands[0].type==2:found.add(i.operands[0].imm)
  starts.append(found)
 def body(code,p,index):return [code.at(x) for x in sorted(_trace(code,p,starts[index]|{p}).code)]
 mapping=dict(parent['address_map']);pairs=[]
 # The source CFGs containing actual vtable stores2EB55E and3129EF have unique
 # direct-call entries2EB540 and3129E0. Match the entire paired constructor.
 for p in (0x2eb540,0x3129e0):
  oi=body(oc,p,0);prefix=[base.norm(i,old) for i in oi[:5]];matches=[]
  for q in starts[1]:
   try:
    cursor=q;probe=[]
    for _ in prefix:
     i=nc.at(cursor);probe.append(base.norm(i,new));cursor+=i.size
    if prefix!=probe:continue
    ni=body(nc,q,1)
    if base.full_tokens(oi,old)==base.full_tokens(ni,new):matches.append((q,ni))
   except Exception:continue
  assert len(matches)==1,(hex(p),[(hex(q),len(ins)) for q,ins in matches])
  q,ni=matches[0];pairs.append((p,q,oi,ni))
  for x,y in zip(oi,ni):
   for xo,yo in zip(x.operands,y.operands):
    if xo.type==yo.type==2 and base.containing(old,xo.imm) and base.containing(new,yo.imm):
     key=f'{xo.imm:08x}';assert key not in mapping or mapping[key]==yo.imm;mapping[key]=yo.imm
 slots=list(parent['verified_virtual_slots'])
 for table,offset in ((0x418c40,0x14),(0x419ee8,0)):
  p,q=table+offset,mapping[f'{table:08x}']+offset
  x,y=int.from_bytes(old.read(p,4),'little'),int.from_bytes(new.read(q,4),'little');oi,ni=body(oc,x,0),body(nc,y,1)
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(x),hex(y));pairs.append((x,y,oi,ni))
  key=f'{x:08x}';assert key not in mapping or mapping[key]==y;mapping[key]=y
  slots.append(dict(old_slot=p,later_slot=q,old_entry=x,later_entry=y,complete_body_equal=True))
 a.output.mkdir(parents=True,exist_ok=False);rows=[]
 for p,q,oi,ni in pairs:
  row=dict(old_entry=p,later_entry=q,instructions=len(oi),complete_body_equal=True)
  for label,ins in (('disc',oi),('later',ni)):
   file=a.output/f'{p:08x}-{label}.asm';file.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins));row[label+'_listing']=dict(file=file.name,sha256=sha(file))
  rows.append(row)
 result=dict(parent,tool_sha256=sha(__file__),parent_input_bindings_sha256=sha(a.parent),additional_routines=rows,address_map=mapping,verified_virtual_slots=slots,
  scope='Parent world bindings plus actual dispatcher/listener constructor stores2EB55E and3129EF, complete paired constructors and slot14/slot0 callees. Input rebinding only.')
 file=a.output/'report.json';file.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(sha256=sha(file),rows=len(rows),slots=len(slots))))
if __name__=='__main__':main()
