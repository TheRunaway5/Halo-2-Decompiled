#include "halo2/source_runtime_dsp_section.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/source_sound_startup_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
Json load(const std::filesystem::path&p){std::ifstream f(p);check(bool(f),"Missing original DSP reference input");return Json::parse(f);}
std::vector<std::byte> bytes(const std::string&hex){check(hex.size()%2==0,"Malformed DSP reference bytes");std::vector<std::byte> out(hex.size()/2);const auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};for(std::size_t i=0;i<out.size();++i)out[i]=std::byte((digit(hex[i*2])<<4)|digit(hex[i*2+1]));return out;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;out.reserve(b.size()*2);for(auto v:b){auto n=std::to_integer<unsigned>(v);out+=d[n>>4];out+=d[n&15];}return out;}
SourcePhysicsWorldRuntimeAbi import(const Json&j){SourcePhysicsWorldRuntimeAbi a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();a.eflags=j.at("eflags");a.x87_status=j.at("x87_status");a.x87_depth=j.at("x87_depth");a.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control"),j.at("mxcsr")};check(a.x87_depth<=8,"Sparse original x87 input unsupported");for(unsigned i=0;i<a.x87_depth;++i){auto raw=bytes(j.at("x87_logical_hex")[i]);check(raw.size()==10,"Original x87 extent differs");std::memcpy(&a.x87[i],raw.data(),10);}return a;}
std::vector<std::uint32_t> instructions;
void save(const std::filesystem::path&p,std::uint32_t pc,const SourcePhysicsWorldRuntimeAbi&a,const SourceRuntimeMemory&m){
 Json raw=Json::array(),regions=Json::array();for(unsigned i=0;i<8;++i)raw.push_back(hex({reinterpret_cast<const std::byte*>(&a.x87[i]),10}));
 m.visit_committed([&](auto address,auto b){regions.push_back({{"address",address},{"size",b.size()},{"writable",!m.is_read_only(address,b.size())},{"hex",hex(b)}});});
 Json j{{"instructions",instructions},{"pc",pc},{"abi",{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr},{"x87_tag",a.x87_depth==8?0:65535},{"x87_logical_hex",raw}}},{"regions",regions}};
 std::ofstream f(p);f<<j.dump()<<'\n';check(bool(f),"Cannot retain native DSP prefix state");
}
struct DspBoundary {};
}
int main(int argc,char**argv)try{
 check(argc==5,"Supply XBE ORIGINAL_SECTION_REPORT ORIGINAL_ENV_CAPTURE OUTPUT_DIRECTORY");if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
 auto original=load(argv[2]);check(original.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong DSP reference version");
 const auto va=original.at("section_va").get<std::uint32_t>(),size=original.at("section_bytes").get<unsigned>();
 auto image=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);SourceRuntimeDspSection section(image,argv[1]);
 {
  SourceRuntimeDspSection deferred(image,argv[1],SourceRuntimeDspSection::LoadPolicy::on_demand);
  check(deferred.address()==0&&deferred.resident_references()==0,"Deferred DSP adapter loaded an unrequested reference image");deferred.require_quiescent();
  try{SourceRuntimeDspSection::CallScope scope(&deferred);check(deferred.retain()==va,"Deferred actual DSP resource has a different address");check(std::ranges::equal(deferred.retained_bytes(),image.view(va,size)),"Deferred actual DSP bytes differ");throw DspBoundary{};}catch(const DspBoundary&){}
  check(deferred.active_references()==0&&deferred.resident_references()==1,"Aborted first deferred request leaked ownership");deferred.require_quiescent();
  check(deferred.retain()==va&&deferred.resident_references()==2,"Deferred DSP adapter lost normal subsequent retention");deferred.release();
 }
 check(section.address()==va&&section.resident_references()==1&&section.active_references()==0,"Native preloaded DSP ownership is not real/quiescent");
 const auto &load_row=original.at("rows")[0];const auto &free_row=original.at("rows")[1];check(load_row.at("before").at("references")==1&&load_row.at("after").at("references")==2&&free_row.at("after").at("references")==1,"Original paired DSP reference protocol changed");
 check(section.retain()==load_row.at("result")&&section.resident_references()==2&&std::ranges::equal(section.retained_bytes(),image.view(va,size)),"Actual native DSP retain differs from original resource/result");
 bool rejected=false;try{section.require_quiescent();}catch(const std::exception&){rejected=true;}check(rejected,"Outstanding native DSP request was exported as quiescent");
 try{SourceRuntimeDspSection::CallScope scope(&section);section.retain();check(section.resident_references()==3,"Nested native DSP retain is absent");throw DspBoundary{};}catch(const DspBoundary&){}
 check(section.active_references()==1&&section.resident_references()==2,"Failed nested native call lost existing DSP ownership");section.release();section.require_quiescent();
 rejected=false;try{section.release();}catch(const std::exception&){rejected=true;}check(rejected&&section.resident_references()==1,"Unmatched unload silently removed baseline DSP ownership");
 auto saved=image.checkpoint();image.restore(saved);check(section.retain()==va,"Native DSP owner did not rebind immutable restored source bytes");section.release();
 // A real native producer now replaces the earlier test-fixture XLoadSection
 // result. The required DSP download remains unavailable at its real entry.
 const auto before=load(std::filesystem::path(argv[3])/"environment/before.json");SourceRuntimeMemory bank;
 for(const auto&r:before.at("regions"))bank.mount(r.at("address"),bytes(r.at("hex")),r.at("writable").get<bool>()?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"genuine original sound caller ABI");
 bank.mount(va,image.view(va,size),SourceRuntimeMemory::Access::ReadOnly,"actual owned preloaded TU1.5 DSPImage input");SourceRuntimeDspSection live(bank,argv[1]);auto abi=import(before.at("abi"));
 const std::filesystem::path out(argv[4]);std::filesystem::create_directories(out);save(out/"before.json",0x1972c0u,abi,bank);
 SourcePhysicsWorldRuntimeServices services;services.instruction=[](auto pc){instructions.push_back(pc);};unsigned retains{},downloads{};services.required_owner=[&](auto&m,const auto&call,auto&a,const auto&observe){
  if(call.target==0x166a80u){source_run_physics_live_support_tu15(m,call.target,a,*a.floating_point,services,observe);return;}
  if(call.target==0x2df851u){save(out/"retain-before.json",call.target,a,bank);check(live.dispatch(m,call,a),"Real native DSP load request was not dispatched");++retains;check(a.gp[0]==va&&live.active_references()==1,"Native DSP load returned before actual resource retention");instructions.clear();save(out/"retain-after.json",call.instruction+5u,a,bank);return;}
  check(call.target==0x38594du,"Native DSP constructor reached an unexpected dependency");++downloads;save(out/"download-before.json",call.target,a,bank);check(live.resident_references()==2&&live.retained_bytes().size()==size,"Actual DSP data was not pinned for its consumer");throw DspBoundary{};
 };
 rejected=false;try{SourceRuntimeDspSection::CallScope scope(&live);source_run_sound_startup_tu15(bank,0x1972c0u,abi,*abi.floating_point,services);}catch(const DspBoundary&){rejected=true;}
 check(rejected&&retains==1&&downloads==1&&live.active_references()==0&&live.resident_references()==1,"Missing DSP provider invented a result or leaked native residency");
 Json report{{"original_sha256",original.at("original_sha256")},{"original_section_report",std::filesystem::absolute(argv[2]).string()},{"native_resource_retains",retains},{"unavailable_dsp_requests",downloads},{"fixture_callee_results_used",false},{"native_device_or_pcm_verified",false},{"native_instruction_trace_captured",true},{"resident_references_after_abort",live.resident_references()}};
 std::ofstream f(out/"report.json");f<<report.dump(2)<<'\n';check(bool(f),"Cannot retain native DSP provider evidence");
 std::cout<<"PASS actual native DSP section ownership: original1->2->1 retention, immutable bytes, nesting/abort cleanup, restored binding, quiescence/unmatched-unload guards; original1972C0 reaches real38594D after native resource retention; no fixture result, device/DSP success or PCM claim\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
