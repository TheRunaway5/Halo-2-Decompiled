#include "halo2/source_sound_startup_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
Json load(const std::filesystem::path &p){std::ifstream f(p);check(bool(f),"Original sound state unavailable: "+p.string());return Json::parse(f);}
std::vector<std::byte> bytes(const std::string &hex){
 check(hex.size()%2==0,"Malformed original byte bank");std::vector<std::byte> out(hex.size()/2);
 const auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);throw std::runtime_error("Malformed original hex digit");};
 for(std::size_t i=0;i<out.size();++i)out[i]=std::byte((digit(hex[i*2])<<4)|digit(hex[i*2+1]));return out;
}
SourcePhysicsWorldRuntimeAbi import(const Json &j){
 SourcePhysicsWorldRuntimeAbi a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();a.eflags=j.at("eflags");a.x87_status=j.at("x87_status");a.x87_depth=j.at("x87_depth");
 check(a.x87_depth<=8,"Original x87 depth exceeds its actual register file");
 for(unsigned i=0;i<a.x87_depth;++i){auto raw=bytes(j.at("x87_logical_hex")[i]);check(raw.size()==10,"Original x87 value extent differs");std::memcpy(&a.x87[i],raw.data(),10);}
 a.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control"),j.at("mxcsr")};return a;
}
void compare_abi(const SourcePhysicsWorldRuntimeAbi &a,const Json &j,const std::string &why){
 const auto expected=import(j);check(a.gp==expected.gp,why+" GP registers differ");check(a.xmm==expected.xmm,why+" SSE registers differ");check((a.eflags&0xcd5u)==(expected.eflags&0xcd5u),why+" defined flags differ");
 check(a.x87_status==expected.x87_status && a.x87_depth==expected.x87_depth && !a.active_mmx,why+" x87 status/depth differs");
 check(a.floating_point && (a.floating_point->x87_control_word&0x1f3fu)==(expected.floating_point->x87_control_word&0x1f3fu) && a.floating_point->mxcsr==expected.floating_point->mxcsr,why+" FP controls/status differ");
 for(unsigned i=0;i<a.x87_depth;++i)check(std::memcmp(&a.x87[i],&expected.x87[i],10)==0,why+" actual extended x87 bits differ");
}
struct Retained {
 SourceRuntimeMemory memory;
 std::map<std::uint32_t,std::size_t> known;
 void mount(const Json &r){auto raw=bytes(r.at("hex"));const auto p=r.at("address").get<std::uint32_t>();check(raw.size()==r.at("size"),"Actual original bank size differs");
  memory.mount(p,raw,r.at("writable").get<bool>()?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"genuine original TU1.5 sound ABI backing");known.emplace(p,raw.size());}
 void check_state(const Json &state,const std::string &why){for(const auto &r:state.at("regions")){
  const auto p=r.at("address").get<std::uint32_t>();const auto expected=bytes(r.at("hex"));check(known.contains(p)&&known.at(p)==expected.size(),why+" unretained actual original region");const auto actual=memory.view(p,expected.size());
  const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin(),expected.end());if(mismatch.first!=actual.end())throw std::runtime_error(why+" bytes differ at "+std::to_string(p+std::uint32_t(mismatch.first-actual.begin())));
 }}
 std::size_t external_input(const Json &before,const Json &after){std::size_t changed{};
  for(const auto &r:after.at("regions")){
   const auto p=r.at("address").get<std::uint32_t>();const auto actual=bytes(r.at("hex"));
   const auto prior=std::find_if(before.at("regions").begin(),before.at("regions").end(),[&](const auto &q){return q.at("address")==p;});
   if(!known.contains(p)){mount(r);changed+=actual.size();continue;}
   check(prior!=before.at("regions").end(),"Original I/O delta lacks its captured prior backing");const auto initial=bytes(prior->at("hex"));check(initial.size()==actual.size(),"Original I/O changed region extent");
   for(std::size_t start=0;start<actual.size();){if(initial[start]==actual[start]){++start;continue;}auto end=start+1;while(end<actual.size()&&initial[end]!=actual[end])++end;
    check(r.at("writable").get<bool>(),"Original I/O changed retained immutable source bytes");memory.write(p+std::uint32_t(start),std::span(actual).subspan(start,end-start));changed+=end-start;start=end;
   }
  }return changed;
 }
};
struct MissingIo {};
}
int main(int argc,char **argv)try{
 check(argc==3,"Supply original TU1.5 CAPTURE_DIRECTORY CPU_ORACLE_REPORT");
 const std::filesystem::path directory(argv[1]),oracle_path(argv[2]);if(directory.empty()||oracle_path.empty()||!std::filesystem::is_regular_file(directory/"report.json")||!std::filesystem::is_regular_file(oracle_path))return 77;
 const auto oracle=load(oracle_path),capture=load(directory/"report.json");const auto hardware=load(oracle.at("original_results_path").get<std::string>());constexpr auto pin="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd";
 check(oracle.at("status")=="pass"&&oracle.at("original_sha256")==pin&&capture.at("original_sha256")==pin,"Original sound authority/replay is not accepted");
 check(oracle.at("original_code_patches").empty()&&!oracle.at("native_expected_data_used").get<bool>()&&!oracle.at("original_callee_returns_supplied").get<bool>(),"Original CPU comparison supplied an expected result");
 const auto computed_state=[&](Json observed,unsigned identity){
  const auto &cpu=hardware.at("cases").at(identity-1);check(cpu.at("id")==identity,"Original CPU interval identity differs");
  for(auto &region:observed.at("regions")){
   const auto &banks=cpu.at("final_banks");const auto found=std::find_if(banks.begin(),banks.end(),[&](const auto &b){return b.at("address")==region.at("address");});
   check(found!=banks.end(),"Original CPU interval omitted retained backing");region["hex"]=found->at("hex");
  }return observed;
 };
 unsigned whole{},io{},fragments{};std::size_t external_bytes{};
 for(const auto &owner:oracle.at("owners")){
  const auto name=owner.at("owner").get<std::string>();const auto folder=directory/name;const auto original=load(folder/"report.json"),before=load(folder/"before.json"),after=load(folder/"after.json");
  check(original.at("original_returned_normally").get<bool>()&&original.at("supplied_callee_returns").empty(),"Original owner did not return naturally");
  Retained retained;for(const auto &r:before.at("regions"))retained.mount(r);auto abi=import(before.at("abi"));std::vector<std::uint32_t> trace;unsigned next{};
  SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};
  const auto check_trace=[&]{const auto identity=owner.at("fragment_ids").at(next).get<unsigned>();check(trace==oracle.at("fragments").at(identity-1).at("instructions").get<decltype(trace)>(),name+" source instruction order differs");trace.clear();++fragments;};
  services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe){
   if(call.target==0x166a80u){source_run_physics_live_support_tu15(memory,call.target,state,*state.floating_point,services,observe);return;}
   check(next<original.at("calls").size(),name+" requested an unobserved original I/O owner");const auto request=load(folder/("call-"+std::string(next+1<10?"0":"")+std::to_string(next+1)+".json"));
   check(call.target==request.at("entry")&&!call.tail,name+" original external call target differs");compare_abi(state,request.at("before").at("abi"),name+" external entry");const auto computed=computed_state(request.at("before"),owner.at("fragment_ids").at(next));retained.check_state(computed,name+" exact original CPU interval");check_trace();
   // Test-only actual external state includes original I/O outputs and the
   // explicitly reported unowned private stack residue. Every CPU bank was
   // first compared against unchanged original i386 execution, with no masks.
   // Production still requires real section/device/DSP owners.
   external_bytes+=retained.external_input(computed,request.at("after"));state=import(request.at("after").at("abi"));++next;++io;
  };
  source_run_sound_startup_tu15(retained.memory,owner.at("entry"),abi,*abi.floating_point,services);
  check(next==original.at("calls").size(),name+" bypassed real original I/O requests");check_trace();compare_abi(abi,after.at("abi"),name+" completed owner");retained.check_state(computed_state(after,owner.at("fragment_ids").at(next)),name+" exact completed CPU owner");++whole;
  // An unavailable provider stops at the real first call. It supplies no
  // source false/true, device identity, availability byte or skipped phase.
  Retained missing;for(const auto &r:before.at("regions"))missing.mount(r);auto stopped=import(before.at("abi"));SourcePhysicsWorldRuntimeServices unavailable;
  unavailable.required_owner=[&](auto &,const auto &,auto &,const auto &){throw MissingIo{};};bool rejected=false;
  try{source_run_sound_startup_tu15(missing.memory,owner.at("entry"),stopped,*stopped.floating_point,unavailable);}catch(const MissingIo&){rejected=true;}
  check(rejected,name+" missing I/O owner returned a fabricated source result");const auto first=load(folder/"call-01.json");compare_abi(stopped,first.at("before").at("abi"),name+" unavailable owner");missing.check_state(computed_state(first.at("before"),owner.at("fragment_ids").at(0)),name+" unavailable owner");
 }
 check(whole==2&&io==16&&fragments==18,"Original sound coverage changed unexpectedly");
 std::cout<<"PASS original TU1.5 sound startup CPU: "<<whole<<" genuine composed owners, "<<io<<" observed external I/O inputs, "<<fragments<<" exact CPU intervals, "<<external_bytes<<" external bytes; GP/SSE/x87/defined flags, retained banks and instruction order; two unavailable-owner stops; no native device/DSP/PCM claim\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
