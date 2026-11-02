#include "halo2/native_tu15_vehicle_flip_handoff.hpp"
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <thread>
using namespace halo2;
void check(bool yes){if(!yes)std::abort();}
struct Memory final:SourcePhysicsContactMemory {std::uint32_t returned=0xf24ddu;void read(std::uint32_t,std::span<std::byte>out)const override{check(out.size()==4);for(unsigned n=0;n<4;++n)out[n]=std::byte(returned>>(n*8u));}void write(std::uint32_t,std::span<const std::byte>)override{std::abort();}};
SourcePhysicsContactRuntimeAbi input(){SourcePhysicsContactRuntimeAbi a;a.gp={0xe2a10032,2,3,4,0x600e1000,6,7,8};a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};a.x87_status=0x3000;a.x87_depth=2;a.x87[0]=.75L;a.x87[1]=-.5L;for(unsigned n=0;n<8;++n)a.xmm[n]={n,n+1,n+2,n+3};return a;}
int main(){Memory memory,other;const SourcePhysicsContactRuntimeCall call{0xf24d8,0xf1dd0,false};auto a=input();check(!native_tu15_handoff_vehicle_flip(memory,call,a));NativeTu15VehicleFlipHandoffActivity activity;bool trace{};unsigned calls{};
 {NativeTu15VehicleFlipHandoffBinding scope(memory,[&](auto object,auto sp,const auto&actual,auto&fp,unsigned depth,const auto&){check(object==input().gp[0]&&sp==input().gp[4]&&actual.gp==input().gp&&depth==2);++calls;fp.mxcsr|=0x20u;return true;},{0x600e0000,0x600f0000,&trace,nullptr,true},&activity);
  const auto before=a;check(native_tu15_handoff_vehicle_flip(memory,call,a));check(a.gp[0]==(before.gp[0]&0xffffff00u|1u)&&a.gp[4]==before.gp[4]+4u&&a.xmm==before.xmm&&a.x87_depth==before.x87_depth&&std::memcmp(&a.x87[0],&before.x87[0],10)==0&&a.floating_point->mxcsr==0x1fa0u);for(unsigned n=1;n<8;++n)if(n!=4)check(a.gp[n]==before.gp[n]);
  {NativeTu15VehicleFlipHandoffBinding nested(memory,[](auto,auto,const auto&,auto&,unsigned,const auto&){return false;},{0x600e0000,0x600f0000,nullptr,nullptr,true});a=input();check(native_tu15_handoff_vehicle_flip(memory,call,a));check((a.gp[0]&255u)==0);}
  const auto refused=[&](auto c,auto state,const auto&observe){bool threw{};try{native_tu15_handoff_vehicle_flip(memory,c,state,observe);}catch(const std::runtime_error&){threw=true;}check(threw);};
  refused(SourcePhysicsContactRuntimeCall{0xf2091,0xf1dd0,false},input(),SourcePhysicsContactRecordObserver{});memory.returned=0;refused(call,input(),SourcePhysicsContactRecordObserver{});memory.returned=0xf24dd;
  auto invalid=input();invalid.x87_depth=6;refused(call,invalid,SourcePhysicsContactRecordObserver{});invalid=input();invalid.active_mmx=SourcePhysicsMmxAlias{};refused(call,invalid,SourcePhysicsContactRecordObserver{});invalid=input();invalid.eflags|=0x400;refused(call,invalid,SourcePhysicsContactRecordObserver{});invalid=input();invalid.floating_point.reset();refused(call,invalid,SourcePhysicsContactRecordObserver{});
  trace=true;refused(call,input(),SourcePhysicsContactRecordObserver{});trace=false;bool wrong_thread{};std::thread thread([&]{auto state=input();try{native_tu15_handoff_vehicle_flip(memory,call,state);}catch(const std::runtime_error&){wrong_thread=true;}});thread.join();check(wrong_thread);a=input();check(!native_tu15_handoff_vehicle_flip(other,call,a));
 }
 check(calls==1&&activity.completed==1&&activity.refusals==8);a=input();check(!native_tu15_handoff_vehicle_flip(memory,call,a));
 {NativeTu15VehicleFlipHandoffBinding failure(memory,[](auto,auto,const auto&,auto&fp,unsigned,const auto&)->bool{fp.mxcsr|=0x20;throw std::runtime_error("declared owner failure");},{0x600e0000,0x600f0000,nullptr,nullptr,true});const auto before=a;bool threw{};try{native_tu15_handoff_vehicle_flip(memory,call,a);}catch(const std::runtime_error&){threw=true;}check(threw&&a.gp==before.gp&&a.floating_point->mxcsr==0x1fa0);}
 {NativeTu15VehicleFlipHandoffBinding restricted(memory,[](auto,auto,const auto&,auto&,unsigned,const auto&){return true;},{0x600e0000,0x600f0000,nullptr,nullptr,false});bool threw{};try{a=input();native_tu15_handoff_vehicle_flip(memory,call,a,[](const auto&){});}catch(const std::runtime_error&){threw=true;}check(threw);}
}
