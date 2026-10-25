#!/usr/bin/env python3
"""Compare native vehicle selectors and retained literal controls with original CPU evidence."""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
KERNEL='src/gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp'
TEST='tests/source_vehicle_choice_native_tu15_tests.cpp'
ENTRIES=(0x15cb60,0x15cc70,0x15cdf0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--negative-control',action='store_true')
    parser.add_argument('--installed-routes',action='store_true')
    parser.add_argument('--include-flying',action='store_true')
    parser.add_argument('--negative-flying-control',action='store_true')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    original=Xbe(NEW_DEFAULT)
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original authority changed')
    reference=json.loads(args.reference.read_text());reference_path=Path(reference['required_original_reference']['path'])
    if not reference['passed'] or sha(reference_path)!=reference['required_original_reference']['sha256']:
        raise ValueError('Unchanged original selector evidence is required')
    fixture=json.loads(lzma.decompress(reference_path.read_bytes()))
    if fixture['original_sha256']!=NEW_PIN:raise ValueError('Original selector authority changed')
    entries=ENTRIES+(0x15cd30,) if args.include_flying else ENTRIES
    tables=((0x15cc50,7),(0x15cd1c,4),(0x15ced0,6))+(((0x15cddc,4),) if args.include_flying else ())
    if {row['entry'] for row in fixture['cases']}!=set(entries):raise ValueError('Reference families differ from declared native cohort')
    if args.negative_flying_control and (not args.include_flying or args.negative_control):
        raise ValueError('Use a separate four-family flying negative control')
    fixture['read_only_banks']=[[at,original.read(at,count*4).hex()]for at,count in tables]
    fixture_path=owner/'original.json';fixture_path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
    source=owner/'source';source.mkdir();pins={}
    paths=[*ROOT.joinpath('include').rglob('*'),*ROOT.joinpath('src').rglob('*.hpp'),ROOT/TEST]
    for path in paths:
        if not path.is_file() or path.suffix not in ('.hpp','.h','.def','.inc','.inl','.cpp'):continue
        name=path.relative_to(ROOT);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
        before=sha(path);shutil.copyfile(path,target)
        if sha(path)!=before or sha(target)!=before:raise ValueError('Source changed during freeze')
        pins[str(name)]=before
    module=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';module_pin=sha(module);text=module.read_text()
    if not args.installed_routes:
        from native_vehicle_choice_kernel_tu15 import strip_native_vehicle_choice
        text=strip_native_vehicle_choice(text)
    helpers='''#pragma once
namespace vehicle_choice_literal_routes {
using namespace halo2::physics_world_runtime_detail;
using halo2::SourcePhysicsContactImage;
inline std::uint32_t sign_extend(std::uint32_t v,unsigned n){return n==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))):throw std::runtime_error("Unqualified sign extension");}
inline std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Unqualified image");return p;}
'''
    literal=[];body_pins={}
    for entry in entries:
        anchor=f'static void run_{entry:x}(WorldState &s) {{\n';start=text.index(anchor);end=text.index('\n}\n\n',start)+2
        body=text[start:end];literal.append(body+'\n');body_pins[hex(entry)]=hashlib.sha256(body.encode()).hexdigest()
    helpers+=''.join(literal)+'inline void run(std::uint32_t entry,WorldState& s){switch(entry){'
    helpers+=''.join(f'case 0x{entry:x}u:run_{entry:x}(s);break;'for entry in entries)+'default:throw std::runtime_error("Unknown literal selector");}}\n}\n'
    (source/'tests/vehicle_choice_literal_routes.hpp').write_text(helpers)
    mutation=None
    if args.negative_control:
        target=source/KERNEL;before=target.read_text();needle='s.gp[2]=s.gp[0]*5u;'
        if before.count(needle)!=1:raise ValueError('Native Random mutation seam changed')
        target.write_text(before.replace(needle,'s.gp[2]=s.gp[0]*4u;'))
        mutation=dict(path=KERNEL,before=pins[KERNEL],after=sha(target));pins[KERNEL]=sha(target)
    elif args.negative_flying_control:
        target=source/KERNEL;before=target.read_text();needle='4,{0x15cd6e,0x15cd83},{4,4}};'
        if before.count(needle)!=1:raise ValueError('Native flying reference mutation seam changed')
        target.write_text(before.replace(needle,'4,{0x15cd6e,0x15cd83},{5,4}};'))
        mutation=dict(path=KERNEL,before=pins[KERNEL],after=sha(target),kind='wrong flying reference');pins[KERNEL]=sha(target)
    cmake='''cmake_minimum_required(VERSION 3.20)
project(VehicleChoiceNative LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(vehicle_choice_native tests/source_vehicle_choice_native_tu15_tests.cpp)
target_include_directories(vehicle_choice_native PRIVATE include src tests)
target_compile_features(vehicle_choice_native PRIVATE cxx_std_20)
target_compile_options(vehicle_choice_native PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(vehicle_choice_native PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+=f'add_test(NAME vehicle_choice_original COMMAND "${{Python3_EXECUTABLE}}" "{ROOT / "tools/run_test.py"}" -- $<TARGET_FILE:vehicle_choice_native> "{fixture_path}")\n'
    if args.installed_routes:cmake+='target_compile_definitions(vehicle_choice_native PRIVATE HALO2_VEHICLE_CHOICE_INSTALLED=1)\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','vehicle_choice_native','--filter','^vehicle_choice_original$','--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    validation=json.loads((args.output/'validation/report.json').read_text())
    log=(args.output/'validation/tests.log').read_text()if(args.output/'validation/tests.log').exists()else''
    rejected=bool(result and all(step['exit_code']==0 for step in validation['steps']if step['name']!='tests')and
        any(message in log for message in ('Vehicle-choice instruction sequence differs','Vehicle-choice ordered reads differ','Vehicle-choice ordered stores differ')))
    unchanged=all(sha(source/name)==pin for name,pin in pins.items())and sha(module)==module_pin
    passed=unchanged and(rejected if args.negative_control or args.negative_flying_control else result==0)
    report=dict(passed=passed,cases=len(fixture['cases']),comparisons=len(fixture['cases'])*2,
        retry_cases=sum(row['rng_draws']>1 for row in fixture['cases']),native_draws=sum(row['rng_draws']for row in fixture['cases']),
        source_unchanged=unchanged,source_hashes=pins,literal_module_sha256=module_pin,literal_bodies_sha256=body_pins,
        original_reference_sha256=sha(reference_path),original_sha256=NEW_PIN,native_kernel_sha256=pins[KERNEL],
        validation_returncode=result,negative_control=mutation,comparison_rejected_mutation=rejected,
        production_route_installed=args.installed_routes,native_random_match_admission_verified=False,full_vehicle_parity=False,
        families=sorted({row['family'] for row in fixture['cases']}),
        owner=str(owner),retained_large_artifacts=[],tool_sha256=sha(Path(__file__)))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,report=str(args.output/'report.json'))),flush=True)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
