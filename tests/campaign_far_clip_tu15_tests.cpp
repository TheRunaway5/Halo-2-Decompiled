#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/simulation.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <iostream>
using namespace halo2;
#if __has_include("fixtures/retail_cinematic_far_clip_tu15.inc")
namespace {
#include "fixtures/retail_cinematic_far_clip_tu15.inc"
void check(bool value,const char *reason){if(!value)throw FormatError(reason);}
template<class F>void rejects(F f,const char *reason){bool failed{};try{f();}catch(const std::exception &){failed=true;}check(failed,reason);}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t at){return Bytes(m.view(at,4)).u32(0);}
void put(SourceRuntimeMemory &m,std::uint32_t at,std::uint32_t v){m.write(at,std::as_bytes(std::span(&v,1)));}
void only_word(const SourceRuntimeMemory::Checkpoint &a,const SourceRuntimeMemory::Checkpoint &b,std::uint32_t at,std::uint32_t value){
 check(a.regions.size()==b.regions.size(),"Far setter changed bank topology");bool found{};
 for(std::size_t i=0;i<a.regions.size();++i){const auto &x=a.regions[i],&y=b.regions[i];check(x.address==y.address&&x.access==y.access&&x.bytes.size()==y.bytes.size(),"Far setter changed region ownership");
  for(std::size_t j=0;j<x.bytes.size();++j){const auto p=std::uint64_t(x.address)+j;if(p>=at&&p<at+4u){found=true;check(y.bytes[j]==std::byte(value>>((p-at)*8)),"Far setter changed original REAL bits");}else check(x.bytes[j]==y.bytes[j],"Far setter clobbered another retained byte");}}
 check(found,"Far setter destination missing from complete bank");
}
void pure(){
 SourceRuntimeMemory m;
 const std::array<std::uint32_t,2> defaults{0x3c800000u,0x44800000u};
 m.mount(0x473574u,std::as_bytes(std::span(defaults)),SourceRuntimeMemory::Access::ReadOnly,"literal TU1.5 configured plane inputs");
 m.mount(0x515ec0u,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,"declared interface pointer input");
 constexpr std::uint32_t owner=0x90010000u;
 m.mount(owner,std::vector<std::byte>(0x3fc,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared interface bank input");
 for(const auto &c:original_clip_cases){put(m,0x515ec0u,c.owner?owner:0);put(m,owner+0x190u,c.near_word);put(m,owner+0x194u,c.far_word);const auto v=source_campaign_clip_planes_tu15(m);
  check(std::bit_cast<std::uint32_t>(v.x)==c.expected_near&&std::bit_cast<std::uint32_t>(v.y)==c.expected_far,"Configured near/far selection differs from original12B0B");}
 for(const auto &c:original_far_cases){put(m,0x515ec0u,c.owner?owner:0);put(m,owner+0x194u,0xa5a5a5a5u);const auto before=m.checkpoint();unsigned writes{};
  source_campaign_far_clip_tu15(m,[&](auto at,auto bytes){check(at==owner+0x194u&&bytes.size()==4,"Far setter did not publish the exact original word");++writes;m.write(at,bytes);},std::bit_cast<float>(c.word));
  check(writes==unsigned(c.owner),"Null interface changed completion or write admission");if(c.owner)only_word(before,m.checkpoint(),owner+0x194u,c.word);else check(before==m.checkpoint(),"Null interface setter mutated retained state");}
 put(m,0x515ec0u,0);source_campaign_far_clip_tu15(m,{},2048.f);
 put(m,0x515ec0u,owner);rejects([&]{source_campaign_far_clip_tu15(m,{},2048.f);},"Non-null far setter silently omitted its checked writer");
 std::cout<<"PASS72 exact original consumer combinations and16 whole original HS-dispatch store cases; signed-zero/NaN/infinity and null owner retained\n";
}
ScriptProgram one_call(std::string name,std::vector<ScriptValue> args={}){
 ScriptProgram p;p.strings=name+'\0';
 p.nodes.push_back({0xe000,0,4,8,null_datum,0,0xe0010001u});
 p.nodes.push_back({0xe001,0,2,9,args.empty()?null_datum:0xe0020002u,0,0});
 for(unsigned i=0;i<args.size();++i)p.nodes.push_back({std::uint16_t(0xe002+i),args[i].type,args[i].type,9,i+1<args.size()?(std::uint32_t(0xe003+i)<<16)|(i+3):null_datum,0,args[i].bits});
 p.scripts.push_back({"far_clip_bad",0,4,0xe0000000u});return p;
}
}
int main(int argc,char **argv)try{
 pure();if(argc!=3)throw FormatError("Supply TU15_XBE FLOODZONE_MAP");
 if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));auto content=cache.load_content(0,false);
 check(cache.name()=="06b_floodzone","Authored far-clip fixture needs original Floodzone");
 const auto &call=content.scripts.nodes.at(0x68b),&fn=content.scripts.nodes.at(0x68c),&literal=content.scripts.nodes.at(0x68d);
 check(call.salt==0xe9fe&&call.opcode==711&&call.value==0xe9ff068cu&&fn.salt==0xe9ff&&fn.next==0xea00068du&&literal.salt==0xea00&&literal.type==6&&literal.value==0x45000000u,"Actual authored far-clip node identity differs");
 // Isolate exactly the authenticated call/name/literal records. Free pool
 // rows keep salt0; only the outer sibling link is terminated for this test.
 ScriptProgram isolated;isolated.strings=content.scripts.strings;isolated.nodes.resize(0x68e);
 isolated.nodes[0x68b]=call;isolated.nodes[0x68c]=fn;isolated.nodes[0x68d]=literal;
 isolated.nodes[0x68b].next=null_datum;
 isolated.scripts={{"far_clip_authored",0,4,0xe9fe068bu}};
 content.objects.clear();content.scripts=std::move(isolated);
 {
  // Cache OriginalData authenticates assets. This overload does not supply an
  // executable to the native world factory, and no player/Simulation is born.
  CampaignSourceRuntime cold(cache,0,1,0x12345678);auto &cold_world=cold.objects().tu15_world();auto &cold_memory=cold_world.memory();
  check(word(cold_memory,0x473574u)==0x3c800000u&&word(cold_memory,0x473578u)==0x44800000u,
      "Native-cold factory lacks exact original configured plane operands");
  const auto initial=source_campaign_clip_planes_tu15(cold_world);
  check(initial.x==.015625f&&initial.y==1024.f&&word(cold_memory,0x4922f8u)==0x44800000u,
      "Native-cold default configured publication differs");
  source_campaign_far_clip_tu15(cold_world,2048.f);const auto updated=source_campaign_clip_planes_tu15(cold_world);
  check(updated.x==.015625f&&updated.y==2048.f&&word(cold_memory,0x4922f8u)==0x45000000u,
      "Native-cold original far override did not reach configured publication");
  std::cout<<"PASS ordinary native-cold factory without world-XBE path: exact defaults/publication and2048 override; no Simulation/player birth\n";
 }
 auto source=std::make_shared<CampaignSourceRuntime>(cache,std::filesystem::path(argv[1]),0,1,0x12345678);auto &objects=source->objects();auto &world=objects.tu15_world();auto &m=world.memory();
 const auto bank=word(m,0x515ec0u);check(bank&&word(m,bank+0x194u)==0,"Actual interface construction did not clear far override");
 for(auto bits:{0x45000000u,0x80000000u,0x7fc12345u,0x7f800000u}){const auto before=m.checkpoint();source_campaign_far_clip_tu15(world,std::bit_cast<float>(bits));only_word(before,m.checkpoint(),bank+0x194u,bits);}
 source_campaign_far_clip_tu15(world,2048.f);const auto configured=source_campaign_clip_planes_tu15(world);
 check(std::bit_cast<std::uint32_t>(configured.x)==0x3c800000u&&std::bit_cast<std::uint32_t>(configured.y)==0x45000000u&&word(m,0x4922f4u)==0x3c800000u&&word(m,0x4922f8u)==0x45000000u,"Actual configured far override was ignored by the source consumer");
 source_campaign_cinematic_control_tu15(world,true);check(word(m,bank+0x194u)==0x45000000u,"Cinematic start incorrectly cleared the original far override");
 source_campaign_screen_start_tu15(m,true);check(word(m,bank+0x194u)==0x45000000u,"Screen-effect prefix reset incorrectly cleared far override");
 source_campaign_screen_stop_tu15(world);check(word(m,bank+0x194u)==0&&source_campaign_clip_planes_tu15(world).y==1024.f,"Whole original screen stop did not clear far override and restore configured default");
 source_campaign_far_clip_tu15(world,2048.f);source_campaign_cinematic_control_tu15(world,false);check(word(m,bank+0x194u)==0,"Whole original cinematic stop did not clear far override");
 const auto saved=objects.authority_checkpoint();SourceExecutionInputs capture;capture.track_calls();
 {SourceExecutionInputBinding binding(m,capture);source_campaign_far_clip_tu15(world,2048.f);(void)source_campaign_clip_planes_tu15(world);}
 const auto inputs=capture.finish();const auto calls=capture.calls();check(inputs.empty()&&calls.size()==3,"Far source caller publication omitted its checked-writer journal");
 const std::array<std::uint32_t,3> addresses{bank+0x194u,0x4922f4u,0x4922f8u},values{0x45000000u,0x3c800000u,0x45000000u};
 for(unsigned i=0;i<3;++i)check(calls[i].kind==SourceExecutionCallKind::WriteSourceCallInput&&calls[i].arguments.size()==12&&Bytes(calls[i].arguments).u32(0)==addresses[i]&&Bytes(calls[i].arguments).u32(4)==4&&Bytes(calls[i].arguments).u32(8)==values[i],"Far source publication changed exact input address/word/order");
 const auto completed=m.checkpoint();check(objects.commit_authority(objects.prepare_authority(saved)),"Far source checkpoint reset failed");
 SourceExecutionInputs replay(inputs,calls);{SourceExecutionInputBinding binding(m,replay);source_campaign_far_clip_tu15(world,2048.f);(void)source_campaign_clip_planes_tu15(world);}replay.finish();check(m.checkpoint()==completed,"Far source replay changed the complete retained bank");
 world.inspect_render_memory([&](const auto &,const auto &){rejects([&]{source_campaign_far_clip_tu15(world,1024.f);},"Read-only render borrower accepted far control");});
 Simulation simulation(content,0,cache.spawns().front(),{},{},{},{},{},{},source);source_campaign_far_clip_tu15(world,1024.f);
 simulation.start_level();simulation.step({});for(const auto &d:simulation.scripts().diagnostics())std::cerr<<"SCRIPT "<<d<<'\n';
 check(simulation.scripts().diagnostics().empty(),"Authored campaign far-clip host was not dispatched");
 check(word(m,bank+0x194u)==0x45000000u&&source_campaign_clip_planes_tu15(world).y==2048.f,"Actual Floodzone REAL2048 call did not reach the configured projection/W consumer");
 simulation.request_checkpoint();simulation.step({});check(simulation.has_checkpoint(),"Far Simulation checkpoint was not captured");source_campaign_far_clip_tu15(world,128.f);simulation.request_revert();simulation.step({});check(word(m,bank+0x194u)==0x45000000u,"Owning Simulation restore lost far override");
 //491E7C is the submitted camera far after environment clamping, while
 //4922F8 retains the configured2048-unit distance for integer-W clipping.
 //Actual12DCA consumes491E7C independently of configured4922F8.
 SourceViewProjectionInput in;in.position={3,4,5};in.forward={1,0,0};in.up={0,0,1};in.near_distance=.015625f;in.far_distance=128.f;in.projection_program[10]=1;in.projection_program[11]=1;
 const auto camera=source_view_projection(in);const auto source_constants=source_view_shader_constants(camera,in.far_distance,false);const auto old_constants=source_view_shader_constants(camera,1024.f,false);
 for(unsigned row=0;row<4;++row)for(unsigned n=0;n<4;++n)check(source_constants[row][n]==old_constants[row][n]*8.f,"Shader bank did not consume the submitted128 camera far independently of configured2048");
 content.scripts=one_call("cinematic_set_far_clip_distance");Simulation invalid(content,0,cache.spawns().front());invalid.start_level();invalid.step({});
 check(invalid.scripts().diagnostics().size()==1&&invalid.scripts().diagnostics().front()=="far_clip_bad: builtin 'cinematic_set_far_clip_distance': invalid cinematic_set_far_clip_distance arity","Far arity did not reject at its exact host boundary");
 content.scripts=one_call("cinematic_set_far_clip_distance",{ScriptValue::numeric(2048.f,6)});Simulation missing(content,0,cache.spawns().front());missing.start_level();missing.step({});
 check(missing.scripts().diagnostics().size()==1&&missing.scripts().diagnostics().front()=="far_clip_bad: builtin 'cinematic_set_far_clip_distance': Cinematic far clip control requires its retained TU1.5 world","Missing far owner did not reject at its exact boundary");
 std::cout<<"PASS actual06b authored REAL2048 host, exact raw store/full-bank clobber controls, original start/prefix/stop reset semantics, configured-plane publication, source+Simulation restore, record/replay and read-only guard; separate submitted-camera shader normalization/configured clip inputs\n";
 std::cout<<"LIMIT source/pure consumers only; ModelView ABI consumers require a coherent rebuild, and combined GPU/matched-frame proof remains open\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

#else
int main(){std::cout<<"SKIP: generate the optional OG Xbox TU1.5 far-clip oracle using tools/generate_campaign_far_clip_fixture.py\n";return 77;}
#endif
