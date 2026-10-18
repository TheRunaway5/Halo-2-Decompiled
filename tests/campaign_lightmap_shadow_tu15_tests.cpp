#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/simulation.hpp"
#include "halo2/original_data.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *reason){if(!value)throw FormatError(reason);}
template<class F>void rejects(F f,const char *reason){bool failed{};try{f();}catch(const std::exception &){failed=true;}check(failed,reason);}
constexpr std::uint32_t flag=0x515ef5u;
unsigned get(PhysicsLiveWorldTu15 &w){return std::to_integer<unsigned>(w.memory().view(flag,1)[0]);}
unsigned gate(PhysicsLiveWorldTu15 &w){return w.call_actual(0x335b0u)&255u;}
void only_flag(const SourceRuntimeMemory::Checkpoint &a,const SourceRuntimeMemory::Checkpoint &b,unsigned value){
 check(a.regions.size()==b.regions.size(),"Shadow setter changed bank topology");
 bool found{};
 for(std::size_t i=0;i<a.regions.size();++i){const auto &x=a.regions[i],&y=b.regions[i];
  check(x.address==y.address&&x.access==y.access&&x.bytes.size()==y.bytes.size(),"Shadow setter changed region ownership");
  for(std::size_t j=0;j<x.bytes.size();++j){if(std::uint64_t(x.address)+j==flag){found=true;check(y.bytes[j]==std::byte(value),"Shadow setter did not replace the complete source byte");}
   else check(x.bytes[j]==y.bytes[j],"Shadow setter clobbered another retained bank byte");}
 }
 check(found,"Shadow source byte absent from complete bank");
}
struct Program {
 ScriptProgram p;
 std::uint32_t node(ScriptNode n){const auto i=std::uint32_t(p.nodes.size());n.salt=std::uint16_t(0xe000+i);p.nodes.push_back(n);return (std::uint32_t(n.salt)<<16)|i;}
 std::uint32_t lit(unsigned bits){return node({0,8,8,9,null_datum,0,bits});}
 std::uint32_t global(){return node({0,8,8,13,null_datum,0,0});}
 std::uint32_t call(std::string name,std::vector<std::uint32_t> args={},unsigned type=4){const auto at=std::uint32_t(p.strings.size());p.strings+=name;p.strings+='\0';for(unsigned i=0;i<args.size();++i)p.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;const auto fn=node({0,0,2,9,args.empty()?null_datum:args[0],at,0});return node({0,0,std::uint16_t(type),8,null_datum,at,fn});}
 Program(bool wrong_arity=false){
  p.globals.push_back({"phase",8,lit(0)});
  if(wrong_arity){p.scripts.push_back({"lightmap_shadow",0,4,call("cinematic_lightmap_shadow_enable",{lit(1)})});return;}
  const auto root=call("begin",{call("cinematic_start"),call("set",{global(),lit(1)},8),
   call("cinematic_lightmap_shadow_enable"),call("set",{global(),lit(2)},8),
   call("cinematic_lightmap_shadow_disable"),call("set",{global(),lit(3)},8),
   call("cinematic_lightmap_shadow_enable"),call("set",{global(),lit(4)},8),
   call("cinematic_stop"),call("set",{global(),lit(5)},8)});
  p.scripts.push_back({"lightmap_shadow",0,4,root});
 }
};
}
int main(int argc,char **argv)try{
 if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
 if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
 auto content=cache.load_content(0,false);content.objects.clear();content.scripts=Program{}.p;
 auto source=std::make_shared<CampaignSourceRuntime>(cache,std::filesystem::path(argv[1]),0,1,0x12345678);
 auto &objects=source->objects();auto &world=objects.tu15_world();auto &memory=world.memory();
 check(get(world)==0&&gate(world)==1,"Ordinary campaign initial shadow admission differs from original335B0");
 unsigned writes{};const int observer_owner{};
 source_runtime_register_write_observer(memory,&observer_owner,[&](auto at,auto n){check(at==flag&&n==1,"Checked shadow setter wrote outside its one-byte source extent");++writes;});
 for(bool enabled:{false,true,true,false}){const auto before=memory.checkpoint();source_campaign_lightmap_shadow_tu15(world,enabled);only_flag(before,memory.checkpoint(),enabled);}
 source_runtime_unregister_write_observer(memory,&observer_owner);
 check(writes==4&&get(world)==0&&gate(world)==1,"Repeated inactive cinematic shadow requests differ");
 source_campaign_lightmap_shadow_tu15(world,true);
 check(gate(world)==1,"Noncinematic shadow admission was disabled by host state");
 source_campaign_cinematic_control_tu15(world,true);
 check(get(world)==0&&gate(world)==0,"Actual cinematic start did not reset and gate lightmap shadows");
 source_campaign_lightmap_shadow_tu15(world,true);check(get(world)==1&&gate(world)==1,"Active cinematic enable did not reach original335B0");
 const auto saved=objects.authority_checkpoint();
 source_campaign_lightmap_shadow_tu15(world,false);check(get(world)==0&&gate(world)==0,"Active cinematic disable did not reach original335B0");
 check(objects.commit_authority(objects.prepare_authority(saved)),"Source shadow checkpoint restore failed");
 check(get(world)==1&&gate(world)==1,"Source shadow byte was lost from complete checkpoint");
 source_campaign_cinematic_control_tu15(world,false);check(get(world)==0&&gate(world)==1,"Actual cinematic stop did not reset and leave ordinary shadows admitted");
 // The existing public writer must be recorded as two whole one-byte calls.
 const auto start=objects.authority_checkpoint();SourceExecutionInputs capture;capture.track_calls();
 {SourceExecutionInputBinding bound(memory,capture);source_campaign_lightmap_shadow_tu15(world,true);source_campaign_lightmap_shadow_tu15(world,false);}
 const auto inputs=capture.finish();const auto calls=capture.calls();
 check(inputs.empty()&&calls.size()==2,"Shadow setter fabricated external operands or omitted journal calls");
 for(unsigned n=0;n<2;++n){const auto &c=calls[n];check(c.kind==SourceExecutionCallKind::WriteSourceCallInput&&c.arguments.size()==9&&Bytes(c.arguments).u32(0)==flag&&Bytes(c.arguments).u32(4)==1&&c.arguments[8]==std::byte(n?0:1),"Shadow setter changed the existing exact caller-input transcript");}
 const auto completed=memory.checkpoint();
 check(objects.commit_authority(objects.prepare_authority(start)),"Shadow replay checkpoint reset failed");
 SourceExecutionInputs replay(inputs,calls);
 {SourceExecutionInputBinding bound(memory,replay);source_campaign_lightmap_shadow_tu15(world,true);source_campaign_lightmap_shadow_tu15(world,false);}
 replay.finish();check(memory.checkpoint()==completed,"Same recorded shadow calls did not reproduce the complete retained bank");
 SourceExecutionInputs wrong(inputs,calls);
 {SourceExecutionInputBinding bound(memory,wrong);rejects([&]{source_campaign_lightmap_shadow_tu15(world,false);},"Wrong recorded shadow value was admitted");}
 rejects([&]{wrong.finish();},"Mismatched shadow replay was published");
 check(get(world)==0,"Mismatched shadow replay mutated the source byte before admission");
 world.inspect_render_memory([&](const auto &,const auto &){rejects([&]{source_campaign_lightmap_shadow_tu15(world,true);},"Read-only renderer borrowed world accepted a shadow write");});
 check(get(world)==0,"Rejected render-borrow shadow control mutated source state");
 // The raw source-world restore above has no native Simulation borrower.
 // Once Simulation borrows the world, its owning checkpoint route also
 // restores/rebinds the native object-lifetime owner and every source view.
 Simulation simulation(content,0,cache.spawns().front(),{},{},{},{},{},{},source);
 source_campaign_lightmap_shadow_tu15(world,true);simulation.request_checkpoint();simulation.step({});
 check(simulation.has_checkpoint()&&get(world)==1,"Simulation did not save the retained source shadow byte");
 source_campaign_lightmap_shadow_tu15(world,false);simulation.request_revert();simulation.step({});
 check(get(world)==1,"Simulation checkpoint lost the source shadow flag");
 source_campaign_lightmap_shadow_tu15(world,false);
 std::vector<std::pair<unsigned,int>> seen;
 source_runtime_register_write_observer(memory,&observer_owner,[&](auto at,auto n){if(at==flag&&n==1)seen.emplace_back(get(world),int(simulation.scripts().globals().at(0).number()));});
 simulation.start_level();simulation.step({});source_runtime_unregister_write_observer(memory,&observer_owner);
 for(const auto &d:simulation.scripts().diagnostics())std::cerr<<"SCRIPT "<<d<<'\n';
 check(simulation.scripts().diagnostics().empty(),"Campaign lightmap shadow host was not dispatched");
 // Write observers run before publication, so record the prior byte. Phase
 // values prove each source write precedes the next VM expression.
 check(seen==std::vector<std::pair<unsigned,int>>{{0,0},{0,1},{1,2},{0,3},{1,4}}&&
  simulation.scripts().globals()[0].number()==5&&get(world)==0&&gate(world)==1,
  "Lightmap shadow host writes were deferred or escaped source start/stop ordering");
 content.scripts=Program(true).p;Simulation invalid(content,0,cache.spawns().front());
 invalid.start_level();const auto before_bad=get(world);invalid.step({});
 check(invalid.scripts().diagnostics().size()==1&&
  invalid.scripts().diagnostics().front()=="lightmap_shadow: builtin 'cinematic_lightmap_shadow_enable': invalid cinematic_lightmap_shadow_enable arity"&&
  get(world)==before_bad,"Invalid shadow host arity did not reject at the exact arity boundary");
 content.scripts=Program{}.p;Simulation missing(content,0,cache.spawns().front());missing.start_level();missing.step({});
 check(missing.scripts().diagnostics().size()==1&&
  missing.scripts().diagnostics().front()=="lightmap_shadow: builtin 'cinematic_lightmap_shadow_enable': Cinematic lightmap shadow control requires its retained TU1.5 world",
  "Missing campaign owner did not reject at the exact source-owner boundary");
 std::cout<<"PASS native campaign zero-argument enable/disable, exact one-byte bank writes, source start/stop and335B0 gating, checkpoint and checked-writer record/replay, active host ordering, read-only rejection and invalid arity/owner controls\n";
 std::cout<<"LIMIT source admission only; live campaign shadow submission, matched rendered shadow pixels and full cutscene parity remain open\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
