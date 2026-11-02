#!/usr/bin/env python3
"""Coherent3190D0 native merge/contact/lifetime/allocator composition."""
from pathlib import Path
from validate_native_island_lifetime_actual_before_tu15 import once
def composed_fragment(fragment):
 source=Path(__file__).with_name('observe_native_island_contact_merge_actual_before_tu15.cpp').read_text()
 first=source.index('  NativeTu15ArrayAllocatorMemory allocator(heap_memory);');end=source.index('\n  native_tu15_merge_island_contact_records(',first)
 block=source[first:end].replace('services.','contact.').replace('record_call','child_record_call').replace(';J native_calls=J::array();',';')
 lines=block.splitlines();lines=[('  const auto child_record_call=[](std::uint32_t,std::uint32_t,J){};'if 'const auto child_record_call='in line else line)for line in lines];block='\n'.join(lines)
 first=fragment.index('  const auto invoke=');end=fragment.index('\n  const auto survivor=',first)
 replacement='''  ActivationMemory heap_memory(native,0u,0xffffffffu);NativeTu15IslandContactMergeServices contact;
  NativeTu15IslandContactMergeStats contacts;NativeTu15IslandLifetimeStats lifetime;unsigned unknown_activations{};
'''+block+'''
  const auto record=[&](std::uint32_t owner,std::uint32_t receiver,std::uint32_t esi,J args){native_calls.push_back(J{{"entry",owner},{"this",receiver},{"formal_ESI",esi},{"arguments",args},{"before_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"after_fp",J::array({fp.control_word,fp.status,fp.mxcsr})}});};
  services.activate_island=[&](auto,auto,auto&){++unknown_activations;throw std::runtime_error("Native merged sleeping island requires shared activation binding qualification");};
  services.merge_contact_records=[&](auto x,auto y,auto&state){record(0x318de0u,0,x.identity,J::array({y.identity}));native_tu15_merge_island_contact_records(heap_memory,x,y,{{0x72001000u}},state,contact,{},&contacts);};
  services.reserve_array=[&](auto header,auto capacity,auto stride,auto&state){record(0x2e31e0u,0,0,J::array({header.identity,capacity,stride}));native_tu15_reserve_array(heap_memory,header,capacity,stride,state,backing);};
  services.destroy_island=[&](auto island,auto owner,auto disposing,auto&state){check(owner.identity==0x322100u,"Unknown merged island destructor");record(owner.identity,island.identity,0,J::array({disposing}));(void)native_tu15_destroy_physics_island(heap_memory,island,disposing,state,contact.lifetime,{},&lifetime);};'''
 fragment=fragment[:first]+replacement+fragment[end:]
 fragment=once(fragment,'different==0&&live&&service_differences.empty()','different==0&&live&&service_differences.empty()&&!unbound_heap&&!unbound_duplicates&&!unknown_activations')
 fragment=once(fragment,'{"calls",stats.calls}','{"native_contact_copies",contacts.temporary_copies},{"native_contact_comparisons",contacts.comparisons},{"native_contact_duplicates",contacts.duplicates},{"native_temporary_constructors",contacts.lifetime.temporary_constructors},{"native_temporary_destructors",contacts.lifetime.temporary_destructors},{"native_island_owned_releases",lifetime.owned_array_releases},{"native_island_disposals",lifetime.island_disposals},{"unknown_activations",unknown_activations},{"unbound_heap",unbound_heap},{"unbound_duplicates",unbound_duplicates},{"calls",stats.calls}')
 return fragment.replace('{"remaining_typed_children_execute_original_CPU",true}','{"remaining_typed_children_execute_original_CPU",false}')
def main():
 base=Path(__file__).with_name('validate_native_island_merge_actual_before_tu15.py');text=base.read_text()
 units=['island_contact_merge','island_lifetime','array_allocator','array_growth','array_reserve','stack_arena']
 text=once(text,"files=[kernel,header,observer,Path(__file__)","extra_units="+repr(units)+";files=[*[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in extra_units],*[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in extra_units],ROOT/'tools/observe_native_island_contact_merge_actual_before_tu15.cpp',ROOT/'tools/validate_native_island_merge_actual_before_tu15.py',kernel,header,observer,Path(__file__)")
 text=once(text,'+observer.read_text())','+composed_fragment(observer.read_text()))')
 text=once(text,"project=scratch/'project';project.mkdir();","project=scratch/'project';project.mkdir();\n  for name in extra_units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))\n  ")
 text=once(text,'  for variant in [',"  cmake+='add_library(children OBJECT "+' '.join(n+'.cpp'for n in units)+")\\ntarget_include_directories(children PRIVATE \\\"'+str(ROOT/'include')+'\\\")\\ntarget_compile_features(children PRIVATE cxx_std_20)\\n'\n  for variant in [")
 text=once(text,'$<TARGET_OBJECTS:driver> {cpp}','$<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:children> {cpp}')
 text=once(text,'driver.write_text(',"driver.write_text('#include \\\"halo2/native_tu15_island_contact_merge.hpp\\\"\\n#include \\\"halo2/native_tu15_array_allocator.hpp\\\"\\n#include \\\"halo2/native_tu15_stack_arena.hpp\\\"\\n'+")
 text=once(text,'remaining_typed_children_execute_original_CPU=True','remaining_typed_children_execute_original_CPU=False')
 text=once(text,"remaining=['Native318DE0 contact-record merge and its lifetime helpers','Native322100/3213A0 island destructor','Compose native reserve/activation service binding','Live World selection and positive wake/listener']","remaining=['Positive duplicate cleanup and sleeping-island shared activation composition','Live World selection and positive wake/listener','Unwind/fault/alias and additional action/record rows']")
 namespace={'__file__':__file__,'__name__':'native_island_merge_native_children_base','composed_fragment':composed_fragment}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
