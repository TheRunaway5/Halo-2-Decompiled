#!/usr/bin/env python3
"""Qualify V3 native banks against original bytes and prior named PCM evidence.

All payloads, sources, malformed controls and CMake builds belong to the outer
run_test.py owner. No live tag/voice binder or original Xbox DSP is claimed.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('preparation','waveforms','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work=owner/('qualification-'+args.output.name);work.mkdir(exist_ok=False)
    args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,goal_complete=False,runtime_binding_verified=False,
                original_xbox_waveform_or_device_verified=False,retained_large_artifacts=[])
    tracked={}
    try:
        preparation=json.loads(args.preparation.read_text())
        assert preparation['passed'] and preparation['sound_banks']==31 and preparation['inputs_unchanged']
        catalog=Path(preparation['catalog']).resolve(strict=True)
        assert catalog.is_relative_to(owner) and sha(catalog)==preparation['catalog_sha256']
        cases=json.loads(catalog.read_text())['cases']
        assert len(cases)==preparation['contexts']
        tracked={Path(path):digest for path,digest in preparation['inputs'].items()}
        tracked.update({p.resolve():sha(p)for p in (args.preparation,args.waveforms,catalog,Path(__file__))})
        for row in cases:
            p=Path(row['directory'])/'manifest.json';assert sha(p)==row['sha256'];tracked[p]=row['sha256']
        payloads=catalog.parent/'payloads'
        for p in payloads.iterdir():tracked[p]=sha(p)
        manifests={row['directory']:json.loads((Path(row['directory'])/'manifest.json').read_text())for row in cases}
        glass=next(row for row in cases if row['name'].endswith('glass_breakable'))
        plain=next(row for row in cases if len(manifests[row['directory']]['pitch_ranges'])==1 and
            manifests[row['directory']]['original_contract']['promotion']is None and
            manifests[row['directory']]['original_contract']['pitch_ranges'][0]['used_flags_hex']is not None)
        absent=next(row for row in cases if manifests[row['directory']]['original_contract']['pitch_ranges'][0]['used_flags_hex']is None)
        used=next(row for row in cases if len(manifests[row['directory']]['pitch_ranges'][0]['permutations'])>1 and
            not bytes.fromhex(manifests[row['directory']]['original_contract']['sound_class_hex'])[10]&0x20)
        rejections=[];mutations=[]
        def variant(label,case,mutate,*,fidelity=False,tamper=False):
            directory=work/label;directory.mkdir()
            source=Path(case['directory']);manifest=copy.deepcopy(manifests[str(source)])
            if mutate:mutate(manifest)
            data=bytearray((source/'resources.bin').read_bytes())
            if tamper:
                data[0]^=1;manifest['encoded_file']['sha256']=hashlib.sha256(data).hexdigest()
            (directory/'resources.bin').write_bytes(data)
            path=directory/'manifest.json';path.write_text(json.dumps(manifest,indent=2)+'\n')
            row=dict(directory=str(directory),sha256=sha(path),name=label)
            if fidelity:
                row.update(reference=case['reference'],name=case['name'],map=case['map'])
                mutations.append(row)
            else:rejections.append(row)
        def literal(data,field,index,value):
            raw=bytearray.fromhex(data[field]);raw[index]=value;data[field]=raw.hex()
        def flip_literal(data,field,index):
            raw=bytearray.fromhex(data[field]);raw[index]^=1;data[field]=raw.hex()
        variant('unknown-schema',plain,lambda d:d.update(schema='unknown'))
        variant('missing-pitch',glass,lambda d:d['pitch_ranges'].pop())
        variant('root-pitch-count',glass,lambda d:literal(d,'literal_sound_hex',10,2))
        variant('missing-promotion',glass,lambda d:d['original_contract'].update(promotion=None))
        variant('invented-promotion',plain,lambda d:d['original_contract'].update(promotion=manifests[glass['directory']]['original_contract']['promotion']))
        variant('promotion-rule-count',glass,lambda d:d['original_contract']['promotion']['rules_hex'].pop())
        variant('promotion-counter-count',glass,lambda d:d['original_contract']['promotion']['counters_hex'].pop())
        # A structurally valid block with a rule selecting a nonexistent pitch.
        def wrong_rule(d):
            p=d['original_contract']['promotion'];r=bytearray.fromhex(p['rules_hex'][0]);r[0]=3;p['rules_hex'][0]=r.hex()
        variant('promotion-rule-pitch',glass,wrong_rule)
        variant('missing-required-used',used,lambda d:d['original_contract']['pitch_ranges'][0].update(used_flags_hex=None))
        variant('used-extent',used,lambda d:d['original_contract']['pitch_ranges'][0].update(used_flags_hex='00'*8))
        variant('absent-used-empty-string',absent,lambda d:d['original_contract']['pitch_ranges'][0].update(used_flags_hex=''))
        variant('literal-chunk-size',plain,lambda d:flip_literal(d['pitch_ranges'][0]['permutations'][0]['chunks'][0],'literal_hex',4))
        variant('sector-overflow',plain,lambda d:d['pitch_ranges'][0]['permutations'][0]['chunks'][0].update(sector_bytes=0xffffffff))
        variant('bank-chunk-offset',plain,lambda d:d['pitch_ranges'][0]['permutations'][0]['chunks'][0].update(offset=0xffffffff))
        variant('invalid-signed-range-index',plain,lambda d:literal(d['pitch_ranges'][0],'literal_hex',1,255))
        variant('import-id-mismatch',plain,lambda d:d['pitch_ranges'][0]['permutations'][0].update(import_name_id=d['pitch_ranges'][0]['permutations'][0]['import_name_id']^1))
        variant('encoded-tamper',plain,None,tamper=True)
        rejections.append(dict(name='manifest-pin',directory=plain['directory'],sha256='0'*64))
        variant('incomplete-original-class-owner',plain,lambda d:d['original_contract']['sound_classes_hex'].pop())
        variant('missing-original-class-identity',plain,lambda d:d['provenance'].pop('class_datum'))
        variant('none-original-identity',plain,lambda d:d['provenance'].update(sound_datum='ffffffff'))
        variant('malformed-original-identity',plain,lambda d:d['provenance'].update(gestalt_datum='invalid!'))
        variant('malformed-map-digest',plain,lambda d:d['provenance'].update(map_sha256='0'*63))
        variant('incomplete-original-class-row',plain,lambda d:d['original_contract']['sound_classes_hex'].__setitem__(0,'00'*91))
        def changed_selected_class(d):
            c=d['original_contract'];index=c['sound_class'];row=bytearray.fromhex(c['sound_classes_hex'][index]);row[0]^=1;c['sound_classes_hex'][index]=row.hex()
        variant('inconsistent-selected-original-class',plain,changed_selected_class)
        def swapped_rules(d):
            p=d['original_contract']['promotion'];p['rules_hex'][0],p['rules_hex'][1]=p['rules_hex'][1],p['rules_hex'][0]
        variant('changed-rule-order',glass,swapped_rules,fidelity=True)
        variant('changed-promotion-counter',glass,lambda d:d['original_contract']['promotion']['counters_hex'].__setitem__(0,'01000000'),fidelity=True)
        def changed_used(d):
            p=d['original_contract']['pitch_ranges'][0];raw=bytearray.fromhex(p['used_flags_hex']);raw[0]^=1;p['used_flags_hex']=raw.hex()
        variant('changed-initial-used-state',used,changed_used,fidelity=True)
        variant('changed-import-stringid',plain,lambda d:d['original_contract']['pitch_ranges'][0].update(import_name_id=d['original_contract']['pitch_ranges'][0]['import_name_id']^1),fidelity=True)
        variant('changed-literal-chunk-word8',plain,lambda d:flip_literal(d['pitch_ranges'][0]['permutations'][0]['chunks'][0],'literal_hex',8),fidelity=True)
        def changed_unused_class(d):
            c=d['original_contract'];index=(c['sound_class']+1)%54;row=bytearray.fromhex(c['sound_classes_hex'][index]);row[0]^=1;c['sound_classes_hex'][index]=row.hex()
        variant('changed-unused-original-class',plain,changed_unused_class,fidelity=True)
        variant('changed-original-map-digest',plain,lambda d:d['provenance'].update(map_sha256='0'*64),fidelity=True)
        legacy=[]
        for version in (1,2):
            directory=work/f'legacy-v{version}';directory.mkdir();m=copy.deepcopy(manifests[plain['directory']])
            m['schema']=f'H2_PREPARED_ORIGINAL_SOUND_BANK_V{version}'
            m['literal_pitch_range_hex']=m['pitch_ranges'][0]['literal_hex'];m['permutations']=m.pop('pitch_ranges')[0]['permutations']
            if version==1:m['original_contract']['pitch_ranges'][0].pop('used_flags_hex')
            (directory/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');os.link(Path(plain['directory'])/'resources.bin',directory/'resources.bin')
            legacy.append(dict(directory=str(directory),sha256=sha(directory/'manifest.json'),schema=version))
        plan=work/'controls.json';plan.write_text(json.dumps(dict(cases=cases,waveforms=str(args.waveforms.resolve()),rejections=rejections,fidelity_mutations=mutations,legacy=legacy),separators=(',',':'))+'\n')
        project=work/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for path in (project/'include').rglob('*'):
            if path.is_file():tracked[ROOT/path.relative_to(project)]=sha(path)
        files=['src/audio/audio.cpp','src/audio/mcc_audio.cpp','src/audio/vista_audio_framing.cpp',
               'src/audio/vista_audio_decode.cpp','src/assets/prepared_original_sound_bank.cpp',
               'tests/prepared_destruction_sound_bank_tu15_tests.cpp']
        for name in files:
            p=ROOT/name;tracked[p]=sha(p);shutil.copyfile(p,project/p.name);assert sha(project/p.name)==tracked[p]
        readers_path=ROOT/'src/assets/cache.cpp';tracked[readers_path]=sha(readers_path);text=readers_path.read_text()
        (project/'binary_readers.cpp').write_text(text)
        (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(OriginalDestructionBanks LANGUAGES CXX)
include(CTest)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(nlohmann_json CONFIG REQUIRED)
pkg_check_modules(AUDIO REQUIRED IMPORTED_TARGET libavcodec libavformat libswresample libavutil opus sdl2)
add_executable(original_banks audio.cpp mcc_audio.cpp vista_audio_framing.cpp vista_audio_decode.cpp prepared_original_sound_bank.cpp binary_readers.cpp prepared_destruction_sound_bank_tu15_tests.cpp)
target_compile_features(original_banks PRIVATE cxx_std_20)
target_compile_definitions(original_banks PRIVATE HALO2_CACHE_BYTES_ONLY)
target_compile_options(original_banks PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off)
target_link_options(original_banks PRIVATE -Wl,--gc-sections)
target_include_directories(original_banks PRIVATE include)
target_link_libraries(original_banks PRIVATE PkgConfig::AUDIO OpenSSL::Crypto nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
add_test(NAME native_original_destruction_banks COMMAND "'''+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:original_banks> "'+str(plan)+'" "${CMAKE_BINARY_DIR}/evidence/original-banks.json")\nset_tests_properties(native_original_destruction_banks PROPERTIES TIMEOUT 300)\n')
        results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(work/'builds'),'--output',str(args.output/compiler),'--target','original_banks',
                '--filter','^native_original_destruction_banks$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
                '--jobs','2','--test-jobs','1','--retain-report','evidence/original-banks.json']
            result=subprocess.run(command)
            results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
        report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,
            contexts=len(cases),malformed_controls=len(rejections),fidelity_controls=len(mutations),legacy_controls=len(legacy),
            preparation=str(args.preparation),preparation_sha256=sha(args.preparation),
            original_waveform_audit=str(args.waveforms),original_waveform_audit_sha256=sha(args.waveforms),
            source_hashes={str(p.relative_to(ROOT)):digest for p,digest in tracked.items()if p.is_relative_to(ROOT)and not p.is_relative_to(owner)},
            binary_reader_source_sha256=tracked[readers_path],binary_reader_compilation='Whole unchanged cache.cpp with its HALO2_CACHE_BYTES_ONLY owner')
    except BaseException as error:
        report['error']=str(error);raise
    finally:
        changed=[str(p)for p,digest in tracked.items()if not p.is_file()or sha(p)!=digest]
        report['inputs_unchanged']=not changed;report['changed_inputs']=changed
        if changed:report['passed']=False
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
