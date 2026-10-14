#!/usr/bin/env python3
"""Verify genuine TU1.5 Hunter AI/input/perception frames against host hardware.

Requires untouched original code, no supplied game returns, exact instruction
and write streams, full hardware CPU exit except Linux IF and all retained
non-stack bytes. Hostile firing, full boot, rendering and weapons remain open.
"""
import argparse,json
from pathlib import Path
from audit_rendering import file_sha
from verify_particle_frame_evidence_tu15 import equal_banks
ROOT=Path(__file__).resolve().parents[1]
DEFAULT_PAIRS=['hunter-ai-team-frame-original-v1:hunter-ai-frame-root-native-v4','hunter-coupled-ai-frame1-original-v1:hunter-coupled-ai-frame1-native-v2','hunter-coupled-ai-frame2-original-v1:hunter-coupled-ai-frame2-native-v2','hunter-coupled-ai-frame4-original-v1:hunter-coupled-ai-frame4-native-v1']

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base',type=Path,default=ROOT/'analysis/weapons-tu15-goal-20261013')
    ap.add_argument('--generation',default='ai-frame-native-functions-v6')
    ap.add_argument('--build',default='ai-frame-root-native-build-v4')
    ap.add_argument('--module',type=Path,default=ROOT/'src/source_ai_frame_tu15_generated.cpp')
    ap.add_argument('--pair',action='append',help='Original directory:native directory within base')
    ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();base=a.base.resolve();assert not a.output.exists()
    generation_path=base/a.generation/'generation.json';generation=json.loads(generation_path.read_text())
    build_path=base/a.build/'build.json';build=json.loads(build_path.read_text())
    assert build['passed'] and not build['root_drift_after_compile']
    assert build['module_sha256']==generation['generated_cpp_sha256']==file_sha(a.module)
    assert file_sha(base/a.build/'replay-native')==build['binary_sha256']
    assert all(str(ROOT/'build-sandbox') not in p for command in build['commands'] for p in command)
    cases=[]
    for pair in a.pair or DEFAULT_PAIRS:
        original_name,native_name=pair.split(':');original=base/original_name;native=base/native_name
        r=json.loads((original/'report.json').read_text());h=json.loads((original/'hardware/report.json').read_text())
        assert r['completed'] and r['entry']==0x1d0670 and not r['instruction_patches'] and not r['supplied_game_services']
        assert h['emulator_retained_state_matches_hardware'] and h['original_report_sha256']==file_sha(original/'report.json')
        for name,digest in r['files_sha256'].items():assert file_sha(original/name)==digest
        assert file_sha(original/'hardware/hardware-after.bin')==h['hardware_after_sha256']
        assert file_sha(original/'hardware/hardware-exit.bin')==h['hardware_exit_sha256']
        assert r['original_functions']['001ec640']==r['original_functions']['000c92c0']==1
        assert (native/'native-trace.bin').read_bytes()==(original/'original-trace.bin').read_bytes()
        assert (native/'native-stores.bin').read_bytes()==(original/'original-stores.bin').read_bytes()
        expected=bytearray((original/'hardware/hardware-exit.bin').read_bytes());assert len(expected)==180 and expected[161]&2
        expected[161]&=0xfd;assert (native/'native-exit.bin').read_bytes()==expected
        compared=equal_banks(native/'native-after.bin',original/'hardware/hardware-after.bin');assert compared==58970198
        cases.append(dict(original=original_name,native=native_name,instructions=r['original_instructions'],writes=r['retained_stores'],retained_bytes=compared,original_report_sha256=file_sha(original/'report.json'),hardware_report_sha256=file_sha(original/'hardware/report.json'),native_after_sha256=file_sha(native/'native-after.bin'),native_exit_sha256=file_sha(native/'native-exit.bin'),trace_exact=True,ordered_writes_exact=True,complete_hardware_cpu_exit_except_IF=True))
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),generation_sha256=file_sha(generation_path),build_sha256=file_sha(build_path),module_sha256=file_sha(a.module),whole_bodies=len(generation['complete_native_entries']),lowered_instructions=len(generation['lowered_instructions']),instructions=sum(r['instructions'] for r in cases),retained_writes=sum(r['writes'] for r in cases),cases=cases,actual_npc_unit_input_producer_verified=True,hostile_firing_verified=False,complete_campaign_boot_verified=False,renderer_integration_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(passed=True,cases=len(cases),instructions=result['instructions'],writes=result['retained_writes'])))
if __name__=='__main__':main()
