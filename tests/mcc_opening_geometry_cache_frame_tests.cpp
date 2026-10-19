#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_geometry_cache_frame_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> decode(const std::string& s) {
 std::vector<std::byte> bytes;
 for(std::size_t n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
 return bytes;
}
void original_frame_fixture(const char* path) {
 using namespace halo2;
 std::ifstream input(path);check(bool(input),"Original frame fixture absent");
 const auto fixture=nlohmann::json::parse(input);
 check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original frame image differs");
 unsigned count{};
 for(const auto& row:fixture.at("cases")) {
  SourceRuntimeMemory memory;
  memory.mount(0x1000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"declared empty caller pools and stack");
  memory.mount(0x4f2d60,std::vector<std::byte>(0x24),SourceRuntimeMemory::Access::ReadWrite,"declared geometry globals");
  const auto flag=std::byte(row.at("source_flag").get<unsigned>());
  memory.mount(0x48c754,std::span(&flag,1),SourceRuntimeMemory::Access::ReadOnly,"original pressure warning flag");
  const auto put=[&](unsigned at,unsigned value){memory.write(at,std::as_bytes(std::span(&value,1)));};
  const auto physical=row.at("physical").get<unsigned>();
  put(0x4f2d6c,row.at("present").get<bool>()?physical:0u);
  put(0x4f2d60,row.at("predicted"));
  memory.write(physical,decode(row.at("before_hex")));
  put(row.at("stack"),row.at("stop"));
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("input_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
  SourcePhysicsWorldRuntimeServices services;
  services.required_owner=[](auto&,auto&,auto&,auto&){throw std::runtime_error("Original empty-pool frame reached an undeclared service");};
  source_run_geometry_cache_frame_tu15(memory,0x130890,abi,{0x27f,0x1f80},services);
  check(std::ranges::equal(memory.view(physical,0x70),decode(row.at("after_hex"))),"Whole original geometry frame bank differs");
  check(abi.gp==row.at("output_gp").get<decltype(abi.gp)>()&&abi.eflags==row.at("output_eflags"),"Whole original geometry frame caller ABI differs");
  ++count;
 }
 check(count==3,"Original null, ordinary and rollover cases required");
}
}
int main(int argc,char** argv)try {
 using namespace halo2;
 if(argc!=4 || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
 original_frame_fixture(argv[3]);
 const int lock=::open("/run/user/1000/halo2-tag-match-validation.lock",O_CREAT|O_RDWR,0600);
 check(lock>=0&&::flock(lock,LOCK_EX)==0,"Cannot acquire the native-world test lock");
 struct Lock{int fd;~Lock(){::close(fd);}} owned_lock{lock};
 Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
 check(cache.is_mcc()&&cache.name()=="00a_introduction","Requires actual MCC opening cache");
 // Match the application: native declared startup data, not the executable's
 // complete .data bank. The executable is only the offline fixture authority.
 PhysicsLiveWorldTu15 world(cache,{},std::filesystem::temp_directory_path()/"geometry-state");
 world.configure_campaign_session(1,1,30,0x12345678u);world.initialize_map();
 const auto word=[&](unsigned at){return Bytes(world.memory().view(at,4)).u32(0);};
 const auto physical=word(0x4f2d6cu);check(physical,"Original geometry cache was not constructed");
 unsigned frames{},models{},sections{};const auto initial=word(physical+0x38u);
 const auto advance=[&]{const auto before=word(physical+0x38u);world.begin_model_frame();++frames;
  check(before<0x7fffffffu&&word(physical+0x38u)==before+1,"Presentation did not advance original geometry cache age");
  for(unsigned n=0;n<8;++n)check(word(physical+0x44u+n*4u)==0x7fffffffu,"Original geometry frame did not reset all priority limits");};
 advance();
 for(const auto& tag:cache.tags())if(tag.address&&tag.group=="mode"&&
   (tag.name.find("phantom")!=std::string::npos||tag.name.find("arbiter")!=std::string::npos||
    tag.name.find("prophet")!=std::string::npos||tag.name.find("masterchief")!=std::string::npos)) {
  advance();world.preload_render_model(tag.datum);++models;
 }
 // Eight completed frames expire the original five-frame protection of the
 // preceding shot's geometry. Keep the original cache capacity and policies.
 for(unsigned n=0;n<8;++n)advance();
 const auto bsp=word(0x4ecc18u);
 for(unsigned kind=0;kind<2;++kind) {
  const auto count=word(bsp+(kind?0x138u:0x9cu)),base=word(bsp+(kind?0x13cu:0xa0u));
  for(unsigned n=0;n<count;++n) {
   const auto row=base+n*(kind?200u:176u);
   if(!Bytes(world.memory().view(row+0x24u,2)).u16(0))continue;
   auto ready=world.request_render_geometry(2u,row+0x28u);
   if(!ready)ready=world.request_render_geometry(0u,row+0x28u);
   check(ready&&word(row+0x50u),"Original MCC BSP publication failed after previous-shot cache pressure");++sections;
  }
 }
 check(models>=6&&sections==77&&word(physical+0x38u)==initial+frames,"Actual opening pressure bank changed");
 // Dialogue uses the same real geometry publisher for encoded mouth/lipsync
 // sections. Exercise its MCC resource owner before full cinematic playback.
 for(unsigned n=0;n<8;++n)advance();
 unsigned facial{};const auto gestalt=word(0x52d664u);
 const auto extra_count=word(gestalt+0x50u),extra=word(gestalt+0x54u);
 for(unsigned n=0;n<extra_count;++n) {
  const auto row=extra+n*0x2cu;
  if(!word(row+0xcu))continue;
  advance();
  auto ready=world.request_render_geometry(2u,row+8u);
  if(!ready)ready=world.request_render_geometry(0u,row+8u);
  check(ready&&word(row)&&word(row+4u),"Actual MCC facial resource did not publish its encoded section");++facial;
 }
 check(facial>31,"Actual opening facial-resource cohort missing");
 std::cout<<"PASS actual MCC opening: "<<frames<<" complete original geometry frames, "<<models
          <<" prior-shot model owners, "<<sections<<" BSP and "<<facial<<" facial sections; source capacity and readiness preserved\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
