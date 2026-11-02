#include "audio/native_tu15_sound_kernel.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <vector>
using namespace halo2;
namespace {
using J=nlohmann::json;
struct Fault {std::uint32_t address,bytes;};
struct Memory final:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::array<std::byte,4096>> pages;
 void map(std::uint32_t a,std::uint32_t n){for(std::uint32_t p=a>>12;p<=((a+n-1)>>12);++p)pages.try_emplace(p);}
 void read(std::uint32_t a,std::span<std::byte> out)const override{
  const auto first=a,n=std::uint32_t(out.size());for(auto& b:out){auto p=pages.find(a>>12);if(p==pages.end())throw Fault{first,n};b=p->second[a++&4095u];}}
 void write(std::uint32_t a,std::span<const std::byte> in)override{
  const auto first=a,n=std::uint32_t(in.size());for(auto b:in){auto p=pages.find(a>>12);if(p==pages.end())throw Fault{first,n};p->second[a++&4095u]=b;}}
 void put(std::uint32_t a,unsigned n,std::uint32_t v){std::array<std::byte,4> b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(k*8));write(a,std::span(b).first(n));}
};
std::uint32_t value(std::span<const std::byte> b){std::uint32_t v{};for(unsigned k=0;k<b.size();++k)v|=std::to_integer<std::uint32_t>(b[k])<<(k*8);return v;}
void need(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
}
int main(int argc,char**argv)try{
 if(argc!=3)throw std::runtime_error("Supply independent original JSONL fixture and report");
 if(!*argv[1])return 77;
 std::ifstream file(argv[1]);need(bool(file),"Original packet fixture missing");std::string line;
 std::getline(file,line);const auto header=J::parse(line);
 need(header.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
 header.at("raw_abi_only")==true&&header.at("platform_results_supplied").empty(),"Packet fixture authority differs");
 unsigned cases{},returns{},faults{},stores_count{};std::map<std::uint32_t,unsigned> entries;
 while(std::getline(file,line)){
  if(line.empty())continue;const auto row=J::parse(line);const auto entry=row.at("entry").get<std::uint32_t>();
  const auto table=native_tu15_sound::resource_kernels();
  const auto kernel=std::ranges::find(table,entry,&native_tu15_sound::Kernel::entry);need(kernel!=table.end(),"Original packet entry has no native kernel");
  Memory memory;for(const auto& q:header.at("regions"))memory.map(q[0],q[1]);
  for(const auto& q:row.at("fields"))memory.put(q[0],q[1],q[2]);
  auto expected=memory;
  SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
  abi.eflags=row.value("eflags",0x202u);abi.fs_base=0x12345000;abi.x87_depth=4;abi.x87={1.L,-1.L,0.L,-0.L,7.L,9.L,11.L,13.L};
  abi.x87_status=row.at("fp")[2];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("fp")[0],row.at("fp")[1]};
  const auto initial=abi;J stores=J::array(),calls=J::array();std::optional<Fault> fault;
  native_tu15_sound::Services services;
  const auto allowed=header.value("native_cpu_callees",std::vector<std::uint32_t>{});
  services.required_owner=[&](auto& m,const auto& call,auto& a,const auto& observer){
   need(std::ranges::find(allowed,call.target)!=allowed.end(),"Stream callee lacks its declared native CPU owner");
   const auto get=[&](std::uint32_t at){std::array<std::byte,4> b{};m.read(at,b);return value(b);};
   const unsigned count=call.target==0x21f260u?3u:
       (call.target==0x2e4d60u||call.target==0x21f3b0u||call.target==0x21f3f0u)?1u:0u;
   need(count!=0,"Stream CPU callee ABI is unavailable");J args=J::array();
   for(unsigned n=0;n<count;++n)args.push_back(get(a.gp[4]+4u+n*4u));
   calls.push_back({{"target",call.target},{"gp",a.gp},{"return",get(a.gp[4])},{"args",args}});
   const auto k=std::ranges::find(table,call.target,&native_tu15_sound::Kernel::entry);
   need(k!=table.end(),"Declared stream CPU callee is not native");k->run(m,a,*a.floating_point,observer,services);
  };
  try{kernel->run(memory,abi,*abi.floating_point,[&](const auto& q){stores.push_back(J::array({q.instruction,q.address,q.written.size(),value(q.before),value(q.written)}));},services);}
  catch(const Fault& f){fault=f;}
  const bool expected_fault=!row.at("fault").is_null();
  if(bool(fault)!=expected_fault || (fault&&(fault->address!=row["fault"][0]||fault->bytes!=row["fault"][1]))){
   std::cerr<<"Packet case "<<row["case"]<<" entry "<<std::hex<<entry<<" fault differs\n";return 1;}
  if(stores!=row.at("stores")){std::cerr<<"Packet case "<<row["case"]<<" entry "<<std::hex<<entry<<" ordered stores differ: "<<stores.dump()<<" expected "<<row["stores"].dump()<<'\n';return 1;}
  if(calls!=row.value("calls",J::array())){std::cerr<<"Packet case "<<row["case"]<<" callee ABI differs\n";return 1;}
  for(const auto& q:row["stores"]){std::array<std::byte,4> before{};expected.read(q[1],std::span(before).first(q[2]));
   need(value(std::span(before).first(q[2]))==q[3],"Original store preimage differs");expected.put(q[1],q[2],q[4]);}
  if(header.value("source_stack_stores_observed",false)){
   need(row.contains("source_stack_stores"),"Original stack capture missing");
   for(const auto& q:row.at("source_stack_stores")){
    const auto at=q[0].get<std::uint32_t>(),bytes=q[1].get<unsigned>();
    need(at>=0x02000000u&&at+bytes<=0x02010000u,"Original stack capture outside declared storage");
    std::array<std::byte,4> before{};expected.read(at,std::span(before).first(bytes));
    need(value(std::span(before).first(bytes))==q[2],"Original stack preimage differs");expected.put(at,bytes,q[3]);
   }
  }
  if(memory.pages!=expected.pages){std::cerr<<"Packet case "<<row["case"]<<" entry "<<std::hex<<entry<<" memory differs outside original stores\n";return 1;}
  if(!fault&&abi.gp!=row.at("out_gp").get<decltype(abi.gp)>()){
   std::cerr<<"Packet case "<<row["case"]<<" entry "<<std::hex<<entry<<" registers differ: "<<J(abi.gp).dump()<<" expected "<<row["out_gp"].dump()<<'\n';return 1;}
  if(header.value("caller_direction_preserved",false))need((abi.eflags&0x400u)==(initial.eflags&0x400u),"Core datum owner changed the caller direction flag");
  need(abi.xmm==initial.xmm&&abi.x87_depth==initial.x87_depth&&abi.x87_status==initial.x87_status&&
       abi.floating_point->x87_control_word==initial.floating_point->x87_control_word&&abi.floating_point->mxcsr==initial.floating_point->mxcsr&&
       abi.fs_base==initial.fs_base&&!abi.active_mmx,"Integer packet kernel changed caller FP/SIMD/FS state");
  constexpr auto extended_bytes=std::numeric_limits<long double>::digits==64&&sizeof(long double)>10?10:sizeof(long double);
  for(unsigned n=0;n<8;++n)need(std::memcmp(&abi.x87[n],&initial.x87[n],extended_bytes)==0,"Integer packet kernel changed caller extended values");
  ++cases;++entries[entry];stores_count+=stores.size();if(fault)++faults;else ++returns;
 }
 const auto required=header.value("required_entries",std::vector<std::uint32_t>{0x21f360u,0x2b98d0u,0x2b9950u});
 need(cases>=header.value("minimum_cases",1500u)&&entries.size()==required.size()&&faults>0,"Original stream coverage incomplete");
 for(const auto entry:required)need(entries.contains(entry),"Required original stream entry absent");
 const J report{{"passed",true},{"cases",cases},{"returns",returns},{"faults",faults},{"ordered_stores",stores_count},
 {"all_general_registers_on_return",true},{"caller_fp_simd_preserved",true},{"all_mapped_native_bytes_checked",true},
 {"callee_stack_scratch_excluded",!header.value("source_stack_stores_observed",false)},
 {"original_stack_bytes_checked",header.value("source_stack_stores_observed",false)},
 {"caller_direction_preserved_checked",header.value("caller_direction_preserved",false)},
 {"callee_entry_gprs_arguments_checked",!header.value("native_cpu_callees",std::vector<std::uint32_t>{}).empty()},
 {"native_world_voice_or_device_admission",false},{"multi_chunk_loop_playback_verified",false}};
 std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

