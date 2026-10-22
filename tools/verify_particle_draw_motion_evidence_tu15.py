#!/usr/bin/env python3
"""Verify the bounded TU1.5 particle draw, motion, natural drain and portal evidence.

Every weapon remains open for complete feature and visual parity. These are
native CPU/source-bank comparisons for one manual MP Wraith charge diagnostic,
one declared view, and its source stop request; no GPU or Xbox pixel claim.
"""
from source_layout import source_file
import argparse,json,struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import regions
from verify_particle_frame_evidence_tu15 import equal_banks,pool_counts

ROOT=Path(__file__).resolve().parents[1]

def reader(path):
    rows=list(regions(path))
    def read(address,size):
        for p,b in rows:
            if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
        raise AssertionError(hex(address))
    return read

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base',type=Path,default=ROOT/'analysis/weapons-tu15-goal-20261013')
    ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();base=a.base.resolve();assert not a.output.exists()
    comparisons=[]
    def native(original,output):
        r=json.loads((original/'report.json').read_text());h=json.loads((original/'hardware/report.json').read_text())
        assert r['completed'] and not r['supplied_game_services'] and not r['instruction_patches']
        assert h['emulator_retained_state_matches_hardware'] and h['original_report_sha256']==file_sha(original/'report.json')
        assert (output/'native-trace.bin').read_bytes()==(original/'original-trace.bin').read_bytes()
        assert (output/'native-stores.bin').read_bytes()==(original/'original-stores.bin').read_bytes()
        exit=bytearray((original/'hardware/hardware-exit.bin').read_bytes());assert len(exit)==180 and struct.unpack_from('<I',exit,160)[0]&0x200
        exit[161]&=0xfd;assert (output/'native-exit.bin').read_bytes()==exit
        count=equal_banks(output/'native-after.bin',original/'hardware/hardware-after.bin')
        comparisons.append(dict(original=str(original.relative_to(base)),native=str(output.relative_to(base)),retained_bytes=count,instructions=r['original_instructions'],writes=r['retained_stores'],original_report_sha256=file_sha(original/'report.json'),hardware_report_sha256=file_sha(original/'hardware/report.json'),native_files_sha256={p.name:file_sha(p) for p in output.iterdir() if p.is_file()}))
    for name,count in [('particle-motion-current-native-regression-v1',11),('particle-draw-current-native-regression-v1',7),('particle-motion-api-native-v2',3)]:
        p=base/name;r=json.loads((p/'report.json').read_text());assert r['passed'] and len(r['cases'])==count
        for c in r['cases']:native(base/c['original'],p/c['original'])
    native(base/'particle-submitted-world-draw-original-v2',base/'particle-submitted-world-draw-native-v2')
    stop=base/'particle-drawn-stop-original-complete-v2';r=json.loads((stop/'report.json').read_text());assert r['passed'] and len(r['updates'])==35
    case=Path(r['case']);assert r['case_build_sha256']==file_sha(case/'build.json')
    assert r['lifetime_sha256']==file_sha(case/'particle-lifetime.json')
    for c in r['updates']:
        n=c['frame'];native(stop/f'frame{n}-original',stop/f'frame{n}-native')
        assert equal_banks(case/f'particle-frame{n}/after.bin',stop/f'frame{n}-original/hardware/hardware-after.bin')==59166806
    lifetime=json.loads((case/'particle-lifetime.json').read_text());assert lifetime['stages'][0]['live_particles']==44 and lifetime['stages'][-1]['salt']==0 and lifetime['stages'][-1]['source_tick']==77
    # One ordinary draw created four actual source callback records and stamps.
    read=reader(case/'world-draw/after.bin');u=lambda p:int.from_bytes(read(p,4),'little')
    assert u(0x4c2aa0)==4 and u(0x4c6854)==2;queue=u(0x4c2aa8);stamps=[]
    for n in range(4):
        p=queue+n*112;assert u(p)==3 and u(p+32)==0x176b30
        stamps.append(u(u(p+40)+0x30))
    assert stamps==[2]*4
    # Whole175F90 recomputes all active output fields from the retained camera.
    portal=base/'particle-portal-producer-original-v2';p=json.loads((portal/'report.json').read_text());h=json.loads((portal/'hardware/report.json').read_text());assert p['completed'] and h['emulator_retained_state_matches_hardware']
    assert not p['supplied_game_services'] and not p['instruction_patches']
    before=reader(portal/'before.bin');after=reader(portal/'hardware/hardware-after.bin');collector=p['portal_caller']['collector']
    groups=int.from_bytes(before(collector,2),'little');clusters=int.from_bytes(before(collector+0xa6c,2),'little');frustums=int.from_bytes(before(collector+0x1970,2),'little')
    ranges=[(0,2),(4,groups*444),(0xa6c,2),(0xa6e,clusters*26),(0x1770,clusters*4),(0x1970,2),(0x1974,frustums*264)]
    for offset,size in ranges:assert before(collector+offset,size)==after(collector+offset,size)
    assert (groups,clusters,frustums)==(1,4,4)
    generations=[]
    for name,source,count,instructions in [('particle-frame-native-functions-v10','source_effect_particle_frame_tu15_generated.cpp',53,2661),('particle-draw-native-functions-v8','source_effect_particle_draw_tu15_generated.cpp',18,1528)]:
        g=json.loads((base/name/'generation.json').read_text());assert len(g['complete_native_entries'])==count and len(g['lowered_instructions'])==instructions
        assert g['generated_cpp_sha256']==file_sha(source_file(ROOT, source))
        generations.append(dict(directory=name,functions=count,instructions=instructions,generation_sha256=file_sha(base/name/'generation.json'),source_sha256=g['generated_cpp_sha256']))
    components=json.loads((base/'particle-draw-root-components-v3/report.json').read_text());assert components['passed'] and not components['root_drift_after_compile']
    fault=json.loads((base/'particle-queued-draw-original-v2/report.json').read_text());assert not fault['completed'] and fault['exit_pc']==0x404bcd and fault['source_memory_faults'][0]['address']==8
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),native_owner_comparisons=comparisons,native_owner_comparison_count=len(comparisons),drawn_stop_report_sha256=file_sha(stop/'report.json'),drawn_stop_api_frames=35,additional_live_motion_api_frames=3,particles_before_stop=44,drained_at_particle_update=35,effect_retirement_tick=77,transparent_commands=4,location_draw_stamps=stamps,portal_producer=dict(groups=groups,clusters=clusters,frustums=frustums,active_fields_byte_identical=True,initialized_original_report_sha256=file_sha(portal/'report.json'),hardware_report_sha256=file_sha(portal/'hardware/report.json'),initialization_was_late_diagnostic=True),generations=generations,root_component_report_sha256=file_sha(base/'particle-draw-root-components-v3/report.json'),queued_callback_fault=dict(instruction=0x404bcd,address=8,original_report_sha256=file_sha(base/'particle-queued-draw-original-v2/report.json')),one_draw_submission_only=True,complete_camera_producer_verified=False,repeated_rendering_verified=False,renderer_integration_verified=False,installed_runtime_changed=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    assert len(comparisons)==57
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,native_owner_comparisons=57,drawn_stop_api_frames=35,particles_before_stop=44,portal_frustums_byte_identical=4)))

if __name__=='__main__':main()
