#!/usr/bin/env python3
"""Audit untouched instructions and every measured classifier store/read."""
import argparse, collections, hashlib, json
from pathlib import Path
from oracle_mp_unit_locomotion_1_1 import ROOT,OUT,FROZEN,digest,encoded,frozen_write,vectors
from xbe import Xbe

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);out=p.parse_args().output_dir
 d=json.loads((out/'report.json').read_text());assert d['source_pins']==FROZEN
 assert digest(ROOT/'tools/oracle_mp_unit_locomotion_1_1.py')==d['oracle_sha256']
 for name,h in FROZEN.items():assert digest(ROOT/name)==h,name
 assert not d['instruction_patches']
 correlation=d['prior_owner_correlation'];prior=json.loads((ROOT/correlation['report']).read_text())
 assert correlation['byte_identical'] and digest(ROOT/correlation['report'])==correlation['report_sha256']
 assert hashlib.sha256(encoded({k:prior[k]for k in correlation['keys']})).hexdigest()==correlation['runtime_sha256']
 owner=d['source_owner'];unit=bytes.fromhex(owner['body_hex']);body=int(owner['body_address'],16)
 assert len(unit)==owner['body_size'] and unit[0x3dc]==1 and unit[0x3dc:0x3ec].hex()==owner['physical_raw10_hex']
 packet=d['source_packet_call'];leaf=prior['calls'][packet['call_id']]
 assert leaf['entry']=='001f52f0' and [packet['request'],packet['result']]==leaf['packets']
 x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');sites={};rows=0
 for q in d['instructions']:
  path=out/q['file'];assert digest(path)==q['sha256'];lines=path.read_text().splitlines();assert len(lines)==q['rows'];rows+=len(lines)
  for line in lines:
   a,b,*_=line.split();a=int(a,16);v=bytes.fromhex(b);assert x.read(a,len(v))==v;sites[a]=v
 table=d['jump_table'];assert x.read(int(table['pointer'],16),24).hex()==table['raw18_hex']
 counts=collections.Counter();classes=collections.Counter();store_sites=collections.Counter();reads=0;stores=0;points=0;paired={}
 source_request=bytearray.fromhex(packet['request']['raw_hex']);source_result=bytearray.fromhex(packet['result']['raw_hex'])
 unit_sha=hashlib.sha256(unit).hexdigest()
 for c in d['cases']:
  assert c['id']==sum(counts.values());counts[c['route']]+=1
  request=bytearray.fromhex(c['request_before_hex']);result=bytearray.fromhex(c['result_before_hex'])
  expected_request=source_request.copy()
  for n,b in enumerate(c['vector']['bits']):expected_request[0xdc+4*n:0xe0+4*n]=int(b,16).to_bytes(4,'little')
  if c['route']=='wrapper_flag20':expected_request[0x14]|=0x20
  else:expected_request[0x14]&=0xdf
  assert request==expected_request and request.hex()==c['request_after_hex']
  mask_result=result.copy();mask_result[4:9]=source_result[4:9];assert mask_result==source_result
  assert c['unit_before_sha256']==c['unit_after_sha256']==unit_sha
  assert c['physical_before_hex']==c['physical_after_hex']==owner['physical_raw10_hex'] and not c['services']
  rp,op=int(c['request_pointer'],16),int(c['result_pointer'],16)
  buffers=[(rp,request),(op,result),(body,unit),(int(table['pointer'],16),bytes.fromhex(table['raw18_hex']))]
  ordered=sorted([(e['order'],'read',e)for e in c['reads']]+[(e['order'],'store',e)for e in c['stores']])
  for _,kind,e in ordered:
   a=int(e['instruction'],16);assert a in sites
   ptr=int(e['address'],16);size=e['size'];raw=bytes.fromhex(e['raw_hex'if kind=='read'else'before_hex']);assert len(raw)==size
   matched=False
   for base,value in buffers:
    if base<=ptr and ptr+size<=base+len(value):
     assert value[ptr-base:ptr-base+size]==raw,(c['id'],e);matched=True
     if kind=='store':
      assert base==op and isinstance(value,bytearray)
      value[ptr-base:ptr-base+size]=bytes.fromhex(e['written_hex'])
     break
   if kind=='store':
    assert matched
    assert (a,ptr-op,size)in((0x1f5349,4,4),(0x1f5351,4,4),(0x1f5362,4,4),(0x1f536a,4,4),(0x1e94eb,4,4),(0x1e94ee,8,1))
    store_sites[e['instruction']]+=1;stores+=1
   else:
    # The only read outside retained packets/unit/table is original RET stack.
    assert matched or sites[a]==b'\xc3';reads+=1
  assert result.hex()==c['result_after_hex']
  for point in c['points']:
   assert sites[int(point['instruction'],16)].hex()==point['bytes']
   assert len(point['cpu']['fp80'])==8 and point['cpu']['fpcw']=='027f';points+=1
  classification=int.from_bytes(result[4:8],'little');classes[str(classification)]+=1
  key=(tuple(c['vector']['bits']),c['result_before_hex'])
  if c['route']=='leaf':paired[key]=c['result_after_hex']
  elif c['route']=='wrapper':assert paired[key]==c['result_after_hex']
  else:
   assert classification==1 and result[8]==1 and [e['instruction']for e in c['stores']]==['001e94eb','001e94ee']
 assert len(d['cases'])==len(vectors())*6 and len(store_sites)==6
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),prior_pins=len(FROZEN),
  byte_identical_owner_runtime_verified=True,vectors=len(vectors()),cases=len(d['cases']),routes=dict(counts),classes=dict(classes),
  ordered_store_sites=dict(store_sites),ordered_stores=stores,reads=reads,cpu_instruction_points=points,
  source_unit_unchanged=True,complete_request_result_buffers_replayed=True,wrapper_leaf_pairs_equal=True,
  original_instruction_rows=rows,original_instruction_bytes_verified=True,nonneutral_inputs_explicit=True,
  live_device_admission=False,other_physical_states_executed=False,hardware_fp_exception_parity=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
