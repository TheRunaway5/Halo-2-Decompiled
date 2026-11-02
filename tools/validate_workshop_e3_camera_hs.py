"""Run actual E3 camera callers through native HS/effects and observer frames.

Borrow completed, immutable providers from the frozen engine build. Historical
private mode adds the authenticated MCC454->Xbox452 declaration only to test
metadata; --production-profile requires its already admitted production table.
The explicit camera-only decoded
Cache fixture preserves all55 authored camera calls while complete unmodified
E3 scripts retain their other22 requirements. This is not mission admission.
All preparations/compiler/world outputs use run_test.py owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import shlex
import shutil
import subprocess
import sys


def digest(path):
    with path.open('rb')as file:return hashlib.file_digest(file,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation',type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--borrow-completed-providers',action='store_true',help='Permit a live GPU baseline after its systems/physics targets complete; all borrowed providers stay pinned')
    parser.add_argument('--delivery-only',action='store_true',help='Check complete unchanged native metadata with the completed Cache library; no World/HS execution')
    parser.add_argument('--camera-fixture-delivery',action='store_true',help='Check camera-only decoded input delivery/restoration with the Cache library; no World/HS execution')
    parser.add_argument('--gpu',action='store_true',help='Check the actual E3 native observer/source BSP/Vulkan camera consumer as well as HS/frames')
    parser.add_argument('--production-profile',action='store_true',help='Require the admitted production camera declaration; do not generate a private override')
    parser.add_argument('--base-cache',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    if args.camera_fixture_delivery:args.delivery_only=True
    assert not(args.gpu and args.delivery_only)
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    completed_provider_lines=[];baseline_pin=None;baseline_status=None
    if not args.delivery_only:
        assert args.baseline is not None
        baseline=json.loads(args.baseline.read_text());baseline_pin=digest(args.baseline);baseline_status=baseline['status']
        if not baseline['passed']:
            assert args.borrow_completed_providers and baseline_status=='running','Native baseline/providers must finish before borrowing'
            journal=json.loads((args.preparation.parent/'session/session.json').read_text())
            os.kill(journal['pid'],0)
            cmdline=(Path('/proc')/str(journal['pid'])/'cmdline').read_bytes().split(b'\0')
            assert any(v.endswith(b'/tools/macos/test_session.py')for v in cmdline)
            assert journal['status']=='active'and journal['commands'][-1]['exit_code']is None
            assert '--build-worker'in journal['commands'][-1]['argv']
            log=(args.baseline.parent/'build.log').read_text()
            for target in ('halo2_physics_world_runtime','halo2_systems'):
                lines=[line for line in log.splitlines()if 'Built target '+target in line]
                assert lines,'Borrowed native provider is not complete: '+target
                completed_provider_lines.append(lines[-1])
    prep=json.loads(args.preparation.read_text());source=Path(prep['source']).resolve(strict=True);build=source.parent/'build'
    if not prep['overlays']:
        assert prep['sealed_inventory_verified']
    else:
        assert not prep['sealed_inventory_verified']and prep['base_sealed_inventory_verified']and prep['overlay_inventory_verified']
    assert source.name=='source'and source.is_relative_to(root/'.local/test-scratch')
    if not args.delivery_only:
        assert Path(baseline['build']).resolve(strict=True)==build
        assert not baseline['build_only']
        if baseline['passed']:assert not baseline['validation_not_run']
    sys.path.insert(0,str(source/'tools'));import steam_workshop as workshop
    from build_launcher_engine import source_hashes
    from audit_workshop_compatibility import Pe64,xbox_catalog,TU_PIN
    sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    inventory=source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory,sort_keys=True).encode()).hexdigest()==prep['production_inventory_sha256']
    source_refresh=None
    if prep['overlays']:
        path=Path(prep['source_refresh_receipt']).resolve(strict=True)
        assert digest(path)==prep['source_refresh_sha256']
        source_refresh=json.loads(path.read_text())
        assert source_refresh['passed']and Path(source_refresh['source'])==source and source_refresh['changes']==prep['overlays']
        assert source_refresh['production_inventory_sha256']==prep['production_inventory_sha256']
        base_path=Path(source_refresh['base_preparation']).resolve(strict=True)
        assert digest(base_path)==source_refresh['base_preparation_sha256']
        base=json.loads(base_path.read_text())
        assert base['sealed_inventory_verified']and not base['overlays']and base['source']==str(source)
        assert base['production_inventory_sha256']==source_refresh['production_inventory_before']==prep['base_production_inventory_sha256']
        restored=dict(inventory)
        for row in source_refresh['changes']:
            assert digest(source/row['path'])==row['sha256']
            if row['production']:
                if row['before']is None:restored.pop(row['path'])
                else:restored[row['path']]=row['before']
        expected=json.loads((root/base['selected_runtime']).parent.joinpath('source-hashes.json').read_text())
        assert restored==expected and hashlib.sha256(json.dumps(restored,sort_keys=True).encode()).hexdigest()==base['production_inventory_sha256']
        component=Path(source_refresh['component']);assert digest(component)==source_refresh['component_sha256']
        assert json.loads(component.read_text())['passed']
    selected={name:(root/name).read_bytes()for name in ('gameplay-runtime-selection.json','frontend-runtime-selection.json')}
    runtime=root/prep['selected_runtime'];assert digest(runtime)==prep['runtime_sha256']
    pe=Pe64(Path.home()/'.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    image=Xbe(Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()==TU_PIN
    camera=pe.builtin(454);original=[r for r in xbox_catalog(image)['camera_set_animation']
        if r['xbox_return_type']==camera['return_type']and r['xbox_arguments']==camera['arguments']]
    assert len(original)==1 and original[0]['xbox_opcode']==452 and original[0]['xbox_evaluator']=='0x2b27b0'
    assert camera['name']=='camera_set_animation'and camera['return_type']==4 and camera['arguments']==[36,11]
    profile=json.loads((source/'tools/mcc-script-layout.json').read_text())
    project=owner/'project';project.mkdir();private=project/'private/campaign/scripting/mcc_script_layout.inc'
    if args.production_profile:
        admitted=[r for r in profile['builtins']if r['mcc_opcode']==454]
        assert len(profile['builtins'])==412 and admitted==[dict(**camera,**original[0])]
        assert not private.exists()
    else:
        assert len(profile['builtins'])==411 and not any(r['mcc_opcode']==454 for r in profile['builtins'])
        profile['builtins'].append(dict(**camera,**original[0]))
        generated=runpy.run_path(str(source/'tools/generate_mcc_script_layout.py'))['generated'](profile)
        private.parent.mkdir(parents=True);private.write_text(generated)
    test=root/'tests/native_workshop_e3_camera_hs_tests.cpp';test_pin=digest(test);shutil.copyfile(test,project/'camera_hs.cpp')
    extra_pins={};shader_pins={}
    if args.gpu:
        assert baseline['passed']
        for name in ('tests/native_workshop_e3_camera_gpu.cpp','tests/native_workshop_e3_camera_gpu.hpp'):
            extra_pins[name]=digest(root/name);shutil.copyfile(root/name,project/Path(name).name)
        (project/'.tools').symlink_to(source/'.tools',target_is_directory=True)
        shader_pins={str(path):digest(path)for path in (build/'shaders').glob('*.spv')};assert shader_pins
    (project/'tools').symlink_to(source/'tools',target_is_directory=True)
    layout=source/'src/campaign/scripting/mcc_script_layout.cpp';layout_pin=digest(layout)
    link=build/('CMakeFiles/workshop_mod_probe.dir/link.txt'if args.delivery_only else 'CMakeFiles/native_mcc_scenery_animation_gpu_tests.dir/link.txt'if args.gpu else 'CMakeFiles/native_mcc_scenery_animation_world_tests.dir/link.txt');link_pin=digest(link)
    words=shlex.split(link.read_text());words=words[words.index('-o')+2:];libraries={};links=[]
    for word in words:
        if word.endswith('.a')or('.so'in word and not word.startswith('-')):
            path=Path(word);path=path if path.is_absolute()else build/path;path=path.resolve(strict=True)
            libraries[str(path)]=digest(path);links.append(str(path))
        else:assert word.startswith('-');links.append(word)
    if args.delivery_only:assert str(build/'libhalo2_cache.a')in libraries
    else:assert str(build/'libhalo2_systems.a')in libraries and str(build/'libhalo2_physics_world_runtime.a')in libraries
    if args.gpu:assert str(build/'libhalo2_renderer.a')in libraries
    catalog=workshop.scan();mod=next(m for m in catalog['mods']if m['id']=='3360515088')
    assert not catalog['errors']and not mod['error'];item=Path(mod['directory']);cases=[]
    for name in ('earthcity_e3','earthcity_e3_potato'):
        path=workshop.source_owned(item,'maps/'+name+'.map');pin=workshop.pin(path)
        prepared=workshop.prepare_cached(item,name,args.base_cache,owner/(name+'-prepared'))
        assert workshop.pin(path)==pin
        cases.append(dict(name=name,source=str(path),source_pin=pin,prepared=prepared))
    cmake='''cmake_minimum_required(VERSION 3.25)
project(WorkshopE3CameraHs LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
include('''+json.dumps(str(source/'cmake/TestStorage.cmake'))+''')
add_executable(camera_hs camera_hs.cpp '''+json.dumps(str(layout))+''')
target_compile_features(camera_hs PRIVATE cxx_std_20)
target_compile_options(camera_hs PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_include_directories(camera_hs PRIVATE private '''+json.dumps(str(source/'include'))+' '+json.dumps(str(source/'src'))+''')
target_link_libraries(camera_hs PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2
'''+''.join(' '+json.dumps(word)+'\n'for word in links)+')\n'
    if args.gpu:
        cmake+='target_sources(camera_hs PRIVATE native_workshop_e3_camera_gpu.cpp)\n'
        cmake+='target_compile_definitions(camera_hs PRIVATE HALO2_E3_CAMERA_GPU=1)\n'
        cmake+='find_program(XVFB_RUN xvfb-run REQUIRED)\n'
    if args.delivery_only:
        cmake+='target_compile_definitions(camera_hs PRIVATE HALO2_E3_CAMERA_DELIVERY_ONLY=1)\n'
        if args.camera_fixture_delivery:cmake+='target_compile_definitions(camera_hs PRIVATE HALO2_E3_CAMERA_FIXTURE_DELIVERY_ONLY=1)\n'
        cmake+='target_compile_options(camera_hs PRIVATE -ffunction-sections -fdata-sections)\n'
        cmake+='target_link_options(camera_hs PRIVATE -Wl,--gc-sections)\n'
    prefix='native_e3_camera_fixture_delivery_'if args.camera_fixture_delivery else 'native_e3_camera_delivery_'if args.delivery_only else 'native_e3_camera_gpu_'if args.gpu else 'native_e3_camera_'
    for case in cases:
        p=case['prepared'];argv=[p['map'],p['shared_maps'],p['prepared_root']]
        executable='${XVFB_RUN} -a $<TARGET_FILE:camera_hs>'if args.gpu else 'camera_hs'
        cmake+='add_test(NAME '+prefix+case['name']+' COMMAND '+executable+' '+ ' '.join(json.dumps(str(v))for v in argv)+' "${CMAKE_CURRENT_BINARY_DIR}/'+case['name']+'.json")\n'
        cmake+='set_tests_properties('+prefix+case['name']+' PROPERTIES TIMEOUT 300)\n'
        if args.gpu:cmake+='set_tests_properties('+prefix+case['name']+' PROPERTIES ENVIRONMENT '+json.dumps('SDL_VIDEODRIVER=x11;HALO2_NATIVE_GPU_SHADER_DIR='+str(build/'shaders'))+')\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(root/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
        '--output',str(args.output),'--target','camera_hs','--filter','^'+prefix,
        '--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
    for case in cases:command+=['--retain-report',case['name']+'.json']
    result=subprocess.run(command)
    evidence=[]
    for case in cases:
        path=args.output/'evidence'/(case['name']+'.json')
        if path.is_file():evidence.append(json.loads(path.read_text()))
    accepted=result.returncode==0 and len(evidence)==len(cases)and all(row['passed']for row in evidence)
    if accepted and args.delivery_only:
        accepted=all(not row['native_HS_executed']and row['camera_only_fixture_used']==args.camera_fixture_delivery and row['issue_count']==0 for row in evidence)
        if args.camera_fixture_delivery:
            accepted=accepted and all(row['decoded_scenario_restored']and row['fixture_authored_camera_calls']==55 and row['complete_script_requirements_retained']==22 for row in evidence)
    elif accepted:
        accepted=all(row['native_HS_executed']and row['decoded_scenario_restored']and row['HS_calls']==110 and
            row['frames']==330 and len(row['calls'])==55 for row in evidence)
        if args.gpu:accepted=accepted and all(row['GPU']['passed']and row['GPU']['camera_control_pairs']==55 and row['GPU']['raw_captures_removed']for row in evidence)
    assert digest(test)==test_pin and digest(layout)==layout_pin and digest(link)==link_pin and source_hashes(source)==inventory
    assert all(digest(Path(path))==pin for path,pin in libraries.items())
    assert all(digest(root/name)==pin for name,pin in extra_pins.items())
    assert all(digest(Path(path))==pin for path,pin in shader_pins.items())
    assert all(workshop.pin(Path(case['source']))==case['source_pin']for case in cases)
    assert all((root/name).read_bytes()==data for name,data in selected.items())and digest(runtime)==prep['runtime_sha256']
    for case in cases:case.pop('prepared')
    report=dict(scope=__doc__,passed=accepted,exit_code=result.returncode,
        preparation=str(args.preparation),preparation_sha256=digest(args.preparation),
        baseline=str(args.baseline)if args.baseline else None,baseline_sha256=digest(args.baseline)if args.baseline else None,
        baseline_sha256_at_start=baseline_pin,baseline_status_at_start=baseline_status,completed_provider_lines=completed_provider_lines,
        source_inventory_sha256=prep['production_inventory_sha256'],test_sha256=test_pin,layout_sha256=layout_pin,
        declared_source_refresh=prep.get('source_refresh_receipt'),source_overlay_inventory_verified=bool(source_refresh),
        helper_sha256=digest(Path(__file__)),libraries=libraries,link_template_sha256=link_pin,cases=cases,
        GPU_requested=args.gpu,GPU_test_sources=extra_pins,GPU_shaders=shader_pins,
        authenticated_mcc=camera,authenticated_xbox=original[0],production_camera_admission_changed=False,
        production_profile_used=args.production_profile,test_camera_declaration_added=not args.production_profile,
        delivery_only=args.delivery_only,camera_fixture_delivery=args.camera_fixture_delivery,native_HS_requested=not args.delivery_only,
        native_HS_completed_cases=sum(bool(row.get('native_HS_executed'))for row in evidence),
        selected_runtime_unchanged=True,owned_scratch=str(owner),retained_large_artifacts=[])
    args.output.mkdir(parents=True,exist_ok=True);(args.output/'consumer.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
