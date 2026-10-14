#include "halo2/physics_pool_host_storage.hpp"
#include "halo2/source_runtime_image.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace {
using J=nlohmann::json;
using namespace halo2;
void check(bool ok,const std::string &why) { if(!ok)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
  check(!(s.size()&1u),"Invalid physics pool fixture byte extent");
  std::vector<std::byte> b;
  for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return b;
}
std::vector<std::byte> file(const std::filesystem::path &p) {
  std::ifstream in(p,std::ios::binary|std::ios::ate);
  check(bool(in),"Missing original physics pool backing: "+p.string());
  const auto size=in.tellg();check(size>=0,"Invalid physics pool backing extent");
  std::vector<std::byte> b(static_cast<std::size_t>(size));in.seekg(0);
  check(bool(in.read(reinterpret_cast<char *>(b.data()),b.size())),"Truncated physics pool backing");return b;
}
void same(std::span<const std::byte> a,std::span<const std::byte>b,const std::string &why) {
  check(a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin()),why);
}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
  std::uint32_t v=0;const auto b=m.view(p,4);
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void put(SourceRuntimeMemory&m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4>b;
  for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);
}
SourcePhysicsWorldRuntimeAbi abi(const J &j) {
  SourcePhysicsWorldRuntimeAbi a{};
  a.gp=j.at("gp").get<decltype(a.gp)>();a.eflags=j.at("eflags");
  a.x87_status=j.at("x87_status");
  a.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control").get<std::uint16_t>(),j.at("mxcsr")};
  for(unsigned i=0;i<8;++i) {
    const auto h=j.at("xmm").at(i).get<std::string>();check(h.size()==32,"Original XMM extent");
    for(unsigned k=0;k<4;++k)a.xmm[i][k]=std::stoul(h.substr(24-8*k,8),nullptr,16);
  }
  return a;
}
void same_abi(const SourcePhysicsWorldRuntimeAbi&a,const J&j,const std::string&why) {
  const auto b=abi(j);
  if(a.gp!=b.gp) {
    std::ostringstream s;s<<why<<" GP"<<std::hex;
    for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])s<<' '<<i<<':'<<a.gp[i]<<'/'<<b.gp[i];
    throw std::runtime_error(s.str());
  }
  check((a.eflags&0xcd5u)==(b.eflags&0xcd5u),why+" defined flags");
  check(a.xmm==b.xmm && a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
      a.floating_point->mxcsr==b.floating_point->mxcsr && a.x87_status==b.x87_status &&
      !a.x87_depth && !a.active_mmx,why+" retained FP state");
}
}
int main(int argc,char**argv) {
  try {
    check(argc==3,"Supply original TU1.5 physics pool fixture and selected executable");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
#if !defined(__linux__)
    return 77;
#endif
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);
    const auto directory=std::filesystem::path(argv[1]).parent_path();
    check(fixture.at("sequences").size()==3,"All original pool address layouts");
    const auto stack=fixture.at("stack").get<std::uint32_t>(),
        stack_bytes=fixture.at("stack_bytes").get<std::uint32_t>();
    const auto globals=fixture.at("globals").get<std::array<std::uint32_t,3>>();
    std::size_t steps=0,stores_count=0,resources_count=0,retained=0,resumed_steps=0;
    for(const auto &sequence:fixture.at("sequences")) {
      auto m=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
      for(unsigned i=0;i<3;++i)same(m.view(globals[i],4),bytes(sequence.at("initial_globals")[i]),"Original initial pool globals");
      m.mount(stack,std::vector<std::byte>(stack_bytes),SourceRuntimeMemory::Access::ReadWrite,
          "declared standalone source pool caller stack");
      // Without a platform owner, the complete source producer stops at the
      // actual import ABI rather than publishing a fabricated empty pool.
      auto stopped=abi(sequence.at("steps")[0].at("entry_abi"));
      put(m,stopped.gp[4],fixture.at("stop"));bool missing=false;
      try {source_run_physics_pool_startup_runtime(m,0x2284c0,stopped,
          SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},{});}
      catch(const std::runtime_error&){missing=true;}
      check(missing,"Missing native pool platform owner did not stop");
      same_abi(stopped,sequence.at("resources")[0].at("entry_abi"),"Exact original allocation frontier");
      for(unsigned i=0;i<3;++i)same(m.view(globals[i],4),bytes(sequence.at("initial_globals")[i]),"Missing owner changed pool publication");
      std::size_t resource_index=0;
      const auto observer=[&](const SourcePhysicsPoolResourceEvent &event) {
        check(resource_index<sequence.at("resources").size(),"Extra native pool resource request");
        const auto &e=sequence.at("resources")[resource_index++];
        check(event.allocation==e.at("allocation") && event.request==e.at("request").get<decltype(event.request)>() &&
            event.result==e.at("result").get<decltype(event.result)>(),"Actual native pool resource contract differs");
        if(event.allocation) same(m.view(event.result[1],event.result[2]),file(directory/e.at("initial").get<std::string>()),
            "Actual native initial pages differ");
      };
      {
        SourcePhysicsPoolHostStorage host(m,sequence.at("first_token"),observer);
        std::unique_ptr<SourceRuntimeMemory> replica;
        std::vector<SourcePhysicsPoolResourceEvent> replica_resources;
        std::unique_ptr<SourcePhysicsPoolHostStorage> replica_host;
        std::size_t sequence_step=0;
        for(const auto &row:sequence.at("steps")) {
          const auto label=sequence.at("name").get<std::string>()+" "+row.at("kind").get<std::string>()+" "+std::to_string(steps);
          auto a=abi(row.at("entry_abi"));put(m,a.gp[4],fixture.at("stop"));
          std::vector<SourcePhysicsContactRecordStore> stores;
          const auto observe=[&](const SourcePhysicsContactRecordStore &s) {
            if(stack<=s.address && s.address<stack+stack_bytes)return;
            stores.push_back(s);
          };
          check(resource_index==row.at("resource_begin"),label+" initial resource boundary");
          source_run_physics_pool_startup_with_native_storage(m,row.at("entry"),a,{0x27f,0x1f80},{},host,observe);
          same_abi(a,row.at("exit_abi"),label);
          check(stores.size()==row.at("stores").size(),label+" original store count");
          for(std::size_t i=0;i<stores.size();++i) {
            const auto &s=stores[i];const auto &e=row.at("stores")[i];
            std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<s.instruction;
            check(fixture.at("canonical_instruction_map").at(key.str())==e.at("instruction") &&
                s.address==e.at("address") && s.before==bytes(e.at("before")) && s.written==bytes(e.at("written")),
                label+" ordered original store "+std::to_string(i));
          }
          for(unsigned i=0;i<3;++i)check(word(m,globals[i])==row.at("globals")[i],label+" publication order");
          check(resource_index==row.at("resource_end") && host.live_allocations()==row.at("live_allocations"),label+" actual resource lifetime");
          if(replica) {
            auto resumed=abi(row.at("entry_abi"));put(*replica,resumed.gp[4],fixture.at("stop"));
            std::vector<SourcePhysicsContactRecordStore> resumed_stores;replica_resources.clear();
            source_run_physics_pool_startup_with_native_storage(*replica,row.at("entry"),resumed,{0x27f,0x1f80},{},*replica_host,
                [&](const auto &s){if(!(stack<=s.address && s.address<stack+stack_bytes))resumed_stores.push_back(s);});
            same_abi(resumed,row.at("exit_abi"),label+" resumed");
            check(resumed_stores.size()==stores.size(),label+" resumed store count");
            for(std::size_t i=0;i<stores.size();++i)
              check(resumed_stores[i].instruction==stores[i].instruction && resumed_stores[i].address==stores[i].address &&
                  resumed_stores[i].before==stores[i].before && resumed_stores[i].written==stores[i].written,
                  label+" resumed source write "+std::to_string(i));
            const auto first=row.at("resource_begin").get<std::size_t>();
            check(replica_resources.size()==row.at("resource_end").get<std::size_t>()-first,label+" resumed resources");
            for(std::size_t i=0;i<replica_resources.size();++i) {
              const auto &e=sequence.at("resources")[first+i];const auto &actual=replica_resources[i];
              check(actual.allocation==e.at("allocation") && actual.request==e.at("request").get<decltype(actual.request)>() &&
                  actual.result==e.at("result").get<decltype(actual.result)>(),label+" resumed native resource request");
            }
            check(replica_host->ownership_checkpoint()==host.ownership_checkpoint(),label+" resumed resource registry");
            ++resumed_steps;
          }
          if(++sequence_step==8) {
            // Continue an occupied original pool in another bank. Deliberately
            // give the new host a different initial token; only the imported
            // allocation cursor/registry may select future resource addresses.
            replica=std::make_unique<SourceRuntimeMemory>();
            replica_host=std::make_unique<SourcePhysicsPoolHostStorage>(*replica,0x70000000u,
                [&](const auto &event){replica_resources.push_back(event);});
            const auto saved=m.checkpoint();
            auto prepared=replica_host->prepare_ownership(host.ownership_checkpoint(),saved);
            replica->restore(saved);
            check(replica_host->commit_ownership(std::move(prepared)),label+" admitted pool import");
          }
          stores_count+=stores.size();++steps;
        }
        check(replica && replica->checkpoint()==m.checkpoint(),"Complete resumed source pool bank differs");
        check(resource_index==sequence.at("resources").size(),"Complete native resource sequence");
        for(const auto &bank:sequence.at("final_storage")) {
          const auto b=file(directory/bank.at("storage").get<std::string>());
          same(m.view(bank.at("address"),bank.at("bytes")),b,"Complete retained native pool bytes");retained+=b.size();
        }
      }
      // C++ owner cleanup releases remaining host backing; it is not the
      // original occupied physical-component destructor or world teardown.
      for(const auto &bank:sequence.at("final_storage")) {
        bool unavailable=false;try{m.view(bank.at("address"),1);}catch(const std::exception&){unavailable=true;}
        check(unavailable,"Native pool owner retained disposed host storage");
      }
      resources_count+=resource_index;
    }
    check(steps==3552 && stores_count==122451,"Complete original physics pool lifecycle corpus");
    check(resumed_steps==3528,"Complete resumed source pool lifecycle corpus");
    std::cout<<steps<<" fresh TU1.5 pool lifecycle calls, "<<stores_count<<" ordered writes, "
        <<resources_count<<" actual host allocation/release requests and "<<retained<<" final retained bytes match; "
        <<resumed_steps<<" subsequent source calls also match after staged cross-owner import\n";
  } catch(const std::exception&e) {std::cerr<<e.what()<<'\n';return 1;}
}
