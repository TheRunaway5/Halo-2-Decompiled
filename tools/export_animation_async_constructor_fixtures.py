#!/usr/bin/env python3
"""Export source-complete async initialization/store prefixes; no handle outcome."""
import argparse,json
from pathlib import Path
from oracle_mp_animation_residency_1_1 import sha

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--owning',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=json.loads(a.source.read_text());o=json.loads(a.owning.read_text())
 assert d['owning_source_sha256']==sha(a.owning)
 lines=['SOURCE '+sha(a.source),'BIND 004e7938 004e9eb8 004e9ebc 004e7928 004e792c 004e7930 004e7924 005098a0 005018a0 00122350']
 descriptor=o['stages'][-1]['before']['source_worker_stack_descriptor12']
 for c in d['cases']:
  lines+=['CASE '+c['name'],'MEM 004e7924 '+c['before_hex'],'MEM 005098a0 '+descriptor,'AFTER 004e7924 '+c['after_hex'],'AFTER 005098a0 '+descriptor]
  for q in c['stores']:
   lines.append('STORE '+q['instruction']+' '+q['address']+' '+f'{int.from_bytes(bytes.fromhex(q["before"]),"little"):08x}'+' '+f'{int.from_bytes(bytes.fromhex(q["written"]),"little"):08x}')
  lines+=['MISSING native_create_mutex','END']
 raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),cases=len(d['cases']),stores=sum(len(q['stores'])for q in d['cases']))))
if __name__=='__main__':main()
