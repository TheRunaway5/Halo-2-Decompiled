#!/usr/bin/env python3
"""Independently verify the retained literal PC postprocessor evidence."""
import argparse,hashlib,json
from pathlib import Path
from pc_postprocess_inspect import ROOT,OUT,PE,VISTA,sha

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 a=OUT/'vista-measured';b=OUT/'vista-repeat';ra=json.loads((a/'report.json').read_text());rb=json.loads((b/'report.json').read_text());assert ra==rb
 paths=sorted(p.relative_to(a)for p in a.rglob('*')if p.is_file());assert paths==sorted(p.relative_to(b)for p in b.rglob('*')if p.is_file())
 pins={}
 for rel in paths:
  assert(a/rel).read_bytes()==(b/rel).read_bytes();pins[str(a/rel)]=sha(a/rel);pins[str(b/rel)]=sha(b/rel)
 for field,path in [('executable_sha256',VISTA),('tool_sha256',ROOT/'tools/pc_postprocess_vista_oracle.py'),('helper_sha256',ROOT/'tools/pc_postprocess_inspect.py'),('reader_sha256',ROOT/'tools/inspect_pc_combat_tag_sample.py'),('xbox_report_sha256',ROOT/'analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json')]:
  assert ra[field]==sha(path);pins[str(path)]=sha(path)
 for row in ra['cache_inputs']:assert row['sha256']==sha(Path(row['path']));pins[row['path']]=row['sha256']
 pe=PE(VISTA)
 for address in (0x46efdb,0x428403):assert(a/f'{address:08x}.asm').read_text()==pe.listing(address)
 instructions,calls=pe.trace(0x46efdb);executed=[int(p,16)for p in ra['executed_instruction_addresses']]
 assert len(executed)==382 and len(instructions)==573 and all(p in instructions and instructions[p].mnemonic!='call'for p in executed)
 cr,_=pe.trace(0x428403);assert cr[0x4284ac].op_str=='0x427edd'and cr[0x4284b1].op_str=='0x46efdb'
 xs=json.loads((ROOT/'analysis/multiplayer/title-update-1.5/combat-postprocess/measured/report.json').read_text());xr=next(x for x in xs['images']if x['image_label']=='owned_1_5');xstores=[r for r in xr['ordered_stores']if r['region']=='original_cache_metadata'];vstores=[r for r in ra['ordered_stores']if r['region']=='actual_resident_metadata'];assert len(xstores)==len(vstores)==41
 for x,v in zip(xstores,vstores):assert(x['tag']['group'],x['tag']['name'],x['after_hex'])==(v['tag']['group'],v['tag']['name'],v['after_hex'])
 sample=ROOT/'analysis/multiplayer/pc-gameplay-tag-comparison-20261011/corrected-sample/report.json';s=json.loads(sample.read_text());pins[str(sample)]=sha(sample)
 for label,platform in s['platforms'].items():
  for x,v in zip(vstores,platform['rows']):
   assert(x['tag']['group'],x['tag']['name'],x['after_hex'])==(v['tag_group'],v['tag_name'],v['xbox_processed_hex'])
   assert v['pc_raw_hex']==(x['before_hex']if label=='supplied_vista'else x['after_hex'])
 assert ra['metadata_store_count']==41 and ra['actual_changed_store_count']==38 and len(ra['changed_tags'])==13 and not ra['original_services_or_callback_returns_supplied']
 r=dict(scope='Retained unchanged Vista MP-path execution, literal caller, exact repeats and independent raw/processed PC field cross-check. No whole PC loader/gameplay claim.',all_checks_passed=True,metadata_destinations=41,actual_changes=38,changed_tags=13,executed_original_instructions=382,original_services_or_returns_supplied=0,complete_native_matches_proven=0,complete_mcc_loaders_executed=0,pins=pins,verifier_sha256=sha(Path(__file__)))
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:v for k,v in r.items()if k!='pins'}))
if __name__=='__main__':main()
