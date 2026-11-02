#!/usr/bin/env python3
"""Native actual-primary raw routing/header/pool composition, unchanged original."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:120];return s.replace(old,new,1)
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_build_actual_tu15.py';source=base.read_text()
 source=source.replace('native_tu15_island_partition_build','native_tu15_raw_allocator').replace('native_tu15_island_partition_graph','native_tu15_array_allocator').replace('observe_native_island_partition_build_actual_tu15.cpp','observe_native_raw_allocator_actual_tu15.cpp')
 source=once(source,"names=['island_partition_build','island_partition_graph']","names=['raw_allocator','array_allocator']")
 source=once(source,'for owner in(0x37c2d0,0x37c460,0x37c3d0,0x37c190,0x37c250):','for owner in(0x149bf0,0x149c40,0x233160,0x2331a0):')
 begin=source.index("     if target==0x2e31e0:");end=source.index('\n    metadata[str(pc)]=row',begin)
 source=source[:begin]+'''     if owner==0x233160:row.update(service_arguments=2,ecx_consumed=True,result_consumed=True)
     elif owner==0x2331a0:row.update(service_arguments=3,ecx_consumed=True,result_consumed=False)
     elif target==0x149930:row.update(service_arguments=0,ecx_consumed=False,result_consumed=True)'''+source[end:]
 begin=source.index("  variants={'wrong_build_flag':");end=source.index('\n  for name,s in variants.items():',begin)
 source=source[:begin]+'''  variants={'wrong_allocation_tag':text.replace('block,0x02345656u','block,0x02345657u'),'wrong_release_tag':text.replace('block,0xdeadbeefu','block,0xdeadbef0u')}'''+source[end:]
 source=source.replace('partition_wrong_build_flag','partition_wrong_allocation_tag').replace('partition_wrong_group_size','partition_wrong_release_tag').replace('remaining_allocator_children_execute_original_CPU=True','remaining_allocator_children_execute_original_CPU=False')
 source=once(source,"scope='Complete259-site graph build/group extraction/destruction plus134-site native union/compression; declared topology, actual09 allocator backing and genuine typed original allocator children only'","scope='Complete114-site raw allocation/release/tagged-header family composed with qualified native233250/233420 actual primary pool; actual09 heap backing and declared byte/class inputs; no native original CPU children'")
 source=once(source,"remaining=['Native raw149BF0/149C40 routes and actual allocation/free children','Native actual321EB0 edge collection and temp arena lifetime','Complete482-site3187C0 parent and genuine live active positive repartition']","remaining=['Fixed/secondary232AA0/232AC0 ownership and lazy default heap','Large/exhausted pool fault/alias profiles','Whole native group and482-site parent/live composition']")
 source=once(source,"Path(__file__)];pins=","ROOT/'tools/validate_native_island_partition_build_actual_tu15.py',Path(__file__)];pins=")
 namespace={'__file__':__file__,'__name__':'raw_allocator_base'};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
