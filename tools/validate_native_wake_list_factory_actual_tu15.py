#!/usr/bin/env python3
"""ActualWake2F8030 vs the exact frozen World native factory lambda."""
import inspect,os,hashlib
from pathlib import Path
import validate_native_wrapped_factory_actual_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 assert os.environ.get('HALO2_TEST_SCRATCH')
 source_root=Path(os.environ['HALO2_NATIVE_WAKE_FACTORY_SOURCE_ROOT']).resolve()
 overlay=Path(os.environ['HALO2_NATIVE_ACTIVATION_FACTORY_SERVICES_STAGE']).resolve()
 world_path=source_root/'src/physics/native_tu15_world_integration_residuals.cpp'
 if os.environ.get('HALO2_NATIVE_WAKE_FACTORY_WORLD_OVERLAY'):
  world_path=Path(os.environ['HALO2_NATIVE_WAKE_FACTORY_WORLD_OVERLAY']).resolve()/'staged/src/physics/native_tu15_world_integration_residuals.cpp'
 world=world_path.read_text()
 start=world.index('    create.agents.classify_shape=');end=world.index('    create.agents.motion_kind=',start)
 factory=world[start:end]
 scope=''' NativeTu15WorldIntegrationStats factory_stats;auto*stats=&factory_stats;auto&m=memory;
      const NativeTu15PhysicsFrameScratch scratch{PRIVATE,65536};const std::uint64_t end=std::uint64_t(PRIVATE)+65536;
      struct{unsigned x87_depth{};}state;NativeTu15PairHandlerServices create;auto&pair_storage=growth;
      const auto unqualified_pair_service=[](const std::string&why){throw std::runtime_error(why);};
'''+factory
 scope+='''      NativeTu15ObjectActivationNativeServices native_factory;
      native_factory.create_agent=[&](auto owner,auto first,auto second,auto context,auto manager,auto&fp,unsigned depth){check(depth==state.x87_depth,"Factory supplier lost actual depth");return create.agents.create_agent(owner,first,second,context,manager,fp);};
'''
 code=inspect.getsource(parent.main)
 code=code.replace("typed['owner']==0x2f1cc0", "typed['owner']==0x2f8030")
 code=code.replace("entry=typed['owner'],receiver=0,", "entry=typed['owner'],receiver=typed.get('actual_activation_call',{}).get('receiver',typed['arguments'][2]),")
 code=code.replace("'actual_elongation_wrapped_'", "'actual_wake_list_factory_'")
 anchor="namespace=dict(base.__dict__,OWNERS=OWNERS);exec(meta_source,namespace)"
 assert code.count(anchor)==1
 dedup="    unique={}\n    for row in sites:\n        address=row['instruction'];assert address not in unique or unique[address]==row;unique[address]=row\n    sites=list(unique.values())\n    return dict(sites=sites"
 code=code.replace(anchor,"meta_source=meta_source.replace('    return dict(sites=sites',"+repr(dedup)+");"+anchor)
 anchor="namespace=dict(base.__dict__,OWNERS=OWNERS);exec(meta_source,namespace)"
 code=code.replace(anchor,"meta_source=meta_source.replace('            sites.append(dict(instruction=pc,operands=operands))','            if pc in (0x18502e,0x185035):operands=[]\\n            sites.append(dict(instruction=pc,operands=operands))');"+anchor)
 code=code.replace('root cdBody','root cdBody')
 extra_owners=(0x2f8030,0x2f7d20,0x3061f0,0x306220,0x1cbe70,0x185020,0x2e98d0,
  0x2e31e0,0x2e3250,0x2e3270,0x2e2e30,0x2f81f0,0x2f3670,0x2f5530,0x303c10,0x3040d0,0x2f3620)
 owners=tuple(dict.fromkeys((*parent.OWNERS,*extra_owners)))
 units=tuple(dict.fromkeys((*parent.UNITS,*('native_tu15_'+name+'.cpp'for name in(
  'array_reserve','list_contact_agent','collection_contact_agent','collection_filter',
  'reversed_transformed_contact_agent')))))
 headers=''.join('#include "halo2/native_tu15_'+name+'.hpp"\n'for name in(
  'world_integration','physics_frame_residuals','pair_handlers','list_contact_agent','collection_contact_agent','collection_filter',
  'wrapped_contact_agent','convex_agent_binding','contact_agent_factory_input_observer','object_activation_residuals'))
 utility='''
static std::uint32_t word(const halo2::SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t out{};for(unsigned i=0;i<n;++i)out|=std::to_integer<unsigned>(b[i])<<(8u*i);return out;}
static void put(halo2::SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8u*i));m.write(p,b);}
'''
 transform='driver='+repr(headers)+'+driver;driver=driver.replace("int main(int argc,char**argv)",'+repr(utility)+'+"int main(int argc,char**argv)");'
 transform+='driver=driver.replace("4096","65536").replace("NativeTu15WrappedFactoryBindingActivity binding;",'+repr(scope)+');'
 old='result=native_tu15_create_bound_wrapped_contact_factory(memory,{0x2f1cc0u},input,{PRIVATE,4096},fp,0,growth,observe,&binding).identity;'
 # The private extent has been expanded by the preceding transformation.
 old=old.replace('4096','65536')
 new='result=native_factory.create_agent({0x2f8030u},{args[0]},{args[1]},{args[2]},{args[3]},fp,0).identity;'
 transform+='assert driver.count('+repr(old)+')==1;driver=driver.replace('+repr(old)+','+repr(new)+');'
 prefix='check(!failed&&!required&&result==c.at("return_word")&&'
 suffix=',"Actual native wrapped factory return/activity differs");'
 transform+='start_check=driver.index('+repr(prefix)+');end_check=driver.index('+repr(suffix)+',start_check);driver=driver[:start_check]+'+repr(prefix+'factory_stats.native_list_agent_factory_calls>0&&factory_stats.native_list_agent_factory_refusals==0')+'+driver[end_check:];'
 transform+='shutil.copyfile(Path('+repr(str(overlay/'staged/include/halo2/native_tu15_object_activation_residuals.hpp'))+'),project/"include/halo2/native_tu15_object_activation_residuals.hpp");'
 anchor="(project/'driver.cpp').write_text(driver)";assert code.count(anchor)==1;code=code.replace(anchor,transform+anchor)
 start=code.index("  kernel=project/");end=code.index('  frozen=a.output/',start)
 code=code[:start]+"  mutants=()\n"+code[end:]
 code=code.replace("report.update(","report.update(world_factory_source_root="+repr(str(source_root))+",world_factory_cpp_source="+repr(str(world_path))+",world_factory_lambda_sha256="+repr(hashlib.sha256(factory.encode()).hexdigest())+",actual_wake_factory=True,source_code_dispatch_fetch_operand_exclusions=[0x18502e,0x185035]," )
 env=dict(parent.__dict__);env.update(OWNERS=owners,UNITS=units,main=None);exec(code,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
