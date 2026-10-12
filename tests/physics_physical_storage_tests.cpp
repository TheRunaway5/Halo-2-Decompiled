#include "halo2/physics_physical_storage_runtime.hpp"
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
  check(!(s.size()&1u),"Invalid physics physical stage fixture byte extent");
  std::vector<std::byte> b;
  for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return b;
}
std::vector<std::byte> file(const std::filesystem::path &p) {
  std::ifstream in(p,std::ios::binary|std::ios::ate);
  check(bool(in),"Missing original physics physical stage backing: "+p.string());
  const auto size=in.tellg();check(size>=0,"Invalid physics physical stage backing extent");
  std::vector<std::byte> b(static_cast<std::size_t>(size));in.seekg(0);
  check(bool(in.read(reinterpret_cast<char *>(b.data()),b.size())),"Truncated physics physical stage backing");return b;
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
    check(argc==3,"Supply original TU1.5 physics physical stage fixture and selected executable");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
#if !defined(__linux__)
    return 77;
#endif
    {
      SourceRuntimeMemory pages;SourceRuntimeHostPhysicalMemory host(pages);
      const auto base=host.allocate({0x2000u,0x20000u,0x21fffu,0x1000u,4u});
      check(base!=0,"Native texture-stage backing allocation failed");
      put(pages,base+0x1ffcu,0x12345678u);
      host.protect(base,0x2000u,0x404u);
      check(word(pages,base+0x1ffcu)==0x12345678u,"Native texture-stage protection changed retained bytes");
      host.protect(base+0x1000u,0x1000u,4u);
      for(const auto q:std::array<std::array<std::uint32_t,3>,4>{{
          {base,0x1000u,0x104u},{base,0x1000u,0x204u},
          {base+1u,0x1000u,0x404u},{base+0x1000u,0x2000u,0x404u}}}) {
        bool rejected=false;try{host.protect(q[0],q[1],q[2]);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"Native physical protection admitted unsupported flags or escaped its allocation");
      }
    }
    std::ifstream in(argv[1]);const auto fixture=J::parse(in);
    const auto directory=std::filesystem::path(argv[1]).parent_path();
    check(fixture.at("sequences").size()==3,"All original physical stage request sequences");
    const auto stack=fixture.at("stack").get<std::uint32_t>(),
        stack_bytes=fixture.at("stack_bytes").get<std::uint32_t>();
    const auto globals=fixture.at("globals").get<std::array<std::uint32_t,8>>();
    std::size_t steps=0,stores_count=0,resources_count=0,retained=0;
    for(const auto &sequence:fixture.at("sequences")) {
      auto m=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
      for(unsigned i=0;i<8;++i)same(m.view(globals[i],4),bytes(sequence.at("initial_globals")[i]),"Original initial physical stage globals");
      m.mount(stack,std::vector<std::byte>(stack_bytes),SourceRuntimeMemory::Access::ReadWrite,
          "declared standalone source physical stage caller stack");
      // Without a platform owner, the complete source producer stops at the
      // actual import ABI with preceding source publication intact.
      {
      auto negative=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
      negative.mount(stack,std::vector<std::byte>(stack_bytes),SourceRuntimeMemory::Access::ReadWrite,
          "independent missing-owner source caller stack");
      auto stopped=abi(sequence.at("steps")[0].at("entry_abi"));
      put(negative,stopped.gp[4],fixture.at("stop"));bool missing=false;
      try {source_run_physics_physical_storage_runtime(negative,0x12cb70,stopped,
          SourcePhysicsContactImage::TitleUpdate15,{0x27f,0x1f80},{});}
      catch(const std::runtime_error&){missing=true;}
      check(missing,"Missing native physical stage platform owner did not stop");
      same_abi(stopped,sequence.at("resources")[0].at("entry_abi"),"Exact original allocation frontier");
      for(unsigned i=0;i<8;++i)check(word(negative,globals[i])==sequence.at("steps")[0].at("globals")[i],"Missing owner changed source stage prefix");
      }
      std::size_t resource_index=0;
      const auto observer=[&](const SourcePhysicsPhysicalResourceEvent &event) {
        check(resource_index<sequence.at("resources").size(),"Extra native physical stage resource request");
        const auto &e=sequence.at("resources")[resource_index++];
        check(event.allocation==e.at("allocation") && event.request==e.at("request").get<decltype(event.request)>() &&
            event.result==e.at("result").get<std::uint32_t>(),"Actual native physical stage resource contract differs");
        if(event.allocation) same(m.view(event.result,event.request[0]),file(directory/e.at("initial").get<std::string>()),
            "Actual native initial pages differ");
      };
      {
        SourceRuntimeHostPhysicalMemory host(m);
        for(const auto &row:sequence.at("steps")) {
          const auto label=sequence.at("name").get<std::string>()+" "+row.at("kind").get<std::string>()+" "+std::to_string(steps);
          auto a=abi(row.at("entry_abi"));put(m,a.gp[4],fixture.at("stop"));
          unsigned argument_index=0;
          for(const auto &arg:row.at("arguments"))put(m,a.gp[4]+4+4*argument_index++,arg);
          for(const auto &input:row.at("inputs"))m.write(input.at("address"),bytes(input.at("bytes")));
          std::vector<SourcePhysicsContactRecordStore> stores;
          const auto observe=[&](const SourcePhysicsContactRecordStore &s) {
            if(stack<=s.address && s.address<stack+stack_bytes)return;
            stores.push_back(s);
          };
          check(resource_index==row.at("resource_begin"),label+" initial resource boundary");
          source_run_physics_physical_storage_with_native_pages(m,row.at("entry"),a,{0x27f,0x1f80},{},host,observe,observer);
          same_abi(a,row.at("exit_abi"),label);
          check(stores.size()==row.at("stores").size(),label+" original store count");
          for(std::size_t i=0;i<stores.size();++i) {
            const auto &s=stores[i];const auto &e=row.at("stores")[i];
            std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(8)<<s.instruction;
            check(fixture.at("canonical_instruction_map").at(key.str())==e.at("instruction") &&
                s.address==e.at("address") && s.before==bytes(e.at("before")) && s.written==bytes(e.at("written")),
                label+" ordered original store "+std::to_string(i));
          }
          for(unsigned i=0;i<8;++i)check(word(m,globals[i])==row.at("globals")[i],label+" publication order");
          check(resource_index==row.at("resource_end"),label+" actual resource lifetime");
          stores_count+=stores.size();++steps;
        }
        check(resource_index==sequence.at("resources").size(),"Complete native resource sequence");
        for(const auto &bank:sequence.at("final_storage")) {
          const auto b=file(directory/bank.at("storage").get<std::string>());
          same(m.view(bank.at("address"),bank.at("bytes")),b,"Complete retained native physical stage bytes");retained+=b.size();
        }
      }
      // C++ owner cleanup releases remaining host backing; it is not the
      // original heap destruction or world teardown.
      for(const auto &bank:sequence.at("final_storage")) {
        bool unavailable=false;try{m.view(bank.at("address"),1);}catch(const std::exception&){unavailable=true;}
        check(unavailable,"Native pool owner retained disposed host storage");
      }
      resources_count+=resource_index;
    }
    check(steps==48 && stores_count==216,"Complete original fresh physical stage corpus");
    std::cout<<steps<<" fresh TU1.5 physical stage lifecycle calls, "<<stores_count<<" ordered writes, "
        <<resources_count<<" actual host allocation/protection requests and "<<retained<<" final retained bytes match\n";
  } catch(const std::exception&e) {std::cerr<<e.what()<<'\n';return 1;}
}
