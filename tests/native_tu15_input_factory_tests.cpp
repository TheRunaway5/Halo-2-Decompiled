#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_input_startup_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool ok,const char*why){++checks;if(!ok)throw std::runtime_error(why);}
constexpr std::array<std::byte,8> scalars{std::byte{},std::byte{},std::byte{0xf0},std::byte{0x42},std::byte{},std::byte{},std::byte{0x70},std::byte{0x42}};
void share(SourceRuntimeMemory&m){for(unsigned i=0;i<2;++i)m.mount(0x455a90u+i*4u,std::span(scalars).subspan(i*4,4),SourceRuntimeMemory::Access::ReadOnly,"existing authentic shared scalar word");}
void reject(SourceRuntimeMemory&m){const auto prior=m.checkpoint();bool failed{};try{mount_native_tu15_input_startup_data(m);}catch(const std::exception&){failed=true;}check(failed&&m.checkpoint()==prior,"rejected input composition partially mutated existing owner");++negatives;}
}
int main(int argc,char**argv)try{
 check(argc<=2,"usage: input-factory-test [ORIGINAL_XBE|--dump]");auto memory=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=memory.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>bytes;for(auto b:r.bytes)bytes.push_back(std::to_integer<unsigned>(b));j.push_back({{"address",r.address},{"bytes",bytes},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,2>{{{0x4f2970u,0x2c8u},{0x52d4a8u,0x1bcu}}}){
  const auto owner=std::ranges::find(cold.regions,p,&SourceRuntimeMemory::CheckpointRegion::address);check(owner!=cold.regions.end()&&owner->bytes.size()==n&&owner->access==SourceRuntimeMemory::Access::ReadWrite&&!owner->page_bytes&&owner->committed.empty(),"input constructor cold extent/ownership differs");check(std::ranges::all_of(memory.view(p,n),[](auto b){return b==std::byte{};})&&memory.native_address(p,n).value>UINT32_MAX,"input factory preconstructed handles/presence/samples or narrowed backing");bool failed{};try{memory.validate_mutable_view(p,n+1);}catch(const std::out_of_range&){failed=true;}check(failed&&memory.checkpoint()==cold,"input cold region borrowed adjacent storage");++negatives;
 }
 for(unsigned i=0;i<2;++i){const auto owner=std::ranges::find(cold.regions,0x455a90u+i*4u,&SourceRuntimeMemory::CheckpointRegion::address);check(owner!=cold.regions.end()&&owner->bytes.size()==4&&owner->access==SourceRuntimeMemory::Access::ReadOnly&&owner->owner=="native TU1.5 authored biped definition conversion operand"&&std::ranges::equal(owner->bytes,std::span(scalars).subspan(i*4,4)),"input composition replaced original shared4-byte scalar owners");}
 check(Bytes(memory.view(0x4f2a9c,16)).u32(0)==0&&memory.view(0x4f2a89,1)[0]==std::byte{}&&std::ranges::all_of(memory.view(0x4f2aac,0x48),[](auto b){return b==std::byte{};}),"camera gate was fabricated instead of cold input storage");
 SourceRuntimeMemory standalone;mount_native_tu15_input_startup_data(standalone);check(standalone.checkpoint().regions.size()==4&&standalone.is_read_only(0x455a90,8)&&std::ranges::equal(standalone.view(0x455a90,8),scalars),"standalone input helper changed original4-owner shape");
 SourceRuntimeMemory shared;share(shared);const auto existing=shared.checkpoint();mount_native_tu15_input_startup_data(shared);const auto composed=shared.checkpoint();check(composed.regions.size()==5,"shared input helper added unexpected owners");for(const auto&r:existing.regions)check(std::ranges::find(composed.regions,r)!=composed.regions.end(),"shared input helper changed preexisting constant provenance");
 for(unsigned p:{0x4f2971u,0x52d4a9u,0x44ad31u}){SourceRuntimeMemory m;m.mount(p,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"foreign overlapping byte");reject(m);}
 for(unsigned kind=0;kind<5;++kind){SourceRuntimeMemory m;if(kind==0)m.mount(0x455a90,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadOnly,"wrong scalar");if(kind==1)m.mount(0x455a90,scalars,SourceRuntimeMemory::Access::ReadWrite,"mutable scalar");if(kind==2)m.mount(0x455a90,std::span(scalars).first(4),SourceRuntimeMemory::Access::ReadOnly,"missing high scalar");if(kind==3)m.mount(0x455a94,std::span(scalars).subspan(4),SourceRuntimeMemory::Access::ReadOnly,"missing low scalar");if(kind==4)m.mount(0x455a90,std::span(scalars).first(2),SourceRuntimeMemory::Access::ReadOnly,"truncated scalar");reject(m);}
 reject(shared); // Duplicate constructor composition preserves every owner.
 if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,4>{{{0x4f2970,0x2c8},{0x52d4a8,0x1bc},{0x44ad30,16},{0x455a90,8}}}){if(p==0x455a90){for(unsigned i=0;i<2;++i)check(std::ranges::equal(memory.view(p+i*4,4),original.view(p+i*4,4)),"shared input scalar differs from authenticated original");}else check(std::ranges::equal(memory.view(p,n),original.view(p,n)),"input constructor data differs from authenticated original");}}
 check(memory.checkpoint()==cold,"input factory controls changed native cold source");std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives; exact cold controller/preset data, authentic tables, shared scalar provenance and no partial composition; no SDLactivation/ready controller/GPU claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
