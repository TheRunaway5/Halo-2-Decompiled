#include "halo2/source_startup_sound_storage_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void reject(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught,"Invalid sound arena prefix accepted");}
}
int main(int argc,char** argv){try{
 check(argc==2,"Expected original sound arena prefix fixture");std::ifstream input(argv[1]);Json reference;input>>reference;
 check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original prefix fixture differs");
 SourceRuntimeMemory memory;mount_native_tu15_startup_sound_storage_data(memory);
 memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared main-prefix stack");
 memory.mount(0x41b6d4,std::array<std::byte,4>{std::byte{0xb8},std::byte{},std::byte{},std::byte{0x80}},SourceRuntimeMemory::Access::ReadOnly,"actual kernel allocation import");
 SourcePhysicsPoolHostStorage raw(memory,0x10000000);
 SourcePhysicsWorldRuntimeAbi abi{};abi.gp=reference.at("input_gp").get<decltype(abi.gp)>();abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 Json stores=Json::array(),instructions=Json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){instructions.push_back(pc);};
 services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Unowned sound arena prefix dependency");};
 SourcePhysicsContactRecordObserver observer=[&](const auto& store){std::uint64_t value=0;for(std::size_t i=0;i<store.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(store.written[i]))<<(8*i);stores.push_back({store.instruction,store.address,store.written.size(),value});};
 auto invalid=abi;invalid.gp[3]=1;reject([&]{source_prepare_native_tu15_sound_arena(memory,invalid,{0x27f,0x1f80},services,raw,observer);});check(raw.live_allocations()==0,"Rejected prefix allocated storage");
 source_prepare_native_tu15_sound_arena(memory,abi,{0x27f,0x1f80},services,raw,observer);
 check(abi.gp==reference.at("returned_gp").get<decltype(abi.gp)>(),"Native sound arena prefix registers differ");
 check((abi.eflags&0xcc5)==(reference.at("returned_eflags").get<std::uint32_t>()&0xcc5),"Native sound arena prefix defined flags differ");
 check(instructions==reference.at("instructions")&&stores==reference.at("source_stores"),"Native sound arena prefix instruction/store order differs");
 check(raw.live_allocations()==1,"Native sound arena prefix raw owner differs");
 check(std::ranges::all_of(memory.view(0x10000000,0x23000),[](auto b){return b==std::byte{};}),"Native sound arena prefix was not actually zeroed by platform allocation");
 const auto native=memory.native_address(0x10000000,0x23000);check(native.value>0xffffffffu&&memory.source_address(native,0x23000)==0x10000000,"Native sound arena prefix lost wide backing");
 reject([&]{source_prepare_native_tu15_sound_arena(memory,abi,{0x27f,0x1f80},services,raw,observer);});
 std::cout<<"Original main12008..1201F sound arena prefix: "<<instructions.size()<<" instructions; actual0x23000 native64 allocation\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
