#!/usr/bin/env python3
"""Qualify proper native TU1.5 integer time conversion from real platform input.

Original32AE00 executes unsigned64 division over declared numerical inputs.
Original3268D0->37C5C0->32BBAE->33A4D3 consumes the real native platform clock
and supplies genuine input FILETIME and original seconds observations. No
authored timestamp or supplied seconds result is used by the production owner.
Run with run_test.py. Original ELFs, private banks and builds are disposable.
"""
import argparse,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_constraint_initializer_tu15 as initialization
BACKEND='unchanged-tu15-native-clock-recorded-filetime-integer-conversion-v1'
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
EPOCH_DELTA=0xfe624e212ac18000

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def division_cases(image,metadata,scratch):
    owned=scratch/'physical-division';owned.mkdir();observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_constraint_step_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
    code=_Code(image,'.text');body=complete_body(code,0x32ae00);assert len(body.code)==43
    m=dict(sites=initialization.descriptors([code.at(pc)for pc in sorted(body.code)]),regions=[])
    (owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')));section=next(s for s in image.sections if s.name=='.text');(owned/'text.bin').write_bytes(image.section_bytes(section))
    # The source's paired ADD/ADC is authenticated directly in the actual
    # TU1.5 body. The declared arithmetic inputs exercise modulo64 wrap.
    assert code.at(0x32bbc4).mnemonic=='add'and code.at(0x32bbc4).operands[1].imm==0x2ac18000
    assert code.at(0x32bbce).mnemonic=='adc'and code.at(0x32bbce).operands[1].imm&0xffffffff==0xfe624e21
    rng=random.Random(0x37c5c0);ticks=[0,1,9999999,10000000,116444736000000000-1,116444736000000000,116444736000000001,0xffffffffffffffff]
    ticks += [((n*10000000)-EPOCH_DELTA+delta)&0xffffffffffffffff for n in(0,1,0x7fffffff,0x80000000,0xffffffff,0x100000000,0x1ffffffff)for delta in(-1,0,1,9999999)]
    ticks += [rng.getrandbits(64)for _ in range(256)];output=[]
    for n,filetime in enumerate(ticks):
        numerator=(filetime+EPOCH_DELTA)&0xffffffffffffffff;stack=bytearray(0x10000);struct.pack_into('<5I',stack,SP-STACK,STOP,numerator&0xffffffff,numerator>>32,10000000,0);(owned/'stack.bin').write_bytes(stack)
        fp=[((0x27f,0x37f,0x7f)[n%3]&~0xc00)|((n%4)<<10),((n%8)<<11)|(0x453f if n%3==0 else 0),(0x1f80,0xffc0,0x5fa5)[n%3]];environment=bytearray(28);struct.pack_into('<H',environment,0,fp[0]);struct.pack_into('<H',environment,4,fp[1]);struct.pack_into('<H',environment,8,0xffff);(owned/'environment.bin').write_bytes(environment)
        assembly=f'''.code32
.section .original,"ax"
.incbin "{owned/'text.bin'}"
.section .stack,"aw"
.incbin "{owned/'stack.bin'}"
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry:.long 0x32ae00
sse_control:.long {fp[2]}
environment:.incbin "{owned/'environment.bin'}"
'''
        (owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS {{\n.original 0x{section.va:x} : {{ *(.original) }}\n.stack 0x{STACK:x} : {{ *(.stack) }}\n.bridge 0x70000000 : {{ *(.bridge) }}\n}}\n')
        subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
        observed=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'observed.json'),hex(STOP)],text=True,capture_output=True);assert observed.returncode==0,(n,observed.stderr)
        actual=json.loads((owned/'observed.json').read_text());assert actual['boundary_gp'][4]==SP+20 and actual['after_fp']==fp
        output.append(dict(filetime=filetime,before_fp=fp,**actual))
    return output

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=initialization.metadata(image,scratch)
    divisions=division_cases(image,meta,scratch);clock_inputs=[c for c in initialization.input_cases(image)if c['name'].startswith('date_day_1_platform_')or c['name'].startswith('date_day_10_platform_')]
    for case in clock_inputs:case['discover_log_boundary']=True
    real_clocks=initialization.physical(image,meta,clock_inputs,scratch);assert all(c['actual_time']is not None for c in real_clocks)
    fixture=scratch/'native-clock.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,division_cases=divisions,real_clock_cases=real_clocks),separators=(',',':')))
    project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_constraint_clock.cpp';text=source.read_text();(project/source.name).write_text(text);assert text.count('0xfe624e212ac18000u')==1;(project/'mutated.cpp').write_text(text.replace('0xfe624e212ac18000u','0xfe624e212ac18001u'));(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and "Actual original unsigned time conversion differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Original time arithmetic rejects changed epoch:",r.stderr.strip())\n')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_clock_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for kind in('original','negative'):
        target='native_constraint_clock_'+kind;cpp=source.name if kind=='original'else'mutated.cpp';cmake+=f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_clock_tests.cpp" "{project}/{cpp}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\n';command=f'$<TARGET_FILE:{target}> "{fixture}"'if kind=='original'else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"';cmake+=f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project/'CMakeLists.txt').write_text(cmake);results=[]
    for compiler in('g++','clang++'):
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','native_constraint_clock_original','--target','native_constraint_clock_negative','--filter','^native_constraint_clock_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
    files=[source,ROOT/'include/halo2/native_tu15_constraint_clock.hpp',ROOT/'tests/native_tu15_constraint_clock_tests.cpp',Path(initialization.__file__),ROOT/'tools/observe_native_constraint_initializer_tu15.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',Path(__file__)];hashes={str(p.relative_to(ROOT)):sha(project/p.name if p==source else p)for p in files}
    report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,required_backend=BACKEND,image_sha256=NEW_PIN,listing_sha256=meta['listing_sha256'],original_division_cases=len(divisions),genuine_original_clock_cases=len(real_clocks),original_instructions=sum(c['original_instructions']for c in divisions+real_clocks),source_hashes=hashes,current_sources_match_snapshot=all(sha(ROOT/p)==value for p,value in hashes.items()),actual_input_units='100ns FILETIME since1601-01-01UTC',native_math='unsigned64 epoch subtraction modulo2^64, unsigned divide10^7, low32 return',production_queries_independent_clock=False,source_input_callback_exactly_once=True,actual_two_live_x87_values_preserved=True,missing_and_failed_source_explicit=True,clock_source_sha256=meta['clock_source_sha256'],clock_instruction_sha256=meta['clock_instruction_sha256'],sdk_sha256=meta['sdk_sha256'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,owned_scratch=str(scratch),retained_large_artifacts=[],negative_control='Source authenticated epoch delta changed by1',excluded=['world UtcFiletime receipt/adjustment integration','source logger body','unavailable platform input fallback'])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_division_cases','genuine_original_clock_cases')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
