#!/usr/bin/env python3
"""Validate original lens compatibility and the actual replica-presentation GPU owner."""
import json,os,shutil,subprocess,sys
from pathlib import Path
from build_launcher_engine import source_hashes
from validate_weapon_runtime_tu15 import private_display
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source';build=owner/'build'
    pins=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023/source-hashes.json').read_text());assert source_hashes(source)==pins
    for name in ('source_light_lens_tu15_tests.cpp','source_map_effect_lens_tu15_tests.cpp'):
        shutil.copyfile(ROOT/'tests'/name,source/'tests'/name)
    base=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens';out=base/'validation';n=2
    while out.exists():out=base/f'validation-{n}';n+=1
    out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','native_lens_presentation_tu15_tests','--target','lens_query_gpu_tests',
        '--target','lens_draw_gpu_tests','--target','campaign_lens_tu15_tests',
        '--target','lens_draw_handoff_tu15_tests',
        '--target','source_effect_replica_render_tu15_tests','--target','grenade_continuation_tests',
        '--target','source_light_lens_tu15_tests',
        '--target','source_map_effect_lens_tu15_tests',
        '--target','source_attached_effect_tests','--target','source_attached_effect_network_tests',
        '--target','multiplayer_tests',
        '--filter','^(source_native_lens_presentation_tu15|source_lens_query_gpu|original_tu15_lens_draw_gpu|source_campaign_lens_camera_tu15|original_tu15_lens_draw_handoff|original_tu15_effect_part_lens_requests|original_tu15_light_lens_requests|source_map_effect_lens_tu15|native_grenade_continuation|native_tu15_attached_effect_readback|native_tu15_attached_effect_udp|native_multiplayer)$',
        '--cmake-arg=-DHALO2_MCC_NATIVE_LENS_MAP='+str(ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/coagulation.map'),
        '--cmake-arg=-DHALO2_MCC_SCENERY_EFFECT_MAP='+str(ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/elongation.map'),
        '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE=/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',
        '--jobs','2','--test-jobs','1','--output',str(out/'cmake')]
    env=dict(os.environ,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',HALO2_NATIVE_GPU_SHADER_DIR=str(build/'shaders'),XDG_CACHE_HOME=str(owner/'graphics-cache'))
    layer=ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d';library=ROOT/'.tools/validation/usr/lib'
    assert (layer/'VkLayer_khronos_validation.json').is_file() and (library/'libVkLayer_khronos_validation.so').is_file()
    env['VK_LAYER_PATH']=str(layer);env['LD_LIBRARY_PATH']=str(library)+((':'+env['LD_LIBRARY_PATH']) if env.get('LD_LIBRARY_PATH') else '')
    with private_display(owner) as display:
        env['DISPLAY']=display
        with (out/'command.log').open('w') as log:code=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT).returncode
    report=build/'native-lens-presentation.json'
    if report.exists():shutil.copyfile(report,out/report.name)
    removed=[]
    for name in ['lens-query-gpu-validation','lens-draw-gpu-validation']:
        p=build/name
        if p.exists():
            if (p/'report.json').exists():shutil.copyfile(p/'report.json',out/(name+'.json'))
            shutil.rmtree(p);removed.append(dict(path=str(p),removed=not p.exists()))
    assert source_hashes(source)==pins
    (out/'receipt.json').write_text(json.dumps(dict(scope=__doc__,command=command,exit_code=code,generated_GPU_captures_removed=removed,source_unchanged=True,launcher_gameplay_verified=False),indent=2)+'\n')
    print(json.dumps(dict(exit_code=code,report=str(out/'receipt.json'))),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
