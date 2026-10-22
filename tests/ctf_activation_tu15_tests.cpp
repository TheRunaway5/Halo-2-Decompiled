#include "halo2/source_ctf_activation_tu15.hpp"
#include "ctf_activation_test_support.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> decode(const std::string& hex) {
  if(hex.size()%2)throw std::runtime_error("Odd original byte width");
  std::vector<std::byte> r(hex.size()/2);
  for(std::size_t i=0;i<r.size();++i)r[i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
  return r;
}
std::string digest(std::span<const std::byte> bytes) {
  unsigned char d[SHA256_DIGEST_LENGTH];SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),d);
  constexpr char h[]="0123456789abcdef";std::string result;
  for(unsigned char c:d) {result+=h[c>>4];result+=h[c&15];}return result;
}
void append(std::vector<std::byte>& bytes,std::uint32_t word,unsigned width=4) {
  for(unsigned i=0;i<width;++i)bytes.push_back(std::byte(word>>(i*8)));
}
struct Missing {std::uint32_t target;};
struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
struct Memory : SourcePhysicsContactMemory {
  std::vector<Bank> banks;std::vector<std::byte> trace,stores;
  std::uint32_t pc{},previous{};std::size_t instructions{},store_bytes{};
  void instruction(std::uint32_t p) {
    pc=p;if(p!=previous) {append(trace,p);++instructions;}previous=p;
  }
  void read(std::uint32_t p,std::span<std::byte> to) const override {
    for(const auto& b:banks)if(p>=b.address&&std::uint64_t(p)+to.size()<=std::uint64_t(b.address)+b.bytes.size()) {
      std::copy_n(b.bytes.begin()+(p-b.address),to.size(),to.begin());return;
    }
    throw std::runtime_error("Undeclared source read at "+std::to_string(p)+" pc="+std::to_string(pc));
  }
  void write(std::uint32_t p,std::span<const std::byte> from) override {
    for(auto& b:banks)if(p>=b.address&&std::uint64_t(p)+from.size()<=std::uint64_t(b.address)+b.bytes.size()) {
      for(std::size_t i=0;i<from.size();++i) {
        append(stores,pc);append(stores,p+std::uint32_t(i));stores.push_back(from[i]);++store_bytes;
      }
      std::copy(from.begin(),from.end(),b.bytes.begin()+(p-b.address));return;
    }
    throw std::runtime_error("Undeclared source write at "+std::to_string(p));
  }
};
Json execute(const Json& row,const Json& fixture) {
  const auto& blobs=fixture.at("blobs");
  Memory memory;
  std::map<std::uint32_t,std::string> inputs;
  for(const auto& b:fixture.at("base_banks"))inputs[b.at("address")]=b.at("blob");
  for(const auto& b:row.at("banks"))inputs[b.at("address")]=b.at("blob");
  for(const auto& [address,blob]:inputs)memory.banks.push_back({address,decode(blobs.at(blob))});
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202;
  for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)abi.xmm[i][j]=0x3f000000+i*0x100+j;
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](std::uint32_t pc){memory.instruction(pc);};
  std::function<void(std::uint32_t,SourcePhysicsWorldRuntimeAbi&)> dispatch;
  dispatch=[&](std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& state) {
    if(source_ctf_activation_tu15_owns_entry(entry))source_run_ctf_activation_tu15(memory,entry,state,{0x27f,0x1f80},services);
    else if(source_ctf_activation_test_support_owns_entry(entry))source_run_ctf_activation_test_support(memory,entry,state,{0x27f,0x1f80},services);
    else throw Missing{entry};
  };
  services.required_owner=[&](SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall& call,
      SourcePhysicsWorldRuntimeAbi& state,const SourcePhysicsContactRecordObserver&) {dispatch(call.target,state);};
  std::uint32_t boundary=0;
  try {
    dispatch(0x3825d0,abi);
    if(row.at("initialize")) {abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();dispatch(0x249460,abi);}
    abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202;
    dispatch(row.at("entry"),abi);
  } catch(const Missing& m) {boundary=m.target;}
  Json banks=Json::object();for(const auto& b:memory.banks) {
    const auto hash=digest(b.bytes);if(hash!=inputs.at(b.address))banks[std::to_string(b.address)]=hash;
  }
  Json x87=Json::array();constexpr char hex[]="0123456789abcdef";
  for(unsigned i=0;i<abi.x87_depth;++i) {
    std::array<std::uint8_t,10> raw;std::memcpy(raw.data(),&abi.x87[i],10);std::string value;
    for(auto c:raw) {value+=hex[c>>4];value+=hex[c&15];}x87.push_back(value);
  }
  return Json{{"boundary",boundary},{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags&0x8d5},
    {"x87",x87},{"fs_base",abi.fs_base},
    {"x87_status",abi.x87_status&0xfdc0},{"x87_control",abi.floating_point->x87_control_word},
    {"mxcsr",abi.floating_point->mxcsr&0xffffffc0},{"trace_sha256",digest(memory.trace)},
    {"instructions",memory.instructions},{"store_sha256",digest(memory.stores)},
    {"store_bytes",memory.store_bytes},{"banks_sha256",banks}};
}
}
int main(int argc,char** argv) {
  try {
    if(argc<2||argc>3)throw std::runtime_error("Usage: ctf_activation_tu15_tests original-fixture.json [actual.json]");
    // First activation reaches the complete shared +C callback through the
    // original engine vtable. It must resolve in production, even though the
    // isolated fixture retains independent original support for other callees.
    if(!source_ctf_activation_tu15_owns_entry(0xa6780u))
      throw std::runtime_error("Original first-activation callback has no production owner");
    if(!source_ctf_activation_tu15_owns_entry(0x24a810u))
      throw std::runtime_error("Original player/equipment admission callback has no production owner");
    if(!source_ctf_activation_tu15_owns_entry(0x106350u))
      throw std::runtime_error("Original real-weapon classification has no production owner");
    if(!source_ctf_activation_tu15_owns_entry(0x1a87c0u))
      throw std::runtime_error("Original CTF event-request construction has no production owner");
    if(!source_ctf_activation_tu15_owns_entry(0x160d40u))
      throw std::runtime_error("Original actual carrier-player registry refresh has no production owner");
    if(!source_ctf_activation_tu15_owns_entry(0x24a4b0u))
      throw std::runtime_error("Original CTF actual objective-drop notification has no production owner");
    for(auto entry:{0x24a070u,0x24dae0u,0x24b6e0u,0x24b4c0u,0x249b20u,
        0x24d5f0u,0x165610u,0x249a40u,0x24b950u,0x24c490u})
      if(!source_ctf_activation_tu15_owns_entry(entry))
        throw std::runtime_error("Original CTF tick/registry/timer/return has no production owner");
    Json fixture;std::ifstream input(argv[1]);input>>fixture;
    Json results=Json::array();std::size_t count=0,complete=0;
    for(const auto& row:fixture.at("cases")) {
      const auto actual=execute(row,fixture);results.push_back({{"name",row.at("name")},{"actual",actual}});
      if(argc==3) {std::ofstream output(argv[2]);output<<results.dump()<<'\n';}
      auto expected=fixture.at("base_expected");expected.update(row.at("expected"));
      if(actual!=expected) {
        std::cerr<<"First original/native ABI mismatch: "<<row.at("name")<<'\n';
        for(auto it=actual.begin();it!=actual.end();++it)if(it.value()!=expected.at(it.key()))
          std::cerr<<it.key()<<" native="<<it.value()<<" original="<<expected.at(it.key())<<'\n';
        return 1;
      }
      ++count;if(actual.at("boundary")==0)++complete;
    }
    std::cout<<"Complete original CTF/Assault ABI owners: "<<count<<" cases, "<<complete
        <<" complete returns; all other cases stop at exact unsupplied original owners. Full mode remains gated.\n";
    return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
