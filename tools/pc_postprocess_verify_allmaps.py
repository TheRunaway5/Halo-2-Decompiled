#!/usr/bin/env python3
"""Check all-map original PC execution outputs, repeats and raw input offsets."""
import argparse,hashlib,json
from pathlib import Path
from pc_postprocess_inspect import ROOT,OUT,PE,VISTA,sha
from pc_postprocess_vista_allmaps import scenario_header,ENTRY,CALLER

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 measured=OUT/'vista-allmaps-measured';repeat=OUT/'vista-allmaps-repeat';top=json.loads((measured/'report.json').read_text());assert top==json.loads((repeat/'report.json').read_text())
 files=sorted(p.relative_to(measured)for p in measured.rglob('*')if p.is_file());assert files==sorted(p.relative_to(repeat)for p in repeat.rglob('*')if p.is_file())
 pins={}
 for rel in files:
  assert(measured/rel).read_bytes()==(repeat/rel).read_bytes();pins[str(measured/rel)]=sha(measured/rel);pins[str(repeat/rel)]=sha(repeat/rel)
 for field,path in [('tool_sha256',ROOT/'tools/pc_postprocess_vista_allmaps.py'),('original_lockout_tool_sha256',ROOT/'tools/pc_postprocess_vista_oracle.py'),('helper_sha256',ROOT/'tools/pc_postprocess_inspect.py'),('reader_sha256',ROOT/'tools/inspect_pc_combat_tag_sample.py'),('executable_sha256',VISTA),('source_field_contract_sha256',ROOT/'analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json')]:
  assert top[field]==sha(path);pins[str(path)]=sha(path)
 inventory=[scenario_header(Path(r['path']))for r in top['inventory']];assert inventory==top['inventory']
 selected=[r for r in inventory if r['scenario_type']==1];assert len(selected)==23 and {Path(r['path']).stem for r in selected}=={r['map']for r in top['maps']}
 pe=PE(VISTA);source,_=pe.trace(ENTRY);old=json.loads((OUT/'vista-measured/report.json').read_text());oldstores=[r for r in old['ordered_stores']if r['region']=='actual_resident_metadata']
 sourcehashes={};raw_reads=0;metrics=[]
 for summary in top['maps']:
  path=measured/summary['report_relative_path'];r=json.loads(path.read_text());assert sha(path)==summary['report_sha256']
  for address in(ENTRY,CALLER):assert(path.parent/f'{address:08x}.asm').read_text()==pe.listing(address)
  assert r['scenario_tag']['datum']==r['scenario_index_header_datum']==summary['scenario_header']['scenario_datum']
  assert len(r['executed_instruction_addresses'])==382 and all(int(p,16)in source and source[int(p,16)].mnemonic!='call'for p in r['executed_instruction_addresses'])
  assert not r['original_services_or_callback_returns_supplied']and r['stack_cleanup_bytes']==4
  for c in r['cache_inputs']:
   if c['path']not in sourcehashes:sourcehashes[c['path']]=sha(Path(c['path']))
   assert sourcehashes[c['path']]==c['sha256']
  stores=[s for s in r['ordered_stores']if s['region']=='actual_resident_metadata'];assert len(stores)==41
  for oldrow,s in zip(oldstores,stores):
   assert(oldrow['tag'],oldrow['instruction'],oldrow['after_hex'])==(s['tag'],s['instruction'],s['after_hex'])
   with Path(s['owner']).open('rb')as f:f.seek(s['file_offset']);raw=f.read(s['size'])
   assert raw.hex()==s['before_hex'];raw_reads+=1
  assert sum(s['before_hex']!=s['after_hex']for s in stores)==38 and len(r['changed_tags'])==13
  for t in r['changed_tags']:
   tag=t['tag'];stem=tag['datum']+'-'+tag['group'].replace('!','_');rawfile=path.parent/'tag-buffers'/(stem+'-raw.bin');processedfile=path.parent/'tag-buffers'/(stem+'-processed.bin')
   assert sha(rawfile)==t['raw_sha256']and sha(processedfile)==t['processed_sha256']
   with Path(t['owner']).open('rb')as f:f.seek(tag['offset']);raw=f.read(tag['size'])
   assert raw==rawfile.read_bytes();private=bytearray(raw)
   for s in stores:
    if s['owner']==t['owner']and s['tag']['datum']==tag['datum']:
     offset=s['tag_relative_offset'];assert private[offset:offset+s['size']].hex()==s['before_hex'];private[offset:offset+s['size']]=bytes.fromhex(s['after_hex'])
   assert bytes(private)==processedfile.read_bytes()
  metrics.append(dict(map=summary['map'],metadata_stores=41,actual_changes=38,changed_tags=13,executed_original_instructions=382))
 pins.update(sourcehashes)
 r=dict(scope='Every supplied Vista multiplayer scenario, unchanged original MP combat path with ABI-bound actual resident metadata. Whole loader, preceding fixups, MCC loader, and live gameplay remain unexecuted.',all_checks_passed=True,multiplayer_maps=23,metadata_stores=943,actual_changes=874,executed_original_instructions=8786,independently_checked_raw_owner_offsets=raw_reads,retained_repeat_files=len(files),source_callbacks_or_returns_supplied=0,per_map=metrics,pins=pins,verifier_sha256=sha(Path(__file__)))
 assert raw_reads==943 and top['metadata_store_count']==943 and top['actual_changed_store_count']==874 and top['executed_original_instructions']==8786
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2,allow_nan=False)+'\n');print(json.dumps({k:v for k,v in r.items()if k not in('pins','per_map')}))
if __name__=='__main__':main()
