#!/usr/bin/env python3
"""Verify an authored Hunter cannon's explicit positive108DB0 transition.

The probe uses actual Hunter and player bodies, a declared nearby player
position and60 neutral source ticks. It invokes the helper explicitly; an AI
actor, production AI fire input, complete campaign boot, particles being drawn
and physical Xbox visual/audio parity remain unverified.
"""
import argparse,json,struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import stores,regions
from verify_particle_frame_evidence_tu15 import equal_banks,pool_counts

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base',type=Path,default=ROOT/'analysis/weapons-tu15-goal-20261013')
    ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();base=a.base.resolve();assert not a.output.exists()
    case=base/'hunter-charge-visible-native-v6';original=base/'hunter-charge-transition-original-v2'
    metadata=json.loads((case/'native.json').read_text());build=json.loads((case/'build.json').read_text())
    r=json.loads((original/'report.json').read_text());h=json.loads((original/'hardware/report.json').read_text())
    assert build['returncode']==0 and metadata['entry']==0x108db0 and metadata['manual_diagnostic']
    assert not metadata['actor_created'] and metadata['effect_instance']!=0xffffffff
    assert r['completed'] and not r['supplied_game_services'] and not r['instruction_patches']
    assert h['emulator_retained_state_matches_hardware'] and h['original_report_sha256']==file_sha(original/'report.json')
    assert (case/'native-trace.bin').read_bytes()==(original/'original-trace.bin').read_bytes()
    stream=bytearray()
    for pc,p,before,after in stores(case/'native-stores.bin'):
        stream.extend(struct.pack('<III',pc,p,len(after))+before+after)
    assert stream==(original/'original-stores.bin').read_bytes()
    expected=bytearray((original/'hardware/hardware-exit.bin').read_bytes());assert len(expected)==180 and expected[161]&2
    expected[161]&=0xfd;assert (case/'native-exit.bin').read_bytes()==expected
    compared=equal_banks(case/'after.bin',original/'hardware/hardware-after.bin');assert compared==58970198
    counts=pool_counts(case/'after.bin')
    assert counts['particles']['count']==0
    for name in ['particle_system','particle_emitter','particle_location']:assert counts[name]['count']==3
    rows=list(regions(case/'after.bin'))
    def read(address,size):
        for p,b in rows:
            if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
        raise AssertionError(hex(address))
    def u(p):return int.from_bytes(read(p,4),'little')
    pool=u(0x4ecbd0);body=u(u(pool+0x44)+(metadata['weapon']&65535)*12+8)
    assert read(body+0x20c,1)==b'\1' and int.from_bytes(read(body+0x20e,2),'little')==30
    assert u(body+0x214)==metadata['effect_instance']
    hunter=u(u(pool+0x44)+(metadata['unit']&65535)*12+8);assert u(hunter+0x12c)==0xffffffff
    assert u(u(0x4f54f4)+0x3c)==1 and u(u(0x51df54)+8)==60
    generated=base/'pool-address-observer-regeneration-v2';g=json.loads((generated/'generation.json').read_text())
    assert g['generated_cpp_sha256']==file_sha(generated/'generated.cpp')==file_sha(ROOT/'src/physics/physics_pool_startup_runtime_generated.cpp')
    assert (case/'source/pool.cpp').read_bytes()==(ROOT/'src/physics/physics_pool_startup_runtime_generated.cpp').read_bytes()
    compile=json.loads((base/'pool-address-observer-root-compile-v1/report.json').read_text());assert compile['passed'] and not compile['root_drift_after_compile']
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),source_case=str(case),source_build_sha256=file_sha(case/'build.json'),original_report_sha256=file_sha(original/'report.json'),hardware_report_sha256=file_sha(original/'hardware/report.json'),original_instructions=r['original_instructions'],retained_writes=r['retained_stores'],native_trace_exact=True,ordered_writes_exact=True,full_exit_exact_except_linux_IF=True,retained_bytes=compared,particle_pools=counts,trigger_state=1,trigger_ticks=30,effect_instance=metadata['effect_instance'],source_tick=60,actual_local_players=1,actor_created=False,manual_helper_diagnostic=True,pool_generated_source_sha256=file_sha(generated/'generated.cpp'),pool_generation_sha256=file_sha(generated/'generation.json'),root_pool_compile_sha256=file_sha(base/'pool-address-observer-root-compile-v1/report.json'),production_ai_fire_input_verified=False,complete_campaign_boot_verified=False,live_particle_emission_verified=False,renderer_integration_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,instructions=r['original_instructions'],writes=r['retained_stores'],particle_systems=3,live_particles=0)))

if __name__=='__main__':main()
