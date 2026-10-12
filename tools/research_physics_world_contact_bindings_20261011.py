#!/usr/bin/env python3
"""Authenticate the allocator static initializer and constructor data anchors."""
import argparse,hashlib,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
import research_mp_sandbox_update_functions as base

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 oldp=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(oldp)==PIN and sha(base.NEW_DEFAULT)==base.NEW_PIN
 old,new=Xbe(oldp),Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;s=new.section('.text');starts=set();candidates=[]
 for i in md.disasm(new.section_bytes(s),s.va):
  if not i.id:continue
  if i.mnemonic=='call' and i.operands[0].type==2:starts.add(i.operands[0].imm)
  if i.mnemonic=='push' and i.operands[0].type==2 and i.operands[0].imm==0x4000:candidates.append(i.address)
 def body(c,p):return [c.at(x) for x in sorted(_trace(c,p,starts|{p}).code)]
 oi=body(oc,0x37d8c0);matches=[]
 for p in candidates:
  try:ni=body(nc,p)
  except Exception:continue
  if base.full_tokens(oi,old)==base.full_tokens(ni,new):matches.append((p,ni))
 assert len(matches)==1,[(hex(p),len(x)) for p,x in matches]
 target,ni=matches[0];oldcall=next(i for i in oi if i.mnemonic=='call');newcall=ni[oi.index(oldcall)]
 assert oldcall.operands[0].imm==0x2dd590
 pairs=[(0x37d8c0,target,oi,ni),(0x2dd590,newcall.operands[0].imm,body(oc,0x2dd590),body(nc,newcall.operands[0].imm))]
 # Actual BSP/object bodies store the retained query and allocator vtables.
 # Locate by a normalized prefix, then require its entire paired body.
 for old_entry in (0x1f1f30,0x1f2710,0x1c66a0,0x22e4e0,0x31f640,0x31d730,0x322590,0x320cc0):
  oi=body(oc,old_entry);prefix=[base.norm(i,old) for i in oi[:5]];matches=[]
  for p in starts:
   try:
    probe=[];cursor=p
    for _ in prefix:
     i=nc.at(cursor);probe.append(base.norm(i,new));cursor+=i.size
    if probe!=prefix:continue
    ni=body(nc,p)
    if base.full_tokens(oi,old)==base.full_tokens(ni,new):matches.append((p,ni))
   except Exception:continue
  assert len(matches)==1,[(hex(p),len(x)) for p,x in matches]
  p,ni=matches[0];pairs.append((old_entry,p,oi,ni))
 owner_path=ROOT/'analysis/multiplayer/sandbox-source/power-pair-manager/final/later-correspondence/complete-report.json'
 assert sha(owner_path)=='5ca584c05b98ce02f201fec83098d57268ffc3f8bd136457f99a4b72b63294b4'
 owners=json.loads(owner_path.read_text())
 for old_entry in (0x321320,0x320e50):
  row=next(x for x in owners['routines'] if int(x['disc_1_1_entry'],16)==old_entry)
  target=int(row['later_entry'],16);oi=body(oc,old_entry);ni=body(nc,target)
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new)
  pairs.append((old_entry,target,oi,ni))
 mapping={};rows=[]
 a.output.mkdir(parents=True,exist_ok=False)
 for p,q,oi,ni in pairs:
  assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(p),hex(q))
  for x,y in zip(oi,ni):
   for xo,yo in zip(x.operands,y.operands):
    if xo.type==yo.type==2 and base.containing(old,xo.imm) and base.containing(new,yo.imm):
     key=f'{xo.imm:08x}';assert key not in mapping or mapping[key]==yo.imm;mapping[key]=yo.imm
  row=dict(old_entry=p,later_entry=q,instructions=len(oi),complete_body_equal=True)
  for label,ins in (('disc',oi),('later',ni)):
   f=a.output/f'{p:08x}-{label}.asm';f.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins));row[label+'_listing']=dict(file=f.name,sha256=sha(f))
  rows.append(row)
 # Actual initializer receiver and arena operand; no nearest-address heuristic.
 assert mapping['00480db0'] and mapping['00480dd0']
 # The static object's vtable is supplied by each original image's data.
 # Its initializer receiver and immediately following arena authenticate this
 # object location; the direct constructor stores the arena/size at +8/+12.
 obj=mapping['00480db0'];arena=mapping['00480dd0'];assert arena-obj==0x20
 old_vtable=int.from_bytes(old.read(0x480db0,4),'little')
 new_vtable=int.from_bytes(new.read(obj,4),'little')
 assert old_vtable==0x45a8fc
 assert old.read(0x480db0+4,0x1c)==new.read(obj+4,0x1c)
 mapping[f'{old_vtable:08x}']=new_vtable
 parent_path=ROOT/'analysis/multiplayer/sandbox-source/physics-world-contact-runtime-20261011/correspondence-v2/complete-report.json'
 assert sha(parent_path)=='257cf7d69e56957067898e7bf2df455f9bc54e93b8851dafc2e8fac74edbb17b'
 parent=json.loads(parent_path.read_text());slots=[]
 for table,offsets in ((0x45a8fc,(0x10,0x14)),(0x45d2e0,(0x30,)),
                      (0x45d278,(0x34,)),(0x45e7a8,(0x10,0x14)),(0x41b610,(8,)),(0x41ba08,(0x28,)),(0x41b8f0,(0x58,)),(0x41b870,(0x58,)),(0x41b7f0,(0x58,))):
  for off in offsets:
   p,q=table+off,mapping[f'{table:08x}']+off
   x,y=int.from_bytes(old.read(p,4),'little'),int.from_bytes(new.read(q,4),'little')
   assert parent['entries'][f'{x:08x}']==y,(hex(x),hex(y))
   slots.append(dict(old_slot=p,later_slot=q,old_entry=x,later_entry=y))
 report=dict(schema=1,tool_sha256=sha(__file__),disc_sha256=PIN,later_sha256=base.NEW_PIN,routines=rows,address_map=mapping,
  input_regions=[dict(old=0x480db0,later=obj,size=0x20),dict(old=0x480dd0,later=arena,size=0x4000)],
  contact_correspondence_sha256=sha(parent_path),verified_virtual_slots=slots,
  scope='Actual complete allocator initializer37D8C0, constructor2DD590 and BSP/object vtable-store bodies1F1F30,1F2710,1C66A0,22E4E0,31F640,31D730,322590,320CC0,321320,320E50; mutable input object/arena and vtable anchors only. No supplied output or gameplay result.')
 p=a.output/'report.json';p.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(sha256=sha(p),mapping={k:hex(v) for k,v in mapping.items()},routines=rows)))
if __name__=='__main__':main()
