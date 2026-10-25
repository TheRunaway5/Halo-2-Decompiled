#!/usr/bin/env python3
"""Render native MCC vehicle families using an authenticated current native build.

The ordinary compiled multiplayer object and libraries are borrowed read-only.
Only the capture entry point is rebuilt. Each native frame is explicit retained
visual reference evidence; raw frames, caches and compiler output are disposable.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
CASES={
    'ghost':('objects\\vehicles\\ghost\\ghost',76,3),
    'banshee':('objects\\vehicles\\banshee\\banshee',76,1),
    'scorpion':('objects\\vehicles\\scorpion\\scorpion',78,1),
    'wraith':('objects\\vehicles\\wraith\\wraith',78,2),
    'warthog-mg':('objects\\vehicles\\warthog\\warthog',76,1),
    'warthog-gauss':('objects\\vehicles\\warthog\\warthog',76,2),
    'spectre':('objects\\vehicles\\spectre\\spectre',76,5),
    'h_turret_ap':('objects\\vehicles\\h_turret_ap\\h_turret_ap',81,1),
    'c_turret_ap':('objects\\vehicles\\c_turret_ap\\c_turret_ap',81,4),
}


def worker(args):
    from PIL import Image
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    capture=owner/'capture';log=owner/'runtime.log';name,parameter,choice=CASES[args.case]
    command=[str(args.runtime),str(args.maps/'coagulation.map'),str(args.maps),'native-mcc',
             '960','540','1',str(capture),name]
    with private_display(owner) as display,log.open('w') as stream:
        environment={key:value for key,value in os.environ.items() if not key.startswith(('HALO2_REFERENCE_','HALO2_CAPTURE_'))}
        environment.update(DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
            XDG_CACHE_HOME=str(owner/'graphics-cache'),HALO2_PIPELINE_CACHE_DIRECTORY=str(args.graphics_cache),
            HALO2_CAPTURE_VEHICLE_VARIANT_PARAMETER=str(parameter),HALO2_CAPTURE_VEHICLE_VARIANT_CHOICE=str(choice))
        if args.camera_azimuth is not None:environment['HALO2_CAPTURE_VEHICLE_CAMERA_AZIMUTH']=str(args.camera_azimuth)
        if args.camera_distance is not None:environment['HALO2_CAPTURE_VEHICLE_CAMERA_DISTANCE']=str(args.camera_distance)
        try:code=subprocess.run(command,env=environment,stdout=stream,stderr=subprocess.STDOUT,timeout=240).returncode
        except subprocess.TimeoutExpired:code=124
    record=json.loads((capture/'report.json').read_text()) if (capture/'report.json').exists() else {}
    qualified=(code==0 and record.get('runtime_cache_source')=='MCC' and
        record.get('native_gameplay_xbe_argument_supplied') is False and
        record.get('udp_lighting_inputs_bit_exact') is True and record.get('source_owners',0)>0)
    report=dict(passed=qualified,case=args.case,exit_code=code,command=command,
        native_capture=record,original_Xbox_pixels_compared=False,visual_inspection_completed=False,
        full_vehicle_parity=False,log_tail=log.read_text(errors='replace')[-12000:])
    image=capture/'vehicle.ppm'
    if image.exists():
        args.image.parent.mkdir(parents=True,exist_ok=True)
        with Image.open(image) as frame:
            if frame.size!=(960,540):raise ValueError('Unexpected native capture viewport')
            frame.convert('RGB').save(args.image)
        if args.image.stat().st_size>1024*1024:raise ValueError('Required reference image exceeds its bounded budget')
        report['retained_reference_image']=dict(path=str(args.image),bytes=args.image.stat().st_size,sha256=sha(args.image))
    encoded=json.dumps(report,indent=2)+'\n'
    if len(encoded.encode())>1024*1024:raise ValueError('Native capture report exceeds its bounded budget')
    args.case_report.parent.mkdir(parents=True,exist_ok=True);args.case_report.write_text(encoded)
    print(json.dumps(dict(case=args.case,passed=report['passed'],report=str(args.case_report))),flush=True)
    return 0 if report['passed'] else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','output','runtime','maps','image','case-report','graphics-cache'):
        parser.add_argument('--'+name,type=Path)
    parser.add_argument('--case-worker',action='store_true')
    parser.add_argument('--case',choices=CASES)
    parser.add_argument('--only-case',action='append',choices=CASES)
    parser.add_argument('--camera-azimuth',type=float)
    parser.add_argument('--camera-distance',type=float)
    args=parser.parse_args()
    if args.case_worker:return worker(args)
    if not all((args.prepared,args.cohort,args.refresh,args.output)):parser.error('Supply the three source receipts and output')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());borrowed=Path(cohort['owner']);build=borrowed/'build';frozen=borrowed/'source'
    if owner==borrowed or Path(prepared['runner_scratch'])!=borrowed or Path(refresh['owner'])!=borrowed:
        raise ValueError('A separate authenticated current build owner is required')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for change in cohort['changes']:expected[change['path']]=change['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(frozen/name)!=pin for name,pin in expected.items()):raise ValueError('Borrowed frozen source drifted')
    source=owner/'source';source.mkdir();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    before=sha(ROOT/'tools/capture_vehicle_shading.cpp');shutil.copyfile(ROOT/'tools/capture_vehicle_shading.cpp',source/'capture.cpp')
    if sha(source/'capture.cpp')!=before or sha(ROOT/'tools/capture_vehicle_shading.cpp')!=before:
        raise ValueError('Capture entry changed while copying')
    link_file=build/'CMakeFiles/vehicle_weapon_hud_runtime_tests.dir/link.txt';link=shlex.split(link_file.read_text());at=link.index('-o')
    objects=[(build/name).resolve(strict=True) for name in link[:at] if name.endswith('multiplayer_runtime.cpp.o')]
    if len(objects)!=1:raise ValueError('One ordinary compiled multiplayer object is required')
    pins={str(objects[0]):sha(objects[0])};resolved=[str(objects[0])]
    native_link_file=build/'CMakeFiles/halo2_native.dir/link.txt'
    native_link=shlex.split(native_link_file.read_text())
    caches=[(build/name).resolve(strict=True) for name in native_link
            if name.endswith('native_vulkan_pipeline_cache.cpp.o')]
    wrappers=[name for name in native_link if name.startswith('-Wl,--wrap=vk')]
    if len(caches)!=1 or len(wrappers)!=4:raise ValueError('Ordinary native cache owner/link wrappers differ')
    pins[str(caches[0])]=sha(caches[0]);resolved.append(str(caches[0]));resolved.extend(wrappers)
    for name in link[at+2:]:
        if name.startswith('-'):resolved.append(name);continue
        path=(build/name).resolve(strict=True);pins[str(path)]=sha(path);resolved.append(str(path))
    q=lambda value:'"'+str(value).replace('\\','/').replace('"','\\"')+'"'
    maps=Path(prepared['runtime_cache_inputs']['maps_directory']);graphics=owner/'vulkan-cache';graphics.mkdir()
    cmake='''cmake_minimum_required(VERSION 3.20)
project(VehicleModelsNativeMcc LANGUAGES CXX)
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
add_executable(vehicle_models_mcc capture.cpp)
target_compile_features(vehicle_models_mcc PRIVATE cxx_std_20)
target_compile_definitions(vehicle_models_mcc PRIVATE HALO2_LIVE_VEHICLE_MODEL_CAPTURE=1)
target_compile_options(vehicle_models_mcc PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake+='target_include_directories(vehicle_models_mcc PRIVATE ${SDL2_INCLUDE_DIRS} '+q(frozen/'include')+' '+q(frozen/'src')+')\n'
    cmake+='target_link_libraries(vehicle_models_mcc PRIVATE "-Wl,--start-group" '+' '.join(map(q,resolved))+' "-Wl,--end-group")\n'
    chosen=args.only_case or list(CASES)
    camera_args=(' --camera-azimuth '+str(args.camera_azimuth) if args.camera_azimuth is not None else '')+(
        ' --camera-distance '+str(args.camera_distance) if args.camera_distance is not None else '')
    for case in chosen:
        cmake+='add_test(NAME vehicle_goal_model_'+case+' COMMAND "${Python3_EXECUTABLE}" '+q(ROOT/'tools/run_test.py')+' -- "${Python3_EXECUTABLE}" -B '+q(Path(__file__))+' --case-worker --runtime $<TARGET_FILE:vehicle_models_mcc> --maps '+q(maps)+' --case '+case+' --case-report '+q(args.output/'cases'/(case+'.json'))+' --image '+q(args.output/'images'/(case+'.png'))+' --graphics-cache '+q(graphics)+camera_args+')\n'
        cmake+='set_tests_properties(vehicle_goal_model_'+case+' PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE)\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--output',str(args.output/'validation'),'--target','vehicle_models_mcc','--filter','^vehicle_goal_model_',
        '--jobs','2','--test-jobs','1']
    receipt=args.output/'report.json'
    report=dict(passed=False,status='running',owner=str(owner),borrowed_owner=str(borrowed),
        borrowed_native_objects_and_libraries_sha256=pins,borrowed_link_command_sha256=sha(link_file),
        native_cache_link_command_sha256=sha(native_link_file),ordinary_native_cache_owner=True,
        capture_entry_sha256=before,source_receipts_sha256={str(p):sha(p) for p in (args.prepared,args.cohort,args.refresh)},
        command=command,full_vehicle_parity=False,original_Xbox_pixels_compared=False,
        selected_cases=chosen,camera_survey=dict(azimuth=args.camera_azimuth,distance=args.camera_distance),
        raw_frames_and_build_products_retained=False,required_reference_images_explicitly_retained=True,
        tool_sha256=sha(Path(__file__)))
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    unchanged=all(sha(Path(name))==pin for name,pin in pins.items())
    cases=[]
    for case in chosen:
        path=args.output/'cases'/(case+'.json');data=json.loads(path.read_text()) if path.exists() else {}
        cases.append(dict(case=case,passed=data.get('passed',False),report=str(path),
            report_sha256=sha(path) if path.exists() else None,reference_image=data.get('retained_reference_image')))
    report.update(passed=result.returncode==0 and unchanged and all(row['passed'] for row in cases),
        status='completed',validation_returncode=result.returncode,borrowed_inputs_unchanged=unchanged,cases=cases,
        visual_inspection_completed=False)
    receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases),report=str(receipt))),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
