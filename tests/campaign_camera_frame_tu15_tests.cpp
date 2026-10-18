#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/simulation.hpp"
#if __has_include("fixtures/campaign_source_frame_interval_tu15.inc")
#include "fixtures/campaign_source_frame_interval_tu15.inc"
#define HALO2_HAVE_CAMERA_INTERVAL_ORACLE 1
#else
#define HALO2_HAVE_CAMERA_INTERVAL_ORACLE 0
#endif
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
#include <ranges>
using namespace halo2;
void check(bool b,const char*s){if(!b)throw FormatError(s);}
std::vector<std::byte> bank(const SourceRuntimeMemory&m,std::uint32_t at,std::size_t n){const auto v=m.view(at,n);return{v.begin(),v.end()};}
std::array<std::byte,36> pose(const SourceCampaignCameraViewTu15&v){std::array<std::byte,36>b;std::array<Vec3,3>a{v.position,v.forward,v.up};std::memcpy(b.data(),a.data(),36);return b;}
struct Program {
 ScriptProgram p;
 std::uint32_t node(ScriptNode n){const auto i=std::uint32_t(p.nodes.size());n.salt=std::uint16_t(0xe000+i);p.nodes.push_back(n);return(std::uint32_t(n.salt)<<16)|i;}
 std::uint32_t literal(std::uint32_t v,unsigned type){return node({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,v});}
 std::uint32_t call(std::string name,std::vector<std::uint32_t>args={}){const auto at=std::uint32_t(p.strings.size());p.strings+=name;p.strings+='\0';for(unsigned i=0;i<args.size();++i)p.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;const auto f=node({0,0,2,9,args.empty()?null_datum:args[0],at,0});return node({0,0,4,8,null_datum,at,f});}
 Program(bool animation=false){
  std::vector<std::uint32_t> commands{call("cinematic_start"),call("camera_control",{literal(1,5)})};
  if(animation)commands.push_back(call("camera_set_animation_relative",{
      literal(0xe9d90864u,36),literal(0x06001c9eu,11),literal(null_datum,45),literal(2,14)}));
  const auto root=call("begin",std::move(commands));p.scripts.push_back({"camera_frame_start",0,4,root});
 }
};
void run(const char*xbe,const char*map,bool points,bool native_cold){
 Cache cache(map,std::filesystem::path(map).parent_path(),OriginalData::load_xbe(xbe));auto content=cache.load_content(0,false);content.objects.clear();content.scripts=Program{!points}.p;
 unsigned legacy_camera_loads{};
 const auto loader=[&](auto g,auto n){++legacy_camera_loads;return cache.load_named_animation(g,n);};
 auto source=native_cold?std::make_shared<CampaignSourceRuntime>(cache,0,1,0x12345678):
     std::make_shared<CampaignSourceRuntime>(cache,std::filesystem::path(xbe),0,1,0x12345678);
 if(native_cold) {
  const auto admitted=source->objects().authority_checkpoint();
  auto&world=source->objects().tu15_world();const auto unchanged=world.memory().checkpoint();
  unsigned rejected_schemas{};
  for(unsigned fault=0;fault<9;++fault) {
   auto invalid=admitted;
   const auto owner=std::ranges::find(invalid.world.bank.regions,0x5020b4u,
       &SourceRuntimeMemory::CheckpointRegion::address);
   check(owner!=invalid.world.bank.regions.end(),"Native cold world lacks complete type-runtime owner");
   switch(fault) {
   case 0:invalid.world.bank.regions.erase(owner);break;
   case 1:owner->bytes.pop_back();break;
   case 2:owner->bytes.push_back(std::byte{});break;
   case 3:owner->access=SourceRuntimeMemory::Access::ReadOnly;break;
   case 4:owner->owner+=" invalid";break;
   case 5:owner->page_bytes=4096;break;
   case 6:owner->committed={1};break;
   case 7:owner->address=0x5020b8u;owner->bytes={std::byte{}};
       owner->owner="native TU1.5 cold 102A54 scene placement policy byte";break;
   case 8:owner->address+=1;break;
   }
   bool rejected=false;
   try {(void)source->objects().prepare_authority(invalid);}
   catch(const FormatError&error) {rejected=std::string(error.what())==
       "TU1.5 authority lacks its complete object-type runtime owner";}
   check(rejected,"Public world restore did not reject missing/malformed/old partial type owner at staging");
   check(world.memory().checkpoint()==unchanged,"Rejected type owner changed live world before restore");
   ++rejected_schemas;
  }
  SourceExecutionInputs capture;capture.track_calls();
  {SourceExecutionInputBinding binding(world.memory(),capture);source_campaign_camera_frame_tu15(world,.125f);}
  std::vector<SourceExecutionInput> inputs;{const auto values=capture.finish();inputs.assign(values.begin(),values.end());}
  std::vector<SourceExecutionCall> calls;{const auto values=capture.calls();calls.assign(values.begin(),values.end());}
  const auto after=world.memory().checkpoint();
  check(source->objects().commit_authority(source->objects().prepare_authority(admitted)),"Native cold quiescent authority restore failed");
  {SourceExecutionInputs replay(inputs,calls);{SourceExecutionInputBinding binding(world.memory(),replay);source_campaign_camera_frame_tu15(world,.125f);}(void)replay.finish();}
  check(world.memory().checkpoint()==after,"Native cold unborrowed camera call replay changed complete source bank");
  std::cout<<"PASS native-cold publictype schemas="<<rejected_schemas<<" andwhole-bank pre-player camera replay\n";
 }
 // Raw-world restore is legal before Simulation creates native lifetime borrowers.
 {
  auto &cold=source->objects().tu15_world();const auto before=bank(cold.memory(),0x4f64a4u,0x358u);
  const auto checkpoint=source->objects().authority_checkpoint();source_campaign_camera_frame_tu15(cold,0);
  check(source->objects().commit_authority(source->objects().prepare_authority(checkpoint)),"Unborrowed native source camera checkpoint failed");
  check(bank(cold.memory(),0x4f64a4u,0x358u)==before,"Unborrowed source restore changed observer");
 }
 Simulation sim(content,0,cache.spawns().front(),loader,{},{},{},{},{},source);
 auto&w=source->objects().tu15_world();auto&m=w.memory();
 const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};const auto users=word(0x4f54f0u);
 check(users&&word(users+12u)!=null_datum,"Camera frame has no actual registered native player");
 sim.start_level();sim.step({});check(sim.scripts().diagnostics().empty(),"Real camera-control host fixture faulted");check(sim.presentation().scripted&&sim.presentation().cinematic,"Camera host fixture did not admit source presentation");
 check(legacy_camera_loads==0,"Source camera cut redundantly decoded a second legacy camera clip");
 source_campaign_camera_field_of_view_tu15(w,60,0);
 if(points){source_campaign_camera_point_tu15(w,26,0);}
 else{source_campaign_camera_animation_tu15(w,0xe9d90864,0x06001c9e,null_datum,2);}
 unsigned frames{},zero{},one{},many{};
 auto frame=[&](float raw,bool paused=false){
  const float incoming=source_campaign_frame_interval_tu15(raw);
  const auto accepted=sim.advance_frame_time(incoming,paused);for(int n=0;n<accepted.ticks;++n)sim.step({});
  sim.advance_cinematic_render_time(accepted.elapsed,accepted.ticks,accepted.ticks);
  const auto working_before=bank(m,0x4f64a4u+0x10cu,0x24u);
  std::vector<SourcePhysicsContactRecordStore> stores;
  w.observe_physics_stores([&](auto,const auto&store){stores.push_back(store);});
  SourceExecutionInputs tape;tape.track_calls();
  {SourceExecutionInputBinding binding(m,tape);sim.advance_cinematic_camera(incoming);}
  w.observe_physics_stores({});
  (void)tape.finish();const auto calls=tape.calls();check(calls.size()==2,"Camera phase did not journal exactly two real source calls");
  check(calls[0].kind==SourceExecutionCallKind::CallActualSource&&calls[1].kind==SourceExecutionCallKind::CallActualSource,"Camera phase bypassed ordinary source public calls");
  check(Bytes(calls[0].arguments).u32(0)==0x157fe0u&&Bytes(calls[1].arguments).u32(0)==0x1720e0u,"Camera original frame order differs");
  check(Bytes(calls[0].arguments).u32(4)==1u&&Bytes(calls[1].arguments).u32(4)==1u&&
        Bytes(calls[0].arguments).u32(8)==std::bit_cast<std::uint32_t>(incoming)&&Bytes(calls[1].arguments).u32(8)==std::bit_cast<std::uint32_t>(incoming),
        "Camera frame replaced raw incoming delta with accepted effect elapsed");
  auto v=source_campaign_camera_view_tu15(m,0);check(v.has_value(),"Active camera frame has no completed source observer");
  if(!points){
   const auto scratch=w.prepare_source_call().gp[4]+0x100u;const std::array<std::byte,56>clear{};w.write_source_call_input(scratch,clear);auto abi=w.prepare_source_call(std::array{scratch,scratch+52u});w.run_actual(0x16fe00u,abi);check((abi.gp[0]&255u)!=0,"Native source sampler rejected real admitted clip");
   const auto sample=m.view(scratch,52);std::array<std::byte,36>expected;std::memcpy(expected.data(),sample.data()+40,12);std::memcpy(expected.data()+12,sample.data()+4,12);std::memcpy(expected.data()+24,sample.data()+28,12);
   const auto command=word(0x4f64a8u);std::array<std::byte,36> produced;
   const auto command_eye=m.view(command+4u,12u),command_forward=m.view(command+0x2cu,12u),command_up=m.view(command+0x38u,12u);
   std::memcpy(produced.data(),command_eye.data(),12u);std::memcpy(produced.data()+12u,command_forward.data(),12u);std::memcpy(produced.data()+24u,command_up.data(),12u);
   check(produced==expected,"Actual native director command differs from its admitted native source sampler");
  }
  const auto command=word(0x4f64a8u);
  std::size_t copies{};bool incoming_store{},motion_store{},output_store{};
  for(const auto&store:stores){
   if(store.instruction==0x157ff6u&&store.address==0x4f5508u)incoming_store=Bytes(store.written).u32(0)==std::bit_cast<std::uint32_t>(incoming);
   if(store.instruction==0x172ce4u&&store.address>=0x4f64acu&&store.address<0x4f6558u){
    check(store.address==0x4f64acu+4u*copies,"Actual observer command stores were reordered");
    check(store.written==bank(m,command+4u*copies,4u),"Actual observer command store lost its director value");++copies;
   }
   motion_store|=store.instruction>=0x172d00u&&store.instruction<0x173e30u;
   output_store|=store.instruction>=0x173e30u&&store.instruction<0x175000u&&store.address>=0x4f655cu&&store.address<0x4f65b0u;
  }
  check(incoming_store&&copies==43u&&output_store,"Camera frame lacks actual incoming, adoption or completed output stores");
  check(motion_store==(incoming>0),"Actual observer motion admission disagrees with original positive incoming branch");
  if(incoming==0)check(bank(m,0x4f64a4u+0x10cu,0x24u)==working_before,"Zero incoming changed the observer working pose");
  std::array<std::byte,36> completed;
  const auto eye=m.view(0x4f655cu,12),forward=m.view(0x4f657cu,12),up=m.view(0x4f6588u,12);
  std::memcpy(completed.data(),eye.data(),12);std::memcpy(completed.data()+12,forward.data(),12);std::memcpy(completed.data()+24,up.data(),12);
  check(pose(*v)==completed,"Read-only camera consumer changed completed source bits");
  const auto before=bank(m,0x4f64a4,0x358);
  w.inspect_render_memory([&](const auto&,const auto&){auto read=source_campaign_camera_view_tu15(m,0);check(read&&pose(*read)==pose(*v),"Read-only consumer changed camera");bool denied=false;try{source_campaign_camera_frame_tu15(w,raw);}catch(...){denied=true;}check(denied,"Render borrow admitted camera source update");});
  check(bank(m,0x4f64a4,0x358)==before,"Read-only camera changed source observer bank");
  ++frames;zero+=accepted.ticks==0;one+=accepted.ticks==1;many+=accepted.ticks>1;return*v;
 };
 const auto first=frame(0);const auto zero_tick_positive=frame(1.f/120.f);
 if(!points)check(pose(zero_tick_positive)!=pose(first),"Positive incoming with no game tick did not adopt camera motion");
 for(float dt:std::array{1.f/30.f,.125f,0.f,1.f/30.f}){(void)frame(dt);}
 const auto before_cut=*source_campaign_camera_view_tu15(m,0);
 if(points){source_campaign_camera_point_tu15(w,27,120);}
 else{source_campaign_camera_animation_tu15(w,0xe9d90864,0x06001c9f,null_datum,2);}
 // Original16F150 point setter runs157FE0(0),1720E0(0.0001) itself;
 // animation setter only publishes director state. Outer zero preserves the
 // completed working camera that exists after the actual setter returns.
 const auto after_setter=*source_campaign_camera_view_tu15(m,0);
 if(!points)check(pose(after_setter)==pose(before_cut),"Animation setter changed completed camera before observer phase");
 const auto zero_cut=frame(0);check(pose(zero_cut)==pose(after_setter),"Zero incoming camera cut changed the completed working pose");
 const auto cut=frame(1.f/30.f);check(pose(cut)!=pose(before_cut),"Positive incoming authored cut retained departed shot");
 source_campaign_camera_field_of_view_tu15(w,50,0);const auto fov=frame(1.f/30.f);const auto director=word(0x51df6cu);check(std::bit_cast<std::uint32_t>(fov.horizontal_field_of_view)==word(director+0x10u),"Camera frame did not adopt actual source FOV");
 // Actual paused clock149205 returns zero game ticks; callerESI still supplies positive raw camera delta.
 const auto clock=word(word(0x51df54u)+8u);
 const auto paused_view=frame(.125f,true);(void)paused_view;
 (void)frame(100.f,true);check(word(word(0x51df54u)+8u)==clock,"Zero camera phase advanced source game ticks");
 sim.request_checkpoint();sim.step({});check(sim.has_checkpoint(),"Owning Simulation did not capture campaign checkpoint");
 const auto saved_observer=bank(m,0x4f64a4u,0x358u);const auto restores=sim.checkpoint_restores();
 for(int i=0;i<3;++i)(void)frame(1.f/30.f);
 sim.request_revert();sim.step({});check(sim.checkpoint_restores()==restores+1,"Owning Simulation did not restore camera checkpoint");
 check(bank(m,0x4f64a4u,0x358u)==saved_observer,"Owning Simulation camera restore lost completed observer state");
 check(source_campaign_camera_view_tu15(m,0).has_value(),"Restored source camera lost native user identity");
 std::cout<<"PASS "<<(points?"03b points":"00a animations")<<" real native registered player; frames="<<frames<<" zero="<<zero<<" one="<<one<<" multi="<<many<<" cut/FOV/read-only/paused raw positive/source checkpoint\n";
 check(zero&&one&&many,"Native camera frame fixtures lost tick-schedule coverage");
}
int main(int argc,char**argv)try{
#if !HALO2_HAVE_CAMERA_INTERVAL_ORACLE
 (void)argc;(void)argv;
 std::cout<<"SKIP: generate the local TU1.5 incoming camera interval oracle first.\n";
 return 77;
#else
 if((argc!=4&&argc!=5) || !std::filesystem::is_regular_file(argv[1]) ||
    !std::filesystem::is_regular_file(argv[2]) || !std::filesystem::is_regular_file(argv[3])) {
  std::cout<<"SKIP: requires owned OG Xbox TU1.5 XBE,00a and03b maps.\n";
  return 77;
 }
 for(const auto c:interval_cases)check(std::bit_cast<std::uint32_t>(source_campaign_frame_interval_tu15(std::bit_cast<float>(c[0])))==c[1],"Original binary32 incoming frame clamp differs");
 for(float x:std::array{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){bool rejected=false;try{(void)source_campaign_frame_interval_tu15(x);}catch(const FormatError&){rejected=true;}check(rejected,"Nonfinite native incoming interval was admitted");}
 const bool native_cold=argc==5&&std::string_view(argv[4])=="--native-cold";
 if(argc==5&&!native_cold)throw FormatError("Unknown camera fixture mode");
 run(argv[1],argv[2],false,native_cold);
 run(argv[1],argv[3],true,native_cold);
 std::cout<<"LIMIT host RSQRT estimate policy retained; strict original frame numeric parity remains open. No GPU or installed runtime proof.\n";
#endif
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
