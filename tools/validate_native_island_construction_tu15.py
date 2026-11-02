#!/usr/bin/env python3
"""Source island constructor, exact initialized and preserved object DATA."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:100];return s.replace(old,new,1)
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_graph_tu15.py';source=base.read_text().replace('native_tu15_island_partition_graph','native_tu15_island_construction').replace('observe_native_island_partition_graph_tu15.cpp','observe_native_island_construction_tu15.cpp')
 source=once(source,'for owner in(0x37c170,0x37c190,0x37c250):','for owner in(0x3212c0,):');begin=source.index('  cases=[dict(');end=source.index(";save(scratch/'cases.json'",begin)
 source=source[:begin]+"  cases=[dict(name=f'declared-world-{hex(world)}',owner=0x3212c0,parents=[],edges=[],world=world)for world in[0,0x1003000,0x7fffffff,0xffffffff]]"+source[end:]
 begin=source.index("  variants={'wrong_union_root':");end=source.index('\n  for name,s in variants.items():',begin);source=source[:begin]+'''  variants={'wrong_inline_body':text.replace('p+0x3cu,p+0x48u','p+0x3cu,p+0x4cu'),'wrong_inline_capacity':text.replace('p+0x44u,0x80000001u','p+0x44u,0x80000000u')}'''+source[end:]
 source=source.replace('graph_wrong_union_root','graph_wrong_inline_body').replace('graph_missing_edge_compression','graph_wrong_inline_capacity').replace("scope='Pure140-site graph helper frontier on declared valid forests; no live3187C0 branch or original caller captured'", "scope='Entire49-site3212C0 constructor with declared world DATA; exact initialized and preserved allocation/ref/header bytes, not a captured allocated-island caller'")
 source=once(source,'Path(__file__)];pins=',"ROOT/'tools/validate_native_island_partition_graph_tu15.py',Path(__file__)];pins=")
 driver=(root/'tools/observe_native_island_partition_graph_tu15.cpp').read_text();begin=driver.index('std::vector<std::uint32_t>arguments;');end=driver.index('\n  auto entry=',begin);driver=driver[:begin]+'''std::vector<std::uint32_t>arguments{c.at("world")};'''+driver[end:]
 driver=once(driver,'NativeTu15IslandPartitionGraphStats activity;std::uint32_t result{};','std::uint32_t result{};');begin=driver.index('  if(owner==0x37c170u)result=');end=driver.index('\n  const auto source_after=',begin);driver=driver[:begin]+'''  check(owner==0x3212c0u,"Unqualified island constructor owner");result=native_tu15_construct_physics_island(memory,{graph},{c.at("world")}).identity;'''+driver[end:]
 driver=driver.replace('(owner==0x37c250u||std::uint32_t(original.regs().rax)==result)','(std::uint32_t(original.regs().rax)==result)').replace('{"native_word_publications",activity.parent_publications}','{"declared_world",c.at("world")}');observer=root/'tools/observe_native_island_construction_tu15.cpp'
 if not observer.exists():observer.write_text(driver)
 else:assert observer.read_text()==driver
 namespace={'__file__':__file__,'__name__':'island_constructor_base'};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
