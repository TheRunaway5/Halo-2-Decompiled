#include "halo2/source_startup_sound_storage_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <sys/mman.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void put(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.write(p,bytes);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p){const auto b=memory.view(p,4);std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return value;}
void constant(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));memory.mount(p,bytes,SourceRuntimeMemory::Access::ReadOnly,"original immutable helper/import identity");}
std::vector<std::byte> unhex(const std::string& text){auto n=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> result(text.size()/2);for(std::size_t i=0;i<result.size();++i)result[i]=std::byte((n(text[i*2])<<4)|n(text[i*2+1]));return result;}
void same(const SourceRuntimeMemory& memory,std::uint32_t p,unsigned count,const Json& hex){const auto expected=unhex(hex);check(expected.size()==count&&std::ranges::equal(memory.view(p,count),expected),"Native sound CPU prefix bytes differ");}
struct RequiredDriver {};
}
int main(int argc,char** argv){try{
 check(argc==2,"Expected original sound storage prefix fixture");std::ifstream input(argv[1]);Json expected;input>>expected;
 check(expected.at("passed")==true&&expected.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong sound CPU prefix reference");
 SourceRuntimeMemory memory;mount_native_tu15_startup_sound_storage_data(memory);
 memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared caller stack and arena object");
 memory.mount(0x4f2950,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,"declared previous game arena bounds/CRC");
 memory.mount(0x51df2c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared previous game arena object");
 memory.mount(0x4f2cf0,std::array<std::byte,52>{},SourceRuntimeMemory::Access::ReadWrite,"declared existing physical partition bounds");
 memory.mount(0x56e225,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"declared cold CRC flag");
 memory.mount(0x56e258,std::vector<std::byte>(0x400),SourceRuntimeMemory::Access::ReadWrite,"declared CRC table backing");
 constexpr std::uint32_t fs=0x60200000;memory.mount(fs,std::vector<std::byte>(0x15000),SourceRuntimeMemory::Access::ReadWrite,"declared retained passive CRT/FS/tick context");
 memory.mount(0x573840,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared retained CRT pointer");
 auto* mapped=::mmap(nullptr,0x4000000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);check(mapped!=MAP_FAILED,"Native physical input allocation failed");
 std::shared_ptr<void> lifetime(mapped,[](void* p){::munmap(p,0x4000000);});auto backing=std::make_shared<SourceRuntimeMemoryBacking>(lifetime,std::span(static_cast<std::byte*>(mapped),0x4000000));
 memory.mount(0x81000000,backing,SourceRuntimeMemory::Access::ReadWrite,"actual declared native physical pages");
 constant(memory,0x45f1d8,0x127210);constant(memory,0x474cbc,0x486038);constant(memory,0x474cb8,0x486034);constant(memory,0x486034,0x45f048);
 memory.mount(0x45f048,std::array<std::byte,8>{std::byte{0xf0},std::byte{0xf3},std::byte{0x11},std::byte{},std::byte{0x20},std::byte{0xf4},std::byte{0x11},std::byte{}},SourceRuntimeMemory::Access::ReadOnly,"actual raw allocator vtable");
 constant(memory,0x41b6d4,0x800000b8);constant(memory,0x41b648,0x800000b6);constant(memory,0x41b668,0x80000080);constant(memory,0x41b774,fs+0x12000);
 put(memory,fs+0x28,fs+0x13000);put(memory,0x573840,fs+0x14000);put(memory,fs+0x14014,1);
 put(memory,0x4f2950,0x81000000);put(memory,0x4f2954,0x100);put(memory,0x4f2958,0x200);put(memory,0x4f295c,0x12345678);
 put(memory,0x51df2c,0x2001000);put(memory,0x2001000,0x45f1d8);put(memory,0x4f2cfc,0x1000000);put(memory,0x4f2d10,0x5000000);
 Json allocations=Json::array(),protections=Json::array(),instructions=Json::array(),stores=Json::array();unsigned utc=0,ticks=0;
 SourcePhysicsPoolHostStorage raw(memory,0x10000000,[&](const auto& event){check(event.allocation,"Unexpected sound raw free");allocations.push_back({{"source",event.result[1]},{"requested",event.request[2]},{"reserved",event.result[2]},{"allocation_type",event.request[3]}});});
 SourcePhysicsContactRecordObserver observe=[&](const auto& store){std::uint64_t value=0;check(store.written.size()<=8,"Unexpected sound store width");for(std::size_t i=0;i<store.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(store.written[i]))<<(8*i);stores.push_back({store.instruction,store.address,store.written.size(),value});};
 SourcePhysicsWorldRuntimeAbi abi{};for(unsigned i=0;i<8;++i)abi.gp[i]=0x11000000+i*0x111111;abi.gp[3]=0;abi.gp[4]=0x2008000;abi.eflags=0x202;abi.fs_base=fs;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 SourcePhysicsWorldRuntimeAbi stopped{};SourcePhysicsWorldRuntimeServices services;
 services.instruction=[&](auto pc){if(pc==0x33b595){put(memory,fs+0x12000,expected.at("tick_inputs").at(ticks++));}instructions.push_back(pc);};
 services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
  check(&bank==&memory,"Foreign sound CPU memory");
  if(call.target==0x224ec0){check(call.instruction==0x128050&&!call.tail&&word(memory,state.gp[4])==0x128055,"Wrong required native sound driver frame");stopped=state;throw RequiredDriver{};}
  if(raw.try_call(bank,call,state))return;
  if(source_startup_sound_storage_tu15_owns_entry(call.target)){source_run_startup_sound_storage_tu15(bank,call.target,state,{0x27f,0x1f80},services,observer);return;}
  if(call.target==0x800000b6){const auto sp=state.gp[4];check(call.instruction==0x2db4d1&&call.tail&&word(memory,sp)==0x21f0a3&&word(memory,sp+8)==0x300000&&word(memory,sp+12)==4,"Sound physical protection frame differs");const auto source=word(memory,sp+4);const auto native=memory.native_address(source,0x300000);check(::mprotect(reinterpret_cast<void*>(native.value),0x300000,PROT_READ|PROT_WRITE)==0,"Actual native sound protection failed");protections.push_back({{"source",source},{"bytes",0x300000},{"protection",4}});state.gp[4]+=16;return;}
  if(call.target==0x80000080){const auto sp=state.gp[4];check(call.instruction==0x33a4d3&&call.tail&&word(memory,sp)==0x32bbbc&&word(memory,sp+4)==state.gp[5]-8,"Actual UTC output frame differs");const auto time=expected.at("utc_filetime").at(utc++).get<std::uint64_t>();put(memory,word(memory,sp+4),std::uint32_t(time));put(memory,word(memory,sp+4)+4,std::uint32_t(time>>32));state.gp[4]+=8;return;}
  if(source_physics_live_support_tu15_owns_entry(call.target)){source_run_physics_live_support_tu15(bank,call.target,state,{0x27f,0x1f80},services,observer);return;}
  throw std::runtime_error("Unowned sound CPU dependency "+std::to_string(call.target));
 };
 source_prepare_native_tu15_sound_arena(memory,abi,{0x27f,0x1f80},services,raw,observe);
 check(instructions==expected.at("prefix_instructions")&&stores==expected.at("prefix_source_stores"),"Sound arena prerequisite prefix differs");
 instructions=Json::array();stores=Json::array();put(memory,0x2008000,0x2100000);abi.gp[4]=0x2008000;
 check(abi.gp==expected.at("source_before_gp").get<decltype(abi.gp)>(),"Sound source prefix input registers differ");
 bool stop=false;try{source_run_startup_sound_storage_with_native_storage(memory,0x127ee0,abi,{0x27f,0x1f80},services,raw,observe);}catch(const RequiredDriver&){stop=true;}
 check(stop&&stopped.gp==expected.at("returned_gp").get<decltype(stopped.gp)>(),"Sound CPU prefix stop registers differ");
 check((stopped.eflags&0xcc5)==(expected.at("returned_eflags").get<std::uint32_t>()&0xcc5),"Sound CPU prefix stop flags differ");
 check(instructions==expected.at("instructions")&&stores==expected.at("source_stores"),"Sound CPU prefix instruction/store order differs");
 check(allocations.size()==expected.at("allocations").size()&&protections.size()==1&&utc==1&&ticks==1,"Sound CPU prefix native resources differ");
 for(std::size_t i=0;i<allocations.size();++i)for(const auto* key:{"source","requested","reserved","allocation_type"})check(allocations[i][key]==expected.at("allocations")[i][key],"Sound CPU raw allocation request differs");
 for(const auto& field:expected.at("fields"))same(memory,field.at("address"),field.at("bytes"),field.at("hex"));
 for(const auto& field:expected.at("allocation_bytes"))same(memory,field.at("source"),field.at("bytes"),field.at("hex"));
 same(memory,0x56e258,0x400,expected.at("crc_table_hex"));check(word(memory,fs+0x14014)==expected.at("crt_seed")&&word(memory,0x4f2d10)==expected.at("physical_top"),"Sound CPU derived CRT/physical state differs");
 check(word(memory,0x4f2c44)==0&&word(memory,0x52d674)==0&&memory.view(word(memory,0x4f2c50)+0x78,1)[0]==std::byte{},"Stopped sound prefix invented readiness");
 std::cout<<"Original127EE0 sound prefix: "<<instructions.size()<<" instructions, exact CPU stores, actual native pages; stops224EC0 without result\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
