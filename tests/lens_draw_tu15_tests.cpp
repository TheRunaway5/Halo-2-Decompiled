#include "halo2/source_lens_draw_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_vehicle_motion_math.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
struct Boundary {};
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte>b;for(unsigned n=0;n<hex.size();n+=2)b.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return b;}
}
int main(int argc,char **argv)try {
  check(argc==2 || argc==3,"Supply original lens draw fixture and optional authenticated math correspondence");std::ifstream file(argv[1]);if(!file)return 77;
  std::map<unsigned,unsigned> math_instructions;
  if(argc==3) {
    std::ifstream math_file(argv[2]);check(bool(math_file),"Original CRT math correspondence absent");
    const auto math=nlohmann::json::parse(math_file);
    check(math.at("later_owned_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original CRT correspondence belongs to another image");
    for(const auto &[key,value]:math.at("instruction_map").items())math_instructions.emplace(std::stoul(key,nullptr,16),value.get<unsigned>());
  }
  const auto fixture=nlohmann::json::parse(file);unsigned cases{};
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
      m.mount(b.at("address"),std::move(data),SourceRuntimeMemory::Access::ReadWrite,"declared original lens draw ABI bank");
    }
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);};
    services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe){
      if(c.contains("boundary_target") && call.target==c.at("boundary_target").get<unsigned>())throw Boundary{};
      if(source_lens_draw_tu15_owns_entry(call.target))
        source_run_lens_draw_tu15(memory,call.target,state,{0x27f,0x1f80},services,observe);
      else if(call.target==0x37dc0a) {
        auto fp=state.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80});
        auto math_services=services;math_services.instruction=[&](auto pc){trace.push_back(math_instructions.at(pc));};
        source_run_physics_vehicle_motion_math_runtime(memory,0x37a15a,state,
            SourcePhysicsContactImage::TitleUpdate15,fp,math_services,observe);
      }
      else {std::cerr<<"case="<<cases<<" target="<<std::hex<<call.target<<std::dec<<'\n';throw std::runtime_error("Lens draw reached an unowned original callee");}
    };
    bool boundary{};
    try{source_run_lens_draw_tu15(m,c.value("entry",fixture.at("entry").get<unsigned>()),abi,{0x27f,0x1f80},services);}
    catch(const Boundary&){boundary=true;}
    check(boundary==c.contains("boundary_target"),"Original lens parent boundary differs");
    check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original lens draw GP output differs");
    check((abi.eflags&c.value("eflags_mask",0xcd5u))==(c.at("eflags").get<unsigned>()&c.value("eflags_mask",0xcd5u)),"Original lens draw flags differ");
    if(trace!=c.at("instructions").get<decltype(trace)>())std::cerr<<"case="<<cases<<" variant="<<c.at("variant")<<'\n';
    check(trace==c.at("instructions").get<decltype(trace)>(),"Original lens draw instruction path differs");
    for(unsigned n=0;n<8;++n){const auto hex=c.at("exit_xmm")[n].get<std::string>();for(unsigned k=0;k<4;++k)check(abi.xmm[n][k]==std::stoul(hex.substr(24-k*8,8),nullptr,16),"Original lens draw SSE output differs");}
    for(const auto &reference:c.at("final_banks")){const auto &b=fixture.at("banks").at(reference.get<unsigned>());const auto expected=bytes(b.at("hex"));const auto actual=m.view(b.at("address"),expected.size());if(!std::ranges::equal(actual,expected)){const auto mismatch=std::ranges::mismatch(actual,expected);std::cerr<<"case="<<cases<<" address="<<std::hex<<b.at("address").get<unsigned>()+std::distance(actual.begin(),mismatch.in1)<<" got="<<unsigned(*mismatch.in1)<<" expected="<<unsigned(*mismatch.in2)<<std::dec<<"\n";throw std::runtime_error("Original lens draw retained memory differs");}}
    check(abi.x87_depth==0,"Lens draw retained an unconsumed x87 value");
    check(abi.x87_status==c.at("x87_status").get<unsigned>(),"Lens draw changed original x87 status");
    check(abi.floating_point && abi.floating_point->mxcsr==c.at("mxcsr").get<unsigned>(),"Lens draw changed original MXCSR");
    ++cases;
  }
  std::cout<<"PASS original TU1.5 lens draw: "<<cases<<" instruction/GP/SSE/flag/observed-memory cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
