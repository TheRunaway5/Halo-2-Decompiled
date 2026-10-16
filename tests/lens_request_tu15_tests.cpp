#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte>b;for(unsigned n=0;n<hex.size();n+=2)b.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return b;}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original lens request fixture");std::ifstream file(argv[1]);if(!file)return 77;
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
  std::cout<<"PASS original TU1.5 lens request: "<<cases<<" instruction/GP/SSE/flag/observed-memory cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
