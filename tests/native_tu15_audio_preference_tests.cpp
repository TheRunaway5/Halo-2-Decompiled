#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;using Json=nlohmann::json;
namespace {
constexpr std::uint32_t data=0x2001000,stack=0x2008000,stop=0x2100000;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> raw{};for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));memory.write(p,raw);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p){std::uint32_t value=0;auto raw=memory.view(p,4);for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;}
std::vector<std::byte> unhex(const std::string& text){auto nibble=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};std::vector<std::byte> raw(text.size()/2);for(std::size_t i=0;i<raw.size();++i)raw[i]=std::byte((nibble(text[i*2])<<4)|nibble(text[i*2+1]));return raw;}
template<class F>void reject(F action){bool refused=false;try{action();}catch(const std::exception&){refused=true;}check(refused,"Unsupported native audio preference was accepted");}
void mount(SourceRuntimeMemory& memory,const Json& fixture){
 memory.mount(0x2000000,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"declared original constructor destination/caller");
 memory.mount(0x390be4,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"original SDK configuration output and explicit override input");
 for(const auto& region:fixture.at("readonly_regions"))memory.mount(region.at("address"),unhex(region.at("hex")),SourceRuntimeMemory::Access::ReadOnly,"original immutable listener/environment constructor operands");
 put(memory,0x390be8,0xffffffffu);
}
void reference_case(const Json& fixture,const Json& expected){
 SourceRuntimeMemory memory;mount(memory,fixture);std::ranges::fill(memory.mutable_view(data,0xb4),std::byte{0xa5});put(memory,stack,stop);put(memory,0x390be8,expected.at("explicit_flags"));
 SourcePhysicsWorldRuntimeAbi abi{};for(unsigned i=0;i<8;++i)abi.gp[i]=0x11000000+i*0x111111;abi.gp[1]=data;abi.gp[4]=stack;abi.eflags=0x202;
 Json stores=Json::array();std::size_t instructions=0;SourcePhysicsWorldRuntimeServices services;
 services.instruction=[&](auto pc){check(pc!=0x37da9a&&pc!=0x33c29d&&pc!=0x2e101c,"Genuine explicit preference path required console settings/hardware");++instructions;};
 services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Explicit source preference requires another owner");};
 SourcePhysicsContactRecordObserver observer=[&](const auto& store){std::uint64_t value=0;for(std::size_t i=0;i<store.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(store.written[i]))<<(i*8);stores.push_back({store.instruction,store.address,store.written.size(),value});};
 source_run_sound_sdk_tu15(memory,0x386d4a,abi,{0x27f,0x1f80},services,observer);
 check(abi.gp==expected.at("returned_gp").get<decltype(abi.gp)>()&&(abi.eflags&0xcc5)==(expected.at("returned_eflags").get<std::uint32_t>()&0xcc5),"Original explicit preference native CPU ABI differs");
 check(instructions==expected.at("instructions")&&stores==expected.at("source_stores"),"Original explicit preference native CPU trace differs");
 check(std::ranges::equal(memory.view(data,0xb4),unhex(expected.at("data_hex")))&&std::ranges::equal(memory.view(0x390be4,8),unhex(expected.at("configuration_hex"))),"Original explicit preference constructor bytes differ");
}
}
int main(int argc,char** argv){try{
 check(argc==4,"Expected original fixture, private config directory and report path");std::ifstream input(argv[1]);Json fixture;input>>fixture;check(fixture.at("passed")==true,"Original explicit preference fixture failed");
 for(const auto& expected:fixture.at("cases"))reference_case(fixture,expected);
 const std::filesystem::path directory(argv[2]);std::filesystem::create_directories(directory);const auto config=directory/"menu.cfg";
 {std::ofstream file(config);file<<"pc_speakers 0\n";}const auto saved=native_tu15_load_audio_preference(config);check(saved.speakers==0&&!saved.application_default,"Actual native speaker preference was not read");
 SourceRuntimeMemory memory;mount(memory,fixture);auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
 NativeTu15AudioPreference policy(memory,output,saved);check(policy.flags()==0&&policy.output_device()&&output->output_channels()==2,"Native preference does not own its actual uncompressed stereo route");
 reject([&]{policy.require_quiescent();});policy.publish();check(word(memory,0x390be8)==0&&word(memory,0x390be4)==0,"Native preference did not publish only its original SDK input");reject([&]{policy.publish();});
 // Consume that actual resource-backed input through the genuine SDK body.
 std::ranges::fill(memory.mutable_view(data,0xb4),std::byte{0xa5});put(memory,stack,stop);
 SourcePhysicsWorldRuntimeAbi actual{};actual.gp[1]=data;actual.gp[4]=stack;actual.eflags=0x202;
 std::size_t original_constructor_instructions=0;SourcePhysicsWorldRuntimeServices actual_services;
 actual_services.instruction=[&](auto pc){check(pc!=0x37da9a&&pc!=0x33c29d&&pc!=0x2e101c,"Actual native policy entered console-only fallback");++original_constructor_instructions;};
 actual_services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Actual resource-backed explicit SDK constructor has an unowned dependency");};
 source_run_sound_sdk_tu15(memory,0x386d4a,actual,{0x27f,0x1f80},actual_services);
 check(actual.gp[0]==data&&actual.gp[4]==stack+4&&word(memory,data+4)==1&&word(memory,data)==0x42124c&&word(memory,0x390be4)==policy.flags(),"Actual native preference did not feed the genuine SDK constructor");
 bool wrong_thread=false,foreign_diagnostics=false;std::thread outsider([&]{
  foreign_diagnostics=!policy.quiescent()&&!policy.published();
  try{policy.flags();}catch(const std::exception&){wrong_thread=true;}});outsider.join();
 check(wrong_thread&&foreign_diagnostics,"Native preference lost executor ownership or read foreign diagnostic state");
 auto mismatched=saved;mismatched.speakers=1;reject([&]{NativeTu15AudioPreference other(memory,output,mismatched);});
 policy.withdraw();check(word(memory,0x390be8)==0xffffffffu,"Native preference did not withdraw its owned input");
 for(const auto selected:{2u,3u}){{std::ofstream file(config);file<<"pc_speakers "<<selected<<'\n';}reject([&]{NativeTu15AudioPreference unsupported(memory,output,native_tu15_load_audio_preference(config));});check(word(memory,0x390be8)==0xffffffffu,"Unsupported routes published an SDK input");}
 policy.retire();policy.require_quiescent();reject([&]{policy.flags();});
 for(const auto selected:{0u,1u,4u}){{std::ofstream file(config);file<<"pc_speakers "<<selected<<'\n';}NativeTu15AudioPreference accepted(memory,output,native_tu15_load_audio_preference(config));check(accepted.flags()==0,"Actual two-channel native preference did not retain source stereo semantics");accepted.publish();accepted.retire();accepted.require_quiescent();}
 const auto absent=directory/"uncreated-menu.cfg";check(!std::filesystem::exists(absent),"Fresh native preference fixture already exists");
 const auto default_input=native_tu15_load_audio_preference(absent);check(default_input.speakers==DisplaySettings{}.speakers&&default_input.application_default,"Missing native menu configuration did not retain the actual application default");
 {NativeTu15AudioPreference fresh(memory,output,default_input);fresh.publish();fresh.retire();fresh.require_quiescent();}
 {NativeTu15AudioPreference removed(memory,output,default_input);removed.publish();check(output->device_removed(output->device_id()),"Actual native output removal was not delivered");reject([&]{removed.publish();});reject([&]{removed.flags();});removed.retire();removed.require_quiescent();check(word(memory,0x390be8)==0xffffffffu,"Removed output published a native SDK preference");}
 Json report={{"passed",true},{"original_reference_cases",fixture.at("cases").size()},{"original_instruction_sites",fixture.at("executed_instruction_bytes").size()},{"native_backend",SDL_GetCurrentAudioDriver()},{"native_device_id",output->device_id()},{"actual_output_channels",output->output_channels()},{"native_flags",0},{"console_eeprom_required",false},{"smc_av_pack_value_supplied",false},{"whole_factory_shortcut_used",false},{"unsupported_surround_preferences_rejected",true},{"accepted_native_layouts",Json::array({0,1,4})},{"fresh_native_application_default_verified",true},{"removed_output_preference_rejected",true},{"actual_resource_preference_consumed_by_original_constructor",true},{"original_constructor_instruction_events",original_constructor_instructions},{"source_sdk_factory_completed",false},{"source_sdk_capability_or_ready_results_supplied",false}};
 output.reset();std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save native preference evidence");std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
