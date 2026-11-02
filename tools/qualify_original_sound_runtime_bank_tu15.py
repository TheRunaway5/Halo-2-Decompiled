#!/usr/bin/env python3
"""Qualify original native sound table/resource ownership in real MCC namespaces.

MCC supplies tag identities and native StringIDs only. Original banks supply
all sound behavior and payload data. Live publication, clocks and voices are
not constructed here. Run within tools/run_test.py; all bank/build data is owned.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from validate_mcc_weapon_vehicle_sound_resources import Bank,merge
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_mp_sound_promotion_constants import PROVIDERS,PROVIDERS_PIN

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def symbol(cache,value):
    index=value&0xffffff
    assert index<cache.u32(48)
    at=cache.u32(52)+cache.u32(cache.u32(60)+index*4)
    end=cache.data.find(b'\0',at);assert end>=at
    return cache.data[at:end].decode('utf-8')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('preparation','inventory','waveforms','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--native-prefix',action='store_true',help='Append original rows after complete actual MCC native tables')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work=owner/('runtime-qualification-'+args.output.name);work.mkdir(exist_ok=False);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,goal_complete=False,live_world_or_voice_binding_verified=False,retained_large_artifacts=[])
    pins={}
    try:
        prepared=json.loads(args.preparation.read_text());assert prepared['passed']and prepared['sound_banks']==31
        catalog=Path(prepared['catalog']);assert catalog.is_relative_to(owner)and sha(catalog)==prepared['catalog_sha256']
        pins={Path(p):digest for p,digest in prepared['inputs'].items()}
        for p in (args.preparation,args.inventory,args.waveforms,catalog,Path(__file__),ROOT/'tools/mcc_content.py',ROOT/'tools/validate_mcc_weapon_vehicle_sound_resources.py',PROVIDERS,NEW_DEFAULT):pins[p.resolve()]=sha(p)
        assert sha(PROVIDERS)==PROVIDERS_PIN and sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT);providers=json.loads(PROVIDERS.read_text());constants=[]
        for row in providers['words']:
            raw=image.read(int(row['new_va'],16),4);assert raw.hex()==row['new_hex'];constants.append(int.from_bytes(raw,'little'))
        inventory=json.loads(args.inventory.read_text());assert inventory['passed']and inventory['vehicle_paths']==37
        by_map={row['name']:row for row in inventory['maps']}
        by_original={}
        for case in json.loads(catalog.read_text())['cases']:
            path=Path(case['directory'])/'manifest.json';assert sha(path)==case['sha256'];pins[path]=case['sha256']
            by_original.setdefault(case['map'],[]).append(case)
        groups=[];shared=None;shared_path=None
        try:
            for map_name,cases in sorted(by_original.items()):
                record=by_map[map_name];path=Path(record['mcc_path']).resolve();assert sha(path)==record['mcc_sha256'];pins[path]=record['mcc_sha256']
                local=Bank(path)
                try:
                    needs=local.u32(local.index+12)!=0xffffffff and any(t['offset']is None for t in local.tags.values())
                    if needs:
                        wanted=path.parent/'shared.map'
                        if shared_path!=wanted:
                            if shared:shared.close()
                            shared=Bank(wanted);shared_path=wanted;pins[wanted]=sha(wanted)
                        tags=merge(local,shared)
                    else:tags={datum:(local,t)for datum,t in local.tags.items()}
                    directory=[];bindings=[];imports={};native_names={};table_cache={}
                    for case in cases:
                        candidates=[(bank,t)for bank,t in tags.values()if t['group']=='snd!'and t['name']==case['name']and t['offset']is not None]
                        assert len(candidates)==1,(map_name,case['name'],'ambiguous native tag owner')
                        bank,tag=candidates[0];s=tag['offset'];assert not bank.u16(s)&0x800
                        gestalts=[t for t in bank.tags.values()if t['group']=='ugh!'and t['offset']is not None];assert len(gestalts)==1
                        g=gestalts[0]['offset'];key=(str(bank.path),g)
                        if key not in table_cache:table_cache[key]=(bank.block(g+0x18,4),bank.block(g+0x28,12),bank.block(g+0x30,16))
                        names,ranges,permutations=table_cache[key]
                        def native_name(at):
                            value=bank.u32(names[bank.u16(at)]);name=symbol(bank,value)
                            assert symbol(local,value)==name,(map_name,'shared/local StringID namespace differs',name)
                            native_names[value]=name;return name,value
                        authored={}
                        for pitch in range(bank.data[s+10]):
                            r=ranges[bank.u16(s+8)+pitch];name,value=native_name(r);perms={}
                            for n in range(bank.u16(r+10)):
                                p=permutations[bank.u16(r+8)+n];pname,pvalue=native_name(p);assert pname not in perms;perms[pname]=pvalue
                            assert name not in authored;authored[name]=(value,perms)
                        m=json.loads((Path(case['directory'])/'manifest.json').read_text())
                        def bind(old,name,value):
                            row=dict(original=old,name=name,native=value)
                            assert old not in imports or imports[old]==row,(map_name,'conflicting native StringID resolution',name)
                            imports[old]=row
                        for pitch in m['original_contract']['pitch_ranges']:
                            value,perms=authored[pitch['name']];bind(pitch['import_name_id'],pitch['name'],value)
                            for p in pitch['permutations']:bind(p['import_name_id'],p['name'],perms[p['name']])
                        directory.append({k:tag[k]for k in ('group','name','datum','address','size')})
                        bindings.append(dict(sound=tag['datum'],case=case))
                    group=dict(map=map_name,tags=directory,bindings=bindings,strings=list(imports.values()))
                    if args.native_prefix:
                        data=bytearray();allocations=[];all_tags=[];owners={};source_gestalts={}
                        # Preserve actual serialized slot holes, including
                        # their literal rows; timing never accepts a compacted
                        # directory or invented full-salt placeholder.
                        slot_count=max(b.u32(b.index+24)for b in (local,shared)if b is not None and(b is local or needs))
                        by_slot={datum&65535:(source,t)for datum,(source,t)in tags.items()}
                        null_rows=[]
                        for slot in range(slot_count):
                            if slot in by_slot:continue
                            b=local if slot<local.u32(local.index+24)else shared
                            header_magic=b.u32(b.index)-b.index-32
                            table=b.pointer(b.u32(b.index+8),header_magic,b.u32(b.index+24)*16)
                            literal=b.data[table+slot*16:table+(slot+1)*16]
                            assert int.from_bytes(literal[4:8],'little')==0xffffffff
                            null_rows.append(dict(slot=slot,literal_hex=literal.hex()))
                        for source,t in sorted(tags.values(),key=lambda row:row[1]['address']):
                            all_tags.append({k:t[k]for k in ('group','name','datum','address','size')})
                            if t['offset']is None or t['group']not in ('snd!','ugh!','sncl','matg'):continue
                            if t['group']=='snd!':
                                key=str(source.path)
                                if key not in source_gestalts:
                                    gestalts=[g for g in source.tags.values()if g['group']=='ugh!'and g['offset']is not None];assert len(gestalts)==1
                                    source_gestalts[key]=gestalts[0]['datum']
                                owners[str(t['datum'])]=source_gestalts[key]
                            allocations.append(dict(address=t['address'],bytes=t['size'],offset=len(data)))
                            data.extend(source.data[t['offset']:t['offset']+t['size']])
                        for a,b in zip(allocations,allocations[1:]):assert a['address']+a['bytes']<=b['address'],(map_name,'overlapping literal sound owners')
                        path=work/('native-source-'+str(len(groups))+'.bin');path.write_bytes(data)
                        all_names={}
                        for b in (local,shared):
                            if b is None or(b is not local and not needs):continue
                            for g in b.tags.values():
                                if g['group']!='ugh!'or g['offset']is None:continue
                                for a in b.block(g['offset']+0x18,4):
                                    sid=b.u32(a);name=symbol(local,sid)
                                    assert sid not in all_names or all_names[sid]==name
                                    all_names[sid]=name
                        group['prefix_source']=dict(path=str(path),sha256=sha(path),allocations=allocations,tags=all_tags,owners=owners,
                            slot_count=slot_count,null_rows=null_rows,string_names={str(k):v for k,v in all_names.items()})
                    groups.append(group)
                finally:local.close()
        finally:
            if shared:shared.close()
        case=groups[0]['bindings'][0]['case'];unowned=work/'offline-bank-without-runtime-origin';unowned.mkdir()
        manifest=copy.deepcopy(json.loads((Path(case['directory'])/'manifest.json').read_text()))
        del manifest['original_contract']['sound_classes_hex'];del manifest['provenance']['map_sha256']
        (unowned/'manifest.json').write_text(json.dumps(manifest,separators=(',',':'))+'\n')
        os.link(Path(case['directory'])/'resources.bin',unowned/'resources.bin')
        groups[0]['unowned']=dict(directory=str(unowned),sha256=sha(unowned/'manifest.json'))
        plan=work/'runtime.json';plan.write_text(json.dumps(dict(groups=groups,waveforms=str(args.waveforms.resolve()),promotion_constants=constants),separators=(',',':'))+'\n')
        project=work/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for p in (project/'include').rglob('*'):
            if p.is_file():pins[ROOT/p.relative_to(project)]=sha(p)
        files=['src/assets/prepared_original_sound_bank.cpp','src/audio/original_sound_runtime_bank_tu15.cpp',
            'src/audio/audio.cpp','src/audio/mcc_audio.cpp','src/audio/vista_audio_framing.cpp','src/audio/vista_audio_decode.cpp',
            'src/audio/sound_variation.cpp','src/audio/sound_promotion.cpp','src/audio/source_sound_gain_envelope_tu15.cpp',
            'src/audio/sound_spatial.cpp','tests/original_sound_runtime_bank_tu15_tests.cpp','src/assets/cache.cpp']
        if args.native_prefix:files+=['src/audio/mcc_sound_runtime_delivery.cpp','src/audio/mcc_sound_tag_layout.cpp',
            'src/audio/mcc_sound_class_apply.cpp','src/audio/mcc_sound_class_reference.cpp',
            'src/audio/mcc_original_sound_runtime_delivery_tu15.cpp','src/audio/mcc_sound_timing_reference.cpp',
            'src/audio/mcc_sound_vanilla_reference.cpp','src/audio/mcc_sound_stored_time_reference.cpp',
            'src/audio/mcc_sound_permutation_time_reference.cpp','src/audio/mcc_sound_geometry.cpp',
            'src/assets/mcc/mcc_ordinary_pointer.cpp','src/tags/types.cpp']
        for name in files:
            p=ROOT/name;pins[p]=sha(p);shutil.copyfile(p,project/p.name);assert sha(project/p.name)==pins[p]
        # Freeze the private quoted-include closure required by these actual
        # production sources, including the finite class profile.
        pending=[ROOT/p for p in files];private=set()
        while pending:
            source=pending.pop()
            for name in re.findall(r'^#include "((?:audio|assets)/[^"\n]+)"',source.read_text(),re.M):
                p=ROOT/'src'/name
                if p in private:continue
                private.add(p);pending.append(p);pins[p]=sha(p)
                target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
        if args.native_prefix:
            for name in ('tools/generate_mcc_sound_class_reference.py','tools/data/mcc-sound-class-reference.json',
                         'tests/generate_mcc_sound_class_reference_tests.py','src/audio/mcc_sound_class_reference.inc',
                         'tests/mcc_sound_timing_reference_tests.cpp'):
                p=ROOT/name;pins[p]=sha(p);target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
            for p in private:
                target=project/p.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
        origins={str(p.relative_to(project)):str(p.relative_to(project)) for p in (project/'include').rglob('*')if p.is_file()}
        origins.update({Path(name).name:name for name in files})
        origins.update({str(p.relative_to(ROOT/'src')):str(p.relative_to(ROOT))for p in private})
        origins.update({str(p.relative_to(ROOT)):str(p.relative_to(ROOT))for p in private})
        if args.native_prefix:origins['tests/mcc_sound_timing_reference_tests.cpp']='tests/mcc_sound_timing_reference_tests.cpp'
        (project/'source-origins.json').write_text(json.dumps(origins,sort_keys=True)+'\n')
        # Use the actual CMake compiler arguments to identify the consumed
        # closure. Unused headers in the broad frozen include tree may change
        # concurrently without invalidating this smaller, measured cohort.
        (project/'dependencies.py').write_text('''import hashlib,json,shlex,subprocess,sys
from pathlib import Path
project=Path(sys.argv[1]).resolve();build=Path(sys.argv[2]).resolve();out=build/'evidence'
origins=json.loads((project/'source-origins.json').read_text());results={}
for n,row in enumerate(json.loads((build/'compile_commands.json').read_text())):
 command=row.get('arguments')or shlex.split(row['command']);args=[];i=0
 while i<len(command):
  if command[i]=='-o':i+=2;continue
  args.append('-MM'if command[i]=='-c'else command[i]);i+=1
 dependency=out/('native-'+str(n)+'.d')
 subprocess.run(args+['-MT','native_'+str(n),'-MF',str(dependency)],cwd=row['directory'],check=True)
 raw=dependency.read_text().replace('\\\\\\n','');deps={}
 for value in shlex.split(raw.split(':',1)[1]):
  p=Path(value);p=(Path(row['directory'])/p).resolve()if not p.is_absolute()else p.resolve()
  if p.is_relative_to(project):deps[origins[str(p.relative_to(project))]]=hashlib.sha256(p.read_bytes()).hexdigest()
 results[origins[str(Path(row['file']).resolve().relative_to(project))]]=deps
(out/'dependencies.json').write_text(json.dumps(dict(passed=True,dependencies=results),indent=2)+'\\n')
''')
        (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(OriginalNativeSoundBank LANGUAGES CXX)
include(CTest)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(nlohmann_json CONFIG REQUIRED)
pkg_check_modules(AUDIO REQUIRED IMPORTED_TARGET libavcodec libavformat libswresample libavutil opus sdl2)
add_library(runtime_core STATIC '''+' '.join(Path(p).name for p in files if not p.startswith('tests/'))+''')
target_compile_features(runtime_core PUBLIC cxx_std_20)
target_compile_definitions(runtime_core PUBLIC HALO2_CACHE_BYTES_ONLY '''+('HALO2_TEST_ORIGINAL_NATIVE_PREFIX=1'if args.native_prefix else'')+''')
target_compile_options(runtime_core PUBLIC -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)
target_link_options(runtime_core PUBLIC -Wl,--gc-sections)
target_include_directories(runtime_core PUBLIC include "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(runtime_core PUBLIC PkgConfig::AUDIO OpenSSL::Crypto nlohmann_json::nlohmann_json)
add_executable(runtime_bank original_sound_runtime_bank_tu15_tests.cpp)
target_link_libraries(runtime_bank PRIVATE runtime_core)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
add_custom_target(runtime_bank_dependencies
 COMMAND "'''+sys.executable+'''" "${CMAKE_SOURCE_DIR}/dependencies.py" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
add_dependencies(runtime_bank runtime_bank_dependencies)
add_test(NAME native_original_runtime_bank COMMAND "'''+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:runtime_bank> "'+str(plan)+'" "${CMAKE_BINARY_DIR}/evidence/runtime-bank.json")\nset_tests_properties(native_original_runtime_bank PROPERTIES TIMEOUT 300)\n'+
            ('add_test(NAME native_sound_class_profile_generation COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- "'+sys.executable+'" -B "${CMAKE_SOURCE_DIR}/tests/generate_mcc_sound_class_reference_tests.py")\n'
             'add_executable(timing_contract tests/mcc_sound_timing_reference_tests.cpp)\n'
             'target_link_libraries(timing_contract PRIVATE runtime_core)\n'
             'add_dependencies(runtime_bank timing_contract)\n'
             'add_test(NAME mcc_sound_timing_reference_contract COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:timing_contract>)\n'if args.native_prefix else''))
        results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(work/'builds'),
                '--output',str(args.output/compiler),'--target','runtime_bank','--filter','^(native_original_runtime_bank|native_sound_class_profile_generation|mcc_sound_timing_reference_contract)$',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                '--jobs','2','--test-jobs','1','--retain-report','evidence/runtime-bank.json','--retain-report','evidence/dependencies.json']
            p=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=p.returncode,command=command))
        report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,native_prefix=args.native_prefix,native_namespaces=len(groups),
            bindings=sum(len(g['bindings'])for g in groups),promotion_constants=constants,original_xbe_sha256=NEW_PIN,
            source_hashes={str(p.relative_to(ROOT)):v for p,v in pins.items()if p.is_relative_to(ROOT)and not p.is_relative_to(owner)})
    except BaseException as error:report['error']=str(error);raise
    finally:
        changed=[str(p)for p,v in pins.items()if not p.is_file()or sha(p)!=v];report['inputs_unchanged']=not changed;report['changed_inputs']=changed
        if changed:report['passed']=False
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
