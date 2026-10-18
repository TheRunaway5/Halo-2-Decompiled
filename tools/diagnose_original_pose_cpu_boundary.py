#!/usr/bin/env python3
"""Pin observed RSQRT and state differences without changing the native game.

Use original-emulator and unchanged-original-host observations. A source download
is authenticated against the pinned commit's Git blob. Keep the precision
boundary open until original Xbox CPU results are measured.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import struct
import urllib.request
import base64
from original_xbox_pose_case import sha

def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def real(value):return struct.unpack('<f',struct.pack('<I',value))[0]
def single(value):return real(word(value))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('case','oracle','native','host','output'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']);out=args.output.absolute();out.mkdir(exist_ok=False)
    original=json.loads((args.oracle/'report.json').read_text());host=json.loads((args.host/'report.json').read_text())
    native=json.loads((args.native/'native-observations.json').read_text());call=json.loads((args.case/'call.json').read_text())
    assert original['passed']and host['passed']and native['returned']
    left=json.loads((args.oracle/'CPU-estimates.json').read_text());right=native['actual_host_CPU_estimates']
    assert len(left)==len(right)==148
    assert all((a['instruction'],a['input'])==(b['instruction'],b['input'])for a,b in zip(left,right))
    assert all(a['output']!=b['output']for a,b in zip(left,right))
    assert all(word(1.0/single(math.sqrt(real(a['input']))))==a['output']for a in left)
    state=call['first_person_state'];reconstructed=bytearray((args.case/'before-first-person-state.bin').read_bytes())
    for row in native['memory_writes']:
        raw=bytes.fromhex(row['written']);old=bytes.fromhex(row['before']);start=row['address']
        lo=max(start,state['address']);hi=min(start+len(raw),state['address']+state['bytes'])
        if lo<hi:
            i=lo-state['address'];j=lo-start;n=hi-lo
            assert reconstructed[i:i+n]==old[j:j+n]
            reconstructed[i:i+n]=raw[j:j+n]
    host_state=(args.host/'original-host-first-person-state.bin').read_bytes()
    assert bytes(reconstructed)==host_state
    original_state=(args.case/'after-first-person-state.bin').read_bytes()
    differences=sum(a!=b for a,b in zip(reconstructed,original_state));assert differences>0
    commit='fc24584ce88f0915ad7f04775bb7712c2e3f49ee';source_path='target/i386/ops_sse.h'
    api=f'https://api.github.com/repos/xemu-project/xemu/contents/{source_path}?ref={commit}'
    with urllib.request.urlopen(api,timeout=20)as response:metadata=json.load(response)
    payload=base64.b64decode(metadata['content']);blob=hashlib.sha1(b'blob '+str(len(payload)).encode()+b'\0'+payload).hexdigest()
    assert blob==metadata['sha']
    text=payload.decode();begin=text.index('void glue(helper_rsqrtps, SUFFIX)');end=text.index('\n#if SHIFT == 1',begin)
    assert 'float32_div(float32_one,'in text[begin:end]and 'float32_sqrt(s->ZMM_S(i)'in text[begin:end]
    (out/'xemu-ops_sse.h').write_bytes(payload);shutil.copyfile(Path(__file__),out/Path(__file__).name)
    cpu_description=[line for line in Path('/proc/cpuinfo').read_text().splitlines()if line.startswith(('vendor_id','cpu family','model\t','model name','stepping'))][:5]
    report=dict(passed=True,scope=__doc__,entry=call['entry'],source_tick=call['tick_before'],
        original_emulator_call_replay_exact=True,unchanged_original_host_matches_native=True,
        original_emulator_equals_native=False,instruction_trace_equal=True,
        exact_host_native_source_store_bytes=host['native_written_bytes'],
        exact_host_native_full_state_bytes=len(reconstructed),
        native_full_state_sha256=hashlib.sha256(reconstructed).hexdigest(),
        original_host_full_state_sha256=host['original_host_first_person_state_sha256'],
        original_emulator_full_state_sha256=hashlib.sha256(original_state).hexdigest(),
        host_vs_original_emulator_state_different_bytes=differences,
        observed_estimate_lanes=len(left),same_instruction_and_input_lanes=len(left),different_estimate_outputs=len(left),
        original_emulator_estimates_match_single_sqrt_then_divide=True,
        first_estimate=dict(original_emulator=left[0],actual_host=right[0]),
        first_different_store_instruction=0x2961e1,
        native_x87_status=native['CPU']['x87_status'],original_emulator_x87_status=original['actual_CPU']['x87_status'],
        raw_x87_status_difference_retained=True,
        host_CPU=cpu_description,
        primary_source=dict(commit=commit,path=source_path,git_blob_sha1=blob,sha256=sha(out/'xemu-ops_sse.h'),
            url=f'https://github.com/xemu-project/xemu/blob/{commit}/{source_path}#L843-L853'),
        linked_reports_sha256={str(p.absolute()):sha(p)for p in (args.case/'call.json',args.oracle/'report.json',args.oracle/'CPU-estimates.json',
            args.native/'report.json',args.native/'native-observations.json',args.host/'report.json',args.host/'original-host-observations.json.gz')},
        tool_sha256=sha(Path(__file__)),runner_scratch=str(scratch),large_artifact_retention=[],
        production_game_code_changed=False,physical_xbox_verified=False,full_weapon_parity=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,exact_host_native_state_bytes=len(reconstructed),estimate_lanes=len(left),
        different_original_emulator_estimates=len(left),original_emulator_native_precision_gate_closed=False,
        report=str(out/'report.json'))),flush=True)

if __name__=='__main__':main()
