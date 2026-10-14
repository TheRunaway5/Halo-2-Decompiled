#!/usr/bin/env python3
"""Verify owned particle visibility, initialization and one-live-particle cleanup.

The Wraith charge entry is a diagnostic. The declared native camera and
retail-source portal producer feed original3F3E0; full TU1.5 camera/portal
production, draw submission, installed-runtime delivery and Xbox pixels remain
unverified. The live particle is removed by the original stale-draw rule.
"""
import argparse,json,struct
from pathlib import Path
from audit_rendering import file_sha
from verify_particle_frame_evidence_tu15 import equal_banks,pool_counts
ROOT=Path(__file__).resolve().parents[1]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--base',type=Path,default=ROOT/'analysis/weapons-tu15-goal-20261013');ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();base=a.base.resolve();assert not a.output.exists()
    kernel_dir=base/'particle-visibility-current-native-v1';kernel=json.loads((kernel_dir/'report.json').read_text());assert kernel['passed'] and len(kernel['cases'])==9
    for case in kernel['cases']:
        original=base/case['original'];native=kernel_dir/case['original'];r=json.loads((original/'report.json').read_text());h=json.loads((original/'hardware/report.json').read_text())
        assert r['completed'] and not r['supplied_game_services'] and not r['instruction_patches'] and h['emulator_retained_state_matches_hardware']
        assert file_sha(original/'report.json')==case['original_report_sha256'] and file_sha(original/'hardware/report.json')==case['hardware_report_sha256']
        assert (native/'native-trace.bin').read_bytes()==(original/'original-trace.bin').read_bytes()
        assert (native/'native-stores.bin').read_bytes()==(original/'original-stores.bin').read_bytes()
        expected=bytearray((original/'hardware/hardware-exit.bin').read_bytes());assert len(expected)==180 and struct.unpack_from('<I',expected,160)[0]&0x200;expected[161]&=0xfd
        assert (native/'native-exit.bin').read_bytes()==expected
        assert equal_banks(native/'native-after.bin',original/'hardware/hardware-after.bin')==58970198
        for name,digest in case['files_sha256'].items():assert file_sha(native/name)==digest
    live=base/'wraith-particle-live-stop-native-v1';lifetime=json.loads((live/'particle-lifetime.json').read_text());assert lifetime['passed'] and lifetime['stages'][-1]['salt']==0
    phases=['stop/before.bin','stop/after.bin','particle-frame1/after.bin','particle-frame2/after.bin'];counts=[pool_counts(live/p) for p in phases]
    assert [c['particles']['count'] for c in counts]==[1,1,0,0]
    assert [c['particle_system']['count'] for c in counts]==[4,4,4,0]
    assert [c['particle_emitter']['count'] for c in counts]==[4,4,4,0]
    assert [c['particle_location']['count'] for c in counts]==[4,4,4,0]
    api=[]
    for name in ('particle-live-stop-frame1-original-v1','particle-live-stop-frame2-original-v1'):
        p=base/name;r=json.loads((p/'report.json').read_text());h=json.loads((p/'hardware/report.json').read_text());assert r['completed'] and h['emulator_retained_state_matches_hardware'];f=r['native_api_frame'];native=Path(f['native_after']);assert file_sha(native)==f['native_after_sha256']
        assert file_sha(Path(r['origin_case'])/'build.json')==f['native_build_sha256']
        api.append(dict(original=name,retained_bytes=equal_banks(native,p/'hardware/hardware-after.bin'),original_report_sha256=file_sha(p/'report.json'),hardware_report_sha256=file_sha(p/'hardware/report.json')))
    charge=[]
    for case in (live,live/'stop',base/'wraith-particle-visible-native-v6'):
        o=json.loads((case/'original-comparison-v2.json').read_text());h=json.loads((case/'hardware-verification/report.json').read_text());assert h['passed'] and h['exit_identical_except_platform_IF'] and not h['retained_bank_differences'] and not o['services'] and not o['instruction_patches'];assert o['matched_native_stores']==o['total_native_stores'] and o['final_retained_bytes_compared']==58970198
        assert o['case_before_sha256']==file_sha(case/'before.bin');charge.append(dict(case=str(case.relative_to(base)),original_report_sha256=file_sha(case/'original-comparison-v2.json'),hardware_report_sha256=file_sha(case/'hardware-verification/report.json')))
    visible=json.loads((live/'visibility.json').read_text());assert visible['clusters']==[21,45,44] and visible['containing_cluster']==21 and not visible['dynamic_portal_associations'] and not visible['exhausted_limits']
    generated=json.loads((base/'particle-frame-native-functions-v6/generation.json').read_text());assert len(generated['complete_native_entries'])==47 and len(generated['lowered_instructions'])==2388 and generated['generated_cpp_sha256']==file_sha(ROOT/'src/source_effect_particle_frame_tu15_generated.cpp')
    boundary=json.loads((base/'effect-frame-initialized-original-v3/report.json').read_text());assert not boundary['completed'] and boundary['unowned_boundary']['target']==0x800000a6 and not boundary['supplied_game_services'] and not boundary['instruction_patches']
    components=json.loads((base/'particle-frame-root-components-v4/report.json').read_text());assert components['passed'] and not components['root_drift_after_compile']
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),kernel_report_sha256=file_sha(kernel_dir/'report.json'),native_kernel_cases=9,native_api_cases=api,charge_cases=charge,live_particle_stop_pool_counts=counts,lifetime=lifetime,lifetime_sha256=file_sha(live/'particle-lifetime.json'),visibility=visible,visibility_sha256=file_sha(live/'visibility.json'),generation_report_sha256=file_sha(base/'particle-frame-native-functions-v6/generation.json'),root_component_report_sha256=file_sha(base/'particle-frame-root-components-v4/report.json'),full_frame_unowned_boundary=boundary['unowned_boundary'],full_frame_report_sha256=file_sha(base/'effect-frame-initialized-original-v3/report.json'),live_particle_instances_before_stop=1,stale_draw_particle_removal_verified=True,source_effect_retirement_verified=True,full_tu15_portal_camera_producer_verified=False,original_draw_submission_verified=False,renderer_integration_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(passed=True,native_kernel_cases=9,native_api_cases=2,live_particles_before_stop=1,retirement_tick=lifetime['stages'][-1]['source_tick'])))
if __name__=='__main__':main()
