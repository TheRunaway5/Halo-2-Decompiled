#!/usr/bin/env python3
"""Qualify the reusable explicit native graph allocator binding coherently."""
import inspect
import validate_native_island_partition_native_children_tu15 as base

def binding_fragment(fragment):
 result=base_fragment(fragment);start=result.index('  ActivationMemory heap_memory(');end=result.index('  const auto words=',start)
 result=result[:start]+'''  ActivationMemory heap_memory(native,0u,0xffffffffu);NativeTu15IslandPartitionAllocatorActivity allocation_activity;
  NativeTu15IslandPartitionAllocatorBinding allocation_binding(heap_memory,{},&allocation_activity);
  auto bound=allocation_binding.graph_services();auto&unbound=allocation_activity.unbound_services;
  auto&pool_allocations=allocation_activity.pool_allocations;auto&pool_releases=allocation_activity.pool_releases;
  auto&raw_activity=allocation_activity.raw;
'''+result[end:]
 start=result.index('  NativeTu15IslandPartitionBuildServices services;');end=result.index('  J phase_reports=',start)
 result=result[:start]+'''  NativeTu15IslandPartitionBuildServices services;
  services.reserve_array=[&](auto header,auto capacity,auto stride,auto&context){const auto start=words();bound.reserve_array(header,capacity,stride,context);record(0x2e31e0u,0,J::array({header.identity,capacity,stride}),start,0);};
  services.allocate_group=[&](auto allocator,auto owner,auto bytes,auto memory_class,auto&context){const auto start=words();const auto data=bound.allocate_group(allocator,owner,bytes,memory_class,context);record(owner.identity,allocator,J::array({bytes,memory_class}),start,data);return data;};
  services.release_group=[&](auto allocator,auto owner,auto data,auto&context){const auto start=words();bound.release_group(allocator,owner,data,context);record(owner.identity,allocator,J::array({data}),start,0);};
  services.release_array=[&](const auto&r,auto&context){const auto start=words();bound.release_array(r,context);record(r.owner,r.receiver,J::array({r.data,r.bytes,r.memory_class}),start,0);};
'''+result[end:]
 return '#include "halo2/native_tu15_island_partition_allocator_binding.hpp"\n'+result
base_fragment=base.native_fragment

def main():
 source=inspect.getsource(base.main);needle="units=['array_allocator','array_growth','array_reserve','raw_allocator']";assert source.count(needle)==1;source=source.replace(needle,"units=['array_allocator','array_growth','array_reserve','raw_allocator','island_partition_allocator_binding','stack_arena']")
 scope=dict(base.__dict__,__file__=__file__,native_fragment=binding_fragment,main=None);exec(source,scope);return scope['main']()
if __name__=='__main__':raise SystemExit(main())
