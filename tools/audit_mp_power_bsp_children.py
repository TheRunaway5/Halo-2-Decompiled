#!/usr/bin/env python3
"""Audit original BSP child producers, independent repeats and native fixture authority."""
import argparse,json,collections
from pathlib import Path
from oracle_mp_power_bsp_children_1_1 import ROOT,OUT,verify,sha,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');a=ap.parse_args();count=verify()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json';assert p.read_bytes()==repeat.read_bytes();r=json.loads(p.read_text());sites={};rows=0
 for q in r['instruction_listings']:
  left,right=p.parent/q['file'],repeat.parent/q['file'];assert left.read_bytes()==right.read_bytes()and sha(left)==q['sha256']
  for line in left.read_text().splitlines():
   address,raw,*_=line.split();address=int(address,16);raw=bytes.fromhex(raw);assert old.read(address,len(raw))==raw;sites[address]=raw;rows+=1
 observed={int(q['instruction'],16)for k in('reads','stores')for q in r[k]}
 assert observed<=sites.keys(),[hex(v)for v in observed-sites.keys()]
 cp=OUT/'final/later-correspondence/complete-report.json';c=json.loads(cp.read_text());oc,nc=_Code(old,'.text'),_Code(new,'.text')
 maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']}
 for q in c['routines']:
  left=int(q['disc_1_1_entry'],16);right=maps[left]
  li=[oc.at(x)for x in sorted(_trace(oc,left,set(maps)).code)];ri=[nc.at(x)for x in sorted(_trace(nc,right,set(maps.values())).code)]
  assert full_tokens(li,old)==full_tokens(ri,new)
  for k,ins in(('disc_listing',li),('later_listing',ri)):
   dst=cp.parent/'instructions'/q[k]['file'];assert sha(dst)==q[k]['sha256'];assert dst.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
  for x in q['paired_image_memory_operands']:
   assert old.read(int(x['old_address'],16),x['width']).hex()==x['old_hex'];assert new.read(int(x['later_address'],16),x['width']).hex()==x['later_hex']
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());queries=[];triangles=[]
 for call in r['calls']:
  if call['entry']=='001f2420':
   packet=next(q for q in call['packets']if q['name']=='output_descriptor_c')
   bounds=next(q for q in call['packets']if q['name']=='bounds_20')
   emitted=[q['written_hex']for q in r['stores']if call['order']<q['order']<call['return_order']and q['instruction']=='0030641b']
   keys=[f'{int.from_bytes(bytes.fromhex(v),"little"):08x}'for v in emitted]
   queries.append(dict(call_id=call['id'],phase=call['phase'],shape=call['registers']['ecx'],bounds=bounds,descriptor=packet,ordered_keys=keys))
  if call['entry']=='001f16f0':
   packet=next(q for q in call['packets']if q['name']=='output_triangle_100');raw=bytes.fromhex(packet['after_hex']);n=int.from_bytes(raw[20:24],'little')
   triangles.append(dict(call_id=call['id'],phase=call['phase'],arguments=call['stack16'][:5],optional_transform=call['registers']['edi'],packet=packet,vertex_count=n,vertices_hex=raw[32:32+n*16].hex(),material=int.from_bytes(raw[24:26],'little'),surface_flags=raw[26]))
 world=[q for q in queries if q['phase']=='original_world_physics'];assert len(world)==2 and world[0]['ordered_keys']==['2000081d','20000813']and world[1]['ordered_keys']==[]
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),complete_listings=len(r['instruction_listings']),original_listing_rows_audited=rows,original_read_store_instruction_sites_audited=len(observed),later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,native_fixture_calls=len(e['cases']),native_complete_calls=sum(q['complete_original_call']for q in e['cases']),native_stores=sum(q['stores']for q in e['cases']),native_compared_buffer_bytes=sum(q['input_bytes']for q in e['cases']),source_opcode_visits=dict(sorted(collections.Counter(q['raw_hex']for q in r['reads']if q['instruction']in('00305e4b','00306354')).items())),actual_queries=queries,actual_triangles=triangles,remaining_live_gate='Actual300870/2EFC30 child-agent orchestration, then original per-child narrowphase/contact-manager/solver production and real retained world scheduling. Geometry is produced here from original resident BSP banks; no source-created contact is replayed in native runtime.')
 frozen_write(a.output,encoded(result));print(json.dumps({k:v for k,v in result.items()if k not in('actual_queries','actual_triangles')}))
if __name__=='__main__':main()
