#!/usr/bin/env python3
"""Native convex-agent factory from actual04 payload/shape DATA.

Normal original ABI is declared. Native matrix/cache/support scratch is owned
DATA with independent initial poison, never a saved CPU frame or after-image.
All captured banks, FP and persistent ordered operands/stores are compared;
private helper DATA was qualified separately by inverse/cache/shape proofs.
"""
import inspect,os
import validate_native_activation_collector_actual_tu15 as collector

def main():
 assert os.environ.get('HALO2_TEST_SCRATCH')
 source=inspect.getsource(collector.main)
 begin=source.index(' child.OWNERS=');end=source.index('\n image=',begin);source=source[:begin]+''' child.OWNERS=(0x2e4eb0,0x2f8300,0x2e4550,0x309060,0x309150,0x2e4690,0x2e47b0,0x2e5da0,0x3077c0,0x2e7e10,0x306750,0x2e5db0,0x3077f0,0x1f6610,0x306790,0x2e7d90,0x1cb030,0xa9520,0x31dd70,0xadb50,0x149da0,0x149e00,0x233250,0x233420)
'''+source[end:]
 begin=source.index(' child.cases=lambda:');end=source.index('\n source=inspect.getsource(child.main)',begin);source=source[:begin]+''' first=word(0x10082380+0x1c)+0xc;second=word(0x10082380+0x20)+0xc
 child.cases=lambda:[dict(name=f'actual04_factory_{n}',entry=0x2e4eb0,receiver=0,arguments=[a,b,0,0x10082380],user_words=[])for n,(a,b)in enumerate(((first,second),(second,first)))]
'''+source[end:]
 needle=" env=dict(child.__dict__);env['main']=None;";assert source.count(needle)==1
 extra='''
 old='observer=(ROOT/"tools/observe_native_array_growth_tu15.cpp").read_text();needle='
 old=old.replace('"',chr(39))
 new=old.replace(';needle=', '.replace('+repr('"Original array escaped authenticated complete bodies"')+','+repr('("Original array escaped authenticated complete bodies pc="+std::to_string(pc)).c_str()')+');needle=')
 assert source.count(old)==1;source=source.replace(old,new)
 additions=['native_tu15_convex_agent_factory.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_inverse_geometry.cpp','native_tu15_contact_cache.cpp','native_tu15_contact_shape.cpp','native_tu15_geometry.cpp','native_tu15_activation_services.cpp']
 source=source.replace("'tools/validate_native_activation_collector_actual_tu15.py'", "'tools/validate_native_activation_collector_actual_tu15.py','tools/validate_native_convex_agent_factory_actual_tu15.py','include/halo2/native_tu15_convex_agent_factory.hpp',"+','.join(repr('src/physics/'+p)for p in additions))
 source=source.replace("'native_tu15_island_activation.cpp'):shutil.copyfile", "'native_tu15_island_activation.cpp',"+','.join(repr(p)for p in additions)+"):shutil.copyfile")
 source=source.replace('native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp)', 'native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp '+' '.join(additions)+')')
 # UI belongs only to this native probe. No actual captured bank is excluded.
 source=source.replace("save(fixture/'original.json'", "[c.update(banks=[b for b in c['banks']if b['address']!=base.UI])for c in all_cases];save(fixture/'original.json'")
 needle="(project/'driver.cpp').write_text(driver)"
 prefix=chr(10).join(['#include "halo2/native_tu15_convex_agent_factory.hpp"','#include "halo2/native_tu15_contact_shape.hpp"','#include "halo2/native_tu15_activation_services.hpp"'])+chr(10)
 ui='      memory.banks.emplace(0x4000000u,std::vector<std::byte>(0x10000,std::byte(name.back()==48?0xa5:0x5a)));'+chr(10)+'      J actual_stores='
 event='actual_stores.push_back(J{{"instruction",event.instruction}'
 replacement="driver="+repr(prefix)+"+driver;driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x4000000u&&p<0x4010000u))reads.push_back').replace('      J actual_stores=',"+repr(ui)+").replace("+repr(event)+","+repr('if(event.address>=0x4000000u&&event.address<0x4010000u)return;'+event)+");"+needle
 assert source.count(needle)==1;source=source.replace(needle,replacement)
 # Source private control is never supplied to the kernel. Matrix/cache DATA
 # has native-only ownership and independent poison in the declared UI bank.
 setup=''' + repr('''NativeTu15ConvexAgentServices convex;convex.allocation=growth;
      convex.shape_class=[&](auto,auto owner,auto&){if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();const auto type=native_tu15_activation_type(owner.identity);check(type.has_value(),"Unqualified actual shape class owner "+std::to_string(owner.identity));return *type;};
      convex.cache.shape_center=[&](auto shape,auto owner,auto out,auto&){native_tu15_contact_shape_center(memory,shape,owner,out,observe);};
      convex.cache.support_vertex=[&](auto shape,auto owner,auto direction,auto out,auto&state){const NativeTu15ContactShapeScratch child{0x4002500u,0x4002600u,nullptr};native_tu15_contact_shape_support(memory,shape,owner,direction,out,{0x4002300u,0x4002400u,&child},state,0,observe);};
      convex.cache.vertex_encoding=[&](auto,auto owner,auto&){check(owner.identity==0x1cb030u,"Unqualified actual vertex encoding");return native_tu15_contact_shape_vertex_encoding();};
''') + '''
 source=source.replace('NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;',setup+'NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;')
 dispatch='if(entry==0x2e4eb0u){const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();result=native_tu15_create_convex_contact_agent(memory,{[&]{return args[0];},[&]{return args[1];},[&]{return args[3];}},{0x4002000u,{0x4002100u}},fp,convex,observe).identity;}else '
 source=source.replace('if(entry==0x31f660u)native_tu15_destroy_activation_collector',dispatch+'if(entry==0x31f660u)native_tu15_destroy_activation_collector')
 source=source.replace('if(c.at("entry")==0x2f8090u||', 'if(c.at("entry")==0x2e4eb0u||c.at("entry")==0x2f8090u||')
 # This proof is independent of the collector branches in the reused driver.
 source=source.replace('check(collector_stats.collector_destructions==1u&&collector_stats.unregistrations==2u&&collector_stats.deferred_references==2u, name+" positive actual collector lifetime not reached");', '')
 source=source.replace("target=project/'native_tu15_activation_contact_manager.cpp'", "target=project/'native_tu15_convex_agent_factory.cpp'")
 begin=source.index("mutants=[('collector_vtable'");end=source.index('\\n  for name,before,after in mutants:',begin)
 source=source[:begin]+"mutants=[('derived_vtable','0x41c370u','0x41c371u'),('empty_count','m.word(0x2f838cu,p+0x18u,0u)','m.word(0x2f838cu,p+0x18u,1u)'),('size_word','agent.identity+4u,256u,2','agent.identity+4u,255u,2')]"+source[end:]
 source=source.replace('target_compile_features(cleanup PRIVATE cxx_std_20)', 'target_compile_features(cleanup PRIVATE cxx_std_20)'+chr(92)+'n'+'target_compile_options(cleanup PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)')
'''
 source=source.replace(needle,extra+needle)
 env=dict(collector.__dict__);env['main']=None;exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
