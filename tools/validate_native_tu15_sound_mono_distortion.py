#!/usr/bin/env python3
"""Check native compiled arithmetic against frozen numerical reference fixtures.

No original executable, DSP program/core, cache or opaque state bank is loaded.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',type=Path,required=True)
p.add_argument('--vectors',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args()
raw=a.vectors.read_bytes();fixtures=json.loads(raw)
frames=fixtures['frames'];assert len(frames)==105
keys=('parameters','gain','flags','last_dry_word','histories','output_words')
addresses=('native_parameters_address','native_state_address','native_input_address','native_output_address')
cases=set();results=[];tiers={}
for row in frames:
    inputs=row['input_words'];assert len(inputs)==95 and inputs[8]==0;assert all(isinstance(v,int) and 0<=v<=0xffffff for v in inputs)
    text=' '.join(map(str,inputs))+'\n'
    r=subprocess.run([str(a.probe)],input=text,capture_output=True,text=True,timeout=10)
    assert r.returncode==0 and not r.stderr, (row['case'],row['frame'],r.returncode,r.stderr)
    actual=json.loads(r.stdout);expected=row['expected']
    assert set(expected)==set(keys), 'Unexpected fixture output contract'
    for key in keys:assert actual[key]==expected[key],(row['case'],row['frame'],key)
    assert all(actual[key]>0xffffffff for key in addresses), 'Native allocation is not above4GiB'
    cases.add(row['case']);tiers[row['oracle_tier']]=tiers.get(row['oracle_tier'],0)+1
    results.append({'case':row['case'],'frame':row['frame'],'oracle_tier':row['oracle_tier'],
                    'passed':True,'native_allocations_above4GiB':True})
assert len(cases)==39 and len({(r['case'],r['frame']) for r in frames})==105
assert sorted(tiers.values())==[48,57]
report={'passed':True,'frames':len(frames),'cases':len(cases),'oracle_tiers':tiers,
        'fixtures_sha256':hashlib.sha256(raw).hexdigest(),'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),
        'runtime_original_program_or_core_loaded':False,'whole_DSP_ready':False,'initializer45_complete':False,
        'results':results}
a.report.parent.mkdir(parents=True,exist_ok=True)
a.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='results'}))
