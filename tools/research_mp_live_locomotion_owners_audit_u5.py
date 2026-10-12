#!/usr/bin/env python3
"""Independent literal-byte audit of the command/request/pose correspondence."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,sha,encoded,save
from research_mp_live_locomotion_owners_u5 import OUT

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=OUT/'complete');out=p.parse_args().directory
 d=json.loads((out/'report.json').read_text());assert sha(ROOT/'tools/research_mp_live_locomotion_owners_u5.py')==d['tool_sha256']
 for path,pin in d['source_pins'].items():assert sha(path)==pin
 for path,pin in d['source_inventory_pins'].items():assert sha(path)==pin
 images=[Xbe(OLD),Xbe(NEW)];mappings={r['old_entry']:r['u5_entry']for r in d['owners']};rows=operands=calls=tables=targets=0
 assert len(mappings)==27
 for r in d['owners']:
  assert r['full_topology_matches']==[r['u5_entry']]and r['complete_instruction_and_internal_branch_topology_equal']
  pair=[]
  for image,q in zip(images,r['listings']):
   path=out/q['file'];assert sha(path)==q['sha256'];lines=path.read_text().splitlines();assert len(lines)==q['rows'];addresses=[]
   for line in lines:
    a,b,*_=line.split();a=int(a,16);v=bytes.fromhex(b);assert image.read(a,len(v))==v;addresses.append(a);rows+=1
   pair.append(addresses)
  assert len(pair[0])==len(pair[1]);bijection=dict(zip(*pair))
  for q in r['absolute_image_memory_operands']:
   for image,side in zip(images,('old','u5')):assert image.read(int(q[side+'_va'],16),q['size']).hex()==q[side+'_hex']
   assert q['initial_image_bytes_equal']==(q['old_hex']==q['u5_hex']);operands+=1
  for q in r['direct_calls']:
   assert bijection[int(q['old_site'],16)]==int(q['u5_site'],16)
   old=f'{int(q["old_target"],16):08x}';new=f'{int(q["u5_target"],16):08x}'
   assert q['target_full_mapping_in_this_report']==(old in mappings)
   if old in mappings:assert mappings[old]==new
   calls+=1
  for q in r['jump_tables']:
   for image,side in zip(images,('old','u5')):
    raw=image.read(int(q[side+'_va'],16),q['count']*4)
    assert [f'{int.from_bytes(raw[i:i+4],"little"):08x}'for i in range(0,len(raw),4)]==q[side+'_targets']
   assert all(bijection[int(a,16)]==int(b,16)for a,b in zip(q['old_targets'],q['u5_targets']))
   tables+=1;targets+=q['count']
 deadzone=next(q for r in d['owners']if r['old_entry']=='000de960'for q in r['absolute_image_memory_operands']if q['old_va']=='004654c0')
 assert deadzone['old_hex']==deadzone['u5_hex']=='0bd7233c'and deadzone['u5_va']=='0046a560'
 result=dict(schema=1,report_sha256=sha(out/'report.json'),auditor_sha256=sha(__file__),unique_complete_owner_mappings=len(mappings),
  literal_original_instruction_rows=rows,absolute_image_memory_operands=operands,direct_call_sites=calls,jump_tables=tables,jump_table_targets=targets,
  source_deadzone_operand=deadzone,annotations_used=False,original1_5_dynamic_execution=False,natural_device_admission=False)
 save(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
