#!/usr/bin/env python3
"""Compare native typed capture/prepare composition to unchanged real caller packets."""
import argparse,gzip,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--callers',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    reference=json.loads(a.callers.read_text());assert reference['passed']and reference['backend']=='physical-unchanged-i386-offline'and reference['instruction_patches']==[]and reference['supplied_callee_returns']==[]and reference['supplied_participant_outcomes']==[]
    fixture=ROOT/reference['canonical_fixture']['path'];assert sha(fixture)==reference['canonical_fixture']['sha256']
    with gzip.open(fixture)as stream:canonical=json.load(stream)
    cases={r['name']:r for r in canonical['cases']};rows=[]
    for packet in reference['packets']:
        name=packet['profile'];source=cases['capture_1_True'if name.startswith('caller_live_')else name]
        banks=[dict(address=b['address'],before=b.get('before')or canonical['bank_data'][b['before_ref']])for b in source['banks']]
        rows.append(dict(profile=name,banks=banks,options_value=source['options_value'],seed=packet['initial_gp'][5],before_fp=packet['before_fp'],occupied=packet['older_values']))
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);inputs=scratch/'inputs.json';inputs.write_text(json.dumps(rows));probe=scratch/'probe'
    names=['tools/probe_native_campaign_transition_participants_callers_tu15.cpp','src/campaign/native_campaign_transition_participants_tu15.cpp','include/halo2/native_campaign_transition_participants_tu15.hpp','include/halo2/native_campaign_transition_request_tu15.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_contact_record.hpp',str(Path(__file__).relative_to(ROOT))]
    pins={n:sha(ROOT/n)for n in names};command=['g++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(ROOT/names[0]),str(ROOT/names[1]),'-o',str(probe)]
    build=subprocess.run(command,text=True,capture_output=True);assert build.returncode==0,build.stderr
    execution=subprocess.run([str(probe),str(inputs)],text=True,capture_output=True);assert execution.returncode==0,execution.stderr
    results=[]
    for actual,expected in zip(json.loads(execution.stdout),reference['packets'],strict=True):
        digest=hashlib.sha256(bytes.fromhex(actual['options'])).hexdigest()
        checks=dict(profile=actual['profile']==expected['profile'],options=digest==expected['options_after_sha256'],fp=actual['after_fp']==expected['after_fp'],reads=actual['reads']==expected['persistent_reads'],stores=actual['stores']==expected['persistent_stores'],source_unchanged=actual['source_unchanged'])
        results.append(dict(profile=actual['profile'],checks=checks,passed=all(checks.values()),options_sha256=digest,after_fp=actual['after_fp'],reads=actual['reads'],stores=actual['stores']))
    report=dict(passed=all(r['passed']for r in results)and all(sha(ROOT/n)==v for n,v in pins.items()),scope='Native typed capture_value then prepare_value against actual unchanged191848/191855 caller options hashes, FP and persistent access counts; no CPU register/RET adapter or whole191750 claim',profiles=len(results),actual_original_call_packets=reference['actual_calls'],results=results,source_sha256=pins,original_caller_report=dict(path=str(a.callers),sha256=sha(a.callers)),canonical_fixture=reference['canonical_fixture'],compile_command=command,source_unchanged=all(sha(ROOT/n)==v for n,v in pins.items()),retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'],profiles=len(results),report=str(a.report))));return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
