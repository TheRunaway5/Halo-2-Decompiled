#!/usr/bin/env python3
"""Refresh the ordinary object-loop output owner in its active test cohort."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    base=ROOT/'analysis/combat-presentation-audit-20261022';follow=base/'repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins
    changes={}
    def save(rel,data):
        path=source/rel;before=path.read_bytes() if path.exists() else b''
        if before==data:return
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        changes[rel]=dict(before=sha(before) if before else None,after=sha(data))
    for rel in ('src/audio/multiplayer_audio.cpp','include/halo2/multiplayer_audio.hpp',
            'include/halo2/source_loop_sound_request_tu15.hpp','tests/object_loop_audio_runtime_tu15_tests.cpp',
            'tests/multiplayer_audio_continuation_tests.cpp','include/halo2/source_loop_detail_control_tu15.hpp',
            'src/audio/source_loop_detail_control_tu15.cpp','include/halo2/source_loop_phased_output_tu15.hpp',
            'src/audio/source_loop_phased_output_tu15.cpp'):
        data=(ROOT/rel).read_bytes()
        if rel=='src/audio/multiplayer_audio.cpp' and '--legacy-detail-gap' in sys.argv:
            anchor='    if(definition.flags&0xb2u)';assert data.count(anchor.encode())==1
            data=data.replace(anchor.encode(),b'    if(!definition.details.empty())throw FormatError("Object loop requires its detail owner");\n'+anchor.encode())
        save(rel,data)
    rel='include/halo2/physics_live_world_tu15.hpp';before=(source/rel).read_text()
    block='''// Actual220C60 call arguments and its full68-byte caller location, copied
// before the unchanged original call. arguments[4] is historical source
// backing, never a pointer for a native consumer to retain or dereference.
struct SourceLoopSoundRequestTu15 {
    std::array<std::uint32_t,8> arguments;
    std::array<std::byte,68> location;
};
'''
    include='#include "halo2/source_loop_sound_request_tu15.hpp"'
    if before.count(block)==1:
        before=before.replace(block,'')
        anchor='#include "halo2/source_object_audio_views_tu15.hpp"';assert before.count(anchor)==1
        save(rel,before.replace(anchor,anchor+'\n'+include).encode())
    else:assert before.count(include)==1 and 'struct SourceLoopSoundRequestTu15 {' not in before
    rel='src/physics/physics_live_world_tu15.cpp';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    if 'request.emitter_owner=word(m,loop+0x10u)' not in before:
        anchor='            m.read(request.arguments[4],request.location);'
        a=root.index(anchor)+len(anchor);b=root.index('\n            required(m,{c.target,c.target,false},a,o);',a)
        assert before.count(anchor)==1;save(rel,before.replace(anchor,anchor+root[a:b]).encode())
    rel='cmake/MultiplayerAudioContinuation.cmake';before=(source/rel).read_text()
    anchor='            src/audio/sound_fade.cpp';assert before.count(anchor)==1
    if '            src/audio/source_loop_track_control_tu15.cpp' not in before:
        before=before.replace(anchor,anchor+'\n            src/audio/source_loop_track_control_tu15.cpp')
    if '            src/audio/source_loop_detail_control_tu15.cpp' not in before:
        before=before.replace(anchor,anchor+'\n            src/audio/source_loop_detail_control_tu15.cpp')
    for module in ('source_loop_phased_output_tu15','source_loop_track_update_tu15','source_loop_track_transition_tu15'):
        line='            src/audio/'+module+'.cpp'
        if line not in before:before=before.replace(anchor,anchor+'\n'+line)
    save(rel,before.encode())
    rel='CMakeLists.txt';before=(source/rel).read_text()
    if ('    src/audio/source_loop_detail_control_tu15.cpp)' not in before and
            '    src/audio/source_loop_detail_control_tu15.cpp src/audio/source_loop_phased_output_tu15.cpp)' not in before):
        anchor='    src/audio/source_loop_track_control_tu15.cpp src/audio/source_loop_track_update_tu15.cpp)'
        assert before.count(anchor)==1
        before=before.replace(anchor,anchor[:-1]+'\n    src/audio/source_loop_detail_control_tu15.cpp)')
        anchor='set_source_files_properties(src/audio/source_loop_track_control_tu15.cpp'
        assert before.count(anchor)==2
        before=before.replace(anchor,anchor+' src/audio/source_loop_detail_control_tu15.cpp')
        save(rel,before.encode())
    before=(source/rel).read_text()
    if '    src/audio/source_loop_detail_control_tu15.cpp src/audio/source_loop_phased_output_tu15.cpp)' not in before:
        anchor='    src/audio/source_loop_detail_control_tu15.cpp)';assert before.count(anchor)==1
        before=before.replace(anchor,anchor[:-1]+' src/audio/source_loop_phased_output_tu15.cpp)')
        anchor='set_source_files_properties(src/audio/source_loop_track_control_tu15.cpp src/audio/source_loop_detail_control_tu15.cpp'
        assert before.count(anchor)==2;before=before.replace(anchor,anchor+' src/audio/source_loop_phased_output_tu15.cpp')
        save(rel,before.encode())
    rel='src/gameplay/multiplayer/multiplayer_runtime.cpp';before=(source/rel).read_text();root=(ROOT/rel).read_text()
    if 'audio->bind_object_loop_random_source' not in before:
        anchor='            live_world()->source_objects->tu15_world().sound_variation_provider(cache));'
        assert before.count(anchor)==1
        before=before.replace(anchor,anchor+'\n        audio->bind_object_loop_random_source([desktop=platform.desktop]{return desktop->fresh_secondary_random();});')
        save(rel,before.encode())
    if 'audio->object_loop_frame(audio_elapsed)' not in before:
        anchor='    ~SourceWeaponAudio(){if(world)world->observe_sound_requests({});}'
        start=root.index('    std::vector<SourceLoopSoundRequestTu15> loops;');end=root.index('\n  } source_weapon_audio;',start)
        assert before.count(anchor)==1;before=before.replace(anchor,root[start:end])
        anchor='    source_weapon_audio.world=live_world()->source_objects.get();';assert before.count(anchor)==1
        start=root.index(anchor)+len(anchor);end=root.index('\n    source_weapon_audio.world->observe_sound_requests',start)
        before=before.replace(anchor,anchor+root[start:end])
        anchor='    std::vector<ModelView> views;';assert before.count(anchor)==1
        before=before.replace(anchor,anchor+'\n    SourceObjectAudioViewsTu15 object_audio_views;')
        anchor='      camera.field_of_view=source_angles.projection_angle*57.295779513f;';assert before.count(anchor)==1
        start=root.index(anchor)+len(anchor);end=root.index('\n      if(audio && i==0)',start)
        before=before.replace(anchor,anchor+root[start:end])
        anchor='    timing.vehicle_skin_builds=vehicle_frame_poses.builds();';assert before.count(anchor)==1
        start=root.index('    if(audio && source_weapon_audio.world) {');end=root.index(anchor,start)
        before=before.replace(anchor,root[start:end]+anchor)
        save(rel,before.encode())
    else:
        anchor='    if(audio && source_weapon_audio.world) {';end_anchor='    timing.vehicle_skin_builds='
        assert before.count(anchor)==1 and root.count(anchor)==1
        a=before.index(anchor);b=before.index(end_anchor,a);c=root.index(anchor);d=root.index(end_anchor,c)
        save(rel,(before[:a]+root[c:d]+before[b:]).encode())
    updated=source_hashes(source)
    assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    admission_path=follow/'diagnostic-admission.json';admission=json.loads(admission_path.read_text())
    admission['build_completed']=False;admission['invalidated_by']=__doc__
    admission_path.write_text(json.dumps(admission,indent=2)+'\n')
    out=base/'repair-scenery-audio/object-loop-output-validation';attempt=2
    while out.exists():out=out.with_name('object-loop-output-validation-'+str(attempt));attempt+=1
    out.mkdir()
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    cmake.write_bytes(saved+b'''
add_custom_target(object_loop_output_validation DEPENDS multiplayer_audio_tests
    multiplayer_audio_continuation_tests object_loop_audio_runtime_tu15_tests)
''')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','object_loop_output_validation',
        '--filter','^(multiplayer_authored_audio|native_audio_core_owner_preflight|native_mcc_audio_core_enrollment)$',
        '--jobs','6','--test-jobs','1','--output',str(out)]
    try:
        with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    receipt=dict(scope=__doc__,changes=changes,command=command,exit_code=code,
        legacy_detail_gap_control='--legacy-detail-gap' in sys.argv,
        frozen_source_unchanged=source_hashes(source)==updated,native_source_result_supplied=False,
        launcher_gameplay_verified=False,complete_loop_controller_verified=False)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
    return code
if __name__=='__main__':raise SystemExit(main())
