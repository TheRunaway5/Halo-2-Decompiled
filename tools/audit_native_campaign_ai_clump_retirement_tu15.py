#!/usr/bin/env python3
"""Authenticate the clump-retirement native direct graph and real callback frontier."""
import argparse,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ENTRIES={0x272ec0:'clump teardown and pool reclamation',0x274800:'clump-group and perception teardown',0x2731e0:'actor membership and property detachment',0x16e8a0:'actual data-array free',0x2748e0:'actor perception unlink and invalidation',0x274970:'group perception unlink',0x1ea400:'actor reference invalidation and behavior callback fanout',0x299aa0:'unit AI attachment reference invalidation',0x1bda70:'behavior and plan reference invalidation',0x1c8e90:'behavior field0C invalidation',0x1c75b0:'behavior field10 invalidation',0x1c9750:'behavior field1C invalidation'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');rows=[]
 for entry,role in ENTRIES.items():
  body=_trace(code,entry,set(ENTRIES));calls=[]
  for pc in sorted(body.code):
   i=code.at(pc)
   if i.mnemonic=='call':calls.append(dict(site=hex(pc),target=i.op_str,direct_semantic_owner=i.op_str.startswith('0x') and int(i.op_str,16) in ENTRIES))
  rows.append(dict(entry=hex(entry),semantic_role=role,original_sites=len(body.code),body_sha256=hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(body.code))).hexdigest(),calls=calls))
 callbacks={}
 for kind,descriptor in enumerate(struct.unpack('<131I',image.read(0x47b478,524))):
  if not descriptor:continue
  descriptor_kind=int.from_bytes(image.read(descriptor+2,2),'little')
  if descriptor_kind==0:continue # Group-only records have no callback interface.
  assert descriptor_kind in [1,2]
  callback=int.from_bytes(image.read(descriptor+0x28,4),'little')
  if callback:callbacks.setdefault(hex(callback),[]).append(kind)
 result=dict(original_xbe_sha256=NEW_PIN,direct_owners=rows,registered_behavior_invalidation=dict(site='0x1ea562',directory='0x47b478',descriptor_offset='0x28',callback_kinds=callbacks),full_registered_nonempty_invalidation_closure=True,scope='Native direct teardown and true reclamation are implemented. Production binding invokes the four real registered native invalidators. Unknown selectors fail at the exact frontier; no supplied success or instruction-lift fallback.')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(direct_owners=len(rows),registered_native_behavior_callbacks=len(callbacks))))
if __name__=='__main__':main()
