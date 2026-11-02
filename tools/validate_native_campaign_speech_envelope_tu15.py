#!/usr/bin/env python3
"""Owned original/compiler/sanitizer/semantic controls for native21F860."""
import argparse,hashlib,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CORE='src/audio/native_campaign_speech_envelope_tu15.cpp'
BINDING='src/audio/native_campaign_speech_envelope_tu15_binding.cpp'
COMMON=['tests/native_campaign_speech_envelope_tu15_tests.cpp',
    'tests/native_campaign_speech_envelope_manager_groups.cpp',BINDING,
    'src/audio/native_campaign_speech_smoothing_tu15.cpp','src/audio/native_campaign_speech_smoothing_tu15_binding.cpp',
    'src/audio/native_tu15_sound_kernels_unit_speech.cpp','src/audio/native_tu15_sound_manager.cpp']
FIXTURE='tests/fixtures/native_campaign_speech_envelope_tu15.json.gz'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    modes_group=parser.add_mutually_exclusive_group();modes_group.add_argument('--baseline-only',action='store_true')
    modes_group.add_argument('--controls-only',action='store_true');args=parser.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    owner=Path(os.environ['HALO2_TEST_SCRATCH']);project=owner/'project';project.mkdir()
    pending=[CORE,*COMMON,FIXTURE,'cmake/NativeCampaignSpeechEnvelope.cmake','tools/test_native_campaign_screen_lifecycle_tu15.py'];pins={}
    while pending:
        name=pending.pop()
        if name in pins:continue
        original=ROOT/name;data=original.read_bytes();pins[name]=hashlib.sha256(data).hexdigest()
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        if original.suffix in('.cpp','.hpp','.h'):
            for include in re.findall(r'^\s*#\s*include\s+"([^"]+)"',data.decode(),re.M):
                for base in(original.parent,ROOT/'include',ROOT/'src',ROOT/'tests'):
                    candidate=base/include
                    if candidate.is_file():pending.append(str(candidate.relative_to(ROOT)));break
    sources={path:(project/path).read_text()for path in(CORE,BINDING)};variants={}
    pairs=[
        ('clamped_permutation',CORE,'if(selected!=permutation)return {false,0};','permutation=std::int16_t(selected);'),
        ('unsigned_curve',CORE,'std::bit_cast<std::int8_t>(std::uint8_t(read(memory,permutations+std::uint32_t(first+permutation)*16u+5u,1)))','std::uint8_t(read(memory,permutations+std::uint32_t(first+permutation)*16u+5u,1))'),
        ('fixed_sample_cap',CORE,'const auto sample_index=sample<0?0:(std::int32_t(sample)>count-1?count-1:std::int32_t(sample));','const auto sample_index=sample<0?0:(sample>255?255:sample);'),
        ('early_permutation_gate',CORE,'const auto gestalt=read(memory,0x52d664u)','if(permutation<0)return {false,0};const auto gestalt=read(memory,0x52d664u)'),
        ('wrong_scale',CORE,'scale=read(memory,0x46a0c4u);','scale=0x3b800000u;'),
        ('stale_sample_zero',CORE,'const auto value=read(memory,bytes+offset+std::uint32_t(sample_index),1);','const auto value=(read(memory,bytes+offset+std::uint32_t(sample_index),1),0u);'),
        ('fresh_gestalt',CORE,'resources=read(memory,gestalt+0x54u)','resources=read(memory,read(memory,0x52d664u)+0x54u)'),
        ('unsigned_resource',CORE,'const auto resource_index=word(memory,definition+0xeu)','const auto resource_index=std::int32_t(read(memory,definition+0xeu,2))'),
        ('lost_pending',BINDING,'abi.xmm[0][0]=error.converted_sample_bits();','(void)error;'),
        ('lost_valid_upper',BINDING,'if(result.present)abi.xmm[0][0]=result.bits;','if(result.present)abi.xmm[0]={result.bits,0,0,0};'),
        ('kept_absent_upper',BINDING,'else abi.xmm[0].fill(0);','else abi.xmm[0][0]=0;')]
    if not args.baseline_only:
        for name,path,needle,replacement in pairs:
            text=sources[path];assert text.count(needle)==1,(name,text.count(needle));variant=str(Path(path).with_name(name+'.cpp'))
            (project/variant).write_text(text.replace(needle,replacement));variants[name]=(path,variant)
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],capture_output=True,text=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('scalar lanes differ','ordered reads differ','envelope FP differs','retained bytes differ','input timeline differs','unexpected missing storage','completion differs')),(r.returncode,r.stdout,r.stderr)
print('Expected original speech-envelope mutation rejection:',r.stderr[-1500:])
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeSpeechEnvelope LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/NativeCampaignSpeechEnvelope.cmake)
'''
    for name,(path,variant)in variants.items():
        target='speech_envelope_'+name;files=[variant if source==path else source for source in(CORE,*COMMON)]
        cmake+=f'''add_executable({target} {' '.join(files)})
target_include_directories({target} PRIVATE include src)
target_compile_features({target} PRIVATE cxx_std_20)
target_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME {target} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:{target}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{FIXTURE}")
set_tests_properties({target} PROPERTIES TIMEOUT 30)
'''
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    modes=[('gcc','g++',False)]if args.baseline_only else[('clang-controls','clang++',False)]if args.controls_only else[('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)]
    for kind,compiler,sanitizers in modes:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),
            '--output',str(args.output/kind),'--target','native_campaign_speech_envelope_tu15_tests','--filter',
            '^(native_campaign_speech_envelope_tu15|speech_envelope_.*)$'if kind=='clang-controls'else'^native_campaign_speech_envelope_tu15$',
            '--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
            '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if sanitizers:command+=['--sanitizers']
        if kind=='clang-controls':
            for name in variants:command+=['--target','speech_envelope_'+name]
        result=subprocess.run(command);results.append(dict(kind=kind,command=command,exit_code=result.returncode))
        if result.returncode:break
    unchanged=all(sha(ROOT/path)==pin for path,pin in pins.items());passed=len(results)==len(modes)and all(r['exit_code']==0 for r in results)and unchanged
    (args.output/'validation.json').write_text(json.dumps(dict(passed=passed,baseline_only=args.baseline_only,controls_only=args.controls_only,
        source_sha256=pins,source_unchanged=unchanged,results=results,negative_controls=list(variants),
        owned_scratch=str(owner),retained_large_artifacts=[],actual_registry_tested=True,whole_audio_qualified=False,
        whole_dialogue_qualified=False,live_playback_qualified=False,launcher_promoted=False),indent=2)+'\n');return int(not passed)


if __name__=='__main__':raise SystemExit(main())
