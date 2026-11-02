#!/usr/bin/env python3
"""Qualify native3201B0 with native325E90/320100 on actual1470 BEFORE DATA."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:100];return s.replace(old,new,1)
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_build_actual_tu15.py';source=base.read_text()
 source=source.replace('native_tu15_island_partition_build','native_tu15_island_constraint_detach').replace('native_tu15_island_partition_graph','native_tu15_island_partition_metadata').replace('observe_native_island_partition_build_actual_tu15.cpp','observe_native_island_constraint_detach_actual_tu15.cpp')
 source=once(source,'from build_native_tu15_constraint_before_reference import BeforeInput,build_constraint_reference,sha,save','from build_native_tu15_contact_point_before_reference import PointBeforeInput,build_constraint_reference,sha,save')
 source=once(source,'from validate_native_world_island_merge_stage_tu15 import ARCHIVE,PIN',"ARCHIVE=Path(__file__).resolve().parents[1]/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-merge-point-original-v1470-20261101/iteration-00-native-world-merge-manager-factory-cleanup-vehicle-camera/required-native-contact-point-before/actual-native-before.tar.zst'\nPIN='e234b565b08de52ca00ffd8027b0778dbcc1984659b211f265363e8635c9171f'")
 source=once(source,"names=['island_partition_build','island_partition_graph']","extra_units=['contact_collector_info','island_activation'];names=['island_constraint_detach','island_partition_metadata',*extra_units]")
 source=once(source,'source=BeforeInput(ARCHIVE,PIN', 'source=PointBeforeInput(ARCHIVE,PIN')
 source=once(source,"reference['entry']='Declared graph build/extract/destruction formals, original actual09 allocator BEFORE backing; no live partition topology or caller captured'", "reference['entry']='Declared3201B0 ECXactualisland, stack4actualcollector; authentic1470 point BEFORE DATA, not captured detachment caller/aftercount0 phase'")
 source=once(source,'for owner in(0x37c2d0,0x37c460,0x37c3d0,0x37c190,0x37c250):','for owner in(0x3201b0,0x325e90,0x320100,0x283450):')
 begin=source.index("     if target==0x2e31e0:");end=source.index('\n    metadata[str(pc)]=row',begin);source=source[:begin]+'''     if pc==0x320249:row.update(service_arguments=1,ecx_consumed=True,result_consumed=False)'''+source[end:]
 source=once(source,"  variants={'wrong_build_flag':", "  for unit in extra_units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{unit}.cpp',project/(unit+'.cpp'))\n  variants={'wrong_build_flag':")
 begin=source.index("  variants={'wrong_build_flag':");end=source.index('\n  for name,s in variants.items():',begin);source=source[:begin]+'''  variants={'wrong_partition_flag':text.replace('island.identity+0x28u,1u,1','island.identity+0x28u,0u,1'),'missing_reference_release':text.replace('read(constraint+6u,2)-1u','read(constraint+6u,2)-0u')}'''+source[end:]
 source=source.replace('partition_wrong_build_flag','partition_wrong_partition_flag').replace('partition_wrong_group_size','partition_missing_reference_release')
 source=once(source,"  for target in('driver','graph'):","  cmake+='add_library(children OBJECT contact_collector_info.cpp island_activation.cpp)\\ntarget_include_directories(children PRIVATE \\\"'+str(project/'include')+'\\\")\\ntarget_compile_features(children PRIVATE cxx_std_20)\\n'\n  for target in('driver','graph'):")
 source=once(source,'$<TARGET_OBJECTS:graph> {name}.cpp','$<TARGET_OBJECTS:graph> $<TARGET_OBJECTS:children> {name}.cpp');source=source.replace('remaining_allocator_children_execute_original_CPU=True','remaining_allocator_children_execute_original_CPU=False').replace('declared_graphs=True','declared_formal_detach=True')
 source=once(source,"Path(__file__)];pins=","ROOT/'tools/build_native_tu15_contact_point_before_reference.py',ROOT/'tools/validate_native_island_partition_build_actual_tu15.py',Path(__file__)];pins=")
 source=once(source,"scope='Complete259-site graph build/group extraction/destruction plus134-site native union/compression; declared topology, actual09 allocator backing and genuine typed original allocator children only'","scope='Complete3201B0 +native325E90 +native320100 on unmodified genuine1470 pointcount1/ref2/island-owned collector BEFORE; formal detachment not actual point-parent aftercount0 invocation'")
 source=once(source,"remaining=['Native raw149BF0/149C40 routes and actual allocation/free children','Native actual321EB0 edge collection and temp arena lifetime','Complete482-site3187C0 parent and genuine live active positive repartition']","remaining=['Full point-parent aftercount0 composition','Zero-reference3261D0 and other getter profiles','Positive multi-entry/fault/alias and live delivery']")
 namespace={'__file__':__file__,'__name__':'detach_actual_base'};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
