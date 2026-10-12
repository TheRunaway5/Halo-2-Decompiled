#!/usr/bin/env python3
"""Bind world runtime inputs from actual paired constructors and vtable slots."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
import research_mp_sandbox_update_functions as base

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--correspondence',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 parent_path=ROOT/'analysis/multiplayer/sandbox-source/physics-contact-listener-runtime-20261011/input-bindings-v1/report.json'
 assert sha(parent_path)=='e9d5eae38852a2fd394d4cb9977d19fbdbd4c66314288ff6774e0dc2024c8c38'
 parent=json.loads(parent_path.read_text());assert parent['tool_sha256']==sha(ROOT/'tools/research_physics_contact_listener_bindings_20261011.py')
 paired=json.loads(a.correspondence.read_text());assert paired['disc_1_1_sha256']==PIN and paired['later_owned_xbe_sha256']==base.NEW_PIN
 old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(old_path)==PIN and sha(base.NEW_DEFAULT)==base.NEW_PIN
 old,new=Xbe(old_path),Xbe(base.NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 mapping=dict(parent['address_map']);rows=[]
 for row in parent['routines']:
  row=dict(row)
  for label in ('disc','later'):
   listing=row[label+'_listing'];assert sha(parent_path.parent/listing['file'])==listing['sha256']
  rows.append(row)
 for key,value in paired['address_map'].items():
  assert key not in mapping or mapping[key]==value;mapping[key]=value
 # The actual filter constructor supplies these table anchors. The motion
 # constructor anchors were authenticated in the retained parent binding.
 filter_row=next(row for row in paired['routines'] if row['disc_1_1_entry']=='0027dae0')
 for label in ('disc','later'):
  listing=filter_row[label+'_listing'];assert sha(a.correspondence.parent/'instructions'/listing['file'])==listing['sha256']
 slots=list(parent['verified_virtual_slots'])
 for table,offsets in ((0x41b7f0,(4,8,0x18)),(0x41b8f0,(4,8,0x18)),(0x41b870,(4,8,0x18)),
                       (0x461bc8,(0x0c,)),(0x461bbc,(0x0c,))):
  assert f'{table:08x}' in mapping,hex(table)
  for offset in offsets:
   old_slot=table+offset;later_slot=mapping[f'{table:08x}']+offset
   x,y=int.from_bytes(old.read(old_slot,4),'little'),int.from_bytes(new.read(later_slot,4),'little')
   oi=[oc.at(p) for p in sorted(_trace(oc,x,{x}).code)];ni=[nc.at(p) for p in sorted(_trace(nc,y,{y}).code)]
   assert base.full_tokens(oi,old)==base.full_tokens(ni,new),(hex(table),hex(offset),hex(x),hex(y))
   assert f'{x:08x}' not in mapping or mapping[f'{x:08x}']==y;mapping[f'{x:08x}']=y
   slots.append(dict(old_slot=old_slot,later_slot=later_slot,old_entry=x,later_entry=y,instructions=len(oi),complete_body_equal=True))
 result=dict(schema=1,tool_sha256=sha(__file__),parent_input_bindings_sha256=sha(parent_path),correspondence_sha256=sha(a.correspondence),
  disc_sha256=PIN,later_sha256=base.NEW_PIN,routines=rows,address_map=mapping,input_regions=parent['input_regions'],verified_virtual_slots=slots,
  scope='Actual paired allocator, object, listener, motion and filter constructors; complete actual selected virtual bodies. Input relocation only; no supplied gameplay or output state.')
 a.output.mkdir(parents=True,exist_ok=False);p=a.output/'report.json';p.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(sha256=sha(p),slots=len(slots))))
if __name__=='__main__':main()
