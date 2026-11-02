#include "halo2/native_tu15_contact_time_input_observer.hpp"
#include <cstdlib>
#include <stdexcept>
#include <thread>
using namespace halo2;
void check(bool value){if(!value)std::abort();}
struct Memory final:SourcePhysicsContactMemory {
 mutable unsigned reads{};unsigned writes{};
 void read(std::uint32_t,std::span<std::byte>out)const override{++reads;for(auto&b:out)b=std::byte{0x35};}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;}
};
int main(){
 Memory first,other;const NativeTu15ContactTimeInput input{{0x1234},{0x5678},{0x9012},{0x3456},0x3f800000,{0x27f,0x100,0x1f80},5};
 check(!native_tu15_observe_contact_time_input(first,input));
 NativeTu15ContactTimeInputObserverActivity activity;unsigned outer{},inner{};
 {
  NativeTu15ContactTimeInputObserverBinding scope(first,[&](const auto&m,const auto&i){
   check(&m==&first);check(i.island.identity==input.island.identity&&i.configuration.identity==input.configuration.identity&&i.entry.identity==input.entry.identity&&i.output.identity==input.output.identity&&i.time_bits==input.time_bits&&i.floating_point.control_word==input.floating_point.control_word&&i.floating_point.status==input.floating_point.status&&i.floating_point.mxcsr==input.floating_point.mxcsr&&i.preserved_x87_depth==5);++outer;
  },&activity);
  check(native_tu15_observe_contact_time_input(first,input));check(!native_tu15_observe_contact_time_input(other,input));
  {NativeTu15ContactTimeInputObserverBinding nested(first,[&](const auto&,const auto&){++inner;});check(native_tu15_observe_contact_time_input(first,input));}
  check(native_tu15_observe_contact_time_input(first,input));
  bool wrong_executor{};std::thread thread([&]{try{native_tu15_observe_contact_time_input(first,input);}catch(const std::runtime_error&){wrong_executor=true;}});thread.join();check(wrong_executor);
 }
 check(!native_tu15_observe_contact_time_input(first,input));check(outer==2&&inner==1&&activity.attempts==3&&activity.completed==2&&activity.failures==1);
 {NativeTu15ContactTimeInputObserverBinding failing(first,[](const auto&,const auto&){throw std::runtime_error("diagnostic failure");},&activity);bool propagated{};try{native_tu15_observe_contact_time_input(first,input);}catch(const std::runtime_error&){propagated=true;}check(propagated);}
 check(activity.attempts==4&&activity.completed==2&&activity.failures==2);check(first.reads==0&&first.writes==0&&other.reads==0&&other.writes==0);
}
