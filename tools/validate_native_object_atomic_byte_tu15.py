#!/usr/bin/env python3
"""Physical TU1.5 B3858 byte-update slice and native retained owner checks."""
import argparse
import hashlib
import inspect
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

STAGE = ROOT / 'analysis/physics/native-object-byte-update-stage-20261103/staged'
PC = 0xB3858
BODY = base.UI + 0x1000
BYTE = BODY + 0xD8
OBSERVER = 'tools/observe_native_broad_phase_edges_tu15.cpp'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def cases():
    def row(name, byte, **fields):
        return dict(name=name, entry=PC, receiver=BODY, arguments=[],
                    user_words=[(BYTE - base.UI, 0x39C7E100 | byte)], **fields)
    rows = [row(f'byte_{value:02x}', value) for value in range(256)]
    for value in (0x00, 0x01, 0x81, 0xFF):
        for write in (False, True):
            rows.append(row(f'{"write" if write else "read"}_rejection_{value:02x}', value,
                            fault=dict(address=BYTE, bytes=1, occurrence=1, write=write)))
        rows.append(row(f'published_then_observer_stops_{value:02x}', value,
                        after_store=dict(instruction=PC, occurrence=1)))
    rows.append(row('observer_mutates_committed_byte', 0x81,
                    mutations=[dict(instruction=PC, address=BYTE, written='a7')]))
    return rows


def original(image, rows, scratch):
    base.OWNERS = (PC,)
    text = inspect.getsource(base.original)
    text = text.replace('tools/observe_native_array_growth_tu15.cpp', OBSERVER)
    text = text.replace("('fault','mutations')", "('fault','mutations','after_store')")
    # Invoke the authenticated AND plus its real epilogue, with the ordinary
    # body address in ESI and the existing epilogue's RET target in place.
    marker = "struct.pack_into('<I',stack,SP-STACK,STOP)"
    assert text.count(marker) == 1
    text = text.replace(marker, "struct.pack_into('<I',stack,SP-STACK+0x34,STOP)")
    marker = " mov ${case['receiver']},%ecx"
    assert text.count(marker) == 1
    text = text.replace(marker, " mov ${case['receiver']},%esi")
    ns = dict(base.__dict__)
    exec(text, ns)
    return ns['original'](image, {}, rows, scratch)


def driver():
    prefix = (ROOT / 'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
    return '''#include "halo2/native_tu15_byte_update.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
''' + prefix + r'''
namespace {
constexpr std::uint32_t pc=0xb3858u,address=0x40010d8u;
struct PostStore{};
struct AtomicMemory final:Memory,NativeTu15ByteUpdateStorage {
 NativeTu15ByteUpdate update_byte(std::uint32_t p,std::uint8_t and_mask,std::uint8_t or_mask)override{
  // Whole-operand ownership and injected write rejection precede the read.
  const auto bank=at(p,1);
  if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&
    fault.at("bytes")==1u&&++fault_count==fault.at("occurrence"))throw Fault{};
  std::array<std::byte,1> logical{};read(p,logical);
  return native_tu15_update_byte_value(banks.at(bank->first)[p-bank->first],and_mask,or_mask);
 }
};
struct Unsupported final:SourcePhysicsContactMemory {
 mutable unsigned reads{};unsigned writes{};
 void read(std::uint32_t,std::span<std::byte>)const override{++reads;}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;}
};
void runtime_owner(){
 SourceRuntimeMemory bank;const std::array<std::byte,4> bytes{std::byte{0x81},std::byte{0x71},std::byte{0x43},std::byte{0x65}};
 bank.mount(address,bytes,SourceRuntimeMemory::Access::ReadWrite,"declared native byte owner");
 bank.mount(address+8u,bytes,SourceRuntimeMemory::Access::ReadOnly,"declared read-only byte owner");
 unsigned guards{},writes{},events{};bool deny=true;int guard_owner{},audit_owner{};
 source_runtime_register_write_guard(bank,&guard_owner,[&](auto p,auto n){
  ++guards;check(p==address&&n==1,"Unexpected runtime guard operand");
  check(bank.view(p,1)[0]==std::byte{0x81},"Guard observed premature byte update");
  if(deny)throw Fault{};
 });
 source_runtime_register_write_observer(bank,&audit_owner,[&](auto p,auto n){
  ++writes;check(p==address&&n==1&&bank.view(p,1)[0]==std::byte{0x81},"Runtime write audit phase differs");
 });
 SourcePhysicsContactRecordObserver observer=[&](const auto&e){++events;
  check(e.instruction==pc&&e.address==address&&hex(e.before)=="81"&&hex(e.written)=="80"&&
    bank.view(address,1)[0]==std::byte{0x80},"Native byte event did not follow committed DATA");};
 bool failed{};try{native_tu15_update_retained_byte(bank,pc,address,0xfeu,0u,observer);}catch(const Fault&){failed=true;}
 check(failed&&guards==1&&!writes&&!events&&bank.view(address,4)[0]==std::byte{0x81},"Runtime rejected write changed DATA/events");
 deny=false;native_tu15_update_retained_byte(bank,pc,address,0xfeu,0u,observer);
 check(guards==2&&writes==1&&events==1&&bank.view(address,4)[1]==bytes[1],"Runtime permission/audit count or untouched byte differs");
 failed=false;try{native_tu15_update_retained_byte(bank,pc,address+8u,0xfeu,0u,observer);}catch(const std::runtime_error&){failed=true;}
 check(failed&&guards==2&&writes==1&&events==1&&bank.view(address+8u,4)[0]==std::byte{0x81},"Readonly owner changed DATA/events");
 source_runtime_unregister_write_observer(bank,&audit_owner);source_runtime_unregister_write_guard(bank,&guard_owner);
 Unsupported unsupported;failed=false;try{native_tu15_update_retained_byte(unsupported,pc,address,0xfeu,0u,observer);}catch(const std::runtime_error&e){failed=std::string(e.what()).find("atomic storage owner")!=std::string::npos;}
 check(failed&&!unsupported.reads&&!unsupported.writes&&events==1,"Absent atomic owner read, wrote, or fell back");
}
}
int main(int argc,char**argv)try{
 check(argc==2,"Supply the authenticated B3858 fixture");const auto path=std::filesystem::path(argv[1]);std::ifstream in(path);const auto fixture=J::parse(in);
 check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong Source authority");
 unsigned complete{},faults{},post_stops{},wrong_mask_banks{},split_read_mismatches{};
 for(const auto&c:fixture.at("cases"))for(unsigned mode=0;mode<3;++mode){
  if(mode==1&&(c.contains("fault")||c.contains("after_store")||c.contains("mutations")))continue;
  if(mode==2&&(!c.contains("fault")||!c.at("fault").at("write").get<bool>()))continue;
  AtomicMemory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));
  memory.fault=c.value("fault",J{});memory.log_reads=true;J stores=J::array();unsigned mutations{};
  SourcePhysicsContactRecordObserver observer=[&](const auto&e){
   stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
   if(c.contains("after_store"))throw PostStore{};
   for(const auto&m:c.value("mutations",J::array()))if(m.at("instruction")==e.instruction){const auto bytes=unhex(m.at("written"));memory.write(m.at("address"),bytes);++mutations;}
  };
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{},post{};
  try{
   if(mode==2){std::array<std::byte,1> early{};memory.read(address,early);}
   native_tu15_update_retained_byte(memory,pc,address,mode==1?0xfcu:0xfeu,0u,observer);
  }catch(const Fault&){failed=true;}catch(const PostStore&){post=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.at("declared_fault")&&post==c.at("poststore_stop"),name+" byte completion differs");
  check(sw==status()&&cw==control()&&mx==_mm_getcsr()&&c.at("before_fp")==c.at("after_fp"),name+" byte update changed FP");
  J reads=J::array();for(const auto&e:c.at("reads"))reads.push_back(J::array({e.at("address"),e.at("bytes")}));
  bool bank_difference{};for(const auto&b:c.at("banks"))bank_difference|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());
  if(mode==1){wrong_mask_banks+=bank_difference;continue;}
  if(mode==2){check(!bank_difference&&stores==c.at("writes")&&memory.reads.size()==reads.size()+1u,"Split-read control did not isolate prewrite read");++split_read_mismatches;continue;}
  check(!bank_difference&&memory.reads==reads&&stores==c.at("writes")&&mutations==c.value("mutations",J::array()).size(),name+" B3858 DATA/read/event phase differs");
  complete+=!failed&&!post;faults+=failed;post_stops+=post;
 }
 check(complete==257u&&faults==8u&&post_stops==4u&&wrong_mask_banks==128u&&split_read_mismatches==4u,"Incomplete B3858 discriminator");
 runtime_owner();std::filesystem::create_directories("evidence");std::ofstream out("evidence/byte.json");
 out<<J{{"passed",true},{"full_returns",complete},{"faults",faults},{"post_store_stops",post_stops},{"wrong_mask_banks",wrong_mask_banks},{"split_read_fault_differences",split_read_mismatches},{"runtime_owner_checks",4}}.dump()<<'\n';check(bool(out),"Cannot retain B3858 proof");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    files = [p for p in STAGE.rglob('*') if p.is_file()]
    proof_tools = [Path(__file__), ROOT / OBSERVER, ROOT / 'tools/validate_native_array_growth_tu15.py', ROOT / 'tests/native_tu15_array_growth_tests.cpp']
    pins = {str(p.relative_to(ROOT)): sha(p) for p in (*files, *proof_tools)}
    report = dict(passed=False, owned_scratch=str(scratch), source_hashes=pins, retained_large_artifacts=[], supplied_original_outcomes=False)
    try:
        assert sha(NEW_DEFAULT) == NEW_PIN
        image = Xbe(NEW_DEFAULT)
        instruction = _Code(image, '.text').at(PC)
        assert instruction.mnemonic == 'and' and instruction.op_str == 'byte ptr [esi + 0xd8], 0xfe'
        rows = cases()
        fixture, authority = original(image, rows, scratch)
        save(fixture / 'original.json', dict(image_sha256=NEW_PIN, cases=rows))
        project = scratch / 'project'
        project.mkdir()
        shutil.copytree(ROOT / 'include', project / 'include')
        for p in files:
            rel = p.relative_to(STAGE)
            target = project / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(p, target)
        (project / 'driver.cpp').write_text(driver())
        (project / 'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(nativeByte LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_executable(native_byte driver.cpp src/physics/native_tu15_byte_update.cpp src/runtime/source_runtime_memory.cpp)
target_include_directories(native_byte PRIVATE include src)
target_compile_features(native_byte PRIVATE cxx_std_20)
target_compile_options(native_byte PRIVATE -ffunction-sections -fdata-sections)
target_link_options(native_byte PRIVATE -Wl,--gc-sections)
target_link_libraries(native_byte PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME native_byte COMMAND native_byte "{fixture / 'original.json'}")
''')
        compiled = {str(p.relative_to(project)): sha(p) for p in project.rglob('*') if p.is_file()}
        for cc in ('g++', 'clang++'):
            cmd = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / cc), '--target', 'native_byte', '--filter', '^native_byte$', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + cc, '--jobs', '2', '--test-jobs', '1', '--retain-report', 'evidence/byte.json']
            result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            (args.output / (cc + '-console.log')).write_text(result.stdout[-64000:])
            report.setdefault('results', []).append(dict(compiler=cc, exit_code=result.returncode, command=cmd))
            assert result.returncode == 0, result.stdout[-6000:]
        assert all(sha(ROOT / p) == h for p, h in pins.items())
        assert all(sha(project / p) == h for p, h in compiled.items())
        report.update(passed=True, authority=authority, original_sha256=NEW_PIN, instruction_pc=PC, instruction_bytes=instruction.bytes.hex(), profiles=len(rows), full_returns=sum(not c['declared_fault'] and not c['poststore_stop'] for c in rows), fault_prefixes=sum(c['declared_fault'] for c in rows), post_store_prefixes=sum(c['poststore_stop'] for c in rows), ordered_reads=sum(len(c['reads']) for c in rows), ordered_stores=sum(len(c['writes']) for c in rows), compiled_source_pins=compiled, source_inputs_unchanged=True, scope='Unmodified B3858 plus its original epilogue, exact full retained DATA/FP/ordered operand and event phases. SourceRuntimeMemory policy checks are separate declared native storage controls; full caller/canonical/live admission still separate.')
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file())
        save(args.output / 'report.json', report)


if __name__ == '__main__':
    main()
