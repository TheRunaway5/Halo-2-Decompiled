#!/usr/bin/env python3
"""Compare typed native controller arithmetic with frozen original results."""
from pathlib import Path
import argparse,hashlib,json,subprocess
from collections import Counter
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',type=Path,required=True);p.add_argument('--vectors',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
raw=a.vectors.read_bytes();v=json.loads(raw)
assert len(v['frames'])==116
records=Counter();cases=set();emissions=0
for row in v['frames']:
 words=row['input_words'];assert len(words)==40 and words[1:4]==[0,0,0]
 assert all(type(word)is int and 0<=word<=0xffffff for word in words)
 assert row['record']in(11,12,13,14)
 r=subprocess.run([str(a.probe.resolve())],input=' '.join(map(str,words))+'\n',capture_output=True,text=True,timeout=10)
 assert r.returncode==0 and not r.stderr,(row['case'],row['frame'],row['record'],r.stderr)
 got=json.loads(r.stdout)
 for key,value in row['expected'].items():assert got[key]==value,(row['case'],row['record'],row['frame'],key)
 assert len(got['native_addresses'])==3 and all(address>0xffffffff for address in got['native_addresses'])
 records[row['record']]+=1;cases.add(row['case']);emissions+=got['produced_count']
assert records=={11:29,12:29,13:29,14:29}and len(cases)==11 and emissions==1008
report={'passed':True,'numeric_calls':116,'original_frames':29,'cases':11,'record_calls':dict(records),'emissions':emissions,'individual_additions':8064,'real_typed_allocations_above_4GiB':True,'runtime_original_program_or_core_loaded':False,'source_DSP_producer_admitted':False,'source_DMA_scratch_writeback_implemented':False,'whole_DSP_ready':False,'initializer45_complete':False,'fixture_sha256':hashlib.sha256(raw).hexdigest(),'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest()}
a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
