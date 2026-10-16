#!/usr/bin/env python3
"""Compare compiled native chorus with pinned original numeric boundary values.

This consumes typed numeric fixtures only; it loads no original program/core/XBE
or opaque DSP state bank. The coupled probe owns one1601-cell history object.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',type=Path,required=True);p.add_argument('--coupled-probe',type=Path,required=True)
p.add_argument('--vectors',type=Path,required=True);p.add_argument('--report',type=Path)
a=p.parse_args();v=json.loads(a.vectors.read_text())
assert v['schema']=='H2_TU15_CHORUS_NUMERIC_VECTORS_V1'
assert (v['case_count'],v['original_frame_count'],v['individual_call_count'],v['coupled_batch_count'])==(17,35,140,35)
frames=v['frames'];batches=v['coupled_batches'];assert len(frames)==140 and len(batches)==35
assert len({x['case']for x in frames})==17 and len({(x['case'],x['frame'])for x in frames})==35 and len({(x['case'],x['frame'],x['record'])for x in frames})==140
keys={'flags','gain','depth','cursor','cached_end','output','history','trace','warm_alternate_output_unchanged'}
def run(binary,words):
 r=subprocess.run([str(binary.resolve())],input=' '.join(map(str,words))+'\n',text=True,capture_output=True,timeout=10)
 assert r.returncode==0 and not r.stderr,(binary,r.returncode,r.stderr)
 return json.loads(r.stdout)
for row in frames:
 words=row['input_words'];assert len(words)==512 and all(type(x)is int and 0<=x<=0xffffff for x in words)
 assert words[1:3]==[0,400]and words[3:9]==[0]*6 and set(row['expected'])==keys
 expected=row['expected'];assert len(expected['output'])==32 and len(expected['history'])==401 and len(expected['trace'])==32
 got=run(a.probe,words)
 for key in keys:assert got[key]==expected[key],(row['case'],row['frame'],row['record'],key)
 assert all(x>0xffffffff for x in got['native_addresses'])
endpoints=0;seen=[]
for batch in batches:
 indices=batch['call_indices'];assert len(indices)==4 and all(type(i)is int and 0<=i<len(frames)for i in indices)
 rows=[frames[i]for i in indices];assert [r['record']for r in rows]==[4,5,6,7]
 assert all((r['case'],r['frame'])==(batch['case'],batch['frame'])for r in rows)
 assert all(r['input_words'][47:79]==rows[0]['input_words'][47:79]for r in rows)
 got=run(a.coupled_probe,sum([r['input_words']for r in rows],[]));assert len(got['results'])==4
 for native,original in zip(got['results'],rows):
  for key in keys-{'warm_alternate_output_unchanged'}:assert native[key]==original['expected'][key],(batch['case'],batch['frame'],key)
 assert got['final_owner']==batch['expected_final_owner']and len(got['final_owner'])==1601
 assert got['history_owner_address']>0xffffffff and got['common_modulation_address']>0xffffffff
 assert got['view_addresses']==[got['history_owner_address']+i*400*4 for i in range(4)]
 for j in range(3):
  value=got['results'][j]['history'][-1]
  assert value==rows[j+1]['input_words'][111]==batch['expected_shared_endpoints'][j];endpoints+=1
 seen+=indices
assert sorted(seen)==list(range(140))and endpoints==v['shared_endpoint_observations']==105
report=dict(passed=True,cases=17,original_frames=35,individual_calls=140,coupled_batches=35,coupled_calls=140,source_state_words_compared_in_offline_provenance=72,history_cells_per_call=401,trace_values_exact=26880,one_history_owner_cells=1601,one_common_modulation_owner=True,shared_endpoint_observations=endpoints,real_typed_allocations_above_4GiB=True,runtime_original_program_core_XBE_or_statebank=False,physical_audio_parity=False,full_native_DSP_complete=False,initializer45_complete=False,vectors_sha256=hashlib.sha256(a.vectors.read_bytes()).hexdigest(),probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),coupled_probe_sha256=hashlib.sha256(a.coupled_probe.read_bytes()).hexdigest())
if a.report:a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
