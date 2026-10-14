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
struct ModeBoundary {};
std::uint32_t word(const SourceRuntimeMemory&m,std::uint32_t p){auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
}
int main(int argc,char**argv)try{
 check(argc==5,"Supply ORIGINAL_SDK_CAPTURE ORIGINAL_AUDIO_CAPTURE ACTUAL_EEPROM OUTPUT_DIRECTORY");const std::filesystem::path sdk(argv[1]),original(argv[2]),out(argv[4]);if(!std::filesystem::is_regular_file(original/"report.json")||!std::filesystem::is_regular_file(argv[3]))return 77;
 const auto reference=load(original/"report.json"),before=load(sdk/"before.json");const auto actual=SourceTu15UserSettings::load(argv[3]);check(actual.checksum_valid()&&actual.audio_flags()==reference.at("query_audio_flags"),"Actual EEPROM AudioFlags/checksum differ from the original query");
 check(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original audio-query version");for(const auto*key:{"original_code_patches","original_memory_writes","original_register_writes","supplied_callee_returns"})check(reference.at(key).empty(),"Original audio query was supplied a result");
 std::filesystem::create_directories(out);Json results=Json::array();
 for(const auto valid:{true,false}){
  auto data=std::array<std::byte,96>{};std::copy(actual.bytes().begin(),actual.bytes().end(),data.begin());if(!valid)data[0]^=std::byte{1};SourceTu15UserSettings settings(data,valid?"actual owned EEPROM UserSettings":"declared checksum-damaged actual UserSettings negative input");check(settings.checksum_valid()==valid,"Negative EEPROM input did not fail its actual checksum");
  SourceRuntimeMemory bank;for(const auto&r:before.at("regions"))bank.mount(r.at("address"),bytes(r.at("hex")),r.at("writable").get<bool>()?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"actual original SDK caller input");auto a=import(before.at("abi"));SourceRuntimeSoundSdkResources resources(bank,a.fs_base);resources.bind_user_settings(settings);
  const auto directory=out/(valid?"valid":"invalid");std::filesystem::create_directories(directory);instructions.clear();save(directory/"before.json",0x387877u,a,bank);Json calls=Json::array(),query_writes=Json::array();unsigned queries{},modes{};std::uint32_t query_status{},query_value{};
  SourcePhysicsWorldRuntimeServices services;services.instruction=[](auto pc){instructions.push_back(pc);};services.required_owner=[&](auto&m,const auto&c,auto&abi,const auto&){
   if(c.instruction==0x37dabau&&c.target==0x33c29du){
    ++modes;save(directory/"mode-before.json",c.target,abi,bank);check(word(bank,abi.gp[4])==0x37dabfu&&word(bank,abi.gp[5]-4u)==(valid?settings.audio_flags():0u),"Original audio helper did not preserve actual setting/failure fallback");
    check(std::count(instructions.begin(),instructions.end(),0x37dab6u)==(valid?0:1),"Zero fallback was chosen outside its original negative branch");const auto driver=word(bank,0x52d674u);check(word(bank,driver+0x2ab0u)==0&&std::to_integer<unsigned>(bank.view(driver,1)[0])==0,"Missing mode owner published a source device/ready flag");throw ModeBoundary{};
   }
   const auto id=calls.size()+1;const auto prefix="call-"+std::to_string(id);save(directory/(prefix+"-before.json"),c.target,abi,bank);const bool query=c.instruction==0x2e101cu;const auto return_pc=query?word(bank,abi.gp[4]):c.instruction+6u;const auto original_type=word(bank,abi.gp[5]-8u),original_value=word(bank,abi.gp[5]-4u);
   SourcePhysicsContactRecordObserver observe=[&](const auto&store){query_writes.push_back({{"instruction",store.instruction},{"address",store.address},{"before_hex",hex(store.before)},{"after_hex",hex(store.written)}});};
   check(resources.dispatch(m,c,abi,observe),"SDK audio prefix reached an unexpected native owner");
   if(query){++queries;query_status=abi.gp[0];query_value=word(bank,abi.gp[5]-4u);check(query_status==(valid?0u:0xc000009cu),"Audio query did not produce actual checksum status");check(valid?(word(bank,abi.gp[5]-8u)==4&&query_value==actual.audio_flags()):(word(bank,abi.gp[5]-8u)==original_type&&query_value==original_value),"Audio query changed invalid outputs or lost actual data");}
   calls.push_back({{"id",id},{"kind",query?"audio_query":c.instruction==0x383f92u?"enter":c.instruction==0x387975u?"allocate":"capacity"},{"instruction",c.instruction},{"target",c.target},{"result",abi.gp[0]}});instructions.clear();save(directory/(prefix+"-after.json"),return_pc,abi,bank);
  };
  bool stopped=false;try{SourceRuntimeSoundSdkResources::CallScope scope(&resources);source_run_sound_sdk_tu15(bank,0x387877u,a,*a.floating_point,services);}catch(const ModeBoundary&){stopped=true;}
  check(stopped&&queries==1&&modes==1&&calls.size()==6&&resources.live_allocations()==0&&resources.lock_depth()==0&&!resources.quiescent(),"SDK audio query invented mode/driver success or leaked resources");check(query_writes.size()==(valid?9:0),"Null-length/checksum audio query output-write contract differs");
  bool rejected=false;try{resources.bind_user_settings(actual);}catch(const std::exception&){rejected=true;}check(rejected,"Partial source factory bypassed settings-resource binding barrier");
  Json report{{"original_sha256",reference.at("original_sha256")},{"checksum_valid",valid},{"actual_audio_flags",actual.audio_flags()},{"query_status",query_status},{"query_value_before_source_fallback",query_value},{"query_writes",query_writes},{"native_platform_calls",calls},{"native_audio_queries",queries},{"unavailable_boot_mode_requests",modes},{"native_allocations_after_abort",resources.live_allocations()},{"native_locks_after_abort",resources.lock_depth()},{"partial_source_state_faulted",true},{"fixture_callee_results_used",false},{"boot_mode_value_supplied",false},{"source_device_or_ready_supplied",false},{"native_pcm_verified",false},{"native_instruction_trace_captured",true}};
  std::ofstream f(directory/"report.json");f<<report.dump(2)<<'\n';check(bool(f),"Cannot retain native audio-query prefix");results.push_back({{"directory",valid?"valid":"invalid"},{"query_status",query_status},{"query_writes",query_writes.size()}});
 }
 Json report{{"original_sha256",reference.at("original_sha256")},{"original_audio_report",std::filesystem::absolute(original/"report.json").string()},{"actual_eeprom_resource",std::filesystem::absolute(argv[3]).string()},{"non_key_user_section_only",true},{"audio_field_offset",56},{"cases",results},{"reference_boot_mode_used_as_native_input",false}};std::ofstream f(out/"report.json");f<<report.dump(2)<<'\n';check(bool(f),"Cannot retain audio-query report");std::cout<<"PASS actual owned audio settings query, checksum failure/original fallback, missing boot-mode stop and cleanup"<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
