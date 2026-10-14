#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
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
SourcePhysicsWorldRuntimeAbi import(const Json&j){SourcePhysicsWorldRuntimeAbi a{};a.fs_base=j.at("fs_base");a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();a.eflags=j.at("eflags");a.x87_status=j.at("x87_status");a.x87_depth=j.at("x87_depth");a.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control"),j.at("mxcsr")};check(a.x87_depth<=8,"Sparse original x87 input unsupported");for(unsigned i=0;i<a.x87_depth;++i){auto raw=bytes(j.at("x87_logical_hex")[i]);check(raw.size()==10,"Original x87 extent differs");std::memcpy(&a.x87[i],raw.data(),10);}return a;}
std::vector<std::uint32_t> instructions;
void save(const std::filesystem::path&p,std::uint32_t pc,const SourcePhysicsWorldRuntimeAbi&a,const SourceRuntimeMemory&m){
 Json raw=Json::array(),regions=Json::array();for(unsigned i=0;i<8;++i)raw.push_back(hex({reinterpret_cast<const std::byte*>(&a.x87[i]),10}));
 m.visit_committed([&](auto address,auto b){regions.push_back({{"address",address},{"size",b.size()},{"writable",!m.is_read_only(address,b.size())},{"hex",hex(b)}});});
 Json j{{"instructions",instructions},{"pc",pc},{"abi",{{"gp",a.gp},{"fs_base",a.fs_base},{"xmm",a.xmm},{"eflags",a.eflags},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr},{"x87_tag",a.x87_depth==8?0:65535},{"x87_logical_hex",raw}}},{"regions",regions}};
 std::ofstream f(p);f<<j.dump()<<'\n';check(bool(f),"Cannot retain native DSP prefix state");
}
struct SettingBoundary {};
std::uint32_t word(const SourceRuntimeMemory&m,std::uint32_t p){auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
}
int main(int argc,char**argv)try{
 check(argc==3,"Supply ORIGINAL_SDK_CAPTURE OUTPUT_DIRECTORY");const std::filesystem::path original(argv[1]),out(argv[2]);if(!std::filesystem::is_regular_file(original/"report.json"))return 77;
 const auto reference=load(original/"report.json"),before=load(original/"before.json");
 check(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&reference.at("original_cold_boot").get<bool>(),"Wrong SDK reference version");
 for(const auto*key:{"original_code_patches","original_memory_writes","original_register_writes","supplied_callee_returns"})check(reference.at(key).empty(),"SDK reference was supplied a result");
 check(reference.at("thread_context").at("actual_irql_byte")=="00","Original factory does not have the reached passive context");SourceRuntimeMemory bank;
 for(const auto&r:before.at("regions"))bank.mount(r.at("address"),bytes(r.at("hex")),r.at("writable").get<bool>()?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"genuine original SDK caller and current source state");
 auto a=import(before.at("abi"));check(a.fs_base==reference.at("thread_context").at("actual_fs_base"),"Original FS base was not retained");SourceRuntimeSoundSdkResources resources(bank,a.fs_base);resources.require_quiescent();
 std::filesystem::create_directories(out);save(out/"before.json",0x387877u,a,bank);Json calls=Json::array();unsigned settings{},allocations{},capacities{},enters{};std::uint32_t total_capacity{};
 SourcePhysicsWorldRuntimeServices services;services.instruction=[](auto pc){instructions.push_back(pc);};services.required_owner=[&](auto&m,const auto&c,auto&abi,const auto&){
  if(c.instruction==0x2e101cu){
   check(c.tail&&c.target==word(bank,0x41b6f0u)&&word(bank,abi.gp[4])==0x37dab2u,"Saved-setting kernel request ABI differs");const auto sp=abi.gp[4];check(word(bank,sp+4)==9&&word(bank,sp+16)==4&&word(bank,sp+20)==0,"Saved-setting query arguments differ");++settings;save(out/"setting-before.json",c.target,abi,bank);
   const auto driver=word(bank,0x52d674u),device=word(bank,0x391278u);check(device&&word(bank,device)==0x4211c0u&&word(bank,device+4u)==2&&word(bank,device+0x10u)==device+0x10u,"Original SDK device constructor did not own its actual raw object");
   check(resources.live_allocations()==2&&resources.lock_depth()==1&&word(bank,0x390bc0u)==total_capacity,"Original SDK accounting did not consume actual native capacities");
   check(word(bank,driver+0x2ab0u)==0&&std::to_integer<unsigned>(bank.view(driver,1)[0])==0&&word(bank,0x390bd0u)==0&&word(bank,0x390bd4u)==0&&word(bank,0x390bd8u)==0,"Unfinished SDK constructor supplied a device/ready/capability result");throw SettingBoundary{};
  }
  const auto id=calls.size()+1;const auto prefix="call-"+std::to_string(id);save(out/(prefix+"-before.json"),c.target,abi,bank);check(resources.dispatch(m,c,abi),"SDK prefix reached an unexpected native owner");
  const auto kind=c.instruction==0x383f92u?"enter":c.instruction==0x387975u?"allocate":c.instruction==0x387982u?"capacity":"other";
  check(std::string(kind)!="other","SDK prefix platform order changed");if(std::string(kind)=="enter")++enters;if(std::string(kind)=="allocate")++allocations;if(std::string(kind)=="capacity"){++capacities;total_capacity+=abi.gp[0];}
  calls.push_back({{"id",id},{"kind",kind},{"instruction",c.instruction},{"target",c.target},{"result",abi.gp[0]},{"live_allocations",resources.live_allocations()},{"lock_depth",resources.lock_depth()}});instructions.clear();save(out/(prefix+"-after.json"),c.instruction+6u,abi,bank);
 };
 bool stopped=false;try{SourceRuntimeSoundSdkResources::CallScope scope(&resources);source_run_sound_sdk_tu15(bank,0x387877u,a,*a.floating_point,services);}catch(const SettingBoundary&){stopped=true;}
 check(stopped&&settings==1&&allocations==capacities&&allocations==2&&enters==1&&calls.size()==5,"Original factory prefix did not reach its actual setting dependency");check(resources.live_allocations()==0&&resources.lock_depth()==0&&!resources.quiescent(),"Failed factory leaked resources or exported dangling partial source state");bool rejected=false;try{resources.require_quiescent();}catch(const std::exception&){rejected=true;}check(rejected,"Partial SDK factory became an admissible checkpoint");
 Json report{{"original_sha256",reference.at("original_sha256")},{"original_report",std::filesystem::absolute(original/"report.json").string()},{"original_fs_base",a.fs_base},{"native_passive_context",true},{"native_platform_calls",calls},{"native_allocations",allocations},{"native_capacity_queries",capacities},{"actual_native_total_capacity",total_capacity},{"native_lock_acquisitions",enters},{"unavailable_setting_queries",settings},{"native_allocations_after_abort",resources.live_allocations()},{"native_locks_after_abort",resources.lock_depth()},{"partial_source_state_faulted",true},{"fixture_callee_results_used",false},{"source_device_output_published",false},{"source_driver_initialized",false},{"native_device_dsp_voices_or_pcm_verified",false},{"native_instruction_trace_captured",true}};
 std::ofstream f(out/"report.json");f<<report.dump(2)<<'\n';check(bool(f),"Cannot retain native SDK prefix evidence");std::cout<<"PASS original SDK factory with real native storage/lock, original accounting and mandatory saved-setting stop; allocated="<<total_capacity<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
