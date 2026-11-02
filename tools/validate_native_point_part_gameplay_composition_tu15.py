#!/usr/bin/env python3
"""Full gameplay row removal continued through native PART/material/property.

Ordinary declared coherent gameplay contacts and borrowed body properties;
genuine positive material/no-conveyor path. No captured caller claim.
"""
import hashlib,inspect,json,os,struct,sys
from pathlib import Path
import validate_native_gameplay_contact_removal_tu15 as parent

def main():
 stage=Path(os.environ['HALO2_POINT_PART_STAGE']).resolve();header=stage/'staged/include/halo2/native_tu15_contact_point_binding.hpp';text=header.read_text();start=text.index('        gameplay.release_part_contact=');end=text.index('        NativeTu15IslandConstraintDetachServices detach;',start);callback=text[start:end]
 callback=callback.replace('gameplay.release_part_contact=','services.release_part_contact=')
 callback=callback.replace('auto& state){','auto& state){calls.push_back(J{{"owner",0x1e0a60u},{"slot",slot},{"part",std::uint32_t(signed_part)},{"component",component}});',1)
 original_cases=parent.cases
 def cases():
  rows=[r for r in original_cases()if not r.get('unbound_reference',False)]
  for row in rows:
   if row.get('stop_boundary')!=0x1e0a60:continue
   row.pop('stop_boundary');row.pop('required_owner');u=parent.U
   extra=[(parent.FIRST-u+0x38,0x80000003),(parent.PARTS-u+0x40,parent.FIRST),(parent.PARTS-u+0x44,0),
      (0x902c,u+0x9200),(0x9410,u+0x9600),(0x9608,0xffff0000)]
   for n in range(3):
    extra.extend([(parent.ROWS-u+n*72+0xc,0),(parent.ROWS-u+n*72+0x28,0x3f800000),(parent.ROWS-u+n*72+0x2c,0),(parent.ROWS-u+n*72+0x30,0),(parent.ROWS-u+n*72+0x44,0x20)])
   extra.append((0x9204,0));row['user_words']=[*row['user_words'],*extra]
  return rows
 parent.cases=cases;parent.OWNERS=(*parent.OWNERS,0x1e0a60,0x184c50,0x355d0,0x1d7660,0x31c450,0x31c5e0)
 source=inspect.getsource(parent.main)
 needle="  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')";assert source.count(needle)==1
 source=source.replace(needle,needle+'\n  shutil.copyfile(STAGED_HEADER,project/\'include/halo2/native_tu15_contact_point_binding.hpp\')')
 needle="native=('native_tu15_gameplay_contact_removal.cpp',";assert source.count(needle)==1
 source=source.replace(needle,"native=('native_tu15_array_growth.cpp','native_tu15_body_properties.cpp','native_tu15_empty_part_contact.cpp','native_tu15_part_contact_response.cpp','native_tu15_surface_conveyor.cpp',"+needle[len('native=('):])
 needle="*['src/physics/'+n for n in native],";assert source.count(needle)==1
 source=source.replace(needle,needle+"'src/core/native_tu15_function_evaluation.cpp','include/halo2/native_tu15_part_contact_binding.hpp','tools/validate_native_point_part_gameplay_composition_tu15.py',")
 needle="  observer=(ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()";assert source.count(needle)==1
 globals_setup='''  banks.update({0x4ecc10:struct.pack('<I',U+0x9000),0x4ecc18:struct.pack('<I',U+0x9400)})
  for p in(0x474d04,0x474d10,0x46a0ec,0x46a0d0,0x46a130,0x46a104):banks[p]=image.read(p,4)
  for p in(0x474d04,0x474d10):
   pointer=int.from_bytes(banks[p],'little');banks[pointer]=image.read(pointer,12)
'''
 source=source.replace(needle,globals_setup+needle)
 source=source.replace("driver='#include \"halo2/native_tu15_gameplay_contact_removal.hpp\"", "driver='#include \"halo2/native_tu15_contact_point_binding.hpp\"\\n#include \"halo2/native_tu15_gameplay_contact_removal.hpp\"")
 before='''services.release_part_contact=[&](auto slot,auto part,auto component,auto&){calls.push_back(J{{"owner",0x1e0a60u},{"slot",slot},{"part",std::bit_cast<std::uint32_t>(part)},{"component",component}});throw Required{};};''';assert source.count(before)==1
 setup=r'''NativeTu15ContactPointBindingActivity result;constexpr unsigned incoming_x87_depth=0;const NativeTu15ArrayGrowthServices allocation{};
      NativeTu15ContactPointBindingScratch scratch{0x6000080u,0x6000100u,0x6000120u,0x6000100u};
      const auto valid=[](std::uint32_t p,unsigned bytes){return p&&std::uint64_t(p)+bytes<=0x100000000ull;};
      const auto overlap=[](std::uint32_t a,unsigned an,std::uint32_t b,unsigned bn){return std::uint64_t(a)<std::uint64_t(b)+bn&&std::uint64_t(b)<std::uint64_t(a)+an;};
      const auto bad_profile=[&](const char*why){++result.unsupported_profiles;throw std::runtime_error(why);};
'''+callback
 source=source.replace(before,setup)
 # Rename only the old driver scalar result before the exact callback is added.
 needle="  replacement='''";assert source.count(needle)==1
 private_setup='''  driver=driver.replace('struct Fault{};struct Required{};', 'bool private_data(std::uint32_t p,std::size_t n){return p>=0x6000000u&&std::uint64_t(p-0x6000000u)+n<=0x1000;}struct Fault{};struct Required{};')
  driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
  driver=driver.replace('      J actual_stores=J::array(),calls=J::array();', '      memory.banks.emplace(0x6000000u,std::vector<std::byte>(0x1000,std::byte(0xa5)));J actual_stores=J::array(),calls=J::array();')
  driver=driver.replace('observe=[&](const auto&event){actual_stores.push_back', 'observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
'''
 source=source.replace(needle,private_setup+"  driver=driver.replace('std::uint32_t result{};','std::uint32_t result_word{};').replace('check(result==c.at(\"return_word\")','check(result_word==c.at(\"return_word\")');begin=driver.index('      NativeTu15ArrayGrowthMemory array');end=driver.index('      const auto host_sw=',begin)\n"+needle)
 source=source.replace("' '.join(native)+')", "' '.join(native)+' function_evaluation.cpp) ")
 source=source.replace('kernel=project/native[0];',"kernel=project/'native_tu15_gameplay_contact_removal.cpp';")
 source=source.replace('target_compile_features(remove PRIVATE cxx_std_20)\\n','target_compile_features(remove PRIVATE cxx_std_20)\\ntarget_compile_options(remove PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\\n')
 needle="  driver=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()";assert source.count(needle)==1
 source=source.replace(needle,"  shutil.copyfile(ROOT/'src/core/native_tu15_function_evaluation.cpp',project/'function_evaluation.cpp')\n"+needle)
 environment=dict(parent.__dict__,STAGED_HEADER=header)
 try:exec(source,environment);return environment['main']()
 finally:
  p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json'
  if p.exists():
   r=json.loads(p.read_text());r.update(staged_point_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),qualified_part_callback_sha256=hashlib.sha256(callback.encode()).hexdigest(),scope=__doc__,actual_captured_gameplay_caller_claimed=False,strict_source_private_raw_ledger_claimed=False);p.write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':raise SystemExit(main())
