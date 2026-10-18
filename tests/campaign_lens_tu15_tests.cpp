#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_lens_frame_camera_tu15.hpp"
#include <cstring>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}

using OldQuery=std::vector<SourceLensQueryDraw>(SourceLensQueryFrameTu15::*)(PhysicsLiveWorldTu15 &,unsigned,
    Vec3,Vec3,Vec3,float,const SourceViewProjectionBank &,std::array<std::int16_t,4>,unsigned,unsigned);
using OldDraw=std::vector<SourceLensDraw>(SourceLensDrawFrameTu15::*)(PhysicsLiveWorldTu15 &,unsigned,
    Vec3,Vec3,Vec3,float,const SourceViewProjectionBank &,std::array<std::int16_t,4>,unsigned,unsigned);
// Actual exported old symbols must remain linked for the N2 multiplayer TU.
OldQuery retained_query_signature=static_cast<OldQuery>(&SourceLensQueryFrameTu15::view);
OldDraw retained_draw_signature=static_cast<OldDraw>(&SourceLensDrawFrameTu15::view);
void camera_controls() {
  struct Case {unsigned logical_w,logical_h,physical_w,physical_h;std::array<std::int16_t,4> input,expected;};
  const std::array<Case,6> cases{{
    {640,480,640,480,{0,0,480,640},{0,0,480,640}},
    {1280,960,1280,960,{0,0,960,1280},{0,0,960,1280}},
    {1920,1440,1920,1440,{0,0,1440,1920},{0,0,1440,1920}},
    {640,480,1280,960,{0,0,480,640},{0,0,960,1280}},
    {1280,720,2560,1440,{0,0,720,1280},{0,0,1440,2560}},
    {640,480,1920,1440,{120,160,360,480},{360,480,1080,1440}}
  }};
  for(const auto &c:cases) {
    const auto old=source_lens_frame_camera_tu15({1,2,3},{0,1,0},{0,0,1},.75f,c.input,
        c.logical_w,c.logical_h,c.physical_w,c.physical_h,{.015625f,1024.f});
    const auto changed=source_lens_frame_camera_tu15({1,2,3},{0,1,0},{0,0,1},.75f,c.input,
        c.logical_w,c.logical_h,c.physical_w,c.physical_h,{.125f,128.f});
    const Bytes b(old),n(changed);
    for(unsigned e=0;e<4;++e)check(b.u16(0x30+e*2)==unsigned(c.expected[e]) &&
        b.u16(0x38+e*2)==unsigned(c.expected[e]),"Lens camera lost attachment-pixel rectangle scaling");
    check(b.u32(0x40)==0x3c800000 && b.u32(0x44)==0x44800000 &&
          n.u32(0x40)==0x3e000000 && n.u32(0x44)==0x43000000,"Lens camera default/actual clip words differ");
    for(unsigned at=0;at<old.size();++at)if(at<0x40 || at>=0x48)
        check(old[at]==changed[at],"Lens clip handoff changed an unrelated camera byte");
  }
  bool overflow{};try{(void)source_lens_frame_camera_tu15({},{},{},0,{0,0,480,640},640,480,32768,480,{0,1024});}
  catch(const FormatError &){overflow=true;}check(overflow,"Lens rectangle silently clamped or wrapped hardware extent");
  check(retained_query_signature!=nullptr && retained_draw_signature!=nullptr,"Old lens method ABI was removed");
  std::cout<<"PASS six native lens camera logical/physical/clip-byte controls and retained exported MP signatures\n";
}

std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte>b;for(unsigned n=0;n<hex.size();n+=2)b.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return b;}
}
int main(int argc,char **argv)try {
  if(argc==2 && std::string(argv[1])=="--camera"){camera_controls();return 0;}
  check(argc==2 || argc==3,"Supply campaign original lens fixture and optional owned opening cache");std::ifstream file(argv[1]);if(!file)return 77;
  if(argc==3 && !std::filesystem::exists(argv[2]))return 77;
  const auto fixture=nlohmann::json::parse(file);unsigned cases{};
  check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Campaign lens fixture belongs to another executable");
  check(fixture.at("entry")==0x2db80 && fixture.at("pool_capacity")==511 && fixture.at("cases").size()==8,"Campaign lens fixture shape differs");
  const auto &authored=fixture.at("authored");
  check(authored.at("cache_sha256")=="5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28" &&
        authored.at("effect").at("datum")==0xea03088e && authored.at("lens").at("datum")==0xea050890 &&
        authored.at("bitmap").at("datum")==0xea060891 && authored.at("bitmap_rows").size()==2,"Campaign authored lens identities differ");
  const std::array<std::string,8> variants{"accepted","owner-view","no-view","no-tag","zero-scale","zero-fade","disabled-pool","full-pool"};
  for(unsigned n=0;n<variants.size();++n)check(fixture.at("cases")[n].at("variant")==variants[n],"Campaign lens case order differs");
  if(argc==3) {
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path());
    const auto lens=cache.query_tag({.handle=TagHandle{0xea050890u},.group=TagGroup::from_string("lens"),.require_address=true});
    check(lens && lens->name==authored.at("lens").at("name").get<std::string>(),"Campaign lens cache tag differs");
    check(std::ranges::equal(cache.tag_payload(lens->handle()),bytes(authored.at("lens_bytes"))),"Campaign lens cache bytes differ");
    Mesh mesh;SourceLensDrawFrameTu15 resources(cache,mesh);
    for(unsigned image=0;image<2;++image) {
      const auto name="source lens bitmap "+std::to_string(0xea060891u)+":"+std::to_string(image);
      const auto material=std::ranges::find(mesh.materials,name,&Material::name);
      check(material!=mesh.materials.end() && material->diffuse.width==128 && material->diffuse.height==128 &&
            !material->diffuse.mips.empty(),"Campaign authored flare image was not prepared before Renderer construction");
    }
  }
  for(const auto &c:fixture.at("cases")) {
    SourceRuntimeMemory m;
    for(const auto &reference:c.at("initial_banks")) {
      const auto &b=fixture.at("banks").at(reference.get<unsigned>());
      auto data=b.contains("hex")?bytes(b.at("hex")):std::vector<std::byte>(b.at("zero_bytes").get<unsigned>());
      if(b.contains("patches"))for(const auto &patch:b.at("patches")) {
        const auto value=bytes(patch.at("hex"));const auto offset=patch.at("offset").get<unsigned>();
        check(offset<=data.size() && value.size()<=data.size()-offset,"Scratch fixture patch exceeds its declared bank");
        std::copy(value.begin(),value.end(),data.begin()+offset);
      }
      m.mount(b.at("address"),std::move(data),SourceRuntimeMemory::Access::ReadWrite,"declared original lens request ABI bank");
    }
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);};
    services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe){
      if(source_lens_flare_tu15_owns_entry(call.target))
        source_run_lens_flare_tu15(memory,call.target,state,{0x27f,0x1f80},services,observe);
      else throw std::runtime_error("Lens request reached an unowned original callee");
    };
    source_run_lens_flare_tu15(m,c.value("entry",fixture.at("entry").get<unsigned>()),abi,{0x27f,0x1f80},services);
    check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original lens request GP output differs");
    check((abi.eflags&c.value("eflags_mask",0xcd5u))==(c.at("eflags").get<unsigned>()&c.value("eflags_mask",0xcd5u)),"Original lens request flags differ");
    check(trace==c.at("instructions").get<decltype(trace)>(),"Original lens request instruction path differs");
    for(unsigned n=0;n<8;++n){const auto hex=c.at("exit_xmm")[n].get<std::string>();for(unsigned k=0;k<4;++k)check(abi.xmm[n][k]==std::stoul(hex.substr(24-k*8,8),nullptr,16),"Original lens request SSE output differs");}
    for(const auto &reference:c.at("final_banks")){const auto &b=fixture.at("banks").at(reference.get<unsigned>());const auto expected=bytes(b.at("hex"));const auto actual=m.view(b.at("address"),expected.size());if(!std::ranges::equal(actual,expected)){const auto mismatch=std::ranges::mismatch(actual,expected);std::cerr<<"case="<<cases<<" address="<<std::hex<<b.at("address").get<unsigned>()+std::distance(actual.begin(),mismatch.in1)<<" got="<<unsigned(*mismatch.in1)<<" expected="<<unsigned(*mismatch.in2)<<std::dec<<"\n";throw std::runtime_error("Original lens request retained memory differs");}}
    check(abi.x87_depth==0,"Lens request retained an unconsumed x87 value");
    check(abi.x87_status==c.at("x87_status").get<unsigned>(),"Lens request changed original x87 status");
    check(abi.floating_point && abi.floating_point->mxcsr==c.at("mxcsr").get<unsigned>(),"Lens request changed original MXCSR");
    ++cases;
  }
  std::cout<<"PASS campaign authored TU1.5 lens request/resource preparation: "<<cases<<" instruction/GP/SSE/flag/observed-memory cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
