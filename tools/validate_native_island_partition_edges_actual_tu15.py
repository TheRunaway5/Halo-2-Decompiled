#!/usr/bin/env python3
"""Source321EB0 actual09 read operands with native reserve/pool composition."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:100];return s.replace(old,new,1)
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_build_actual_tu15.py';source=base.read_text().replace('native_tu15_island_partition_build','native_tu15_island_partition_edges').replace('native_tu15_island_partition_graph','native_tu15_island_partition_allocator_binding').replace('observe_native_island_partition_build_actual_tu15.cpp','observe_native_island_partition_edges_actual_tu15.cpp')
 source=once(source,"names=['island_partition_build','island_partition_graph']","extra_units=['array_allocator','array_growth','array_reserve','raw_allocator','stack_arena'];names=['island_partition_edges','island_partition_allocator_binding',*extra_units]")
 source=once(source,'for owner in(0x37c2d0,0x37c460,0x37c3d0,0x37c190,0x37c250):','for owner in(0x321eb0,):')
 begin=source.index("     if target==0x2e31e0:");end=source.index('\n    metadata[str(pc)]=row',begin);source=source[:begin]+'''     if target==0x2e31e0:row.update(service_arguments=3,ecx_consumed=False,result_consumed=False)
     elif target is None:row.update(service_arguments=3 if'0x14'in i.op_str else 1,ecx_consumed=True,result_consumed=False)'''+source[end:]
 source=once(source,"'#include \"halo2/native_tu15_island_partition_edges.hpp\"\\n'+prefix", "'#include \"halo2/native_tu15_island_partition_edges.hpp\"\\n#include \"halo2/native_tu15_island_partition_allocator_binding.hpp\"\\n'+prefix")
 keep='return p<private_first||std::uint64_t(p)+n>private_end;';replacement='return (p>=0x72001000u&&std::uint64_t(p)+n<=0x7200100cu)||(p>=0x72002000u&&std::uint64_t(p)+n<=0x72002080u)||p<private_first||std::uint64_t(p)+n>private_end;'
 source=once(source,"(project/'driver.cpp').write_text(","prefix=prefix.replace("+repr(keep)+","+repr(replacement)+");(project/'driver.cpp').write_text(")
 source=once(source,"  variants={'wrong_build_flag':", "  for unit in extra_units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{unit}.cpp',project/(unit+'.cpp'))\n  variants={'wrong_build_flag':")
 begin=source.index("  variants={'wrong_build_flag':");end=source.index('\n  for name,s in variants.items():',begin);source=source[:begin]+'''  variants={'wrong_constraint_index':text.replace('write(0x322059u,destination+4u,second_index);','write(0x322059u,destination+4u,second_index+1u);'),'wrong_static_contact_admission':text.replace('if(!read(first+0x40u,1)&&!read(second+0x40u,1))','if(true)')}'''+source[end:]
 source=source.replace('partition_wrong_build_flag','partition_wrong_constraint_index').replace('partition_wrong_group_size','partition_wrong_static_contact_admission')
 source=once(source,"  for target in('driver','graph'):","  cmake+='add_library(children OBJECT array_allocator.cpp array_growth.cpp array_reserve.cpp raw_allocator.cpp stack_arena.cpp)\\ntarget_include_directories(children PRIVATE \\\"'+str(project/'include')+'\\\")\\ntarget_compile_features(children PRIVATE cxx_std_20)\\n'\n  for target in('driver','graph'):")
 source=once(source,'$<TARGET_OBJECTS:graph> {name}.cpp','$<TARGET_OBJECTS:graph> $<TARGET_OBJECTS:children> {name}.cpp');source=source.replace('remaining_allocator_children_execute_original_CPU=True','remaining_allocator_children_execute_original_CPU=False')
 source=once(source,"Path(__file__)];pins=","ROOT/'tools/validate_native_island_partition_build_actual_tu15.py',Path(__file__)];pins=")
 source=once(source,"scope='Complete259-site graph build/group extraction/destruction plus134-site native union/compression; declared topology, actual09 allocator backing and genuine typed original allocator children only'","scope='Complete209-site321EB0 kernel with real actual09 island read operands and native array/pool service binding; formal active5/inactive0 selections and output capacities, actions0/contactdynamic0 remain explicit'")
 source=once(source,"remaining=['Native raw149BF0/149C40 routes and actual allocation/free children','Native actual321EB0 edge collection and temp arena lifetime','Complete482-site3187C0 parent and genuine live active positive repartition']","remaining=['Positive action enumeration and dynamic/dynamic contact cohort','Full3187C0 caller composition after genuine pending merges','Additional callback alias/fault/unwind and live delivery']")
 namespace={'__file__':__file__,'__name__':'partition_edges_base'};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
