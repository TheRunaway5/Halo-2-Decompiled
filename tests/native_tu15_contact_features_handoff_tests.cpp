#include "physics/runtime/native_tu15_contact_features_handoff_internal.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
void check(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
struct Memory:SourcePhysicsContactMemory {
    std::array<std::byte,8192>bytes{};
    void read(std::uint32_t p,std::span<std::byte>out)const override{check(p>=0x1000u&&std::uint64_t(p-0x1000u)+out.size()<=bytes.size(),"Feature handoff read escaped owned DATA");std::copy_n(bytes.begin()+p-0x1000u,out.size(),out.begin());}
    void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{read(p,out);}
    void write(std::uint32_t p,std::span<const std::byte>in)override{check(p>=0x1000u&&std::uint64_t(p-0x1000u)+in.size()<=bytes.size(),"Feature handoff write escaped owned DATA");std::copy(in.begin(),in.end(),bytes.begin()+p-0x1000u);}
};
void put(Memory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));m.write(p,b);}
SourcePhysicsContactRuntimeAbi input(unsigned depth=0){SourcePhysicsContactRuntimeAbi a{};for(unsigned n=0;n<8;++n){a.gp[n]=0x2000u+n*16u;for(unsigned k=0;k<4;++k)a.xmm[n][k]=0x7fa00001u+n*4u+k;}a.gp[1]=0x2000u;a.gp[4]=0x1800u;a.eflags=0x246u;a.fs_base=0x1000u;a.x87_depth=depth;a.x87_status=0x100u;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};for(unsigned n=0;n<depth;++n)a.x87[n]=n%2?-.25L*(n+1):.75L*(n+1);return a;}
void packet(Memory&m,const SourcePhysicsContactRuntimeAbi&a,bool second=false,unsigned count=1){put(m,a.gp[4],second?0x30e988u:0x30e970u);put(m,a.gp[4]+4u,0x1900u);put(m,a.gp[4]+8u,count);put(m,a.gp[4]+12u,0x2400u);put(m,0x1900u,0);for(unsigned n=0;n<4;++n)put(m,0x2020u+n*4u,0x3f000000u+n);}
void preserved(const SourcePhysicsContactRuntimeAbi&a,const SourcePhysicsContactRuntimeAbi&b,bool completed=true){for(unsigned n=0;n<8;++n)check(b.gp[n]==a.gp[n]+(n==4&&completed?16u:0u),"Feature handoff changed GP beyond normal RET12");check(a.xmm==b.xmm&&a.eflags==b.eflags&&a.fs_base==b.fs_base,"Feature handoff manufactured volatile output images");check(a.x87_depth==b.x87_depth,"Feature handoff changed live x87 depth");for(unsigned n=0;n<a.x87_depth;++n)check(std::memcmp(&a.x87[n],&b.x87[n],10)==0,"Feature handoff changed a live extended value");}
template<class F>void rejects(F&&f,const char*message){bool rejected{};try{f();}catch(const std::runtime_error&){rejected=true;}check(rejected,message);}
}
int main(){try{
    constexpr SourcePhysicsContactRuntimeCall first{0x30e96d,0x1f6690,false},second{0x30e985,0x1f6690,false};Memory memory,other;NativeTu15ContactFeaturesHandoffActivity activity;bool trace=false;std::vector<std::uint32_t>progress;NativeTu15ContactFeaturesHandoffPolicy policy{0x1000u,0x3000u,true,&trace,&trace,[&](auto pc){progress.push_back(pc);}};
    auto abi=input();packet(memory,abi);const auto initial=abi;check(!native_tu15_handoff_contact_features(memory,first,abi),"Unbound feature memory selected native");preserved(initial,abi,false);
    {
        NativeTu15ContactFeaturesHandoffBinding binding(memory,policy,&activity);
        unsigned observed{};check(native_tu15_handoff_contact_features(memory,first,abi,[&](const auto&e){++observed;check(e.instruction==0x1f66beu&&e.address==0x2400u,"Feature observer changed native store identity");}),"Bound feature call unavailable");preserved(initial,abi);check(observed==1&&progress.size()==1,"Native feature store lost observer/progress");std::array<std::byte,16>out{},source{};memory.read(0x2400,out);memory.read(0x2020,source);check(out==source,"Actual native polygon kernel did not publish its DATA");
        auto unrelated=input();packet(other,unrelated);check(!native_tu15_handoff_contact_features(other,first,unrelated),"Feature memory scope escaped its owner");
        {auto nested_policy=policy;nested_policy.observers_read_only=false;NativeTu15ContactFeaturesHandoffBinding nested(memory,nested_policy);auto a=input();packet(memory,a);rejects([&]{native_tu15_handoff_contact_features(memory,first,a);},"Nested read-only policy was ignored");}
        abi=input();packet(memory,abi,true);check(native_tu15_handoff_contact_features(memory,second,abi),"Nested feature binding did not restore owner");
        for(unsigned fault=0;fault<11;++fault){auto bad=input();packet(memory,bad);auto call=first;switch(fault){case 0:call.instruction++;break;case 1:call.target++;break;case 2:call.tail=true;break;case 3:bad.floating_point.reset();break;case 4:bad.active_mmx=SourcePhysicsMmxAlias{};break;case 5:bad.x87_depth=8;break;case 6:put(memory,bad.gp[4],0x30e988u);break;case 7:bad.gp[4]=0x2ff4u;break;case 8:trace=true;break;case 9:bad.gp[4]=0xfffffff8u;break;case 10:bad.eflags|=0x400u;break;}const auto before=bad;rejects([&]{native_tu15_handoff_contact_features(memory,call,bad);},"Unsupported feature descriptor/context silently fell back");preserved(before,bad,false);trace=false;}
        bool foreign_refused{};std::thread worker([&]{auto foreign=input();try{native_tu15_handoff_contact_features(memory,first,foreign);}catch(const std::runtime_error&){foreign_refused=true;}});worker.join();check(foreign_refused,"Foreign executor used native feature binding");
    }
    abi=input();packet(memory,abi);check(!native_tu15_handoff_contact_features(memory,first,abi),"Feature binding leaked after its scope");
    for(unsigned depth:{0u,1u,3u,7u}){auto actual=input(depth);packet(memory,actual);const auto before=actual;NativeTu15ContactFeaturesHandoffBinding binding(memory,policy,&activity);check(native_tu15_handoff_contact_features(memory,first,actual),"Live numerical feature transport failed");preserved(before,actual);}
    for(const auto owner:{0x2e5e20u,0x3079c0u,0x3067d0u,0x3070f0u,0x1f6690u,0x2e8d40u}){auto actual=input();packet(memory,actual,false,0);put(memory,0x2030u,0x2100u);put(memory,0x2100u,0x2200u);put(memory,0x2234u,0x1f6690u);NativeTu15ContactFeaturesHandoffBinding binding(memory,policy,&activity);check(native_tu15_handoff_contact_features(memory,{first.instruction,owner,false},actual),"Authenticated feature owner refused normal zero-count call");}
    {auto actual=input();packet(memory,actual,false,3);const auto before=actual;NativeTu15ContactFeaturesHandoffBinding binding(memory,policy,&activity);check(native_tu15_handoff_contact_features(memory,{first.instruction,0x2e8d40u,false},actual),"Native sphere feature handoff refused");preserved(before,actual);std::array<std::byte,48>zeros{};memory.read(0x2400u,zeros);check(std::all_of(zeros.begin(),zeros.end(),[](auto b){return b==std::byte{};}),"Native sphere feature output differs");}
    // The actual header used by generated World calls transports through the
    // real WorldState save/load, including retained live numerical values.
    {auto actual=input(3);packet(memory,actual,true);const auto before=actual;SourcePhysicsWorldRuntimeServices services;FP controls{0x27f,0x1f80};FpScope scope(controls);SourcePhysicsContactRecordObserver observer;WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);state.load(actual);NativeTu15ContactFeaturesHandoffBinding binding(memory,policy,&activity);check(handoff_native_contact_features(state,second.target,second.instruction),"Runtime feature helper lost active binding");preserved(before,state.save());}
    {auto actual=input();packet(memory,actual);const auto before=actual;NativeTu15ContactFeaturesHandoffBinding binding(memory,policy,&activity);rejects([&]{native_tu15_handoff_contact_features(memory,first,actual,[](const auto&){throw std::runtime_error("Actual read-only observer failure");});},"Feature observer failure silently fell back");preserved(before,actual,false);}
    check(activity.completed==14&&activity.first_shape_calls==12&&activity.second_shape_calls==2&&activity.swept_calls==1&&activity.soa_calls==1&&activity.box_calls==1&&activity.capsule_calls==1&&activity.polygon_calls==8&&activity.sphere_calls==2,"Feature handoff activity cohorts differ");check(activity.refusals==13&&activity.attempts==27,"Feature handoff refusal/attempt counts differ");
    std::cout<<"Native feature handoff: actual kernel, WorldState helper, RET12/FP/live-value preservation, memory/executor/LIFO policy and explicit refusal controls passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
