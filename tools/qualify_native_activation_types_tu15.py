#!/usr/bin/env python3
"""Physical original type getters versus the actual native owner dispatcher.

The native owner identity selects authenticated original leaf semantics. All
expected values come from unchanged original i386 execution, not from the map
being tested. This is a leaf proof, not shape/agent or gameplay acceptance.
"""
import argparse,ctypes,hashlib,json,os,re,shutil,struct,subprocess,sys,tempfile
from pathlib import Path
from original_audio_cpu import OriginalAudioCpu
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

SOURCE='src/physics/native_tu15_activation_services.cpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    files=[SOURCE,'include/halo2/native_tu15_activation_services.hpp','tools/qualify_native_activation_types_tu15.py']
    pins={n:sha(ROOT/n)for n in files};report=dict(passed=False,source_hashes=pins,retained_large_artifacts=[])
    try:
        # The implementation supplies only the supported owner identities to
        # exercise. Expected type values still come solely from original CPU
        # execution. Require every current case label to be a literal leaf.
        text=(ROOT/SOURCE).read_text();body=text[text.index('std::optional<std::uint32_t> native_tu15_activation_type('):]
        body=body[:body.index('std::uint32_t native_tu15_activation_group_filter(')]
        owners=tuple(int(value,16)for value in re.findall(r'case (0x[0-9a-f]+)u:return [0-9]+;',body))
        assert owners and len(set(owners))==len(owners)and len(owners)==body.count('case ')
        image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(image,'.text');aliases=('rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi');rows=[];bodies={}
        with tempfile.TemporaryDirectory(prefix='activation-types-',dir=scratch)as directory:
            directory=Path(directory);host=OriginalAudioCpu(image,directory/'types-original32.elf',0x27f,0x1fa0)
            try:
                for owner in owners:
                    body=complete_body(code,owner);assert len(body.code)==2
                    ins=[code.at(pc)for pc in sorted(body.code)];assert ins[0].mnemonic=='mov'and ins[0].op_str.startswith('eax, ')and ins[1].mnemonic=='ret'
                    bodies[hex(owner)]={'bytes':b''.join(i.bytes for i in ins).hex(),'sha256':hashlib.sha256(b''.join(i.bytes for i in ins)).hexdigest()}
                    for poison in range(8):
                        gp=[(0x9e3779b1*(n+1)+poison*0x19660d)&0xffffffff for n in range(8)];gp[4]=0x100f000
                        host.write(gp[4],struct.pack('<I',0x3000000));regs=host.run(owner,gp[4],**dict(zip(aliases,gp)))
                        after=[getattr(regs,n)&0xffffffff for n in aliases]
                        # Execution breakpoints set RF; it is debugger state,
                        # not an arithmetic/control flag changed by MOV/RET.
                        assert after[1:4]==gp[1:4] and after[5:]==gp[5:] and after[4]==gp[4]+4 and regs.eflags&~0x10000==0x202,(
                            hex(owner),gp,after,hex(regs.eflags))
                        context=ctypes.create_string_buffer(512);host.trace(14,0,ctypes.addressof(context))
                        assert struct.unpack_from('<HH',context.raw,0)==(0x27f,0)and struct.unpack_from('<I',context.raw,24)[0]==0x1fa0
                        rows.append(dict(owner=owner,incoming_gp=gp,result=after[0]))
            finally:host.close()
            reference=directory/'original.json';reference.write_text(json.dumps(rows))
            project=directory/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/SOURCE,project/'native.cpp')
            driver=r'''#include "halo2/native_tu15_activation_services.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>
using namespace halo2;using J=nlohmann::json;
int main(int argc,char**argv)try{if(argc!=2)throw std::runtime_error("Supply original numeric results");std::ifstream f(argv[1]);J rows;f>>rows;unsigned count{};
for(const auto&r:rows){std::uint16_t before,after;asm volatile("fnstsw %0":"=a"(before));const auto mx=_mm_getcsr();const auto result=native_tu15_activation_type(r.at("owner"));asm volatile("fnstsw %0":"=a"(after));if(!result||*result!=r.at("result")||before!=after||mx!=_mm_getcsr())throw std::runtime_error("Native type getter differs from unchanged original");++count;}
if(native_tu15_activation_type(0xffffffffu))throw std::runtime_error("Unknown native owner was admitted");std::cout<<J{{"passed",true},{"cases",count}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
''';(project/'driver.cpp').write_text(driver)
            (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(types LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(types native.cpp driver.cpp)\ntarget_include_directories(types PRIVATE include)\ntarget_compile_features(types PRIVATE cxx_std_20)\ntarget_link_libraries(types PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME types COMMAND types "'+str(reference)+'")\n')
            commands=[];results=[]
            def run(label,compiler):
                command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--target','types','--filter','^types$','--jobs','1','--test-jobs','1','--scratch-root',str(directory/'builds'),'--output',str(a.output/label),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
                commands.append(command);r=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT);results.append(dict(label=label,exit_code=r.returncode));return r.returncode
            for compiler in('g++','clang++'):assert run(compiler,compiler)==0
            target=project/'native.cpp';text=target.read_text();assert text.count('case 0xad710u:return 10;')==1
            target.write_text(text.replace('case 0xad710u:return 10;','case 0xad710u:return 9;'));assert run('negative-class10','g++')!=0
            assert all(sha(ROOT/n)==digest for n,digest in pins.items())
            report.update(passed=True,scope=__doc__,original_xbe_sha256=NEW_PIN,original_bodies=bodies,original_cases=rows,
                cases=len(rows),owners=len(owners),original_instructions=len(rows)*2,source_code_patches=[],supplied_callee_results=[],
                physical_backend='Unchanged Linux i386 compatibility execution and ptrace',commands=commands,results=results,negative_controls=['class10_returns9'])
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k]for k in('passed','owners','cases','original_instructions')}),flush=True)

if __name__=='__main__':main()
