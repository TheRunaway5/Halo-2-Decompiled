#include "halo2/source_runtime_sound_output.hpp"
#include "halo2/source_sound_startup_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <SDL.h>
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
struct CapabilitiesBoundary {};
std::uint32_t word(const SourceRuntimeMemory&m,std::uint32_t p){auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
void mount(SourceRuntimeMemory&m,const Json&s){for(const auto&r:s.at("regions"))m.mount(r.at("address"),bytes(r.at("hex")),r.at("writable").get<bool>()?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"genuine original driver input");}
void check_abi(const SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsWorldRuntimeAbi&b){check(a.gp==b.gp&&a.xmm==b.xmm&&a.eflags==b.eflags&&a.x87_status==b.x87_status&&a.x87_depth==b.x87_depth&&a.floating_point->x87_control_word==b.floating_point->x87_control_word&&a.floating_point->mxcsr==b.floating_point->mxcsr,"Failed native service changed caller ABI");for(unsigned i=0;i<a.x87_depth;++i)check(std::memcmp(&a.x87[i],&b.x87[i],10)==0,"Failed native service changed x87 values");}
}
int main(int argc,char**argv)try{
 check(argc==3,"Supply ORIGINAL_DEVICE_CAPTURE OUTPUT_DIRECTORY");const std::filesystem::path original(argv[1]),out(argv[2]);if(!std::filesystem::is_regular_file(original/"report.json"))return 77;
 const auto evidence=load(original/"report.json"),before=load(original/"before.json"),create=load(original/"create-before.json");
 check(evidence.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&evidence.at("original_cold_boot").get<bool>(),"Wrong driver reference version");
 for(const auto*key:{"original_code_patches","original_memory_writes","original_register_writes","supplied_callee_returns"})check(evidence.at(key).empty(),"Original driver reference was supplied a result");
 SourceRuntimeMemory bank;mount(bank,before);SourceRuntimeSoundOutput output(bank,{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,512,{}});output.require_quiescent();auto a=import(before.at("abi"));
 std::filesystem::create_directories(out);save(out/"before.json",0x224ec0u,a,bank);unsigned creations{},caps{};std::uint32_t opened{};std::string backend;
 SourcePhysicsWorldRuntimeServices services;services.instruction=[](auto pc){instructions.push_back(pc);};services.required_owner=[&](auto&m,const auto&call,auto&abi,const auto&observe){
  if(call.target==0x166a80u){source_run_physics_live_support_tu15(m,call.target,abi,*abi.floating_point,services,observe);return;}
  if(call.target==0x387877u){save(out/"create-before.json",call.target,abi,bank);check(output.dispatch(m,call,abi),"Actual driver creation was not dispatched");++creations;opened=output.device_id();check(opened&&word(bank,word(bank,0x52d674u)+0x2ab0u)==opened,"Published output lacks real native ownership");
   const auto*driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","Device proof requires a real host audio backend");backend=driver;check(SDL_GetAudioDeviceStatus(opened)==SDL_AUDIO_PLAYING,"Real device callback was not started");instructions.clear();save(out/"create-after.json",call.instruction+5u,abi,bank);return;}
  check(call.target==0x38568eu,"Driver reached an unexpected unresolved dependency");++caps;save(out/"capabilities-before.json",call.target,abi,bank);
  const auto sp=abi.gp[4],driver=word(bank,0x52d674u);check(word(bank,sp+4)==opened&&word(bank,sp+8)==driver+0x2aa0u&&std::to_integer<unsigned>(bank.view(driver,1)[0])==0,"Source capabilities call did not receive the actual native identity");
  bool rejected=false;try{output.require_quiescent();}catch(const std::exception&){rejected=true;}check(rejected,"Live native output escaped checkpoint guards");
  throw CapabilitiesBoundary{};
 };
 bool rejected=false;try{SourceRuntimeSoundOutput::CallScope scope(&output);source_run_sound_startup_tu15(bank,0x224ec0u,a,*a.floating_point,services);}catch(const CapabilitiesBoundary&){rejected=true;}
 const auto driver=word(bank,0x52d674u);check(rejected&&creations==1&&caps==1&&output.quiescent()&&word(bank,driver+0x2ab0u)==0&&std::to_integer<unsigned>(bank.view(driver,1)[0])==0,"Missing capabilities invented readiness or leaked native output");check(SDL_GetAudioDeviceStatus(opened)==SDL_AUDIO_STOPPED,"Aborted device was not closed");
 // Existing resources survive a nested failing call. Duplicate singleton/
 // AddRef requests and unavailable capabilities do not receive fake returns.
 SourceRuntimeMemory nested;mount(nested,create);SourceRuntimeSoundOutput retained(nested,{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});auto n=import(create.at("abi"));const SourcePhysicsContactRuntimeCall call{0x224f26u,0x387877u,false};check(retained.dispatch(nested,call,n),"Real nested device creation failed");const auto existing=retained.device_id();
 try{SourceRuntimeSoundOutput::CallScope scope(&retained);throw CapabilitiesBoundary{};}catch(const CapabilitiesBoundary&){}
 check(retained.device_id()==existing&&SDL_GetAudioDeviceStatus(existing)==SDL_AUDIO_PAUSED,"Nested failure removed an existing output");auto duplicate=import(create.at("abi"));rejected=false;try{retained.dispatch(nested,call,duplicate);}catch(const std::exception&){rejected=true;}check(rejected&&retained.device_id()==existing,"Unrecovered duplicate request invented AddRef");retained.close();retained.require_quiescent();
 // Actual SDL failure must leave the source result/output and ABI untouched.
 SourceRuntimeMemory failed;mount(failed,create);SourceRuntimeSoundOutput missing(failed,{SdlSoundPlatformConfiguration::Delivery::Manual,64,std::string("halo2-nonexistent-output-device-v9")});auto f=import(create.at("abi"));const auto initial=f;rejected=false;try{missing.dispatch(failed,call,f);}catch(const std::exception&){rejected=true;}check(rejected&&missing.quiescent()&&word(failed,driver+0x2ab0u)==0,"Unavailable output was admitted");check_abi(f,initial);
 // If the binding is externally changed, cleanup still closes the device
 // and permanently rejects quiescence rather than overwriting another owner.
 SourceRuntimeMemory damaged;mount(damaged,create);SourceRuntimeSoundOutput faulted(damaged,{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});auto d=import(create.at("abi"));std::uint32_t damaged_id{};
 try{SourceRuntimeSoundOutput::CallScope scope(&faulted);check(faulted.dispatch(damaged,call,d),"Cleanup-fault device creation failed");damaged_id=faulted.device_id();auto slot=damaged.mutable_view(driver+0x2ab0u,4);std::fill(slot.begin(),slot.end(),std::byte{0});throw CapabilitiesBoundary{};}catch(const CapabilitiesBoundary&){}
 check(!faulted.device_id()&&!faulted.quiescent()&&SDL_GetAudioDeviceStatus(damaged_id)==SDL_AUDIO_STOPPED&&word(damaged,driver+0x2ab0u)==0,"Lost cleanup binding was silently accepted or overwrote memory");rejected=false;try{faulted.require_quiescent();}catch(const std::exception&){rejected=true;}check(rejected,"Faulted output was checkpointed as quiescent");
 Json report{{"original_sha256",evidence.at("original_sha256")},{"original_report",std::filesystem::absolute(original/"report.json").string()},{"native_output_creations",creations},{"actual_sdl_device_id",opened},{"native_backend",backend},{"native_callback_observed_playing",true},{"unavailable_capability_requests",caps},{"device_closed_after_abort",true},{"output_withdrawn_after_abort",true},{"fixture_callee_results_used",false},{"native_dsp_voices_or_pcm_verified",false},{"source_driver_initialized",false},{"native_instruction_trace_captured",true}};
 std::ofstream log(out/"report.json");log<<report.dump(2)<<'\n';check(bool(log),"Cannot retain native output evidence");std::cout<<"PASS actual native output creation/callback, required capabilities stop, rollback and ownership guards; backend="<<backend<<" device="<<opened<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
