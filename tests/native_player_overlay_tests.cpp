#include "halo2/native_player_animation.hpp"
#include "halo2/biped_landing.hpp"
#include "halo2/biped_postphysics_phase.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include <bit>
#include <iostream>
#include <fstream>
#include "halo2/native_melee_animation.hpp"

using namespace halo2;
namespace {
void check(bool b,const char *s){if(!b)throw FormatError(s);}
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v){auto b=m.mutable_view(p,4);for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));}
void fp(SourceRuntimeMemory &m,std::uint32_t p,float v){put(m,p,std::bit_cast<std::uint32_t>(v));}
std::array<float,8> packet(const BonePose &p){return {p.rotation[0],p.rotation[1],p.rotation[2],p.rotation[3],p.translation.x,p.translation.y,p.translation.z,p.scale};}
}
unsigned verify_landing_writes() {
 SourceRuntimeMemory m;
 for(const auto [address,size]:std::array<std::pair<std::uint32_t,unsigned>,5>{{
     {0x465000,4096},{0x4e7000,0x5000},{0x518000,4096},
     {0x60000000,0x6000},{0x70000000,65536}}})
  m.mount(address,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,"landing original operand bank");
 constexpr std::uint32_t unit=0x60002000,definition=0x60004000;
 put(m,0x4e78d0,0x60000000);put(m,0x60000044,0x60001000);put(m,0x60001008+12,unit);
 put(m,unit,1);put(m,0x4eb114,0x60003000);put(m,0x60003008+16,definition);
 put(m,0x518444,0x60005000);fp(m,0x465030,1.f);
 unsigned cases=0;
 for(float soft_seconds:{.6f,0.f,5.f})for(float hard_seconds:{0.f,.7f})
 for(int rate:{30,60})for(unsigned input=0;input<160;++input)for(bool force:{false,true}) {
  const SourceLandingDefinition d{soft_seconds,hard_seconds,1.5f,7.f,12.f};
  for(const auto [offset,value]:std::array<std::pair<unsigned,float>,5>{{
      {0x1fc,d.soft_seconds},{0x200,d.hard_seconds},{0x204,d.soft_speed},
      {0x208,d.hard_speed},{0x20c,d.maximum_speed}}})fp(m,definition+offset,value);
  put(m,0x60005000,std::uint32_t(rate)<<16);
  SourceLandingState s{0,2,7};bool forced=force;const float impact=float(input)/8;
  put(m,unit+0x348,0x81|(force?0x40:0));put(m,unit+0x34c,0x07020000);
  SourceUnitAnimationRuntimeAbi abi{};abi.gp[0]=0x12340001;abi.gp[4]=0x70008000;
  abi.xmm[2][0]=std::bit_cast<std::uint32_t>(impact);abi.eflags=2;
  source_run_biped_postphysics_phase_1_1(m,0xe5960,abi,{0x27f,0x1f80},{});
  begin_source_landing(s,d,impact,rate,forced);
  const Bytes actual(m.view(unit+0x34c,4));
  check(actual.u16(0)==std::uint16_t(s.kind) && std::to_integer<unsigned>(actual.slice(2,1)[0])==std::uint8_t(s.elapsed) &&
      std::to_integer<unsigned>(actual.slice(3,1)[0])==std::uint8_t(s.duration),"Landing writes differ from original E5960");
  check(std::to_integer<unsigned>(m.view(unit+0x348,1)[0])==(0x81|(forced?0x40:0)),"Landing force flag differs from original E5960");
  ++cases;
 }
 SourceLandingState soft{0,0,8};for(unsigned tick=0;tick<3;++tick)advance_source_landing(soft,true);
 check(soft.kind==0,"Soft landing ended before original moving half-duration");advance_source_landing(soft,true);
 check(soft.kind==-1 && source_landing_activity(soft,0x0a000014)==0x0a000014,"Soft landing did not release to gait");
 SourceLandingState hard{1,0,8};for(unsigned tick=0;tick<7;++tick)advance_source_landing(hard,true);
 check(source_landing_activity(hard,0)==0x09000020,"Hard landing released on the soft moving gate");
 advance_source_landing(hard,true);check(hard.kind==-1,"Hard landing did not expire on original timer");
 return cases;
}
unsigned verify_melee_arithmetic() {
 std::ifstream fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/melee-animation-1_1.tsv");
 check(bool(fixture),"Original melee arithmetic fixture unavailable");
 char kind;unsigned comparisons=0;
 while(fixture>>kind) {
  if(kind=='S') {
   std::uint32_t seed,expected_seed;unsigned mask;int expected;
   fixture>>seed>>mask>>expected>>expected_seed;
   std::array<bool,4> available{};for(unsigned i=0;i<4;++i)available[i]=mask&(1u<<i);
   check(select_source_melee_candidate(available,seed)==expected && seed==expected_seed,
       "Melee candidate/RNG differs from original EB000");
  }else if(kind=='T') {
   int progress,impact,duration,expected_progress,impacts;std::uint32_t expected_activity;
   fixture>>progress>>impact>>duration>>expected_progress>>expected_activity>>impacts;
   NativeMeleeAction s{0x0e000607,1,std::int8_t(progress),std::int8_t(impact),25,std::int8_t(duration)};
   const bool requested=advance_source_melee_animation(s);
   check(s.progress==expected_progress && s.activity==expected_activity && requested==bool(impacts),
       "Ordinary melee timer/request boundary differs from original EB700");
  }else throw FormatError("Unknown original melee fixture row");
  check(bool(fixture),"Truncated original melee fixture row");++comparisons;
 }
 check(comparisons==1920,"Original melee fixture coverage changed");return comparisons;
}
unsigned verify_melee_geometry() {
 std::ifstream fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/melee-geometry-1_1.tsv");
 check(bool(fixture),"Original melee geometry fixture missing");
 char kind;unsigned cases=0;
 while(fixture>>kind) {
  check(kind=='C',"Original melee geometry case missing");
  unsigned pattern;std::array<std::uint32_t,6> input;std::uint32_t expected;
  fixture>>pattern>>std::hex;for(auto &v:input)fixture>>v;fixture>>expected>>std::dec;
  std::array<Vec3,25> rays;
  for(auto &ray:rays) {std::array<std::uint32_t,3> bits;fixture>>kind>>std::hex;for(auto &v:bits)fixture>>v;fixture>>std::dec;
   check(kind=='R',"Original melee cast row missing");ray={std::bit_cast<float>(bits[0]),std::bit_cast<float>(bits[1]),std::bit_cast<float>(bits[2])};}
  unsigned cast=0;
  const auto result=query_native_melee({}, {std::bit_cast<float>(input[0]),std::bit_cast<float>(input[1]),std::bit_cast<float>(input[2])},
      {std::bit_cast<float>(input[3]),std::bit_cast<float>(input[4]),std::bit_cast<float>(input[5])},
      [&](Vec3,Vec3 delta,std::uint32_t mask)->std::optional<NativeMeleeContact> {
    check(mask==0x2480000f,"Melee query changed its original mask");
    const auto ray=rays.at(cast);
    check(std::abs(ray.x-delta.x)<0.000001f && std::abs(ray.y-delta.y)<0.000001f && std::abs(ray.z-delta.z)<0.000001f,
        "Melee pyramid differs from original D0830 cast sequence");
    const unsigned i=cast++;if(!pattern)return {};
    constexpr std::array<unsigned,5> ids{3,4,3,5,4},types{7,0,7,0,0};
    constexpr std::array<float,5> fractions{.05f,.7f,.01f,.6f,.65f};
    return NativeMeleeContact{0xabcd0000u+ids[i%5],std::uint8_t(types[i%5]),fractions[i%5],{},{},-1};
  });
  check(cast==25 && (result.target?result.target->object:0xffffffffu)==expected,"Melee target preference differs from original D0830");++cases;
 }
 check(cases==48,"Original melee geometry coverage incomplete");return cases;
}
unsigned verify_local_transitions() {
 unsigned cases=0;
 for(unsigned seed=1;seed<17;++seed)for(unsigned progress=1;progress<8;++progress) {
  const BonePose old{{.3f,-.5f,float(seed)},{.2f,.3f,.4f,seed%2?-.5f:.5f},.8f};
  const BonePose now{{.8f,.2f,-float(seed)},{.1f,-.4f,.5f,.7f},1.2f};
  SourceRuntimeMemory m;
  m.mount(0x465000,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"original transition one operand");
  m.mount(0x60000000,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"local transition input banks");
  m.mount(0x70000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"original transition stack");
  fp(m,0x465030,1.f);put(m,0x60000000,progress|(8u<<8)|0x1000000u);
  for(const auto [p,pose]:std::array<std::pair<std::uint32_t,BonePose>,2>{{{0x60000100,now},{0x60000200,old}}}) {
   const auto values=packet(pose);for(unsigned i=0;i<8;++i)fp(m,p+i*4,values[i]);
  }
  SourceUnitAnimationRuntimeAbi abi{};abi.gp[0]=0x60000000;abi.gp[1]=0x60000100;abi.gp[3]=1;abi.gp[4]=0x70008000;abi.eflags=2;
  put(m,abi.gp[4]+4,0);put(m,abi.gp[4]+8,0x60000200);
  source_run_unit_animation_selection_1_1(m,0x1dce60,abi,{0x27f,0x1f80},{});
  std::array<BonePose,1> actual{now},saved{old};blend_native_player_locals(actual,saved,float(progress)/8);
  const auto values=packet(actual[0]);const Bytes expected(m.view(0x60000100,32));
  for(unsigned i=0;i<8;++i)check(std::bit_cast<std::uint32_t>(values[i])==expected.u32(i*4),"Native local transition differs from original1DCE60");
  ++cases;
 }
 return cases;
}
int main(){try {
 std::cout<<"original melee geometry cases="<<verify_melee_geometry()<<'\n';
 std::cout<<"original local transition cases="<<verify_local_transitions()<<'\n';
 std::cout<<"original melee arithmetic comparisons="<<verify_melee_arithmetic()<<'\n';
 std::cout<<"original E5960 landing write comparisons="<<verify_landing_writes()<<'\n';
 unsigned cases=0;
 for(unsigned seed=1;seed<=64;++seed)for(unsigned mask=0;mask<8;++mask) {
  RenderModel model;model.bones.resize(1);model.bones[0].name=17;
  BonePose base{{float(seed)/13,-.3f,.7f},{.2f,-.3f,.4f,.5f},1.3f};
  BonePose delta{{.1f,-float(seed)/27,.4f},{.6f,.7f,-.2f,float(seed)/79},.1f};
  AnimationClip clip;clip.animation_type=1;clip.frame_count=1;clip.nodes={17};clip.components={mask};clip.channels.resize(1);
  const auto d=packet(delta);
  clip.channels[0][0]={{0},{{d[0],d[1],d[2],d[3]}}};
  clip.channels[0][1]={{0},{{d[4],d[5],d[6],0}}};
  clip.channels[0][2]={{0},{{d[7],0,0,0}}};
  SourceRuntimeMemory memory;
  memory.mount(0x50bc00,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"test original sampler globals and local bank");
  memory.mount(0x70000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"test original caller stack");
  constexpr std::uint32_t dest=0x50bd00;
  put(memory,0x50bc4c,1);put(memory,0x50bc8c,dest);
  for(unsigned c=0;c<3;++c) {
   const auto address=0x50be00+c*4;put(memory,address,(mask>>c)&1);
   put(memory,0x50bc80+c*4,address);put(memory,0x50bc74+c*4,0xdead0000+c*4);
  }
  const auto b=packet(base);for(unsigned i=0;i<8;++i)fp(memory,dest+i*4,b[i]);
  SourceUnitAnimationRuntimeAbi abi{};abi.gp[4]=0x70008000;abi.eflags=2;
  SourceUnitAnimationRuntimeServices services;unsigned calls=0;
  // The pure arithmetic leaf's external codec boundary delivers the same
  // decoded channel keys as the production adapter. No pose result is stubbed.
  services.required_owner=[&](auto &m,const auto &call,auto &a,const auto&){
    check(call.target>=0xdead0000&&call.target<=0xdead0008,"Unexpected original overlay callee");
    for(unsigned i=0;i<8;++i)fp(memory,0x50bc20+i*4,d[i]);
    a.gp[4]+=4;++calls;(void)m;
  };
  source_run_unit_animation_selection_1_1(memory,0x287b90,abi,{0x27f,0x1f80},services);
  std::array<BonePose,1> locals{base};apply_native_player_overlay(model,locals,clip,0);
  const auto result=packet(locals[0]);const Bytes actual(memory.view(dest,32));
  for(unsigned i=0;i<8;++i)check(std::bit_cast<std::uint32_t>(result[i])==actual.u32(i*4),"Native body overlay differs from original287B90 arithmetic");
  check(calls==std::popcount(mask),"Original overlay ignored a component mask");
  clip.animation_type=2;locals={base};apply_native_player_overlay(model,locals,clip,0);
  const auto replaced=packet(locals[0]);
  for(unsigned i=0;i<8;++i) {
   const auto component=i<4?1u:i<7?2u:4u;
   check(replaced[i]==((mask&component)?d[i]:b[i]),"Type2 replaced an unauthored component");
  }
  ++cases;
 }
 std::cout<<"PASS "<<cases<<" original287B90 differential masks and type2 component replacements\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
