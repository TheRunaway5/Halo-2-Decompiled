#!/usr/bin/env python3
"""Coherent graph sequence with native raw/header/array/pool children only."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:100];return s.replace(old,new,1)
def native_fragment(fragment):
 begin=fragment.index('  const auto call=[&]');end=fragment.index('  J phase_reports=',begin)
 block='''  ActivationMemory heap_memory(native,0u,0xffffffffu);unsigned unbound{};std::uint64_t pool_allocations{},pool_releases{};
  NativeTu15RawAllocatorStats raw_activity;NativeTu15ArrayAllocatorMemory allocator(heap_memory);
  NativeTu15ArrayAllocatorServices heap;
  heap.allocate_owner=[&](const auto&r,auto&){if(r.owner!=0x233250u){++unbound;throw std::runtime_error("Unqualified native partition pool allocation");}NativeTu15PoolMemory pool(heap_memory,r.receiver);++pool_allocations;return native_tu15_pool_allocate(pool,r.bytes);};
  heap.release_owner=[&](const auto&r,auto&){if(r.owner!=0x233420u){++unbound;throw std::runtime_error("Unqualified native partition pool release");}NativeTu15PoolMemory pool(heap_memory,r.receiver);if(!native_tu15_pool_release(pool,r.data,r.bytes)){++unbound;throw std::runtime_error("Native partition requires a large pool release owner");}++pool_releases;};
  heap.default_heap=[&](auto&)->std::uint32_t{++unbound;throw std::runtime_error("Native partition requires a lazy default heap");};
  NativeTu15ArrayGrowthServices arrays;
  arrays.allocate=[&](const auto&r,auto&context){if(r.owner!=0x149da0u){++unbound;throw std::runtime_error("Unknown native partition array allocation route");}return native_tu15_route_array_allocation(allocator,r.bytes,r.memory_class,context,heap);};
  arrays.release=[&](const auto&r,auto&context){if(r.owner!=0x149e00u){++unbound;throw std::runtime_error("Unknown native partition array release route");}native_tu15_route_array_release(allocator,r.data,r.bytes,r.memory_class,context,heap);};
  NativeTu15ArrayGrowthServices tagged_arrays;tagged_arrays.allocate=heap.allocate_owner;tagged_arrays.release=heap.release_owner;
  NativeTu15RawAllocatorMemory raw_storage(heap_memory);NativeTu15RawAllocatorServices raw;
  raw.allocate_owner=[&](const auto&r,auto&context){if(r.owner!=0x233160u){++unbound;throw std::runtime_error("Unknown native partition raw block allocation");}return native_tu15_allocate_tagged_raw_block(heap_memory,r.receiver,r.bytes,r.memory_class,context,tagged_arrays,{},&raw_activity);};
  raw.release_owner=[&](const auto&r,auto&context){if(r.owner!=0x2331a0u){++unbound;throw std::runtime_error("Unknown native partition raw block release");}native_tu15_release_tagged_raw_block(heap_memory,r.receiver,r.data,context,tagged_arrays,{},&raw_activity);};
  raw.default_heap=heap.default_heap;
  const auto words=[&]{return J::array({fp.control_word,fp.status,fp.mxcsr});};
  const auto record=[&](std::uint32_t owner,std::uint32_t receiver,J args,J before_fp,std::uint32_t result){native_calls.push_back(J{{"entry",owner},{"this",receiver},{"arguments",args},{"before_fp",before_fp},{"after_fp",words()},{"returned_eax",result}});};
  NativeTu15IslandPartitionBuildServices services;
  services.reserve_array=[&](auto header,auto capacity,auto stride,auto&context){const auto start=words();native_tu15_reserve_array(heap_memory,header,capacity,stride,context,arrays);record(0x2e31e0u,0,J::array({header.identity,capacity,stride}),start,0);};
  services.allocate_group=[&](auto source_allocator,auto owner,auto bytes,auto memory_class,auto&context){if(owner.identity!=0x149bf0u){++unbound;throw std::runtime_error("Unknown native partition raw route");}const auto start=words();const auto data=native_tu15_route_raw_allocation(raw_storage,bytes,memory_class,context,raw,&raw_activity);record(owner.identity,source_allocator,J::array({bytes,memory_class}),start,data);return data;};
  services.release_group=[&](auto source_allocator,auto owner,auto data,auto&context){if(owner.identity!=0x149c40u){++unbound;throw std::runtime_error("Unknown native partition raw release route");}const auto start=words();native_tu15_route_raw_release(raw_storage,data,context,raw,&raw_activity);record(owner.identity,source_allocator,J::array({data}),start,0);};
  services.release_array=[&](const auto&r,auto&context){const auto start=words();arrays.release(r,context);record(r.owner,r.receiver,J::array({r.data,r.bytes,r.memory_class}),start,0);};
'''
 fragment=fragment[:begin]+block+fragment[end:]
 fragment=once(fragment,'passed&=different==0&&activity.unbound_services==0;','passed&=different==0&&activity.unbound_services==0&&!unbound&&!raw_activity.unbound_services;')
 fragment=once(fragment,'{"native_builds",activity.builds}','{"native_pool_allocations",pool_allocations},{"native_pool_releases",pool_releases},{"native_raw_allocations",raw_activity.allocations},{"native_raw_releases",raw_activity.releases},{"unbound",unbound},{"native_builds",activity.builds}')
 return fragment.replace('{"remaining_allocator_children_execute_original_CPU",true}','{"remaining_allocator_children_execute_original_CPU",false}')
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_build_actual_tu15.py';source=base.read_text();units=['array_allocator','array_growth','array_reserve','raw_allocator'];source=once(source,"names=['island_partition_build','island_partition_graph']","extra_units="+repr(units)+";names=['island_partition_build','island_partition_graph',*extra_units]")
 source=once(source,"Path(__file__)];pins=","ROOT/'tools/validate_native_island_partition_build_actual_tu15.py',Path(__file__)];pins=")
 source=once(source,"+(ROOT/'tools/observe_native_island_partition_build_actual_tu15.cpp').read_text())","+native_fragment((ROOT/'tools/observe_native_island_partition_build_actual_tu15.cpp').read_text()))")
 source=once(source,"'#include \"halo2/native_tu15_island_partition_build.hpp\"\\n'+prefix", "'#include \"halo2/native_tu15_island_partition_build.hpp\"\\n#include \"halo2/native_tu15_array_reserve.hpp\"\\n#include \"halo2/native_tu15_raw_allocator.hpp\"\\n'+prefix")
 source=once(source,"  variants={'wrong_build_flag':", "  for unit in extra_units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{unit}.cpp',project/(unit+'.cpp'))\n  variants={'wrong_build_flag':")
 source=once(source,"  for target in('driver','graph'):","  cmake+='add_library(children OBJECT "+' '.join(n+'.cpp'for n in units)+")\\ntarget_include_directories(children PRIVATE \\\"'+str(project/'include')+'\\\")\\ntarget_compile_features(children PRIVATE cxx_std_20)\\n'\n  for target in('driver','graph'):")
 source=once(source,'$<TARGET_OBJECTS:graph> {name}.cpp','$<TARGET_OBJECTS:graph> $<TARGET_OBJECTS:children> {name}.cpp')
 source=source.replace('remaining_allocator_children_execute_original_CPU=True','remaining_allocator_children_execute_original_CPU=False')
 source=once(source,"scope='Complete259-site graph build/group extraction/destruction plus134-site native union/compression; declared topology, actual09 allocator backing and genuine typed original allocator children only'","scope='Coherent native graph build/extraction/lifetime+union/compression+raw/header/array/pool sequence; actual09 allocator backing and declared topology; no remaining original CPU children'")
 source=once(source,"remaining=['Native raw149BF0/149C40 routes and actual allocation/free children','Native actual321EB0 edge collection and temp arena lifetime','Complete482-site3187C0 parent and genuine live active positive repartition']","remaining=['Positive fixed/secondary/lazy-default or large/exhausted heap profiles','Native321EB0 edge collector and temp index/edge arenas','Full482-site3187C0 parent and genuine positive active/live partition']")
 namespace={'__file__':__file__,'__name__':'native_partition_children','native_fragment':native_fragment};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
