#!/usr/bin/env python3
"""Exercise source-phase scalar aliases using untouched physical TU1.5 code.

The observer changes declared retained inputs after a source store, on both
the physical original and the native typed algorithm. Original instructions
are not patched and no arithmetic/callee result is injected. All executable
observer copies, raw banks and build files belong to run_test.py scratch.
"""
import copy
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import argparse
from oracle_smg_firing_1_1 import ROOT
from export_mp_physics_contact_lifecycle import segments
from validate_native_lift_scalar_tu15 import bits,STACK,OUT,BASE
import validate_native_lift_power_tu15 as leaf
import oracle_vehicle_motion_math_hardware_20261011 as physical

def fixtures():
    baseline=leaf.inputs(True)[0];rows=[];mutations=[];names=[]
    def write(pc,p,value):return dict(instruction=pc,address=p,written=struct.pack('<I',value).hex())
    patterns=(
      ('late-exponent-and-acceleration',0.26,0.5,[(0x1da54e,STACK+32,bits(3.)),(0x1da54e,STACK+16,bits(-3.))]),
      ('power-base-readback',0.26,0.5,[(0x1da54e,STACK-4,bits(0.5))]),
      ('baked-acceleration-readback',0.26,0.5,[(0x1da56e,STACK+32,bits(5.))]),
      ('late-mass-flag',0.26,0.5,[(0x1da56e,STACK+4,0),(0x1da56e,STACK+8,bits(0.001))]),
      ('captured-clock-cell-reread',0.26,0.5,[(0x1da5ae,0x51df54,OUT+0x100),(0x1da5ae,BASE+4,bits(0.25))]),
      ('late-force-cached-velocity-and-limit',0.26,0.5,[(0x1da5e1,STACK+36,1),(0x1da5e1,STACK+20,bits(99.)),(0x1da5e1,STACK+24,bits(99.))]),
      ('root-step-distance-rereads',0.26,0.5,[(0x1da5e1,STACK+4,bits(-1.)),(0x1da5e1,STACK+12,bits(0.001))]),
      ('root-result-readback',0.26,0.5,[(0x1da643,STACK+4,bits(1.25))]),
      ('late-ratio-destination',0.26,0.5,[(0x1da5e1,STACK+40,OUT+8)]),
      ('late-scratch-pop',0.26,0.5,[(0x1da6ea,STACK-4,bits(-8.))]),
      ('errno-overflow-before-acceleration',2.,1075.,[(0x32e9b7,STACK+16,bits(-3.))]),
      ('errno-domain-before-acceleration',1.,float('inf'),[(0x32e9c3,STACK+16,bits(-3.))]),
    )
    for name,target,exponent,changes in patterns:
        for rounding in range(4):
            for top in (0,5):
                row=dict(baseline,target=bits(target),exponent=bits(exponent),acceleration=bits(3.),mass=bits(2.),velocity=bits(0.5),velocity_limit=bits(2.),position=bits(0.25),scale_by_mass=True,force_acceleration=False,allow_reverse=True,
                    x87_control_word=0x37f|(rounding<<10),mxcsr=0x1f80|(rounding<<13),x87_exception_flags=0,initial_top=top,initial_condition_bits=0x4500,initial_errno=0x12345678,thread_local=True,tls_index=1)
                rows.append(row);names.append(name);mutations.append([write(*change) for change in changes])
    fixture,paired,bodies=leaf.original(rows)
    for case,changes,name in zip(fixture['cases'],mutations,names):
        case.update(observer_mutations=changes,alias_name=name)
        initial={b['address']+i:v for b in case['initial_banks'] for i,v in enumerate(bytes.fromhex(b['hex']))}
        for p,raw in ((OUT+8,bytes(4)),(OUT+0x104,struct.pack('<I',bits(20.)))):
            for i,v in enumerate(raw):
                if p+i in initial:assert initial[p+i]==v
                else:initial[p+i]=v
        case['initial_banks']=[dict(address=p,hex=b.hex()) for p,b in segments(initial)]
    return fixture,paired,bodies

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();paths=(*leaf.SOURCES,'tests/native_lift_scalar_alias_tu15_tests.cpp');report=dict(passed=False,source_sha256={p:leaf.sha(ROOT/p) for p in paths},validator_sha256=leaf.sha(__file__),scratch_cleanup_owner=str(scratch),retained_large_artifacts=[],live_mutating_observer_admitted=False)
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    save()
    try:
        fixture,paired,bodies=fixtures();declared={str(c['id']):c['observer_mutations'] for c in fixture['cases']}
        # Instrument the offline observer, never the original body. POKEDATA
        # is the physical equivalent of an explicitly mutating store observer.
        tracer=(ROOT/'tools/vehicle_motion_math_hardware_trace_20261011.cpp').read_text()
        helper='''
void write_hex(pid_t pid,std::uint32_t p,const std::string& text){
 for(unsigned k=0;k<text.size()/2;){const auto aligned=std::uintptr_t(p+k)&~7ul;auto word=std::uint64_t(request(PTRACE_PEEKDATA,pid,reinterpret_cast<void*>(aligned)));for(unsigned j=(p+k)&7u;j<8&&k<text.size()/2;++j,++k){word=(word&~(255ull<<(j*8)))|(std::stoull(text.substr(k*2,2),nullptr,16)<<(j*8));}request(PTRACE_POKEDATA,pid,reinterpret_cast<void*>(aligned),reinterpret_cast<void*>(word));}
}
'''
        tracer=tracer.replace('int main(int argc,char**argv){try{',helper+'\nint main(int argc,char**argv){try{\n const auto mutations=J::parse(R"ALIASES('+json.dumps(declared,separators=(',',':'))+')ALIASES");\n std::set<std::pair<unsigned,unsigned>> applied;')
        tracer='#include <set>\n'+tracer
        marker='   const auto previous=std::uint32_t(r.rip);'
        assert tracer.count(marker)==1
        tracer=tracer.replace(marker,'''   unsigned mutation_index=0;for(const auto& mutation:mutations.at(key)){
    if(!stores.empty()&&mutation.at("instruction").get<std::uint32_t>()==i.canonical&&applied.emplace(id,mutation_index).second){write_hex(child,mutation.at("address"),mutation.at("written"));result["observer_mutations"].push_back(mutation);}
    ++mutation_index;
   }
'''+marker)
        observer_root=scratch/'observer';(observer_root/'tools').mkdir(parents=True);observer=(observer_root/'tools/vehicle_motion_math_hardware_trace_20261011.cpp');observer.write_text(tracer)
        original_root=physical.ROOT;physical.ROOT=observer_root
        try:oracle=leaf.original_hardware(fixture,paired,scratch/'hardware')
        finally:physical.ROOT=original_root
        observed=json.loads((scratch/'hardware/trace.json').read_text());actual={c['id']:c for c in oracle['cases']};observed_mutations={c['id']:c.get('observer_mutations',[]) for c in observed['cases']}
        image=leaf.Xbe(leaf.NEW_DEFAULT)
        for case in fixture['cases']:
            assert observed_mutations[case['id']]==case['observer_mutations'],(case['id'],case['alias_name'])
            for key in ('exit','final_banks','instructions','writes'):case[key]=actual[case['id']][key]
            # Every possible absolute scalar operand is an authenticated input,
            # including a branch entered only after the observer's input change.
            for key in ('initial_banks','final_banks'):
                data={b['address']+i:v for b in case[key] for i,v in enumerate(bytes.fromhex(b['hex']))}
                for p in (0x46a180,0x46a0d0,0x46a0cc,0x44aac8,0x45695c):
                    for i,v in enumerate(image.read(p,4)):
                        if p+i in data:assert data[p+i]==v
                        else:data[p+i]=v
                case[key]=[dict(address=p,hex=b.hex()) for p,b in segments(data)]
        fixture['oracle_backend']='original_i386_hardware_single_step';fixture_path=scratch/'original-aliases.json';fixture_path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(cases=len(fixture['cases']),observer_mutations=sum(len(c['observer_mutations']) for c in fixture['cases']),alias_patterns=sorted({c['alias_name'] for c in fixture['cases']}),original_bodies=bodies,physical_instructions=oracle['single_stepped_original_instructions'],cpu_model=oracle['cpu_model'],original_sha256=leaf.NEW_PIN,paired_instruction_bytes_changed=oracle['paired_instruction_bytes_changed'],original_instruction_patches=[],observer_source_sha256=leaf.sha(observer),oracle_sha256=leaf.sha(fixture_path),full_fp_status_and_control_checked=True);save()
        source=scratch/'source';shutil.copytree(ROOT/'include',source/'include')
        for path in paths:
            out=source/path;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/path,out)
        units=['src/physics/source_lift_scalar.cpp','src/physics/source_lift_power_tu15.cpp','src/physics/source_power_special_tu15.cpp','src/physics/source_math_error_tu15.cpp','src/physics/native_lift_scalar_consumer_tu15.cpp','tests/native_lift_scalar_alias_tu15_tests.cpp']
        (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(native_lift_alias_tu15 LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json REQUIRED)
add_executable(native_lift_alias_tu15_tests '''+' '.join(units)+''')
target_compile_features(native_lift_alias_tu15_tests PRIVATE cxx_std_20)
target_include_directories(native_lift_alias_tu15_tests PRIVATE include)
target_compile_options(native_lift_alias_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(native_lift_alias_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_lift_alias_tu15 COMMAND native_lift_alias_tu15_tests "'''+str(fixture_path)+'''")
''')
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(source),'--output',str(args.output/'native'),'--scratch-root',str(scratch/'builds'),'--target','native_lift_alias_tu15_tests','--filter','^native_lift_alias_tu15$','--jobs','2','--test-jobs','1'];report['command']=command;save();assert subprocess.run(command,check=False).returncode==0,'Native alias result differs; inspect native/tests.log';report['passed']=True
    except BaseException as error:report['error']=str(error);raise
    finally:save()
    print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()
