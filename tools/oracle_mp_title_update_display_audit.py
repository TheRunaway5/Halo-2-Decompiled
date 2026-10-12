#!/usr/bin/env python3
"""Original-byte and full observed memory replay for build_number production."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_title_update_display import OUT,PINS,sha,save,encoded,Xbe,BUFFER,STACK,STOP
def main():
 p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path,default=OUT);out=p.parse_args().output_dir
 d=json.loads((out/'report.json').read_text());assert sha(Path(__file__).with_name('oracle_mp_title_update_display.py'))==d['oracle_sha256']
 assert d['source_pins']==PINS and not d['instruction_patches']and not d['production_edits']
 reads=stores=instructions=0;texts=[]
 for image in d['images']:
  x=Xbe(image['source_path']);assert sha(image['source_path'])==image['source_sha256']==PINS[image['source_path']]
  sites={}
  for q in image['listings']:
   path=out/q['file'];assert sha(path)==q['sha256'];lines=path.read_text().splitlines();assert len(lines)==q['rows']
   for line in lines:
    a,b,*_=line.split();a=int(a,16);raw=bytes.fromhex(b);assert x.read(a,len(raw))==raw;sites[a]=raw;instructions+=1
  r=image['record'];pointer,marker,callback=struct.unpack('<III',x.read(int(r['address'],16),12))
  assert [f'{pointer:08x}',f'{marker:04x}',f'{callback:08x}']==[r['key_pointer'],r['marker'],r['callback']]
  assert r['key_text']=='build_number'and marker==0xe415 and image['format']['text']=='%hs'
  assert x.read(int(image['literal']['pointer'],16),4).hex()==image['literal']['raw4_hex']
  for c in image['cases']:
   memory={};initial=bytes.fromhex(c['initial512_hex']);abi=struct.pack('<III',STOP,*(int(v,16)for v in c['stack_arguments']))
   def original(a):
    if BUFFER<=a<BUFFER+512:return initial[a-BUFFER]
    if STACK<=a<STACK+len(abi):return abi[a-STACK]
    for section in x.sections:
     if section.va<=a<section.va+section.vsize:
      offset=a-section.va;return x.data[section.raw+offset]if offset<section.rsize else 0
    return 0
   def read(a,n):return bytes(memory.get(p,original(p))for p in range(a,a+n))
   events=sorted([(e['order'],'read',e)for e in c['reads']]+[(e['order'],'store',e)for e in c['stores']])
   for _,kind,e in events:
    assert int(e['instruction'],16)in sites
    a,n=int(e['address'],16),e['size'];raw=bytes.fromhex(e['raw_hex'if kind=='read'else'before_hex'])
    assert len(raw)==n and read(a,n)==raw,(image['id'],c['route'],e)
    if kind=='store':
     for k,v in enumerate(bytes.fromhex(e['written_hex'])):memory[a+k]=v
     stores+=1
    else:reads+=1
   assert read(BUFFER,512).hex()==c['output512_hex']and not c['services']
   assert c['actual_text']==image['literal']['text']
   assert all(int(a,16)in sites for a in c['instructions'])
  texts.append(image['literal']['text'])
 result=dict(schema=1,report_sha256=sha(out/'report.json'),auditor_sha256=sha(__file__),images=len(d['images']),cases=4,
  original_instruction_rows_verified=instructions,all_memory_reads_replayed=reads,all_ordered_stores_replayed=stores,
  complete_output512_buffers_verified=True,source_texts=texts,original_marker_table_key_verified=True,supplied_service_results=False)
 save(out/'audit.json',encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
