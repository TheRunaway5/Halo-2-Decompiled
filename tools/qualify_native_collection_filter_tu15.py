#!/usr/bin/env python3
"""Physical TU1.5 collection-filter operands, real class/resolver/motion calls.

Declared shape graphs use real original constant-method owners. Original code
and native code each compute their own permission, with no callback outcome
or expected after image supplied. Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output.mkdir(parents=True,exist_ok=False)
    work=scratch/'collection-filter'
    work.mkdir()
    report=dict(passed=False,supplied_original_outcomes=False,captured_original_gp_frame=False,
                instruction_patches=[],retained_large_artifacts=[])
    try:
        prepared=args.output/'reference-preparation'
        command=[sys.executable,str(ROOT/'tools/qualify_native_list_contact_agent_tu15.py'),
                 '--capture',str(args.capture.resolve()),'--output',str(prepared),'--prepare-only']
        subprocess.run(command,check=True)
        reference=json.loads((prepared/'report.json').read_text())
        assert reference['prepared'] and reference['validation_not_run'] and not reference['passed']
        assert sha(NEW_DEFAULT)==NEW_PIN
        code=_Code(Xbe(NEW_DEFAULT),'.text');bodies={p:complete_body(code,p) for p in (0x1cbe70,0x2e98d0,0x185020)}
        registers={'eax':'r.rax','ecx':'r.rcx','edx':'r.rdx','ebx':'r.rbx','esi':'r.rsi','edi':'r.rdi'}
        generated=[];owners=[]
        for owner,body in bodies.items():
            instructions=[]
            for pc in sorted(body.code):
                i=code.at(pc);lines=[]
                instructions.append(dict(pc=pc,bytes=bytes(i.bytes).hex(),mnemonic=i.mnemonic,operands=i.op_str))
                for operand in i.operands:
                    if operand.type!=X86_OP_MEM:continue
                    mem=operand.mem;base=i.reg_name(mem.base);index=i.reg_name(mem.index)
                    # Formal/control-frame cells have no native DATA API.
                    if base in ('esp','ebp') or index in ('esp','ebp'):continue
                    #185020's authenticated immutable switch tables are code
                    # operands; the C++ switch replaces those constant reads.
                    if 0x185020<=mem.disp<0x185100:continue
                    assert not mem.segment
                    terms=[str(mem.disp)]
                    if base:terms.append(registers[base])
                    if index:terms.append(registers[index]+'*'+str(mem.scale)+'u')
                    expression='std::uint32_t('+'+'.join(terms)+')'
                    assert not (operand.access&CS_AC_READ and operand.access&CS_AC_WRITE)
                    if operand.access&CS_AC_READ:lines.append(f'address={expression};width={operand.size};read=true;')
                    if operand.access&CS_AC_WRITE:lines.append(f'address={expression};width={operand.size};write=true;')
                if i.mnemonic=='call' and i.op_str not in ('0x185020','0x2e98d0'):
                    assert len(i.operands)==1 and i.operands[0].type==X86_OP_MEM
                    lines.append(f'call=true;target=original.word(address);receiver=r.rcx;return_pc={pc+len(i.bytes)}u;')
                    if pc==0x1cbebd:lines.append('arguments=J::array({original.word(r.rsp),original.word(r.rsp+4u)});')
                if lines:generated.append(f'case {pc}u: '+''.join(lines)+'break;')
            owners.append(dict(owner=owner,instruction_sites=len(body.code),instructions=instructions))
        bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp'
        prefix=bridge.read_text().split('int main(int argc,char**argv)')[0]
        prefix=prefix.replace('for(unsigned n=0;n<=ordinal;++n){go(0x304350);if(n<ordinal)step();}','go(0x2f8030);')
        prefix=prefix.replace('mutable std::size_t reads{};std::size_t writes{};','mutable std::size_t reads{};std::size_t writes{};mutable J ledger=J::array();J stores=J::array();')
        prefix=prefix.replace('++reads;}void write','++reads;ledger.push_back(J::array({p,b.size()}));}void write')
        prefix=prefix.replace('child.write(p,b);++writes;','child.write(p,b);++writes;J bytes=J::array();for(auto v:b)bytes.push_back(std::to_integer<unsigned>(v));stores.push_back(J::array({p,bytes}));')
        driver=work/'driver.cpp'
        driver.write_text('#include "halo2/native_tu15_collection_filter.hpp"\n'+prefix+(ROOT/'tools/observe_native_collection_filter_tu15.cpp').read_text().replace('/* ORIGINAL_OPERANDS */','\n'.join(generated)))
        cases=[]
        def add(name,**options):cases.append(dict(name=name,**options))
        add('ordinary');add('parents',parents=True);add('same_group',same_group=True);add('mask_reject',reject=True)
        add('zero_group',zero_group=True);add('bit31',bit31=True)
        for name,options in (
            ('kind24',dict(kind=24)),('kind10',dict(kind=10)),('unwrap21',dict(kind=24,wrapper=21)),
            ('unwrap23',dict(kind=24,wrapper=23)),('both_wrappers',dict(kind=1,wrapper=2123)),
            ('no_child_key',dict(kind=1,no_key=True)),('null_user',dict(kind=1,user=0)),
            ('sentinel1',dict(kind=1,user=1)),('sentinel16',dict(kind=1,user=16)),
            ('flag_clear',dict(kind=1,flag=0)),('other_root_kind',dict(kind=1,root_kind=2)),
            ('null_body',dict(kind=1,null_body=True)),('motion6',dict(kind=1,motion=6)),
            ('parents',dict(kind=1,parents=True)),('negative_user',dict(kind=1,negative_user=True))):
            add('special_'+name,special=True,**options)
        for kind in (1,4,5,6,7,8):add('special_eligible_'+str(kind),special=True,kind=kind)
        path=work/'cases.json';path.write_text(json.dumps(cases))
        project=work/'project';project.mkdir()
        source=ROOT/'src/physics/native_tu15_collection_filter.cpp';text=source.read_text()
        variants={'native':text}
        for name,old,new in (
            ('wrong_parent','word(p+12u)','word(p+8u)'),
            ('wrong_mask','(row&31u)*4u+28u','(row&31u)*8u+28u'),
            ('omit_type7','case 7:',''),
            ('wrong_special_output','m.byte(0x1cbf5bu,output,0)','m.byte(0x1cbf5bu,output,1)')):
            assert text.count(old)==1,(name,old)
            variants[name]=text.replace(old,new)
        (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True,timeout=90)\nd=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not d["passed"] and d.get("original_completed"),(r.returncode,d)\n')
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeCollectionFilter LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
'''
        retain=[]
        negative={'wrong_parent':'parents','wrong_mask':'bit31','omit_type7':'special_eligible_7','wrong_special_output':'special_eligible_1'}
        for variant,content in variants.items():
            (project/(variant+'.cpp')).write_text(content)
            cmake+=f'add_executable(filter_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp)\ntarget_include_directories(filter_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(filter_{variant} PRIVATE cxx_std_20)\n'
            rows=cases if variant=='native' else [c for c in cases if c['name']==negative[variant]]
            for case in rows:
                name=variant+'_'+case['name'];output='evidence/'+name+'.json';retain.append(output)
                command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
                if variant!='native':command+=f'"{sys.executable}" "{project}/negative.py" '
                command+=f'$<TARGET_FILE:filter_{variant}> "{reference["original_elf"]}" "{args.capture.resolve()}/capture-native-before.json" "{path}" "{case["name"]}" "${{CMAKE_BINARY_DIR}}/{output}"'
                cmake+=f'add_test(NAME collection_{name} COMMAND {command})\nset_tests_properties(collection_{name} PROPERTIES TIMEOUT 110)\n'
        (project/'CMakeLists.txt').write_text(cmake)
        files=[source,ROOT/'include/halo2/native_tu15_collection_filter.hpp',bridge,Path(__file__),ROOT/'tools/observe_native_collection_filter_tu15.cpp']
        pins={str(p.relative_to(ROOT)):sha(p) for p in files};results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(work/'builds'),
                     '--output',str(args.output/compiler),*sum((['--target','filter_'+v] for v in variants),[]),
                     '--filter','^collection_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',
                     *sum((['--retain-report',p] for p in retain),[])]
            result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
        report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,source_hashes=pins,
                      current_sources_unchanged=all(sha(ROOT/p)==v for p,v in pins.items()),
                      source_owners=owners,original_xbe_sha256=NEW_PIN,cases=cases)
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':sys.exit(main())
