#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/source_physics_store_observers_tu15.hpp"
#include <map>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void need(bool yes){if(!yes)throw std::runtime_error("Damage observation contract failed");}
struct Memory final:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::byte> bytes;
 void word(std::uint32_t p,std::uint32_t v){for(unsigned i=0;i<4;++i)bytes[p+i]=std::byte(v>>(8*i));}
 void read(std::uint32_t p,std::span<std::byte> out)const override{for(auto &b:out)b=bytes.at(p++);}
 void write(std::uint32_t p,std::span<const std::byte> in)override{for(auto b:in){need(bytes.contains(p));bytes[p++]=b;}}
 Memory(){
  word(0x4d6d44,0x10000);word(0x4dc04c,0x10000);word(0x10008,4);word(0x10004,0x20000);
  word(0x220a4,0x30000);word(0x3004c,4);word(0x220dc,0x10);
  word(0x4e78d0,0x50000);word(0x4ecbd0,0x50000);word(0x50044,0x60000);word(0x60008,0x40000);word(0x400d4,1);
  for(unsigned i=0;i<0x140;++i)bytes[0x7ff00+i]=std::byte{};
  word(0x80000,0x12345678);word(0x80004,1);word(0x80008,0x80);
 }
};
struct Result {Memory memory;SourcePhysicsContactRuntimeAbi abi{};std::vector<std::uint32_t> instructions;std::vector<SourcePhysicsContactRecordStore> stores;std::array<unsigned,2> notices{};};
Result run(bool selected,SourcePhysicsContactImage image,bool nested,bool listeners=true,unsigned listener_mask=1){
 Result result;auto &a=result.abi;a.gp[4]=0x80000;a.eflags=0x202;
 if(nested)result.memory.word(0x80004,0x80);
 SourcePhysicsContactRuntimeServices services;
 if(listeners)services.instruction=[&](auto pc){result.instructions.push_back(pc);};
 SourcePhysicsContactRecordObserver caller,registered;
 if(listeners && (listener_mask&1))caller=[&](const auto &store){++result.notices[0];result.stores.push_back(store);};
 if(listeners && (listener_mask&2))registered=[&](const auto &store){++result.notices[1];if(!(listener_mask&1))result.stores.push_back(store);};
 const auto observe=source_physics_store_observers_tu15(caller,registered);
 const auto entry=nested?0xa87e0u:0xb6760u;
 (selected?source_run_damage_delivery_runtime_selected_observations:source_run_damage_delivery_runtime)(
  result.memory,entry,a,image,{0x27f,0x1f80},services,observe);
 return result;
}
void same(const Result&a,const Result&b){
 need(a.memory.bytes==b.memory.bytes && a.abi.gp==b.abi.gp && a.abi.xmm==b.abi.xmm && a.abi.eflags==b.abi.eflags && a.abi.fs_base==b.abi.fs_base && a.abi.x87_depth==b.abi.x87_depth && a.abi.x87_status==b.abi.x87_status);
 need(a.stores.size()==b.stores.size());
 for(unsigned i=0;i<a.stores.size();++i)need(a.stores[i].address==b.stores[i].address && a.stores[i].before==b.stores[i].before && a.stores[i].written==b.stores[i].written);
}
}
int main()try{
 for(bool nested:{false,true}){
  auto legacy=run(false,SourcePhysicsContactImage::TitleUpdate15,nested);
  auto selected=run(true,SourcePhysicsContactImage::TitleUpdate15,nested);same(legacy,selected);
  need(!legacy.instructions.empty() && legacy.instructions.size()==selected.instructions.size());
  need(legacy.instructions.front()==(nested?0xa87e0u:0xb6760u));
  need(selected.instructions.front()==(nested?0xb1c30u:0xb7e50u));
  need(legacy.stores.back().instruction==0xb67a7u && selected.stores.back().instruction==0xb7e97u);
  auto disc=run(false,SourcePhysicsContactImage::MapPack11,nested),selected_disc=run(true,SourcePhysicsContactImage::MapPack11,nested);
  same(disc,selected_disc);need(disc.stores==selected_disc.stores && disc.instructions==selected_disc.instructions);
  auto absent=run(true,SourcePhysicsContactImage::TitleUpdate15,nested,false);
  need(absent.memory.bytes==selected.memory.bytes && absent.abi.gp==selected.abi.gp && absent.instructions.empty() && absent.stores.empty());
  for(unsigned mask:{1u,2u,3u}){
   const auto paired=run(true,SourcePhysicsContactImage::TitleUpdate15,nested,true,mask);same(selected,paired);
   need(paired.notices[0]==((mask&1)?selected.stores.size():0) && paired.notices[1]==((mask&2)?selected.stores.size():0));
  }
 }
 std::cout<<"PASS: legacy/disc preservation, selected TU1.5 direct/nested PCs, identical writes/ABI, absent/caller/registered/combined store listeners exactly once\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
