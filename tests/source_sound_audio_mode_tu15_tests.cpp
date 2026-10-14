#include "halo2/source_boot_smc_video_mode.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_tu15_settings.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
using namespace halo2;
using J=nlohmann::json;
using M=SourceRuntimeMemory;
using A=SourcePhysicsWorldRuntimeAbi;
namespace {
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
J read(const std::filesystem::path &p){std::ifstream f(p);check(bool(f),"Missing original boot-mode evidence");return J::parse(f);}
void write(const std::filesystem::path &p,const J &j){std::ofstream f(p);f<<j.dump()<<'\n';check(bool(f),"Cannot retain audio-mode evidence");}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto c:b){auto v=std::to_integer<unsigned>(c);s+=d[v>>4];s+=d[v&15];}return s;}
void put(std::span<std::byte>b,unsigned offset,std::uint32_t value){for(unsigned i=0;i<4;++i)b[offset+i]=std::byte(value>>(8*i));}
std::uint32_t word(std::span<const std::byte>b,unsigned offset=0){std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[offset+i])<<(8*i);return v;}
void checksum(std::array<std::byte,96>&b){std::uint64_t sum{};for(unsigned i=4;i<96;i+=4)sum+=word(b,i);while(sum>>32)sum=(sum&0xffffffffu)+(sum>>32);put(b,0,~std::uint32_t(sum));}
J snapshot(std::uint32_t pc,const A&a,const M&m,const std::vector<std::uint32_t>&trace){
 J regions=J::array(),fp=J::array();for(unsigned i=0;i<8;++i)fp.push_back(hex({reinterpret_cast<const std::byte*>(&a.x87[i]),10}));
 m.visit_committed([&](auto p,auto b){regions.push_back({{"address",p},{"size",b.size()},{"writable",!m.is_read_only(p,b.size())},{"hex",hex(b)}});});
 return {{"pc",pc},{"instructions",trace},{"regions",regions},{"abi",{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr},{"x87_tag",65535},{"x87_logical_hex",fp}}}};
}
}
int main(int argc,char **argv)try{
 check(argc==5,"Supply XBE ORIGINAL_BOOT_CAPTURE ACTUAL_EEPROM OUTPUT");
 if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[3])||
    !std::filesystem::is_regular_file(std::filesystem::path(argv[2])/"report.json"))return 77;
 const std::filesystem::path original(argv[2]),out(argv[4]);const auto ref=read(original/"report.json");
 check(ref.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong TU1.5 reference");
 for(const auto *k:{"original_code_patches","original_memory_writes","original_register_writes","supplied_callee_returns"})check(ref.at(k).empty(),"Boot observation supplied original state");
 const auto&transaction=ref.at("smc_transaction");check(transaction.at("request").at("args")[0]==32&&transaction.at("request").at("args")[1]==4&&transaction.at("request").at("args")[2]==0,"Wrong original SMC request");
 const auto actual=SourceTu15UserSettings::load(argv[3]);check(actual.checksum_valid(),"Original EEPROM checksum is invalid");std::filesystem::create_directories(out);
 J producers=J::array();std::vector<std::uint32_t>responses;for(unsigned i=0;i<256;++i)responses.push_back(i);for(auto v:{0x100u,0x12345678u,0x80000000u,0xffffffffu})responses.push_back(v);
 for(auto prior:{0u,7u,0x12345678u,0xffffffffu})for(auto status:{0u,1u,0x7fffffffu,0x80000000u,0xc000009cu,0xffffffffu})for(auto response:responses)
  producers.push_back({{"prior",prior},{"status",status},{"response",response},{"native_result",source_boot_smc_video_mode_after_read(prior,status,response)}});
 check(source_boot_smc_video_mode_after_read(ref.at("boot_mode_export").at("initial_word"),transaction.at("response").at("status"),transaction.at("response").at("output_word"))==ref.at("boot_mode_export").at("current_word"),"Actual original boot transaction differs");
 std::set<std::uint32_t> flags{actual.audio_flags(),0u,0xffffffffu,0x12345678u,0xffff0000u,0x0000ffffu};for(unsigned i=0;i<32;++i)flags.insert(1u<<i);
 J cases=J::array();unsigned identity{};bool immutable_rejected{},wrong_image_rejected{},missing_owner_rejected{};
 for(unsigned mode=0;mode<8;++mode){
  SourceRuntimeKernelBootSmcVideoMode input{ref.at("boot_mode_export").at("address"),{},"explicit original captured AV-pack data or declared mode-branch input; test only"};put(input.bytes,0,mode);
  const auto image=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15,input);
  check(word(image.view(0x41b784,4))==input.address&&word(image.view(input.address,4))==mode&&image.is_read_only(0x41b784,4)&&image.is_read_only(input.address,4),"Actual data import was not immutable");
  if(mode==0){
   auto mutable_image=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15,input);try{mutable_image.write(input.address,input.bytes);}catch(const std::exception&){immutable_rejected=true;}
   try{auto bad=load_source_runtime_image(argv[1],SourcePhysicsContactImage::MapPack11,input);(void)bad;}catch(const std::exception&){wrong_image_rejected=true;}
   auto unnamed=input;unnamed.owner.clear();try{auto bad=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15,unnamed);(void)bad;}catch(const std::exception&){missing_owner_rejected=true;}
  }
  for(auto value:flags)for(bool valid:{true,false}){
   std::array<std::byte,96> user{};std::copy(actual.bytes().begin(),actual.bytes().end(),user.begin());put(user,56,value);checksum(user);if(!valid)user[0]^=std::byte{1};
   const bool actual_settings=valid&&std::ranges::equal(user,actual.bytes());SourceTu15UserSettings settings(user,actual_settings?"actual owned UserSettings":"declared modified audio/checksum coverage input");check(settings.checksum_valid()==valid,"Declared checksum branch input differs");
   M m;for(const auto [p,n]:{std::pair{0x2e101cu,6u},{0x33c29du,8u},{0x37da9au,66u},{0x41b6f0u,4u},{0x41b784u,4u},{input.address,4u}})m.mount(p,image.view(p,n),M::Access::ReadOnly,"authenticated original code/import or named test data");
   constexpr auto stack=0x30020000u,sp=stack+0x800u,halt=0x30000000u;std::array<std::byte,4096> storage{};for(unsigned i=0;i<storage.size();++i)storage[i]=std::byte((i*37u+11u)&255u);put(storage,sp-stack,halt);m.mount(stack,storage,M::Access::ReadWrite,"declared patterned private caller stack");
   A a{};a.gp={0x13579bdfu,0x2468ace0u,0x11223344u,0xaabbccddu,sp,0x12349876u,0x98765432u,0xfedcba98u};a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
   for(unsigned i=0;i<a.xmm.size();++i)for(unsigned k=0;k<4;++k)a.xmm[i][k]=0x10203040u+i*0x01010101u+k;
   std::vector<std::uint32_t> trace;const auto directory=out/("case-"+std::to_string(++identity));std::filesystem::create_directories(directory);write(directory/"before.json",snapshot(0x37da9a,a,m,trace));J query_writes=J::array();unsigned queries{},mode_reads{};
   SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};services.required_owner=[&](auto&bank,const auto&call,auto&abi,const auto&){
    if(call.instruction==0x37daba&&call.target==0x33c29d){++mode_reads;source_run_sound_sdk_tu15(bank,call.target,abi,*abi.floating_point,services);return;}
    check(call.instruction==0x2e101c&&call.target==word(m.view(0x41b6f0,4)),"Unexpected complete audio helper boundary");++queries;write(directory/"query-before.json",snapshot(call.target,abi,m,trace));
    check(source_try_tu15_audio_settings_query(m,call,abi,settings,[&](const auto&w){query_writes.push_back({{"instruction",w.instruction},{"address",w.address},{"before_hex",hex(w.before)},{"after_hex",hex(w.written)}});}),"Actual native audio query did not own its frame");trace.clear();write(directory/"query-after.json",snapshot(0x37dab2,abi,m,trace));
   };
   source_run_sound_sdk_tu15(m,0x37da9a,a,*a.floating_point,services);check(queries==1&&mode_reads==1&&a.gp[4]==sp+4,"Whole audio helper did not return through actual source owners");write(directory/"after.json",snapshot(halt,a,m,trace));
   cases.push_back({{"id",identity},{"directory",directory.filename().string()},{"mode",mode},{"checksum_valid",valid},{"declared_audio_flags",value},{"user_section_hex",hex(user)},{"actual_owned_settings",actual_settings},{"actual_original_mode",mode==ref.at("boot_mode_export").at("current_word")},{"native_helper_result",a.gp[0]},{"query_writes",query_writes}});
  }
 }
 check(immutable_rejected&&wrong_image_rejected&&missing_owner_rejected,"Explicit boot-data loader lost its input/protection gates");
 write(out/"report.json",{{"original_sha256",ref.at("original_sha256")},{"producer_cases",producers},{"helper_cases",cases},{"immutable_write_rejected",immutable_rejected},{"wrong_image_rejected",wrong_image_rejected},{"missing_owner_rejected",missing_owner_rejected},{"ordinary_live_native_boot_mode_bound",false},{"native_smc_transport_implemented",false},{"reference_or_modified_inputs_test_only",true},{"mode_callee_result_supplied",false},{"native_device_or_pcm_verified",false}});
 std::cout<<"PASS explicit immutable ordinal356 input; "<<producers.size()<<" publication inputs and "<<cases.size()<<" complete source audio-helper cases retained for independent original execution\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
