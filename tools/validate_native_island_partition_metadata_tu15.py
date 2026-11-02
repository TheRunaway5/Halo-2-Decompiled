#!/usr/bin/env python3
"""Original island counter callbacks, with explicit initialized info DATA."""
from pathlib import Path

def once(s,old,new):assert s.count(old)==1,old[:100];return s.replace(old,new,1)
def main():
 root=Path(__file__).resolve().parents[1];base=root/'tools/validate_native_island_partition_graph_tu15.py';source=base.read_text()
 source=source.replace('native_tu15_island_partition_graph','native_tu15_island_partition_metadata').replace('observe_native_island_partition_graph_tu15.cpp','observe_native_island_partition_metadata_tu15.cpp')
 source=once(source,'for owner in(0x37c170,0x37c190,0x37c250):','for owner in(0x3200b0,0x320100,0x320130,0x323420):')
 begin=source.index('  cases=[dict(');end=source.index(";save(scratch/'cases.json'",begin)
 fixture='''  cases=[]
  profiles=[([4,10,20,30,40],[7,8,9,10]),([0x7fffffff,0xffffffff,0x80000000,0x7fffffff,0xffffffff],[0x80000000,0xffffffff,0x80000000,1]),([0x80000000,0,1,2,3],[0xffffffff,0,0xffffffff,0x80000000]),([0,0,0,0,0],[0,0,0,0]),([0xffffffff,8,16,32,64],[0x7fffffff,0x7fffffff,3,5])]
  for n,(words,info)in enumerate(profiles):
   for owner in(0x3200b0,0x320100,0x320130):cases.append(dict(name=f'initialized-info-{hex(owner)}-{n}',owner=owner,parents=[],edges=[],island_words=words,info=info,weight=[0,0x80,0xffffffff,0x80000000,0x7fffffff][n]))
  cases.append(dict(name='actual323420-constant-weight',owner=0x323420,parents=[],edges=[],island_words=[0]*5,info=[0]*4,weight=0))'''
 source=source[:begin]+fixture+source[end:]
 begin=source.index("  variants={'wrong_union_root':");end=source.index('\n  for name,s in variants.items():',begin)
 source=source[:begin]+'''  variants={'wrong_signed_maximum':text.replace('std::bit_cast<std::int32_t>(a)<=std::bit_cast<std::int32_t>(b)','a<=b'),'wrong_removal_sign':text.replace('old_rows-rows','old_rows+rows')}'''+source[end:]
 source=source.replace('graph_wrong_union_root','graph_wrong_signed_maximum').replace('graph_missing_edge_compression','graph_wrong_removal_sign').replace("scope='Pure140-site graph helper frontier on declared valid forests; no live3187C0 branch or original caller captured'", "scope='Pure48-site island metadata frontier on explicitly initialized declared16-byte info DATA; no captured contact-parent ABI, live3187C0 branch, padding or original outcome supplied'")
 source=source.replace('Path(__file__)];pins=',"ROOT/'tools/validate_native_island_partition_graph_tu15.py',Path(__file__)];pins=")
 driver=(root/'tools/observe_native_island_partition_graph_tu15.cpp').read_text();driver=driver.replace('NativeTu15IslandPartitionGraphStats activity','NativeTu15IslandPartitionMetadataStats activity')
 needle='  original.write(bank,before_bytes);native.write(bank,before_bytes);'
 driver=once(driver,needle,'''  constexpr std::uint32_t info=bank+0x300u;
  constexpr std::array<unsigned,5>offsets{8,12,16,20,24};
  for(unsigned n=0;n<offsets.size();++n)put_bank(graph+offsets[n],c.at("island_words")[n]);
  for(unsigned n=0;n<4;++n)put_bank(info+n*4u,c.at("info")[n]);
'''+needle)
 start=driver.index('std::vector<std::uint32_t>arguments;');end=driver.index('\n  auto entry=',start)
 driver=driver[:start]+'''std::vector<std::uint32_t>arguments;if(owner==0x3200b0u||owner==0x320100u)arguments={bank+0x888u,info};else if(owner==0x320130u)arguments={c.at("weight")};'''+driver[end:]
 start=driver.index('  if(owner==0x37c170u)result=');end=driver.index('\n  const auto source_after=',start)
 driver=driver[:start]+'''  if(owner==0x3200b0u)native_tu15_add_island_constraint_info(memory,{graph},{info},{},&activity);
  else if(owner==0x320100u)native_tu15_remove_island_constraint_info(memory,{graph},{info},{},&activity);
  else if(owner==0x320130u)native_tu15_add_island_partition_weight(memory,{graph},arguments[0],{},&activity);
  else if(owner==0x323420u)result=native_tu15_default_motion_partition_weight();
  else throw std::runtime_error("Unqualified metadata helper owner");'''+driver[end:]
 driver=driver.replace('(owner==0x37c250u||std::uint32_t(original.regs().rax)==result)','(owner!=0x323420u||std::uint32_t(original.regs().rax)==result)').replace('activity.parent_publications','activity.additions+activity.removals+activity.weight_updates').replace('"37C2D0 graph build and37C460 group allocation/lifetime composition"','"Actual caller16-byte info initialization/provenance and publication escape"')
 observer=root/'tools/observe_native_island_partition_metadata_tu15.cpp'
 if not observer.exists():observer.write_text(driver)
 else:assert observer.read_text()==driver,'Staged metadata driver changed; do not overwrite qualified source'
 namespace={'__file__':__file__,'__name__':'metadata_base'};exec(compile(source,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
