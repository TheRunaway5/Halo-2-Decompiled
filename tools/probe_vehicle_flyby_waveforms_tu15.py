#!/usr/bin/env python3
"""Compare native-decoded original AP flybys with the nearby MCC Classic sound.

Run through tools/run_test.py. All encoded copies, PCM, compiler outputs and
frozen sources remain in HALO2_TEST_SCRATCH. Only bounded hashes/metrics remain.
This does not alias definitions, import resources, start voices or certify DSP.
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

import numpy as np
from mcc_content import MccCache

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        while block:=stream.read(1024*1024):h.update(block)
    return h.hexdigest()


def require(value,message):
    if not value:raise ValueError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--maps',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--prepared-original-bank',type=Path)
    parser.add_argument('--native-reference-report',type=Path)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=True)
    source=owner/'source';source.mkdir()
    shutil.copytree(ROOT/'include',source/'include')
    header_pins={str(p.relative_to(source)):sha(p) for p in (source/'include').rglob('*') if p.is_file()}
    require(all(sha(ROOT/name)==pin for name,pin in header_pins.items()),'Header source changed while freezing')
    files=['src/audio/audio.cpp','src/audio/mcc_audio.cpp','src/audio/vista_audio_framing.cpp',
           'src/audio/vista_audio_decode.cpp','src/assets/prepared_original_sound_bank.cpp',
           'tools/probe_vehicle_flyby_decode_tu15.cpp','tests/prepared_original_sound_bank_tests.cpp']
    pins={}
    for name in files:
        pins[name]=sha(ROOT/name);target=source/Path(name).name
        shutil.copyfile(ROOT/name,target)
        require(sha(target)==pins[name],'Decoder source changed while freezing')
    # Use the actual production binary readers without linking a live Cache.
    # This is an owned unchanged-body compilation slice, not a replacement.
    cache_cpp=ROOT/'src/assets/cache.cpp';cache_cpp_pin=sha(cache_cpp);cache_text=cache_cpp.read_text()
    begin=cache_text.index('void Bytes::require(');end=cache_text.index('std::span<const std::byte> Cache::cache_bytes()',begin)
    readers=cache_text[begin:end]
    (source/'binary_readers.cpp').write_text('#include "halo2/cache.hpp"\n#include <bit>\n#include <algorithm>\nnamespace halo2 {\n'+readers+'}\n')
    reference=json.loads(args.reference.read_text())
    prepared_bank=None;prepared_pins={}
    if args.prepared_original_bank:
        manifest=args.prepared_original_bank/'manifest.json';resource=args.prepared_original_bank/'resources.bin'
        prepared_bank=json.loads(manifest.read_text())
        require(prepared_bank['schema'] in ('H2_PREPARED_ORIGINAL_SOUND_BANK_V1','H2_PREPARED_ORIGINAL_SOUND_BANK_V2') and
                prepared_bank['name']==reference['name'] and prepared_bank['encoded_file']['path']=='resources.bin' and
                prepared_bank['original_contract']['codec']==reference['codec'] and
                prepared_bank['original_contract']['flags']==int.from_bytes(bytes.fromhex(reference['definition_hex'])[:2],'little'),
                'Prepared original sound metadata differs')
        require(resource.stat().st_size==prepared_bank['encoded_file']['bytes'] and
                sha(resource)==prepared_bank['encoded_file']['sha256'],'Prepared original bank integrity differs')
        require(len(prepared_bank['permutations'])==6,'Prepared bank omitted an original permutation')
        prepared_pins={str(manifest):sha(manifest),str(resource):sha(resource)}
    else:
        require(sha(ROOT/reference['reference_path'])==reference['sha256'],'Original map hash differs')
        for name,pin in reference['resource_files'].items():
            require(sha(ROOT/name)==pin,'Original resource hash differs')
    cases=[]
    for row in reference['encoded_permutations']:
        data=bytearray()
        parts=row['parts']
        if prepared_bank:
            permutation=prepared_bank['permutations'][row['permutation']]
            require(permutation['name']==row['import_name'] and permutation['encoded_sha256']==row['encoded_sha256'],
                    'Prepared permutation identity differs from original authority')
            parts=[dict(source=str(resource),offset=c['offset'],bytes=c['bytes'],sha256=c['encoded_sha256'])
                   for c in permutation['chunks']]
        for part in parts:
            with (ROOT/part['source']).open('rb') as stream:
                stream.seek(part['offset']);raw=stream.read(part['bytes'])
            require(len(raw)==part['bytes'] and hashlib.sha256(raw).hexdigest()==part['sha256'],
                    'Original encoded extent differs')
            data.extend(raw)
        require(hashlib.sha256(data).hexdigest()==row['encoded_sha256'],'Original permutation differs')
        cases.append(dict(namespace='original',name=row['import_name'],permutation=row['permutation'],
                          rate=44100,channels=1,codec=1,samples=0,sound_class=2,chunks=[],data=data))
    cache_path=args.maps/'shared.map';cache_pin=sha(cache_path)
    cache=MccCache(cache_path)
    selected=[t for t in cache.tags.values() if t['group']=='snd!' and
              t['name']==r'sound\vehicles\phantom\phantom_turr_flyby']
    require(len(selected)==1,'Missing exact nearby Classic sound identity')
    tag=selected[0];s=tag['offset'];require(s is not None,'Nearby sound is not resident')
    require(cache.u16(s)==0x7402 and bytes(cache.data[s:s+28]).hex()==
            '02740206f1012e00dc060100ffffffff2404000000000000ffff00ff','Nearby sound complete header differs')
    gestalts=[t for t in cache.tags.values() if t['group']=='ugh!' and t['offset'] is not None]
    require(len(gestalts)==1,'Shared sound has ambiguous gestalt ownership');g=gestalts[0]['offset']
    codec=cache.block(g,3)[cache.data[s+3]]
    require(bytes(cache.data[codec:codec+3])==bytes([3,0,5]),'Nearby codec differs')
    ranges=cache.block(g+0x28,12);r=ranges[cache.u16(s+8)]
    require(cache.data[s+10]==1 and cache.u16(r+10)==6,'Nearby pitch/permutation count differs')
    permutations=cache.block(g+0x30,16);language=cache.block(g+0x60,8)[0]
    chunks=cache.block(language,16)
    bank=args.maps/'sounds_neutral.dat';bank_pin=sha(bank)
    for n in range(6):
        p=permutations[cache.u16(r+8)+n];info=cache.block(p+8,8)[0]
        first,count=cache.u16(info+4),cache.u16(info+6)
        raw=bytearray();descriptors=[]
        for c in chunks[first:first+count]:
            flags=cache.u32(c+4);size=flags&0x1fffffff
            with bank.open('rb') as stream:stream.seek(cache.u32(c));part=stream.read(size)
            require(size and len(part)==size,'Nearby resource extent differs')
            raw.extend(part);descriptors.append([size,cache.u32(c+8),int(bool(flags&0x20000000))])
        cases.append(dict(namespace='mcc',name=tag['name'],permutation=n,rate=48000,channels=1,
                          codec=5,samples=cache.u32(info)//2,sound_class=2,chunks=descriptors,data=raw))
    cache.close()
    rows=[]
    for n,c in enumerate(cases):
        raw=c.pop('data');encoded=owner/f'encoded-{n}.bin';pcm=owner/f'pcm-{n}.f32';encoded.write_bytes(raw)
        c.update(encoded_bytes=len(raw),encoded_sha256=hashlib.sha256(raw).hexdigest(),pcm_path=str(pcm))
        rows.append(f'{encoded} {pcm} '+ ' '.join(str(c[k]) for k in ('rate','channels','codec','samples','sound_class'))+
                    ' '+str(len(c['chunks']))+' '+ ' '.join(str(v) for chunk in c['chunks'] for v in chunk))
    manifest=owner/'inputs.txt';manifest.write_text('\n'.join(rows)+'\n')
    controls=None;expected_refusals=0
    if prepared_bank:
        source_resource=args.prepared_original_bank/'resources.bin';rejections=[]
        alterations=[
            ('resource-path',lambda d:d['encoded_file'].update(path='../resources.bin')),
            ('sector-overflow',lambda d:d['permutations'][0]['chunks'][0].update(sector_bytes=0xffffffff)),
            ('chunk-offset-overflow',lambda d:d['permutations'][0]['chunks'][0].update(offset=0xffffffff)),
            ('original-flags',lambda d:d['permutations'][0]['chunks'][0].update(source_flags=4)),
            ('range-count',lambda d:d.update(literal_pitch_range_hex=d['literal_pitch_range_hex'][:20]+'0700')),
            ('promotion-owner',lambda d:d.update(literal_sound_hex=d['literal_sound_hex'][:24]+'00'+d['literal_sound_hex'][26:])),
            ('empty-interval',lambda d:d['original_contract'].update(pitch_ranges=[])),
            ('payload-tamper',None),('unowned-trailing-sector',None)]
        initial_flags=prepared_bank['schema']=='H2_PREPARED_ORIGINAL_SOUND_BANK_V2'
        if initial_flags:
            alterations.extend([
                ('missing-initial-flags',lambda d:d['original_contract']['pitch_ranges'][0].pop('used_flags_hex')),
                ('initial-flags-extent',lambda d:d['original_contract']['pitch_ranges'][0].update(used_flags_hex='0000'))])
        for n,(label,mutate) in enumerate(alterations):
            directory=owner/('bad-bank-'+str(n));directory.mkdir();data=bytearray(source_resource.read_bytes())
            metadata=copy.deepcopy(prepared_bank)
            if mutate:mutate(metadata)
            elif label=='payload-tamper':data[0]^=1
            else:data.extend(bytes(512))
            if not mutate:metadata['encoded_file'].update(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
            (directory/'resources.bin').write_bytes(data);path=directory/'manifest.json'
            path.write_text(json.dumps(metadata,indent=2)+'\n')
            rejections.append(dict(name=label,directory=str(directory),sha256=sha(path)))
        rejections.append(dict(name='manifest-pin',directory=str(args.prepared_original_bank.resolve()),sha256='0'*64))
        expected_refusals=len(rejections)
        controls=owner/'bank-controls.json';controls.write_text(json.dumps(dict(
            directory=str(args.prepared_original_bank.resolve()),
            sha256=prepared_pins[str(args.prepared_original_bank/'manifest.json')],rejections=rejections,
            expected_refusals=expected_refusals,has_initial_flags=initial_flags))+'\n')
    q=lambda p:'"'+str(p).replace('\\','/').replace('"','\\"')+'"'
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeFlybyDecode LANGUAGES CXX)
enable_testing()
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(nlohmann_json CONFIG REQUIRED)
pkg_check_modules(AUDIO REQUIRED IMPORTED_TARGET libavcodec libavformat libswresample libavutil opus sdl2)
add_executable(flyby_decode audio.cpp mcc_audio.cpp vista_audio_framing.cpp vista_audio_decode.cpp prepared_original_sound_bank.cpp binary_readers.cpp probe_vehicle_flyby_decode_tu15.cpp)
target_compile_features(flyby_decode PRIVATE cxx_std_20)
target_compile_options(flyby_decode PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off)
target_link_options(flyby_decode PRIVATE -Wl,--gc-sections)
target_include_directories(flyby_decode PRIVATE include)
target_link_libraries(flyby_decode PRIVATE PkgConfig::AUDIO OpenSSL::Crypto nlohmann_json::nlohmann_json)
'''
    cmake+='add_test(NAME native_flyby_decode COMMAND flyby_decode '+q(manifest)+(
        ' '+q(args.prepared_original_bank.resolve())+' '+q(prepared_pins[str(args.prepared_original_bank/'manifest.json')]) if prepared_bank else '')+')\n'
    if controls:
        cmake+='''add_executable(sound_bank_controls prepared_original_sound_bank.cpp binary_readers.cpp prepared_original_sound_bank_tests.cpp)
target_compile_features(sound_bank_controls PRIVATE cxx_std_20)
target_include_directories(sound_bank_controls PRIVATE include ${AUDIO_INCLUDE_DIRS})
target_link_libraries(sound_bank_controls PRIVATE OpenSSL::Crypto nlohmann_json::nlohmann_json)
'''
        cmake+='add_test(NAME native_sound_bank_controls COMMAND sound_bank_controls '+q(controls)+')\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
             '--target','flyby_decode',*(['--target','sound_bank_controls'] if controls else []),
             '--filter','^(native_flyby_decode|native_sound_bank_controls)$','--jobs','2','--test-jobs','1',
             '--retain-report','Testing/Temporary/LastTest.log',
             '--output',str(args.output/'validation')]
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report=dict(passed=False,scope=__doc__,owner=str(owner),command=command,source_hashes=pins,
                reference_sha256=sha(args.reference),mcc_map=dict(path=str(cache_path),sha256=cache_pin),
                mcc_bank=dict(path=str(bank),sha256=bank_pin),validation_returncode=result,
                header_hashes=header_pins,tool_sha256=sha(Path(__file__)),
                binary_reader_source_sha256=cache_cpp_pin,binary_reader_body_sha256=hashlib.sha256(readers.encode()).hexdigest(),
                runtime_resources_modified=False,original_executable_executed=False,
                full_audio_parity=False,raw_encoded_or_pcm_retained=False)
    report.update(prepared_original_bank=str(args.prepared_original_bank) if args.prepared_original_bank else None,
                  prepared_bank_hashes=prepared_pins,original_map_or_resource_files_read=not bool(prepared_bank))
    if result==0:
        signals=[]
        for c in cases:
            pcm=Path(c.pop('pcm_path'));raw=pcm.read_bytes();data=np.frombuffer(raw,dtype='<f4').reshape(-1,2)
            require(np.isfinite(data).all() and len(data)>0,'Native PCM is invalid')
            signals.append(data[:,0].astype(np.float64))
            c.update(pcm_frames=len(data),pcm_sha256=hashlib.sha256(raw).hexdigest(),
                     peak=float(np.max(np.abs(data))),rms=float(np.sqrt(np.mean(data.astype(np.float64)**2))));
        pairs=[]
        for a in range(6):
            x=signals[a]
            for b in range(6,12):
                y=signals[b];extent=len(x)+len(y)-1;n=1<<(extent-1).bit_length()
                corr=np.fft.irfft(np.fft.rfft(x,n)*np.conj(np.fft.rfft(y,n)),n)
                lags=np.arange(-4800,4801);values=corr[lags%n]
                # Full-signal cosine: omitted tails contribute to denominator.
                denominator=float(np.sqrt(np.dot(x,x)*np.dot(y,y)))
                index=int(np.argmax(values));lag=int(lags[index])
                pairs.append(dict(original_permutation=a,mcc_permutation=b-6,lag_frames=lag,
                                  full_signal_cosine=float(values[index]/denominator) if denominator else None,
                                  exact_pcm_equal=bool(len(x)==len(y) and np.array_equal(x,y))))
        report.update(cases=cases,comparisons=pairs,passed=True,
                      exact_same_ordinal_pcm=all(p['exact_pcm_equal'] for p in pairs if p['original_permutation']==p['mcc_permutation']))
        if args.native_reference_report:
            prior=json.loads(args.native_reference_report.read_text())
            decoder_names=('src/audio/audio.cpp','src/audio/mcc_audio.cpp','src/audio/vista_audio_framing.cpp','src/audio/vista_audio_decode.cpp')
            require(prior['passed'] and all(prior['source_hashes'][name]==pins[name] for name in decoder_names),
                    'Native decoder reference source differs')
            expected=[(c['permutation'],c['pcm_frames'],c['pcm_sha256']) for c in prior['cases'] if c['namespace']=='original']
            actual=[(c['permutation'],c['pcm_frames'],c['pcm_sha256']) for c in cases if c['namespace']=='original']
            report['original_native_PCM_exact']=actual==expected
            report['native_reference_report_sha256']=sha(args.native_reference_report)
            report['passed'] &= report['original_native_PCM_exact']
    require(sha(cache_path)==cache_pin and sha(bank)==bank_pin,'Input resource changed during comparison')
    report['source_unchanged']=sha(cache_cpp)==cache_cpp_pin and all(sha(ROOT/name)==pin for name,pin in (pins|header_pins).items())
    if prepared_bank and result==0:
        log=(args.output/'validation/evidence/Testing/Temporary/LastTest.log').read_text()
        report['native_prepared_bank_owner_executed']='Native prepared original sound cases=6' in log
        report['passed'] &= report['native_prepared_bank_owner_executed']
        if controls:
            report['native_bank_refusals_passed']=f'PASS native prepared-bank refusals={expected_refusals} original_permutations=6 boundary_controls=2' in log
            report['native_bank_refusals']=expected_refusals
            report['passed'] &= report['native_bank_refusals_passed']
    report['passed'] &= report['source_unchanged']
    require(all(sha(path)==pin for path,pin in prepared_pins.items()),'Prepared bank changed during native decode')
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else result or 1


if __name__=='__main__':raise SystemExit(main())
