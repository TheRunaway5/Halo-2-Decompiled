"""Test real E3 camera syntax with a private, authenticated MCC454 profile.

Production metadata and the selected engine remain unchanged. Compare complete
authored normal/low-spec/Playground banks before and after the private addition;
only the55+55 camera parent opcodes may change. This is translation/guard proof,
not native camera effects, rendered E3 frames, mission admission or gameplay.
Run through run_test.py; official run_tests.py owns each compiler tree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import struct
import subprocess
import sys


def digest(path):
    with path.open('rb')as file:return hashlib.file_digest(file,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation',type=Path,required=True)
    parser.add_argument('--sweep',type=Path,required=True)
    parser.add_argument('--compiler',action='append',choices=['g++','clang++'])
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[1];owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    preparation=json.loads(args.preparation.read_text());source=Path(preparation['source']).resolve(strict=True)
    assert preparation['sealed_inventory_verified']and not preparation['overlays']
    assert source.name=='source'and source.is_relative_to(root/'.local/test-scratch')
    sys.path.insert(0,str(root/'tools'))
    from build_launcher_engine import source_hashes
    from audit_workshop_compatibility import Pe64,xbox_catalog,TU_PIN
    from mcc_content import MccCache
    import steam_workshop as workshop
    sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    inventory=source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory,sort_keys=True).encode()).hexdigest()==preparation['production_inventory_sha256']
    selectors={name:(root/name).read_bytes()for name in ('gameplay-runtime-selection.json','frontend-runtime-selection.json')}
    runtime=root/preparation['selected_runtime'];assert digest(runtime)==preparation['runtime_sha256']
    pe=Pe64(Path.home()/'.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
    image=Xbe(Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    assert hashlib.sha256(image.data).hexdigest()==TU_PIN
    camera=pe.builtin(454);pending=pe.builtin(464)
    assert camera['name']=='camera_set_animation'and camera['return_type']==4 and camera['arguments']==[36,11]
    assert pending['name']=='camera_pan'
    original=[r for r in xbox_catalog(image)['camera_set_animation']
        if r['xbox_return_type']==camera['return_type']and r['xbox_arguments']==camera['arguments']]
    assert len(original)==1 and original[0]['xbox_opcode']==452 and original[0]['xbox_evaluator']=='0x2b27b0'
    profile=json.loads((source/'tools/mcc-script-layout.json').read_text())
    assert len(profile['builtins'])==411 and not any(r['mcc_opcode']in (454,464)for r in profile['builtins'])
    for row in profile['builtins']:
        actual=pe.builtin(row['mcc_opcode'])
        assert all(actual[k]==row[k]for k in ('name','return_type','arguments','mcc_descriptor','mcc_evaluator'))
    project=owner/'project';project.mkdir();paths=[
        'src/campaign/scripting/mcc_script_layout.cpp','src/campaign/scripting/mcc_script_layout.inc',
        'src/campaign/mcc_cairo_vanilla_scripts.inc','src/assets/cache.cpp',
        'src/campaign/scripting/native_mcc_script_builtin.cpp',
        'src/assets/cache_file_snapshot.cpp','src/runtime/source_runtime_snapshot_bytes.cpp',
        'cmake/TestStorage.cmake','cmake/SnapshotStorage.cmake','tools/run_test.py',
        'tools/generate_mcc_script_layout.py','tools/mcc-script-layout.json']
    paths += [p.relative_to(source).as_posix()for p in (source/'include').rglob('*')if p.is_file()]
    pins={name:digest(source/name)for name in paths}
    for name in paths:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source/name,target)
    test=root/'tests/workshop_camera_projection_tests.cpp';test_pin=digest(test)
    shutil.copyfile(test,project/'camera_projection.cpp')
    profile['builtins'].append(dict(**camera,**original[0]))
    generated=runpy.run_path(str(project/'tools/generate_mcc_script_layout.py'))['generated'](profile)
    private=project/'private/campaign/scripting/mcc_script_layout.inc';private.parent.mkdir(parents=True)
    private.write_text(generated)
    sweep=json.loads(args.sweep.read_text());mod=next(m for m in sweep['mods']if m['id']=='3360515088')
    packages=[directory/mod['id']for directory in workshop.roots()if(directory/mod['id']).is_dir()]
    assert len(packages)==1
    map_directory=workshop.source_owned(packages[0],'maps')
    maps=[];source_pins={};inputs=[]
    for entry in mod['maps']:
        path=map_directory/entry['map'];pin=digest(path)
        assert pin==entry['source']['sha256']and path.stat().st_size==entry['source']['bytes']
        source_pins[str(path)]=pin;cache=MccCache(path)
        try:
            scenario=cache.tags[cache.u32(cache.index+12)];base=scenario['offset']
            def bank(field,stride):
                count=cache.u32(base+field)
                if not count:return bytes()
                at=cache.pointer(cache.u32(base+field+4),cache.magic,count*stride)
                return bytes(cache.data[at:at+count*stride])
            nodes,strings,scripts=bank(0x238,20),bank(0x1b0,1),bank(0x1b8,0x28)
            def row(datum):
                index=datum&65535;assert index<len(nodes)//20
                result=struct.unpack_from('<4H3I',nodes,index*20)
                assert result[0]==datum>>16 and result[3]!=0xbaba;return result
            calls=[]
            for index,values in enumerate(struct.iter_unpack('<4H3I',nodes)):
                salt,opcode,_,flags,_,_,function=values
                if not salt or flags==0xbaba or flags&7 or opcode!=454:continue
                fn=row(function);assert fn[2]==2 and fn[3]&1
                end=strings.index(b'\0',fn[5]);assert strings[fn[5]:end].decode()=='camera_set_animation'
                arguments=[];next_=fn[4]
                while next_!=0xffffffff:
                    assert next_ not in arguments;arguments.append(next_);next_=row(next_)[4]
                assert len(arguments)==2 and [row(d)[2]for d in arguments]==[36,11]
                graph=cache.tags[row(arguments[0])[6]]
                assert graph['group']=='jmad'and graph['offset']is not None
                calls.append(dict(root=(salt<<16)|index,function=function,arguments=arguments,
                    graph_datum=row(arguments[0])[6],graph_name=graph['name'],animation_name_id=row(arguments[1])[6]))
            expected=0 if path.stem=='earthcity_playground'else 55;assert len(calls)==expected
            item=dict(name=path.name,calls=calls,requirements=entry['inspection']['script_requirements'])
            for key,data in [('nodes',nodes),('strings',strings),('scripts',scripts)]:
                target=project/(path.stem+'-'+key+'.bin');target.write_bytes(data);item[key]=str(target)
                inputs.append(dict(map=path.name,bank=key,bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
            maps.append(item)
        finally:cache.close()
    (project/'reference.json').write_text(json.dumps(dict(maps=maps,pending_opcode=pending['mcc_opcode']),indent=2)+'\n')
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(WorkshopCameraProjection LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include(cmake/TestStorage.cmake)
include(cmake/SnapshotStorage.cmake)
foreach(mode IN ITEMS before after)
 add_executable(camera_projection_${mode} camera_projection.cpp
  src/campaign/scripting/mcc_script_layout.cpp
  src/campaign/scripting/native_mcc_script_builtin.cpp src/assets/cache.cpp)
 target_include_directories(camera_projection_${mode} PRIVATE include src)
 target_compile_features(camera_projection_${mode} PRIVATE cxx_std_20)
 target_compile_definitions(camera_projection_${mode} PRIVATE HALO2_CACHE_BYTES_ONLY)
 target_compile_options(camera_projection_${mode} PRIVATE -ffunction-sections -fdata-sections)
 target_link_options(camera_projection_${mode} PRIVATE -Wl,--gc-sections)
 target_link_libraries(camera_projection_${mode} PRIVATE halo2_snapshot_storage nlohmann_json::nlohmann_json)
 if(mode STREQUAL "after")
  target_include_directories(camera_projection_${mode} BEFORE PRIVATE private)
  target_compile_definitions(camera_projection_${mode} PRIVATE HALO2_TEST_CAMERA_PROFILED=1)
 else()
  target_compile_definitions(camera_projection_${mode} PRIVATE HALO2_TEST_CAMERA_PROFILED=0)
 endif()
 if(HALO2_SANITIZERS)
  target_compile_options(camera_projection_${mode} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(camera_projection_${mode} PRIVATE -fsanitize=address,undefined)
 endif()
 add_test(NAME workshop_camera_projection_${mode} COMMAND camera_projection_${mode}
  "${CMAKE_CURRENT_SOURCE_DIR}/reference.json" "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()
set_tests_properties(workshop_camera_projection_after PROPERTIES DEPENDS workshop_camera_projection_before)
''')
    results=[]
    matrix=[(c,args.sanitizers)for c in args.compiler]if args.compiler else [('g++',False),('clang++',False),('clang++',True)]
    for compiler,sanitizers in matrix:
        name=compiler+('-sanitizers'if sanitizers else '')
        command=[sys.executable,'-B',str(root/'tools/run_tests.py'),'--source',str(project),
            '--scratch-root',str(owner/'builds'),'--output',str(args.output/name),
            '--target','camera_projection_before','--target','camera_projection_after',
            '--filter','^workshop_camera_projection_(before|after)$','--jobs','2','--test-jobs','1',
            '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log',
            '--retain-report','camera-before.json','--retain-report','camera-after.json']
        if sanitizers:command.append('--sanitizers')
        completed=subprocess.run(command);results.append(dict(compiler=compiler,sanitizers=sanitizers,exit_code=completed.returncode))
        if completed.returncode:break
    assert source_hashes(source)==inventory
    assert all(digest(source/name)==pin for name,pin in pins.items())and digest(test)==test_pin
    assert all(digest(Path(path))==pin for path,pin in source_pins.items())
    assert all((root/name).read_bytes()==data for name,data in selectors.items())
    assert digest(runtime)==preparation['runtime_sha256']
    report=dict(scope=__doc__,passed=all(r['exit_code']==0 for r in results),results=results,
        preparation=str(args.preparation),preparation_sha256=digest(args.preparation),
        source_inventory_sha256=preparation['production_inventory_sha256'],
        source_files=pins,test_sha256=test_pin,helper_sha256=digest(Path(__file__)),
        authenticated_mcc=camera,authenticated_xbox=original[0],pending_camera=pending,
        production_declarations=411,private_declarations=len(profile['builtins']),source_maps=source_pins,
        input_banks=inputs,authored_camera_calls=sum(len(m['calls'])for m in maps),
        production_admission_changed=False,selected_runtime_unchanged=True,
        owned_scratch=str(owner),retained_large_artifacts=[])
    args.output.mkdir(parents=True,exist_ok=True);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
