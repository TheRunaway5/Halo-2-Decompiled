#!/usr/bin/env python3
"""Compare typed native crosstalk with original arithmetic and corrected fixture boundaries."""
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser(description=__doc__)
for name in('probe','vectors','join-vectors','report'):p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();v=json.loads(a.vectors.read_text());j=json.loads(a.join_vectors.read_text())
assert v['schema']=='H2_TU15_CROSSTALK_NUMERIC_VECTORS_V2'and len(v['frames'])==29
assert j['schema']=='H2_TU15_CROSSTALK_CORRECTED_JOIN_NUMERIC_VECTORS_V1'and len(j['frames'])==10 and j['retained_original_owner_frames']==6
keys={'flags','coefficients','pairs','outputs','arithmetic','exit_arithmetic_saturation'};calls=values=0
for row in v['frames']+j['frames']:
 words=row['input_words'];assert len(words)==313 and words[1:4]==[0,0,0]and all(type(w)is int and 0<=w<=0xffffff for w in words)
 assert set(row['expected'])==keys
 r=subprocess.run([str(a.probe.resolve())],input=' '.join(map(str,words))+'\n',capture_output=True,text=True,timeout=10);assert r.returncode==0 and not r.stderr,(row['case'],row['frame'],r.stderr)
 got=json.loads(r.stdout)
 for key in keys:assert got[key]==row['expected'][key],(row['case'],row['frame'],key)
 assert len(got['native_addresses'])==4 and all(x>0xffffffff for x in got['native_addresses'])
 assert len(got['arithmetic'])==64 and all(len(x)==8 for x in got['arithmetic'])and len(got['outputs'])==128
 calls+=1;values+=512
assert calls==39 and values==19968
report={'passed':True,'numeric_calls':calls,'base_original_calls':29,'corrected_fixture_boundary_calls':10,'arithmetic_values':values,'real_typed_allocations_above_4GiB':True,'retained_original_shared_owner_frames':6,'legacy_nonzero_gain_route_join_claim_withdrawn':True,'actual_nonzero_upstream_crosstalk_signal_established':False,'first_six_input_word_producer_established':False,'runtime_original_program_core_XBE_or_statebank':False,'full_native_DSP_complete':False,'initializer45_complete':False,'vectors_sha256':hashlib.sha256(a.vectors.read_bytes()).hexdigest(),'join_vectors_sha256':hashlib.sha256(a.join_vectors.read_bytes()).hexdigest(),'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest()}
a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
