#!/usr/bin/env python3
"""Check the complete retained native trace against frozen original-linked proof.

The probe also compares every call with the previous native value/state code.
No original executable, DSP instruction, interpreter or runtime bank is loaded.
"""
from pathlib import Path
import argparse,gzip,hashlib,json,subprocess,tempfile
def check(value,message):
    if not value: raise RuntimeError(message)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--probe',type=Path,required=True)
parser.add_argument('--cases',type=Path,required=True)
parser.add_argument('--proof',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args()
proof=json.loads(args.proof.read_text());compressed=args.cases.read_bytes();raw=gzip.decompress(compressed)
check(proof['schema']=='H2_RECORD0_CONTEXT_COMPACT_NUMERIC_V1','Unsupported fixture schema')
check(hashlib.sha256(compressed).hexdigest()==proof['cases_gzip_sha256'],'Compressed fixture differs')
check(len(raw)==proof['cases_uncompressed_bytes'] and hashlib.sha256(raw).hexdigest()==proof['cases_uncompressed_sha256'],'Entry fixtures differ')
check(len(json.loads(raw))==proof['explicit_cases']==576,'Entry case count differs')
digest=hashlib.sha256();calls=0;output_bytes=0
with tempfile.TemporaryFile()as errors:
    process=subprocess.Popen([str(args.probe.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors)
    try:
        process.stdin.write(raw);process.stdin.close()
        while True:
            chunk=process.stdout.read(65536)
            if not chunk:break
            digest.update(chunk);calls+=chunk.count(b'\n');output_bytes+=len(chunk)
        result=process.wait(timeout=60);errors.seek(0);stderr=errors.read()
        check(result==0 and not stderr,'Native record0 probe failed: '+stderr.decode(errors='replace'))
    except BaseException:
        if process.poll() is None:process.kill()
        process.wait();raise
check(calls==proof['expected_calls']==1152,'Retained call count differs')
check(digest.hexdigest()==proof['expected_native_stdout_sha256'],'Complete output/state/arithmetic/status/provenance/loop trace differs')
report=dict(passed=True,explicit_cases=576,retained_calls=calls,arithmetic_transfer_steps=proof['arithmetic_transfer_steps'],
    loop_checkpoints=proof['loop_checkpoints'],knowledge_provenance_checks=proof['knowledge_provenance_checks'],
    complete_trace_sha256=digest.hexdigest(),complete_trace_bytes=output_bytes,compressed_fixture_bytes=len(compressed),
    fixture_roundtrip_sha256=hashlib.sha256(raw).hexdigest(),previous_native_value_state_comparisons=True,
    authentic_boot_context_proven=False,all_legal_mod_profiles_admitted=False,initializer45_ready=False,
    original_executable_or_DSP_core_loaded=False)
args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
