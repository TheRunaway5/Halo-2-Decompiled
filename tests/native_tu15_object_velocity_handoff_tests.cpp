#include "halo2/native_tu15_object_velocity_handoff.hpp"
#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
using namespace halo2::physics_vehicle_runtime_detail;
// The owned validator replaces this marker with the exact current helper
// extracted from physics_vehicle_update_generated.cpp and pins its source.
// HALO2_HANDOFF_SOURCE_HELPER
namespace {
void check(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
struct Memory:SourcePhysicsContactMemory{
    std::array<std::byte,8192>bytes{};
    void read(std::uint32_t address,std::span<std::byte>out)const override{check(address>=0x1000u&&std::uint64_t(address-0x1000u)+out.size()<=bytes.size(),"Handoff model read escaped owned data");std::copy_n(bytes.begin()+address-0x1000u,out.size(),out.begin());}
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>input)override{check(address>=0x1000u&&std::uint64_t(address-0x1000u)+input.size()<=bytes.size(),"Handoff model write escaped owned data");std::copy(input.begin(),input.end(),bytes.begin()+address-0x1000u);}
};
void put(Memory&memory,std::uint32_t address,std::uint32_t value){std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));memory.write(address,bytes);}
SourcePhysicsContactRuntimeAbi input(unsigned depth=0){
    SourcePhysicsContactRuntimeAbi abi{};for(unsigned n=0;n<8;++n)abi.gp[n]=0x2000u+n*0x10u;abi.gp[4]=0x1800u;abi.eflags=0x246;abi.fs_base=0x1000;abi.x87_depth=depth;abi.x87_status=0x100;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};for(unsigned n=0;n<depth;++n)abi.x87[n]=n%2?-.25L*(n+1):.75L*(n+1);for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x70000000u+n*4+k;return abi;
}
void packet(Memory&memory,const SourcePhysicsContactRuntimeAbi&abi){put(memory,abi.gp[4],0xf2096);put(memory,abi.gp[4]+4u,0x1900);}
template<class F>void rejects(F&&f,const char*message){bool rejected{};try{f();}catch(const std::runtime_error&){rejected=true;}check(rejected,message);}
}
int main(){try{
    constexpr SourcePhysicsContactRuntimeCall descriptor{0xf2091,0xb9800,false};Memory memory,other;SourcePhysicsContactRecordObserver observe=[](const auto&){};NativeTu15ObjectVelocityHandoffActivity activity;unsigned callbacks{};auto abi=input();packet(memory,abi);const auto before=abi;
    check(!native_tu15_handoff_object_velocity(memory,descriptor,abi,observe),"Unbound memory did not retain its ordinary owner");check(abi.gp==before.gp&&abi.eflags==before.eflags&&abi.xmm==before.xmm,"Unbound handoff changed caller state");
    {
        NativeTu15ObjectVelocityHandoffBinding binding(memory,[&](auto&m,const auto&call,auto&actual,const auto&observer){check(&m==&memory&&call.instruction==descriptor.instruction&&call.target==descriptor.target&&!call.tail,"Actual handoff descriptor differs");check(&observer==&observe,"Handoff changed actual observer ownership");++callbacks;actual.gp[4]+=8u;},&activity);
        check(native_tu15_handoff_object_velocity(memory,descriptor,abi,observe),"Explicit handoff did not run");check(callbacks==1&&activity.attempts==1&&activity.completed==1&&activity.refusals==0,"Native handoff activity differs");
        auto unrelated=input();packet(other,unrelated);check(!native_tu15_handoff_object_velocity(other,descriptor,unrelated,observe),"Memory-scoped handoff escaped its owner");
        unsigned nested{};{NativeTu15ObjectVelocityHandoffBinding inner(memory,[&](auto&,const auto&,auto&actual,const auto&){++nested;actual.gp[4]+=8u;});abi=input();packet(memory,abi);check(native_tu15_handoff_object_velocity(memory,descriptor,abi,observe),"Nested handoff unavailable");}check(nested==1&&callbacks==1,"Nested handoff used the wrong explicit owner");
        abi=input();packet(memory,abi);check(native_tu15_handoff_object_velocity(memory,descriptor,abi,observe)&&callbacks==2,"Nested handoff did not restore outer owner");
        auto bad=input();packet(memory,bad);rejects([&]{native_tu15_handoff_object_velocity(memory,{0xf2092,0xb9800,false},bad,observe);},"Wrong source callsite was admitted");
        bad=input();packet(memory,bad);rejects([&]{native_tu15_handoff_object_velocity(memory,{0xf2091,0xb9801,false},bad,observe);},"Wrong native owner was admitted");
        bad=input();packet(memory,bad);rejects([&]{native_tu15_handoff_object_velocity(memory,{0xf2091,0xb9800,true},bad,observe);},"Tail-call descriptor was admitted");
        bad=input();packet(memory,bad);bad.floating_point.reset();rejects([&]{native_tu15_handoff_object_velocity(memory,descriptor,bad,observe);},"Missing numerical owner was admitted");
        bad=input();packet(memory,bad);bad.active_mmx=SourcePhysicsMmxAlias{};rejects([&]{native_tu15_handoff_object_velocity(memory,descriptor,bad,observe);},"MMX owner was admitted");
        bad=input(6);packet(memory,bad);rejects([&]{native_tu15_handoff_object_velocity(memory,descriptor,bad,observe);},"Unsupported live numerical depth was admitted");
        bad=input();packet(memory,bad);put(memory,bad.gp[4],0xf2097);rejects([&]{native_tu15_handoff_object_velocity(memory,descriptor,bad,observe);},"Wrong retained return packet was admitted");
        bool foreign_refused{};std::thread worker([&]{auto foreign=input();packet(other,foreign);try{native_tu15_handoff_object_velocity(memory,descriptor,foreign,observe);}catch(const std::runtime_error&){foreign_refused=true;}});worker.join();check(foreign_refused,"Foreign executor silently used a fallback owner");check(callbacks==2,"An unsupported descriptor invoked native physics");
    }
    abi=input();packet(memory,abi);check(!native_tu15_handoff_object_velocity(memory,descriptor,abi,observe),"Completed binding leaked into later work");
    for(unsigned depth:{0u,1u,3u,5u}){
        auto actual=input(depth);packet(memory,actual);const auto initial=actual;NativeTu15ObjectVelocityHandoffBinding binding(memory,[&](auto&,const auto&,auto&a,const auto&){a.gp[4]+=8u;a.floating_point->mxcsr|=0x20u;},&activity);check(native_tu15_handoff_object_velocity(memory,descriptor,actual,observe),"Live numerical handoff failed");check(actual.gp[4]==initial.gp[4]+8u&&actual.xmm==initial.xmm&&actual.x87_depth==depth&&actual.floating_point->mxcsr==0x1fa0u,"Handoff changed unrelated inputs or lost genuine FP status");for(unsigned n=0;n<depth;++n)check(std::memcmp(&actual.x87[n],&initial.x87[n],10)==0,"Handoff changed a live extended value");
    }
    for(unsigned fault=0;fault<4;++fault){
        auto actual=input(1);packet(memory,actual);NativeTu15ObjectVelocityHandoffBinding binding(memory,[&](auto&,const auto&,auto&a,const auto&){a.gp[4]+=fault==0?4u:8u;if(fault==1)a.gp[3]^=1u;if(fault==2)a.x87[0]+=1.L;if(fault==3)throw std::runtime_error("Actual native-owner failure");},&activity);rejects([&]{native_tu15_handoff_object_velocity(memory,descriptor,actual,observe);},"Active malformed native completion silently fell back");check(actual.gp[4]!=(0x1800u),"Handoff rolled back authentic partial completion");
    }
    // Exercise the exact generated helper against the actual VehicleState
    // save/load path. These are ABI transport tests, not physics outcomes.
    for(const auto image:{SourcePhysicsContactImage::MapPack11,SourcePhysicsContactImage::TitleUpdate15}){
        auto actual=input(3);packet(memory,actual);const auto initial=actual;SourcePhysicsContactRuntimeServices services;FP controls{0x27f,0x1f80};FpScope scope(controls);VehicleState state(memory,observe,services,image);state.load(actual);unsigned used{};NativeTu15ObjectVelocityHandoffBinding binding(memory,[&](auto&,const auto&,auto&a,const auto&observer){check(&observer==&observe,"Generated helper changed observer ownership");++used;a.gp[4]+=8u;a.floating_point->mxcsr|=0x20u;});const auto handled=handoff_native_object_velocity(state);const auto after=state.save();if(image==SourcePhysicsContactImage::MapPack11){check(!handled&&used==0&&after.gp==initial.gp,"TU1.1 crossed a TU1.5-only handoff");}else{check(handled&&used==1&&after.gp[4]==initial.gp[4]+8u&&after.xmm==initial.xmm&&after.x87_depth==initial.x87_depth&&after.floating_point->mxcsr==0x1fa0u,"Generated helper lost actual RET/FP/SIMD state");for(unsigned n=0;n<initial.x87_depth;++n)check(std::memcmp(&after.x87[n],&initial.x87[n],10)==0,"Generated helper changed a retained extended value");}
    }
    std::cout<<"Native object velocity handoff: memory/executor/LIFO admission, exact source helper, FP/live-value transport and rejection controls passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
