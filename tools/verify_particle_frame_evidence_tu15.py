#!/usr/bin/env python3
"""Verify the retained particle-kernel, source-world API and cleanup evidence.

The tested graph has no live particle instances. Exact empty-system cleanup
does not certify emission, live-particle drain, renderer integration or a weapon.
"""
import argparse,json,struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import regions
ROOT=Path(__file__).resolve().parents[1]

def equal_banks(native,hardware):
    a=list(regions(native));b=list(regions(hardware));assert len(a)==len(b);total=0
    for (p,x),(q,y) in zip(a,b):
        assert p==q and len(x)==len(y)
        if 0x60000000<=p<0x60200000:continue
        assert x==y,hex(p);total+=len(x)
    return total

def pool_counts(path):
    rows=list(regions(path))
    def read(address,size):
        for p,b in rows:
            if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
        raise AssertionError(hex(address))
    def u(address):return int.from_bytes(read(address,4),'little')
    result={}
    for slot in (0x51df74,0x52d714,0x52d718,0x52d71c):
        p=u(slot);result[read(p,32).split(b'\0')[0].decode()]=dict(count=u(p+0x3c),stride=u(p+0x24))
    return result

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--base',type=Path,default=ROOT/'analysis/weapons-tu15-goal-20261013');ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();base=a.base.resolve();assert not a.output.exists();kernel=[];api=[];lifetimes=[]
    gold=['charge-particle-original-after-stop-v2','charge-particle-original-after-stop-frame2-v2']
    native_sets=[('O1',['charge-particle-native-after-stop-v2','charge-particle-native-after-stop-frame2-v2'],'particle-frame-native-build-v4'),('Release O3',['charge-particle-native-release-frame1-v1','charge-particle-native-release-frame2-v1'],'particle-frame-native-release-v1')]
    for optimization,names,build in native_sets:
        for original_name,native_name in zip(gold,names):
            o=base/original_name;n=base/native_name;r=json.loads((o/'report.json').read_text());h=json.loads((o/'hardware/report.json').read_text());assert r['completed'] and not r['supplied_game_services'] and not r['instruction_patches'] and h['emulator_retained_state_matches_hardware']
            assert (n/'native-trace.bin').read_bytes()==(o/'original-trace.bin').read_bytes()
            assert (n/'native-stores.bin').read_bytes()==(o/'original-stores.bin').read_bytes()
            raw=(n/'native-exit.bin').read_bytes();expected=bytearray((o/'hardware/hardware-exit.bin').read_bytes());assert struct.unpack_from('<I',expected,160)[0]&0x200 and not struct.unpack_from('<I',raw,160)[0]&0x200;expected[161]&=0xfd;assert raw==expected
            total=equal_banks(n/'native-after.bin',o/'hardware/hardware-after.bin')
            kernel.append(dict(optimization=optimization,original=original_name,native=native_name,build_sha256=file_sha(base/build/'build.json'),instructions=r['original_instructions'],retained_stores=r['retained_stores'],retained_bytes=total,native_after_sha256=file_sha(n/'native-after.bin'),native_exit_sha256=file_sha(n/'native-exit.bin'),hardware_report_sha256=file_sha(o/'hardware/report.json')))
    for name in ('particle-api-frame1-original-v1','particle-api-frame2-original-v1','particle-api-interleaved-frame1-original-v1','particle-api-interleaved-frame2-original-v1'):
        p=base/name;r=json.loads((p/'report.json').read_text());h=json.loads((p/'hardware/report.json').read_text());assert r['completed'] and h['emulator_retained_state_matches_hardware'];native=Path(r['native_api_frame']['native_after']);assert file_sha(native)==r['native_api_frame']['native_after_sha256'];assert file_sha(Path(r['origin_case'])/'build.json')==r['native_api_frame']['native_build_sha256']
        api.append(dict(original=name,retained_bytes=equal_banks(native,p/'hardware/hardware-after.bin'),native_after_sha256=file_sha(native),original_report_sha256=file_sha(p/'report.json'),hardware_report_sha256=file_sha(p/'hardware/report.json')))
    for name in ('wraith-particle-coupled-native-v2','wraith-particle-coupled-interleaved-v1'):
        p=base/name;l=json.loads((p/'particle-lifetime.json').read_text());assert l['passed'] and l['stages'][-1]['salt']==0 and l['stages'][-1]['particle_head']==0xffffffff
        counts=[pool_counts(p/q) for q in ('stop/after.bin','particle-frame1/after.bin','particle-frame2/after.bin')]
        assert all(c['particles']['count']==0 for c in counts)
        assert [c['particle_system']['count'] for c in counts]==[4,4,0]
        assert [c['particle_emitter']['count'] for c in counts]==[4,0,0]
        assert [c['particle_location']['count'] for c in counts]==[4,0,0]
        lifetimes.append(dict(case=name,lifetime=l,lifetime_sha256=file_sha(p/'particle-lifetime.json'),pool_counts=counts))
    generated=json.loads((base/'particle-frame-native-functions-v1/generation.json').read_text());assert len(generated['complete_native_entries'])==38 and len(generated['lowered_instructions'])==2016
    assert generated['generated_cpp_sha256']==file_sha(ROOT/'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp')
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),generation_report_sha256=file_sha(base/'particle-frame-native-functions-v1/generation.json'),native_kernel_cases=kernel,native_api_cases=api,coupled_lifetimes=lifetimes,empty_system_cleanup_verified=True,live_particle_instances_in_capture=0,live_particle_emission_verified=False,live_particle_drain_verified=False,renderer_frame_integration_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(passed=True,native_kernel_cases=len(kernel),native_api_cases=len(api),coupled_lifetimes=len(lifetimes),live_particle_instances=0)))
if __name__=='__main__':main()
