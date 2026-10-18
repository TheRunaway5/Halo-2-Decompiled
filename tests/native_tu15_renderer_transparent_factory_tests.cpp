#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_renderer_transparent_storage_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
std::uint32_t word(const SourceRuntimeMemory&m,unsigned p){return Bytes(m.view(p,4)).u32(0);}
}
int main(int argc,char**argv)try{
 check(argc==1||argc==3,"usage: transparent-factory-test [MCC_MAP SAVE_DIRECTORY]");
 auto cold=native_tu15_world_memory({0x60212000u,{},"isolated transparent factory clock"});
 check(!word(cold,0x4c2aa8u),"cold factory supplied a fabricated transparent plane pointer");
 check(word(cold,0x473658u)==0xffffffffu,"transparent selector was preconstructed");
 check(std::ranges::all_of(cold.view(0x4c2aa0u,18),[](auto b){return b==std::byte{};})&&!word(cold,0x52dfd0u),"transparent factory fabricated runtime bindings");
 const std::array<char,20> name{"transparent planes"};check(cold.is_read_only(0x45c548u,20)&&std::ranges::equal(cold.view(0x45c548u,20),std::as_bytes(std::span(name))),"transparent factory name/padding provenance differs");
 bool not_ready{};try{native_tu15_renderer_transparent_storage_initial_views(cold);}catch(const std::exception&){not_ready=true;}check(not_ready,"cold transparent factory claimed completed allocations");
 for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,4>{{{0x473658u,4},{0x4c2aa0u,18},{0x52dfd0u,4},{0x45c548u,20}}})check(cold.native_address(p,n).value>UINT32_MAX,"transparent cold backing narrowed to32bits");
 if(argc==3){
  if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;Cache cache(argv[1]);check(cache.is_mcc(),"transparent native integration requires real MCC input");std::filesystem::create_directories(argv[2]);PhysicsLiveWorldTu15 world(cache,argv[2]);
  check(!word(world.memory(),0x4c2aa8u)&&!word(world.memory(),0x4c2aacu)&&!word(world.memory(),0x52dfd0u),"normal World preconstructed transparent resources");
  check((world.call_actual(0x1e0f0u)&255u)==1,"genuine transparent constructor returned failure");
  const auto v=native_tu15_renderer_transparent_storage_initial_views(world.memory());
  check(v.planes.value>UINT32_MAX&&v.indices.value>UINT32_MAX&&v.container.value>UINT32_MAX,"normal World transparent resources lack native64 owner");
  check(v.bucket_count==4096&&v.node_count==2048&&v.node_payload_bytes==16,"normal World original transparent container dimensions differ");
  std::cout<<nlohmann::json{{"map",cache.name()},{"status","PASS"},{"plane_source",word(world.memory(),0x4c2aa8u)},{"index_source",word(world.memory(),0x4c2aacu)},{"container_source",word(world.memory(),0x52dfd0u)},{"planes_native",v.planes.value},{"indices_native",v.indices.value},{"container_native",v.container.value},{"actual_original_constructor",true},{"full_original_idle_hash_freechain_validated",true},{"reference_required",false},{"gpu_or_full_frame_claim",false}}.dump()<<'\n';
 }else std::cout<<"PASS exact cold transparent factory; original constructor resources remain unready\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
