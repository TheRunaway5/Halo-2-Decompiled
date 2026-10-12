#!/usr/bin/env python3
"""Independent read-only consumed options and exact transfer store audit."""
import argparse,json
from pathlib import Path
from oracle_mp_pickup_transfer_options_1_1 import ROOT,BASE,OUT,digest,encoded,frozen_write
from xbe import Xbe

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);args=p.parse_args();out=args.output_dir
 d=json.loads((out/'report.json').read_text());old=json.loads((BASE/'pickup-transfer-measured/report.json').read_text())
 assert digest(ROOT/'tools/oracle_mp_pickup_transfer_options_1_1.py')==d['tool_sha256']
 for name,pin in d['source_pins'].items():assert digest(ROOT/name)==pin,name
 for name in ('preflight','measured_baseline'):
  r=d[name];assert r['byte_identical'] and r['frozen_listing_writes_compared_and_suppressed']
  assert (out/r['file']).read_bytes()==(BASE/'weapon-attachment/report.json').read_bytes()
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 for r in d['original_instruction_bytes']:
  a=int(r['address'],16);raw=bytes.fromhex(r['raw_original_hex']);assert x.read(a,len(raw))==raw
 options=network=stores=correlated=0
 for c,prior in zip(d['cases'],old['cases']):
  assert c['name']==prior['name'];byid={r['id']:r for r in c['calls']}
  for call in c['calls']:
   assert call['entry']!='000b6760'
   if 'frozen_transfer_call_id'in call:
    ref=next(r for r in prior['calls']if r['id']==call['frozen_transfer_call_id'])
    assert call['phase']==ref['phase'] and call['registers']==ref['registers'] and call['stack']==ref['stack'][:4]
    assert call['return_eax']==ref['return_eax']
    assert call['unit_before_hex']==prior['states'][ref['before_state']]['unit']['body_prefix_hex']
    assert call['unit_after_hex']==prior['states'][ref['after_state']]['unit']['body_prefix_hex'];correlated+=1
  for r in c['sites']:
   if r['instruction']=='000cfe66':
    assert r['consumed_options_pointer']==r['options_global_pointer']==r['registers']['ecx']
    assert bytes.fromhex(r['options_prefix_hex'])[12]==int(r['consumed_options_byte_0c'],16)==1
    call=byid[r['cfe60_call_id']];assert call['entry']=='000cfe60' and call['order']<r['order']<call['return_order'];options+=1
   if r['instruction']in ('000cfeb6','000cff59'):
    assert r['stored_network_entity']==r['compared_network_entity']=='ffffffff';network+=1
  for r in c['stores']:
   assert len(bytes.fromhex(r['before_hex']))==len(bytes.fromhex(r['written_hex']))==r['size']
   matching=[q for q in prior['timeline']if q['kind']=='store' and q['phase']==r['phase'] and q['instruction']==r['instruction']]
   assert any(q['address']==r['address'] and q['before_hex']==r['before_hex'] and q['written_hex']==r['written_hex']for q in matching);stores+=1
  for call in c['calls']:
   if call['entry']=='000cfed0':
    ss=[r for r in c['stores']if call['order']<r['order']<call['return_order']]
    sites=[r['instruction']for r in ss]
    assert sites==(['000cff30','000cfe9b','000cfea2']if call['registers']['ecx']=='ffffffff' else ['000cff0f','000cff16','000cfe9b','000cfea2'])
 assert correlated==d['correlated_frozen_transfer_calls']==20 and options==10 and network==20 and stores==39
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),prior_pins=len(d['source_pins']),
  instruction_sites=len(d['original_instruction_bytes']),cases=len(d['cases']),correlated_calls=correlated,consumed_options_compares=options,
  actual_consumed_byte='01',NONE_network_gates=network,executed_network_dirty_callbacks=0,exact_correlated_stores=stores,
  insert_and_drop_store_order_verified=True,instruction_patches=[])
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result,sort_keys=True))
if __name__=='__main__':main()
