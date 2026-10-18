#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_renderer_resource_storage_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
using namespace halo2;
namespace {
constexpr std::array<std::pair<unsigned,unsigned>,10> banks{{{0x494330,1},{0x494338,4},{0x49433c,0x27000},{0x4bb33c,4},{0x4bb340,1},{0x4bb344,4},{0x4bb348,4},{0x4bb34c,4},{0x4bb370,4},{0x4bb374,0x6000}}};
unsigned checks{};
void check(bool ok,const char*why){++checks;if(!ok)throw std::runtime_error(why);}
void append(std::vector<std::byte>&b,unsigned v){for(unsigned n=0;n<4;++n)b.push_back(std::byte(v>>(n*8)));}
void put(SourceRuntimeMemory&m,unsigned p,unsigned v){std::vector<std::byte>b;append(b,v);m.write(p,b);}
std::string digest(std::span<const std::byte>b){std::array<unsigned char,32>hash{};unsigned n{};check(EVP_Digest(b.data(),b.size(),hash.data(),&n,EVP_sha256(),nullptr)==1&&n==32,"cannot authenticate resource-storage CPU receipt");std::string s;for(auto h:hash){s+="0123456789abcdef"[h>>4];s+="0123456789abcdef"[h&15];}return s;}
nlohmann::json run(SourceRuntimeMemory&m,unsigned fill){
 for(const auto[p,n]:banks)m.write(p,std::vector<std::byte>(n,std::byte(fill)));
 constexpr unsigned stack=0x62000000u;m.mount(stack,std::array<std::byte,0x100>{},SourceRuntimeMemory::Access::ReadWrite,"declared1D640caller stack");
 SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x11u,0x22u,0x33u,0x44u,stack+0x80u,0x55u,0x66u,0x77u};abi.eflags=0x202u;const auto entry=abi.gp;put(m,abi.gp[4],0x63000000u);
 std::vector<std::byte>trace,stores;unsigned instructions{},written{};SourcePhysicsWorldRuntimeServices service;
 service.instruction=[&](auto pc){append(trace,pc);++instructions;};service.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("whole1D640 requested fabricated constructor service");};
 source_run_renderer_resource_storage_tu15(m,0x1d640u,abi,{0x27f,0x1f80},service,[&](const auto&s){append(stores,s.instruction);append(stores,s.address);append(stores,unsigned(s.written.size()));stores.insert(stores.end(),s.before.begin(),s.before.end());stores.insert(stores.end(),s.written.begin(),s.written.end());++written;});
 std::vector<std::byte>post;for(const auto[p,n]:banks){const auto bytes=m.view(p,n);if(p==0x494330)check(bytes[0]==std::byte{1},"genuine1D640did not publish completed flag");else check(std::ranges::all_of(bytes,[](auto b){return b==std::byte{};}),"genuine1D640did not clear complete resource owner");append(post,p);append(post,n);post.insert(post.end(),bytes.begin(),bytes.end());}
 m.release(stack);return {{"fill",fill},{"entry_gp",entry},{"exit_gp",abi.gp},{"exit_eflags",abi.eflags},{"instruction_count",instructions},{"store_count",written},{"instruction_sha256",digest(trace)},{"stores_sha256",digest(stores)},{"postimage_sha256",digest(post)}};
}
}
int main(int argc,char**argv)try{
 check(argc<=2,"usage: resource-factory-test [ORIGINAL_FIXTURE|--dump]");auto m=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=m.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>bytes;for(auto b:r.bytes)bytes.push_back(std::to_integer<unsigned>(b));j.push_back({{"address",r.address},{"bytes",bytes},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;}
 for(const auto[p,n]:banks){const auto found=std::ranges::find(cold.regions,p,&SourceRuntimeMemory::CheckpointRegion::address);check(found!=cold.regions.end()&&found->bytes.size()==n&&found->access==SourceRuntimeMemory::Access::ReadWrite&&found->owner.starts_with("source1D640 ")&&!found->page_bytes&&found->committed.empty(),"resource factory lost exact existing owner");check(std::ranges::all_of(m.view(p,n),[](auto b){return b==std::byte{};})&&m.native_address(p,n).value>UINT32_MAX,"cold resources are prepublished or narrowed");bool rejected{};try{m.validate_mutable_view(p,n+1);}catch(const std::out_of_range&){rejected=true;}check(rejected,"resource owner borrowed adjacent byte");}
 const auto cluster=std::ranges::find(cold.regions,0x4d1ee0u,&SourceRuntimeMemory::CheckpointRegion::address);check(cluster!=cold.regions.end()&&cluster->bytes.size()==64&&cluster->owner=="native TU1.5 cold512-cluster render visibility union"&&cluster->access==SourceRuntimeMemory::Access::ReadWrite&&!cluster->page_bytes&&cluster->committed.empty(),"render visibility union lost exact64-byte cold ownership");check(std::ranges::all_of(m.view(0x4d1ee0u,64),[](auto b){return b==std::byte{};})&&m.native_address(0x4d1ee0u,64).value>UINT32_MAX,"cluster union was prepopulated or narrowed");bool cluster_bound{};try{m.validate_mutable_view(0x4d1ee0u,65);}catch(const std::out_of_range&){cluster_bound=true;}check(cluster_bound,"cluster union borrowed neighboring byte");
 check(std::to_integer<unsigned>(m.view(0x494330,1)[0])==0,"cold resource factory forged completed constructor");
 std::optional<nlohmann::json>oracle;if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream f(argv[1]);oracle=nlohmann::json::parse(f);check(oracle->at("format")=="native_renderer_resource_factory_tu15_original_v1"&&oracle->at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle->at("entry")==0x1d640&&oracle->at("body_sha256")=="db626198aada811739afc80a6ea1ed7b875c7bf7b1731260a073fb330308f468"&&oracle->at("instruction_patches").empty()&&oracle->at("supplied_callee_returns").empty(),"foreign or patched1D640original authority");}
 for(unsigned i=0;i<3;++i){m.restore(cold);const auto actual=run(m,std::array<unsigned,3>{0,90,165}[i]);check(actual.at("instruction_count")==46099&&actual.at("store_count")==46089,"whole resource reset trace/write extent differs");if(oracle)check(actual==oracle->at("cases").at(i),"whole1D640GP/flags/ordered-PC/store/full-bank differential differs");}
 m.restore(cold);check(m.checkpoint()==cold,"resource checkpoint retained completed state");
 std::cout<<"PASS "<<checks<<" checks: exact10 resource owners184346B plus64B cluster union/native backing,11 boundary negatives, whole1D640three-fill clears/real flag, GP/flags/46099PC/46089ordered-store/full-bank controls"<<(oracle?" with unchanged-original CPU differential":"")<<"\nLIMIT cold publication and source CPU reset only; no GPU/frame/visual or scheduling claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
