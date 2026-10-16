#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));m.write(p,bytes);}
std::vector<std::byte> unhex(const std::string& hex){auto n=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> result(hex.size()/2);for(std::size_t i=0;i<result.size();++i)result[i]=std::byte((n(hex[i*2])<<4)|n(hex[i*2+1]));return result;}
template<class F>void rejected(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught,"Unowned or invalid sound DMA request was accepted");}
void run(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output){
 SourceRuntimeMemory memory;mount_native_tu15_sound_dma_imports(memory);
 memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared SDK DMA caller stack and descriptor");
 memory.mount(0x390bc4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source loader-zeroed contiguous allocation statistics");
 memory.mount(0x3912a0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source loader-zeroed SDK statistics pointer");
 memory.mount(0x41b640,std::array<std::byte,4>{std::byte{0xa6},std::byte{},std::byte{},std::byte{0x80}},SourceRuntimeMemory::Access::ReadOnly,"existing original physical allocation import");
 SourcePhysicsWorldRuntimeAbi abi{};abi.gp=reference.at("input_gp").get<decltype(abi.gp)>();abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices services;
 services.instruction=[&](auto pc){instructions.push_back(pc);};
 SourcePhysicsContactRecordObserver observe=[&](const auto& store){std::uint64_t value=0;check(store.written.size()<=8,"Unexpected DMA store width");for(std::size_t i=0;i<store.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(store.written[i]))<<(8*i);stores.push_back({store.instruction,store.address,store.written.size(),value});};
 put(memory,0x2008000,0x2100000);source_run_sound_dma_storage_tu15(memory,0x387bb5,abi,{0x27f,0x1f80},services,observe);
 check(instructions==reference.at("init_instructions")&&stores==reference.at("init_stores"),"Original SDK statistics constructor differs");instructions=Json::array();stores=Json::array();
 put(memory,0x2001000,0);put(memory,0x2001004,0);put(memory,0x2001008,reference.at("requested"));
 put(memory,0x2008000,0x2100000);put(memory,0x2008004,0x2001000);put(memory,0x2008008,reference.at("alignment"));put(memory,0x200800c,4);abi.gp[4]=0x2008000;
 {
  SourceRuntimeSoundDmaResources owner(memory,output);check(owner.quiescent()&&owner.device_id()==output->device_id(),"DMA owner lacks retained actual output");
  services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto&){
   const auto before=state;
   if(call.instruction==0x3879dc){
    auto bad=call;bad.tail=true;rejected([&]{owner.dispatch(bank,bad,state);});check(state.gp==before.gp&&owner.allocations().empty(),"Rejected DMA call changed caller/resources");
    SourceRuntimeMemory foreign;rejected([&]{owner.dispatch(foreign,call,state);});
   }
   check(owner.dispatch(bank,call,state),"Reached DMA platform request was not owned");
   if(call.instruction==0x3879dc){
    // Declared poison from the same independent original platform experiment.
    // Original SDK code, rather than the provider, must clear all capacity.
    std::ranges::fill(memory.mutable_view(state.gp[0],owner.capacity(state.gp[0])),std::byte{0xa5});
   }
  };
  SourceRuntimeSoundDmaResources::CallScope scope(&owner);
  source_run_sound_dma_storage_tu15(memory,0x38e71c,abi,{0x27f,0x1f80},services,observe);
  check(abi.gp==reference.at("returned_gp").get<decltype(abi.gp)>(),"Native SDK DMA helper returned different registers");
  check((abi.eflags&0xcc5)==(reference.at("returned_eflags").get<std::uint32_t>()&0xcc5),"Native SDK DMA helper returned different defined flags");
  check(instructions==reference.at("instructions")&&stores==reference.at("source_stores"),"Native SDK DMA CPU instruction/store order differs");
  const auto descriptor=unhex(reference.at("descriptor_hex"));check(std::ranges::equal(memory.view(0x2001000,12),descriptor),"Native SDK DMA descriptor differs");
  const auto allocations=owner.allocations();check(allocations.size()==1&&!owner.quiescent(),"Native SDK DMA lifetime accounting differs");
  const auto& row=allocations[0];check(row.source==0x85000000&&row.physical==0x5000000&&row.requested==reference.at("requested")&&row.capacity==reference.at("capacity")&&
      row.alignment==reference.at("alignment")&&row.protection==4&&row.source_descriptor==0x2001000&&row.native.value>0xffffffffu&&
      memory.source_address(row.native,row.capacity)==row.source,"Native SDK DMA real/wide resource binding differs");
  check(std::ranges::all_of(memory.view(row.source,row.capacity),[](auto b){return b==std::byte{};}),"Native SDK did not clear complete actual DMA capacity");
  rejected([&]{owner.native_address(row.source+4);});
  bool wrong_thread=false;std::thread other([&]{try{owner.allocations();}catch(const std::runtime_error&){wrong_thread=true;}});other.join();check(wrong_thread,"Foreign DMA thread was admitted");
 }
 rejected([&]{memory.view(0x85000000,reference.at("capacity").get<unsigned>());});
 check(output->device_id()!=0,"DMA cleanup lost retained native output");
 std::cout<<"Original SDK DMA size "<<reference.at("requested")<<": "<<instructions.size()<<" exact instructions; "<<reference.at("capacity")<<" real native64 bytes\n";
}
void failure_scope(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output){
 SourceRuntimeMemory memory;mount_native_tu15_sound_dma_imports(memory);
 memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared failed SDK caller");
 memory.mount(0x390bc4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source contiguous statistics");
 memory.mount(0x3912a0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"source statistics pointer");
 memory.mount(0x41b640,std::array<std::byte,4>{std::byte{0xa6},std::byte{},std::byte{},std::byte{0x80}},SourceRuntimeMemory::Access::ReadOnly,"source physical allocation import");
 SourcePhysicsWorldRuntimeAbi abi{};abi.gp=reference.at("input_gp").get<decltype(abi.gp)>();abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 put(memory,0x2008000,0x2100000);SourcePhysicsWorldRuntimeServices services;
 source_run_sound_dma_storage_tu15(memory,0x387bb5,abi,{0x27f,0x1f80},services);
 put(memory,0x2001008,reference.at("requested"));put(memory,0x2008000,0x2100000);put(memory,0x2008004,0x2001000);
 put(memory,0x2008008,reference.at("alignment"));put(memory,0x200800c,4);abi.gp[4]=0x2008000;
 {
  SourceRuntimeSoundDmaResources owner(memory,output);
  services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto&){check(owner.dispatch(bank,call,state),"Failed DMA request was not owned");throw std::runtime_error("declared failure after actual native allocation");};
  rejected([&]{SourceRuntimeSoundDmaResources::CallScope scope(&owner);source_run_sound_dma_storage_tu15(memory,0x38e71c,abi,{0x27f,0x1f80},services);});
  check(!owner.quiescent(),"Partial sound DMA creation was admitted as quiescent");
  rejected([&]{owner.allocations();});rejected([&]{SourceRuntimeSoundDmaResources::CallScope scope(&owner);});
  check(memory.native_address(0x85000000,reference.at("capacity").get<unsigned>()).value>0xffffffffu,"Failed DMA owner lost its retained diagnostic pages");
  check(std::ranges::all_of(memory.view(0x2001000,8),[](auto b){return b==std::byte{};}),"Failed DMA service fabricated descriptor publication");
 }
 rejected([&]{memory.view(0x85000000,reference.at("capacity").get<unsigned>());});
}
}
int main(int argc,char** argv){try{
 check(argc==2,"Supply original DMA reference");std::ifstream input(argv[1]);Json reference;input>>reference;
 check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original DMA reference differs");
 auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});
 const auto* driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","DMA ownership proof requires a real native output backend");
 for(const auto& row:reference.at("cases"))run(row,output);
 failure_scope(reference.at("cases").at(0),output);
 std::cout<<"Actual retained sound output backend: "<<driver<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
