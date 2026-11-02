#!/usr/bin/env python3
"""Native4/8-byte temporary constructors/destructors with native arena/pool."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:100];return s.replace(old,new,1)
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_build_actual_tu15.py';source=base.read_text().replace('native_tu15_island_partition_build','native_tu15_island_partition_temporary').replace('native_tu15_island_partition_graph','native_tu15_island_partition_allocator_binding').replace('observe_native_island_partition_build_actual_tu15.cpp','observe_native_island_partition_temporary_actual_tu15.cpp')
 source=once(source,"names=['island_partition_build','island_partition_graph']","extra_units=['array_allocator','array_growth','array_reserve','raw_allocator','stack_arena'];names=['island_partition_temporary','island_partition_allocator_binding',*extra_units]")
 source=once(source,'for owner in(0x37c2d0,0x37c460,0x37c3d0,0x37c190,0x37c250):','for owner in(0x2eba20,0x2eb850,0x3177e0,0x317870):')
 begin=source.index("     if target==0x2e31e0:");end=source.index('\n    metadata[str(pc)]=row',begin);source=source[:begin]+'''     if target is None:
      row.update(service_arguments=1 if owner in(0x2eba20,0x3177e0)else 3 if'0x14'in i.op_str else 2,ecx_consumed=True,result_consumed=owner in(0x2eba20,0x3177e0))'''+source[end:]
 source=once(source,"'#include \"halo2/native_tu15_island_partition_temporary.hpp\"\\n'+prefix", "'#include \"halo2/native_tu15_island_partition_temporary.hpp\"\\n#include \"halo2/native_tu15_island_partition_allocator_binding.hpp\"\\n#include \"halo2/native_tu15_array_reserve.hpp\"\\n'+prefix")
 source=once(source,"  variants={'wrong_build_flag':", "  for unit in extra_units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{unit}.cpp',project/(unit+'.cpp'))\n  variants={'wrong_build_flag':")
 begin=source.index("  variants={'wrong_build_flag':");end=source.index('\n  for name,s in variants.items():',begin);source=source[:begin]+'''  variants={'wrong_capacity':text.replace('pc.flags,p+8u,count|0x80000000u','pc.flags,p+8u,(count+1u)|0x80000000u'),'missing_zero_pop':text.replace('write(m,o,pc.destructor_cursor,arena+8u,cursor-bytes);','if(bytes)write(m,o,pc.destructor_cursor,arena+8u,cursor-bytes);')}'''+source[end:]
 source=source.replace('partition_wrong_build_flag','partition_wrong_capacity').replace('partition_wrong_group_size','partition_missing_zero_pop')
 source=once(source,"  for target in('driver','graph'):","  cmake+='add_library(children OBJECT array_allocator.cpp array_growth.cpp array_reserve.cpp raw_allocator.cpp stack_arena.cpp)\\ntarget_include_directories(children PRIVATE \\\"'+str(project/'include')+'\\\")\\ntarget_compile_features(children PRIVATE cxx_std_20)\\n'\n  for target in('driver','graph'):")
 source=once(source,'$<TARGET_OBJECTS:graph> {name}.cpp','$<TARGET_OBJECTS:graph> $<TARGET_OBJECTS:children> {name}.cpp');source=source.replace('remaining_allocator_children_execute_original_CPU=True','remaining_allocator_children_execute_original_CPU=False')
 source=once(source,"Path(__file__)];pins=","ROOT/'tools/validate_native_island_partition_build_actual_tu15.py',Path(__file__)];pins=")
 source=once(source,"scope='Complete259-site graph build/group extraction/destruction plus134-site native union/compression; declared topology, actual09 allocator backing and genuine typed original allocator children only'","scope='Entire202-site4/8-byte temporary ctor/dtor family with native stack arena/array/pool backing; actual09 input plus declared counts, zero-free-space and owned growth profiles; no native original CPU children'")
 source=once(source,"remaining=['Native raw149BF0/149C40 routes and actual allocation/free children','Native actual321EB0 edge collection and temp arena lifetime','Complete482-site3187C0 parent and genuine live active positive repartition']","remaining=['Native321EB0/full3187C0 composition and actual live caller','Additional exhausted/fault/unwind/alias allocation profiles']")
 namespace={'__file__':__file__,'__name__':'partition_temp_base'};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
