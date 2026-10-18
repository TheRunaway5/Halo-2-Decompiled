#include "halo2/source_campaign_activation_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <xmmintrin.h>
namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool v,const std::string &why){if(!v)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Odd hex fixture");std::vector<std::byte> out;for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
Json read(const std::filesystem::path&p){std::ifstream f(p);check(bool(f),"Required present fixture is unavailable");Json j;f>>j;return j;}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>> banks;
 auto locate(std::uint32_t a,std::size_t n)const{auto it=banks.upper_bound(a);check(it!=banks.begin(),"Undeclared fixture owner");--it;const auto o=std::uint64_t(a)-it->first;check(o<=it->second.size()&&n<=it->second.size()-o,"Fixture owner extent escaped");return it;}
 void read(std::uint32_t a,std::span<std::byte>v)const override{auto it=locate(a,v.size());std::copy_n(it->second.begin()+(a-it->first),v.size(),v.begin());}
 void write(std::uint32_t a,std::span<const std::byte>v)override{auto it=locate(a,v.size());std::copy(v.begin(),v.end(),banks.at(it->first).begin()+(a-it->first));}
 std::uint32_t word(std::uint32_t a)const{std::array<std::byte,4>b;read(a,b);std::uint32_t v=0;for(unsigned n=0;n<4;++n)v|=std::to_integer<std::uint32_t>(b[n])<<(8*n);return v;}
};
struct MissingCallback {std::uint32_t target;};
void abi_equal(const SourcePhysicsWorldRuntimeAbi&a,const Json&j,const std::string&why){
 check(a.gp==j.at("gp").get<decltype(a.gp)>(),why+" GP");check(a.xmm==j.at("xmm").get<decltype(a.xmm)>(),why+" XMM");check((a.eflags&0xcc5u)==(j.at("eflags").get<unsigned>()&0xcc5u),why+" defined flags");
 check(a.fs_base==0 && a.x87_depth==0 && a.x87_status==0 && !a.active_mmx,why+" retained empty FP/MMX/FS state");
 check(a.floating_point && a.floating_point->x87_control_word==0x27f && a.floating_point->mxcsr==0x1f80,why+" source FP controls");
}
std::vector<std::byte> fixture_code(unsigned id,std::uint32_t canary){
 std::vector<std::byte>b{std::byte{0xc6},std::byte{0x05}};
 const auto word=[&](std::uint32_t v){for(unsigned k=0;k<4;++k)b.push_back(std::byte(v>>(8*k)));};word(canary);b.push_back(std::byte(id+1));
 for(auto pair:{std::pair{0xb8u,0xa0000000u},std::pair{0xb9u,0xc0000000u},std::pair{0xbau,0xd0000000u}}){b.push_back(std::byte(pair.first));word(pair.second|id);}b.push_back(std::byte{0xc3});return b;
}
}
int main(int argc,char**argv)try{
 check(argc==2||argc==3,"Supply fixture directory [--baseline-missing]");const auto root=std::filesystem::path(argv[1]);const bool missing_mode=argc==3;check(!missing_mode||std::string(argv[2])=="--baseline-missing","Unknown control");
 if(!std::filesystem::is_regular_file(root/"report.json"))return 77;
 const auto report=read(root/"report.json");check(report.at("status")=="PASS_UNCHANGED_ORIGINAL_CLEANUP_WITH_DECLARED_FIXTURE_CALLBACKS"&&report.at("suite")=="campaign-cleanup","Invalid original fixture status/suite");
 check(report.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&report.at("entry")==0x13a7e0,"Wrong original body");check(report.at("instruction_patches").empty()&&report.at("engine_results_supplied").empty(),"Fixture supplies engine code/results");
 const std::vector<std::string>names{"empty-active","empty-inactive","sparse-edges","all68","duplicate-target","missing-first","missing-middle"};
 check(report.at("cases").size()==names.size(),"Exact seven cases required");std::set<std::uint32_t>pcs;for(auto v:report.at("static_original_PCs"))pcs.insert(v.get<std::uint32_t>());check(pcs.size()==15&&pcs.contains(0x13a7e6)&&pcs.contains(0x13a810),"Wrong static original coverage");
 unsigned comparisons=0,callbacks=0,stops=0,total_stores=0;
 for(std::size_t index=0;index<names.size();++index){
  const auto&item=report.at("cases").at(index);check(item.at("name")==names[index]&&item.at("file")==names[index]+".json","Fixture name/order/path changed");const auto row=read(root/(names[index]+".json"));check(row.at("name")==names[index]&&row.at("entry")==0x13a7e0,"Wrong case owner");
  for(bool observed:{false,true}){
   Memory memory;for(const auto&[at,data]:row.at("before").items())memory.banks.emplace(std::stoul(at,nullptr,16),bytes(data));const auto pre=memory.banks;
   SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();abi.eflags=row.at("eflags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
   const auto before=abi;std::vector<SourcePhysicsContactRecordStore>stores;std::vector<std::uint32_t>trace;std::size_t call_index=0;bool stopped=false;
   SourcePhysicsContactRecordObserver observer;if(observed)observer=[&](const auto&s){stores.push_back(s);};SourcePhysicsWorldRuntimeServices services;services.instruction=[&](std::uint32_t pc){check(pcs.contains(pc),"Unexpected native instruction owner");trace.push_back(pc);};
   services.required_owner=[&](auto&bank,const auto&call,auto&state,const auto&){
    check(&bank==&memory&&call.instruction==0x13a7f8&&!call.tail,"Wrong callback ABI owner");check(call_index<row.at("calls").size(),"Unexpected callback");const auto&expected=row.at("calls").at(call_index++);check(call.target==expected.at("target"),"Callback target/order differs");abi_equal(state,expected.at("abi"),"Callback boundary");check(memory.word(state.gp[4])==0x13a7fa&&memory.word(state.gp[4])==expected.at("abi").at("stack_return"),"Callback return stack differs");
    const Json*fixture=nullptr;for(const auto&v:row.at("declared_callbacks"))if(v.at("entry")==call.target){check(!fixture,"Duplicate callback fixture");fixture=&v;}
    if(!fixture)throw MissingCallback{call.target};
    const auto id=fixture->at("id").template get<unsigned>();const auto at=fixture->at("canary_address").template get<std::uint32_t>();check(id<68&&at==0x3100000u+id&&fixture->at("canary_value")==id+1&&call.target==0x3001000u+id*0x40u,"Fixture callback scope escaped");check(bytes(fixture->at("code_hex"))==fixture_code(id,at),"Callback program differs from declared MOV/MOV/MOV/MOV/RET fixture");
    // Explicit test-only callback implementation, equivalent to fixture x86.
    // It cannot write the game bank or manufacture an engine readiness result.
    const std::array v{std::byte(id+1)};memory.write(at,v);state.gp[0]=0xa0000000u|id;state.gp[1]=0xc0000000u|id;state.gp[2]=0xd0000000u|id;state.gp[4]+=4;++callbacks;
   };
   if(missing_mode){
    check(!source_campaign_activation_tu15_owns_entry(0x13a7e0),"Baseline unexpectedly owns cleanup");bool rejected=false;try{source_run_campaign_activation_tu15(memory,0x13a7e0,abi,{0x27f,0x1f80},services,observer);}catch(const std::runtime_error&e){rejected=std::string(e.what())=="Required original contact entry unavailable";}check(rejected&&memory.banks==pre&&trace.empty()&&call_index==0&&stores.empty(),"Unsupported baseline mutated fixture");abi_equal(abi,Json{{"gp",before.gp},{"xmm",before.xmm},{"eflags",before.eflags}},"Baseline rejection");std::cout<<"PASS current whole module rejects missing13A7E0 before source stores\n";return 0;
   }
   check(source_campaign_activation_tu15_owns_entry(0x13a7e0),"Missing recovered original13A7E0 owner");
   std::uint16_t host_cw;asm volatile("fnstcw %0":"=m"(host_cw));const auto host_mxcsr=_mm_getcsr();
   try{source_run_campaign_activation_tu15(memory,0x13a7e0,abi,{0x27f,0x1f80},services,observer);}catch(const MissingCallback&e){check(row.at("stopped").size()==1&&row.at("stopped").at(0).at("target")==e.target,"Wrong failure retained");stopped=true;++stops;}
   std::uint16_t after_cw;asm volatile("fnstcw %0":"=m"(after_cw));check(host_cw==after_cw&&host_mxcsr==_mm_getcsr(),"Host FP controls escaped source call");check(stopped==!row.at("stopped").empty()&&call_index==row.at("calls").size(),"Missing callback stop/order differs");
   for(const auto&[at,data]:row.at("after").items())check(memory.banks.at(std::stoul(at,nullptr,16))==bytes(data),"Complete fixture owner bytes differ");abi_equal(abi,row.at("final"),"Final or failed caller");
   std::vector<std::uint32_t>expected_trace;for(auto pc:row.at("trace"))if(pcs.contains(pc.get<std::uint32_t>()))expected_trace.push_back(pc.get<std::uint32_t>());check(trace==expected_trace,"Original dynamic source-PC sequence differs");
   if(observed){check(stores.size()==row.at("stores").size(),"Source store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&s=stores[n];const auto&e=row.at("stores").at(n);check(s.instruction==e.at("pc")&&s.address==e.at("address")&&s.before==bytes(e.at("before"))&&s.written==bytes(e.at("written")),"Source ordered store PC/before/written differs");}total_stores+=stores.size();}++comparisons;
  }
 }
 check(comparisons==14&&stops==4&&callbacks==150&&total_stores>10,"Cleanup coverage lost");std::cout<<"PASS "<<comparisons<<" original/native cases, "<<callbacks<<" declared callbacks, "<<stops<<" preserved missing-owner stops, "<<total_stores<<" ordered original stores\nLIMIT micro-oracle and declared external callbacks; no full activation/readiness/World/GPU claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
