#!/usr/bin/env python3
"""Independent u5 bytes/table, packet/store replay and old CPU correlation."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_unit_locomotion_u5 import ROOT,OUT,OLD_OUT,INPUT_PINS,PINS,NEW,OLD,digest,encoded,frozen_write,Xbe
def main():
 p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path,default=OUT);out=p.parse_args().output_dir
 d=json.loads((out/'report.json').read_text());old_report=json.loads((OLD_OUT/'report.json').read_text())
 assert digest(Path(__file__).with_name('oracle_mp_unit_locomotion_u5.py'))==d['oracle_sha256']
 for name,h in INPUT_PINS.items():assert digest(ROOT/name)==h,name
 for name,h in PINS.items():assert digest(name)==h,name
 assert not d['services']and not d['instruction_patches']and not d['production_edits']
 old,new=Xbe(OLD),Xbe(NEW);mapping={int(r['old'],16):int(r['u5'],16)for r in d['instruction_mapping']}
 sites={};instruction_rows=0
 for q in d['listings']:
  path=out/q['file'];assert digest(path)==q['sha256'];lines=path.read_text().splitlines();assert len(lines)==q['rows'];instruction_rows+=len(lines)
  for line in lines:
   a,b,*_=line.split();a=int(a,16);raw=bytes.fromhex(b);assert new.read(a,len(raw))==raw;sites[a]=raw
 assert old.read(0x1f52f0,0x82)==new.read(mapping[0x1f52f0],0x82)
 tables=d['dispatch_tables'];op,np=int(tables['old_pointer'],16),int(tables['u5_pointer'],16)
 ob,nb=bytes.fromhex(tables['old_raw18_hex']),bytes.fromhex(tables['u5_raw18_hex']);assert old.read(op,24)==ob and new.read(np,24)==nb
 assert [mapping[a]for a in struct.unpack('<6I',ob)]==list(struct.unpack('<6I',nb))
 reads=stores=cpu_points=0
 for c in d['cases']:
  prior=old_report['cases'][c['global_id']]
  assert c['route']==prior['route']and c['vector']==prior['vector']
  buffers=[(int(c['request_pointer'],16),bytearray.fromhex(c['request_before_hex'])),
   (int(c['result_pointer'],16),bytearray.fromhex(c['result_before_hex'])),
   (int(c['physical_pointer'],16),bytearray.fromhex(c['physical_before_hex'])),(np,bytearray(nb)),
   (int(c['points'][0]['cpu']['esp'],16),bytearray(struct.pack('<I',0x3000000)))]
  for _,kind,e in sorted([(r['order'],'read',r)for r in c['reads']]+[(r['order'],'store',r)for r in c['stores']]):
   assert int(e['instruction'],16)in sites;a,n=int(e['address'],16),e['size'];matched=False
   for p,value in buffers:
    if p<=a and a+n<=p+len(value):
     before=bytes.fromhex(e['raw_hex'if kind=='read'else'before_hex']);assert value[a-p:a-p+n]==before
     if kind=='store':
      assert p==int(c['result_pointer'],16);value[a-p:a-p+n]=bytes.fromhex(e['written_hex'])
     matched=True;break
   assert matched,(c['global_id'],e)
   if kind=='store':stores+=1
   else:reads+=1
  for index,name in((0,'request_after_hex'),(1,'result_after_hex'),(2,'physical_after_hex')):
   assert buffers[index][1].hex()==c[name]==prior[name]
  assert c['return_cpu']==prior['return_cpu']and len(c['points'])==len(prior['points'])
  for before,after in zip(prior['points'],c['points']):
   assert before['cpu']==after['cpu']and mapping[int(before['instruction'],16)]==int(after['instruction'],16)
   assert sites[int(after['instruction'],16)].hex()==after['bytes'];cpu_points+=1
  assert len(c['stores'])==len(prior['stores'])
  for before,after in zip(prior['stores'],c['stores']):assert after==dict(before,instruction=f'{mapping[int(before["instruction"],16)]:08x}')
 assert len(d['cases'])==1176
 result=dict(schema=1,report_sha256=digest(out/'report.json'),auditor_sha256=digest(__file__),cases=1176,original_instruction_rows=instruction_rows,
  original_bytes_verified=True,full_leaf130_bytes_identical=True,actual_u5_dispatch_table_verified=True,
  complete_packet_intervals=1176*3,reads_replayed=reads,stores_replayed=stores,cpu_points_correlated=cpu_points,
  full_return_cpu_correlated=1176,old_fixture_inputs_explicit=True,old_trace_addresses_remapped_explicitly=True,
  original_u5_device_or_physical_producer=False,native_gameplay_changes=False)
 frozen_write(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
