#!/usr/bin/env python3
"""Compare complete native2F1CC0/2F0DF0 with unchanged physical TU1.5.

Only protected BEFORE inputs are copied. A declared wrapper input graph points
at each genuine captured shape; allocation, both classifiers and nested factory
execute the original CPU bodies on each side's current mutable storage. No
expected pointer, class, GP postimage or service outcome is supplied to native.
Run inside tools/run_test.py. CMake validation owns and removes its build trees.
"""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2)+'\n')


DRIVER = r'''
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}};try{
 check(argc==6,"Supply ELF, capture, producer ordinal, reversal and output");std::ifstream in(argv[2]);const auto capture=J::parse(in);const unsigned ordinal=std::stoul(argv[3]);const bool reverse=std::stoul(argv[4]);
 Child original;original.start(argv[1],ordinal);const auto entry=original.regs();const auto before=original.fp();check(before.ftw==0,"Wrapped factory requires declared empty incoming x87");const auto source_sp=std::uint32_t(entry.rsp),packet=((source_sp-4u)&~15u)-0x600u;
 auto a=original.word(source_sp+4),b=original.word(source_sp+8);if(reverse)std::swap(a,b);const auto context=original.word(source_sp+12),user_data=original.word(std::uint32_t(entry.rcx)+8u);
 const auto wrapper_shape=packet-0x100u,wrapper_payload=packet-0x120u,native_payload=packet-0x180u;
 // Actual2F1D04 pushes A, then2F1D05 pushes B: ctor first=B, second=A.
 const std::array<std::uint32_t,4> shape{0xabcde123u,0x88776655u,0x11223344u,original.word(b)};
 const std::array<std::uint32_t,4> payload{wrapper_shape,original.word(b+4u),original.word(b+8u),0xaabbccddu};
 original.write(wrapper_shape,{reinterpret_cast<const std::byte*>(shape.data()),sizeof(shape)});original.write(wrapper_payload,{reinterpret_cast<const std::byte*>(payload.data()),sizeof(payload)});b=wrapper_payload;
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
 const std::array<std::uint32_t,5> call_words{0x1ccaca,a,b,context,user_data};original.write(packet-20u,{reinterpret_cast<const std::byte*>(call_words.data()),sizeof(call_words)});auto full_entry=entry;full_entry.rip=0x2f1cc0;full_entry.rcx=0;full_entry.rsp=packet-20u;request(PTRACE_SETREGS,original.pid,nullptr,&full_entry);
 J actual_reads=J::array(),actual_stores=J::array(),actual_calls=J::array();std::size_t original_instructions{};std::uint32_t original_payload{};
 auto bytes=[](const auto& input){J result=J::array();for(auto b:input)result.push_back(std::to_integer<unsigned>(b));return result;};
 auto normalized=[&](std::uint32_t p,std::uint32_t temporary){return p>=temporary&&p<temporary+16u?0xffff0000u+p-temporary:p;};
 for(;;){const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccaca)break;check(++original_instructions<128,"Wrapped factory left authenticated91-site owners");std::uint32_t address{};unsigned width=4;bool read=false,write=false,call=false;std::uint32_t target{},receiver{},return_pc{};J args=J::array();
  switch(pc){
   case 0x2f1cd6:address=0x48c938;read=true;break;case 0x2f1cdc:address=r.rcx;read=true;break;
   case 0x2f1ce3:address=r.rax+16u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f1ce6;args=J::array({original.word(r.rsp),original.word(r.rsp+4u)});break;
   case 0x2f1ce8:address=r.rsi+4u;width=2;write=true;break;
   case 0x2f0e16:address=r.rsi+8u;write=true;break;case 0x2f0e21:address=r.rsi;write=true;break;
   case 0x2f0e27:address=r.rax+8u;read=true;break;case 0x2f0e2a:address=r.rax;read=true;break;
   case 0x2f0e2c:original_payload=r.rsp+0x14u;address=r.rsp+0x20u;write=true;break;
   case 0x2f0e30:address=r.rsp+0x1cu;write=true;break;
   case 0x2f0e34:address=r.rdx+12u;read=true;break;case 0x2f0e37:address=r.rax+4u;read=true;break;case 0x2f0e3a:address=r.rbp;read=true;break;
   case 0x2f0e3d:address=r.rsp+0x18u;write=true;break;case 0x2f0e41:address=r.rsp+0x14u;write=true;break;
   case 0x2f0e45:address=r.rcx;read=true;break;
   case 0x2f0e53:address=r.rax+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f0e56;break;
   case 0x2f0e5a:case 0x2f0e5c:address=r.rcx;read=true;break;
   case 0x2f0e60:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f0e63;break;
   case 0x2f0e71:address=std::uint32_t(r.rax+r.rdi*4u+0x18cu);read=true;break;
   case 0x2f0e7f:call=true;target=r.rdi;return_pc=0x2f0e81;args=J::array({original.word(r.rsp),original.word(r.rsp+4u),original.word(r.rsp+8u),original.word(r.rsp+12u)});report["original_child_payload_bytes"]=bytes(original.read(original_payload,16));break;
   case 0x2f0e88:address=r.rsi+12u;write=true;break;case 0x2f0e8c:address=r.rsi+16u;write=true;break;case 0x2f1d19:address=r.rsi;write=true;break;
  }
  if(read)actual_reads.push_back(J::array({address,width}));
  if(call){const auto f=original.fp();original.step();original.go(return_pc);auto values=args;if(pc==0x2f0e7f)values[0]=normalized(values[0].get<std::uint32_t>(),original_payload);actual_calls.push_back(J{{"entry",target},{"this",receiver},{"arguments",values},{"returned",std::uint32_t(original.regs().rax)},{"before_fp",fp_words(f)},{"after_fp",fp_words(original.fp())}});}else original.step();
  if(write)actual_stores.push_back(J::array({normalized(address,original_payload),bytes(original.read(address,width))}));
 }
 const auto returned=std::uint32_t(original.regs().rax);const auto completed_fp=original.fp();for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());report["original_completed"]=true;
 Child native;native.start(argv[1],ordinal);for(const auto&bank:banks)native.write(bank.address,bank.before);auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto initial_fp=before;request(PTRACE_SETFPREGS,native.pid,nullptr,&initial_fp);Memory memory(native);Bridge dependencies{native,packet-0x1000u,before,report};NativeTu15WrappedContactServices services;unsigned classifications{};
 services.allocation.allocate=[&](const auto& request,auto&fp){return dependencies.call(request.owner,request.receiver,{request.bytes,request.memory_class},0x2f1ce6,fp);};
 services.allocation.release=[&](const auto& request,auto&fp){dependencies.call(request.owner,request.receiver,{request.data,request.bytes,request.memory_class},0x1ccaca,fp);};
 services.dispatch.classify_shape=[&](auto shape,auto owner,auto&fp){return dependencies.call(owner.identity,shape.identity,{},classifications++==0?0x2f0e56u:0x2f0e63u,fp);};
 services.dispatch.create_agent=[&](auto owner,auto first,auto second,auto settings,std::uint32_t data,auto&fp){report["native_child_payload_bytes"]=bytes(native.read(first.identity,16));return NativeTu15ContactAgent{dependencies.call(owner.identity,0,{first.identity,second.identity,settings.identity,data},0x2f0e81,fp)};};
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};const NativeTu15WrappedContactArguments arguments{[&]{return a;},[&]{return b;},[&]{return context;},[&]{return user_data;}};const auto result=native_tu15_create_wrapped_contact_agent(memory,arguments,{native_payload},fp,services);
 J native_stores=J::array(),native_calls=J::array();for(const auto&store:memory.stores)native_stores.push_back(J::array({normalized(store[0].get<std::uint32_t>(),native_payload),store[1]}));for(const auto&call:dependencies.calls){auto args=call.at("arguments");if(args.size()==4)args[0]=normalized(args[0].get<std::uint32_t>(),native_payload);native_calls.push_back(J{{"entry",call.at("entry")},{"this",call.at("this")},{"arguments",args},{"returned",call.at("returned_eax")},{"before_fp",call.at("before_fp")},{"after_fp",call.at("after_fp")}});}
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();check(packet-0x2000u>=private_first&&packet<private_end,"Declared scratch must fit existing private DATA lifetime");std::size_t compared{},different{},private_different{};J diffs=J::array();for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;const bool is_private=address>=private_first&&address<private_end;if(!is_private)++compared;if(actual[n]==bank.after[n])continue;if(is_private){++private_different;continue;}++different;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
 const bool reads_equal=memory.ledger==actual_reads,stores_equal=native_stores==actual_stores,calls_equal=native_calls==actual_calls,fp_equal=J::array({fp.control_word,fp.status,fp.mxcsr})==fp_words(completed_fp),payload_equal=report["native_child_payload_bytes"]==report["original_child_payload_bytes"];
 report.update(J{{"passed",different==0&&fp_equal&&result.identity==returned&&reads_equal&&stores_equal&&calls_equal&&payload_equal},{"source_entries",J::array({0x2f1cc0,0x2f0df0})},{"producer_ordinal",ordinal},{"reversed_payload_inputs",reverse},{"arguments",J::array({a,b,context,user_data})},{"returned_original_agent",returned},{"returned_native_agent",result.identity},{"fp_equal",fp_equal},{"ordered_reads_equal",reads_equal},{"ordered_stores_equal",stores_equal},{"service_calls_equal",calls_equal},{"child_payload_equal",payload_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},{"private_control_stack_different_bytes",private_different},{"first_differences",diffs},{"ordered_original_operand_reads",actual_reads},{"ordered_native_operand_reads",memory.ledger},{"ordered_original_stores",actual_stores},{"ordered_native_stores",native_stores},{"original_calls",actual_calls},{"native_calls",native_calls},{"original_owner_instructions",original_instructions},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
'''


def main(driver_source=DRIVER, source_scope='wrapped', variants_spec=None, extra_sources=(),
         implementation='src/physics/native_tu15_wrapped_contact_agent.cpp',
         public_header='include/halo2/native_tu15_wrapped_contact_agent.hpp',
         linked_sources=('src/physics/native_tu15_geometry.cpp',), case_inputs=(0,1), negative_case=0):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    source = ROOT/implementation
    bridge = ROOT/'tools/observe_native_contact_producer_tu15.cpp'
    files = [source, ROOT/public_header,
             ROOT/'src/physics/native_tu15_contact_agent_factory.cpp',
             ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',
             ROOT/'src/physics/native_tu15_geometry.cpp', ROOT/'include/halo2/native_tu15_geometry.hpp',
             bridge, Path(__file__), *(ROOT/path for path in linked_sources), *extra_sources]
    pins = {str(path.relative_to(ROOT)): sha(path) for path in files}
    try:
        assert sha(NEW_DEFAULT) == NEW_PIN
        prefix = bridge.read_text().split('int main(int argc,char**argv)')[0]
        prefix = prefix.replace('mutable std::size_t reads{};std::size_t writes{};',
                                'mutable std::size_t reads{};std::size_t writes{};mutable J ledger=J::array();J stores=J::array();')
        prefix = prefix.replace('++reads;}void write', '++reads;ledger.push_back(J::array({p,b.size()}));}void write')
        prefix = prefix.replace('child.write(p,b);++writes;', 'child.write(p,b);++writes;J bytes=J::array();for(auto v:b)bytes.push_back(std::to_integer<unsigned>(v));stores.push_back(J::array({p,bytes}));')
        assert prefix.count('stores.push_back') == 1
        driver = scratch/'wrapped.cpp'
        driver.write_text(f'#include "{public_header.removeprefix("include/")}"\n'+prefix+driver_source)
        base = ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
        cases, contexts = [], []
        for name, count in (('gemini-column0', 2), ('gemini-column1', 2), ('elongation-crates', 3)):
            capture = base/('iteration-04-'+name)/'original/required-failure-inputs'
            replay = scratch/('original-'+name)
            command = [sys.executable, str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),
                       '--native-frame-semantics', '--return-breakpoints', '--capture', str(capture), '--output', str(replay)]
            result = subprocess.run(command, text=True, capture_output=True, timeout=180)
            assert (replay/'original-hardware').exists(), result.stderr[-2000:]
            contexts.append(dict(name=name, capture=str(capture), capture_sha256=sha(capture/'capture.json'),
                                 reference_command=command, reference_exit_code=result.returncode))
            for ordinal in range(count):
                for reverse in case_inputs:
                    cases.append(dict(name=f'{name}_{ordinal}_{reverse}', elf=str(replay/'original-hardware'),
                                      capture=str(capture/'capture.json'), ordinal=ordinal, reverse=reverse))
        project = scratch/'project'
        project.mkdir()
        text = source.read_text()
        (project/'native.cpp').write_text(text)
        variants = {}
        boundary=text.index('NativeTu15ContactAgent native_tu15_create_transformed_contact_agent(') if source_scope in ('wrapped','transformed') else 0
        scope=text[:boundary] if source_scope=='wrapped' else text[boundary:]
        for name, old, new in variants_spec or (
            ('wrong_child', 'm.word(shape+12u)', 'm.word(shape+8u)'),
            ('wrong_stride', '(first_class<<5u)+second_class', '(first_class<<4u)+second_class'),
            ('missing_reversal', 'current{[&]{return second;},[&]{return first;}', 'current{[&]{return first;},[&]{return second;}'),
            ('wrong_vtable', '0x41caf0u', '0x41ca9cu'),
            ('wrong_size', 'agent.identity+4u,20u,2', 'agent.identity+4u,40u,2')):
            assert scope.count(old) == 1, old
            changed=scope.replace(old,new)
            variants[name] = changed+text[boundary:] if source_scope=='wrapped' else text[:boundary]+changed
            (project/(name+'.cpp')).write_text(variants[name])
        (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j["passed"] and j.get("original_completed"),(r.returncode,j)\nprint("Completed original wrapped factory rejects mutant")\n')
        cmake = f'''cmake_minimum_required(VERSION 3.20)
project(NativeWrappedContact LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
'''
        retain = []
        for variant in ('native', *variants):
            dependencies=' '.join(f'"{ROOT/path}"' for path in linked_sources)
            cmake += f'add_executable(wrapped_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp "{ROOT}/src/physics/native_tu15_contact_agent_factory.cpp" {dependencies})\ntarget_include_directories(wrapped_{variant} PRIVATE "{ROOT}/include" "{ROOT}/src/physics")\ntarget_compile_features(wrapped_{variant} PRIVATE cxx_std_20)\n'
            for case in cases if variant == 'native' else cases[negative_case:negative_case+1]:
                name = variant+'_'+case['name']
                path = 'evidence/'+name+'.json'
                retain.append(path)
                command = f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
                if variant != 'native':
                    command += f'"{sys.executable}" "{project}/negative.py" '
                command += f'$<TARGET_FILE:wrapped_{variant}> "{case["elf"]}" "{case["capture"]}" {case["ordinal"]} {case["reverse"]} "${{CMAKE_BINARY_DIR}}/{path}"'
                cmake += f'add_test(NAME wrapped_{name} COMMAND {command})\n'
        (project/'CMakeLists.txt').write_text(cmake)
        results = []
        for compiler in ('g++', 'clang++'):
            command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
                       '--scratch-root', str(scratch/'builds'), '--output', str(args.output/compiler),
                       '--target', 'wrapped_native', *sum((['--target', 'wrapped_'+name] for name in variants), []),
                       '--filter', '^wrapped_', '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
                       '--jobs', '1', '--test-jobs', '1', *sum((['--retain-report', path] for path in retain), [])]
            result = subprocess.run(command)
            results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
        report.update(passed=all(row['exit_code'] == 0 for row in results), contexts=contexts,
                      source_scope=source_scope,
                      compiler_results=results, physical_cases_per_compiler=len(cases), negative_controls=list(variants),
                      source_hashes=pins, current_sources_unchanged=all(sha(ROOT/path) == pin for path, pin in pins.items()),
                      image_sha256=NEW_PIN, before_inputs_only=True, supplied_original_outcomes=False,
                      instruction_patches=[], production_native_only_closure=False)
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(path.stat().st_blocks*512 for path in scratch.rglob('*') if path.is_file())
        save(args.output/'report.json', report)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
