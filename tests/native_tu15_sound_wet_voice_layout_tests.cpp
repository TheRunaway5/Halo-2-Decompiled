#include "halo2/native_tu15_sound_wet_return_sources.hpp"
#include "halo2/sound_sdk_voice_dispatch.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool v,const std::string& text){if(!v)throw std::runtime_error(text);}
std::vector<std::byte> unhex(const std::string& text){need(text.size()%2==0,"Odd original bytes");std::vector<std::byte> out;
 for(unsigned i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return out;}
SoundSdkVoiceInitializationData providers(const Json& p){
 SoundSdkVoiceInitializationData out{};out.source_defaults=p.at("source_defaults").get<decltype(out.source_defaults)>();
 out.environment_defaults=p.at("environment_defaults").get<decltype(out.environment_defaults)>();out.pcm_guid=p.at("pcm_guid").get<decltype(out.pcm_guid)>();out.adpcm_guid=p.at("adpcm_guid").get<decltype(out.adpcm_guid)>();
 out.default_format_identity=p.at("default_format_identity");out.spatial_bins_identity=p.at("spatial_bins_identity");out.parent_bins_identity=p.at("parent_bins_identity");out.channel_bins_table_identity=p.at("channel_bins_table_identity");
 out.unsigned_correction_bits=p.at("unsigned_correction_bits");out.frequency_reciprocal_bits=p.at("frequency_reciprocal_bits");
 for(const auto& row:p.at("immutable"))out.immutable.push_back({row.at("identity"),unhex(row.at("hex"))});return out;
}
struct Memory {
 std::map<std::uint32_t,std::vector<std::byte>> regions;
 std::span<std::byte> view(std::uint32_t address,std::size_t bytes){auto end=regions.upper_bound(address);need(end!=regions.begin(),"Unowned native source identity");auto at=std::prev(end);
  const auto offset=std::uint64_t(address)-at->first;need(offset+bytes<=at->second.size(),"Unowned native source extent");return std::span(at->second).subspan(offset,bytes);}
 void put(std::uint32_t address,unsigned value,unsigned bytes=4){auto raw=view(address,bytes);for(unsigned i=0;i<bytes;++i)raw[i]=std::byte(value>>(8*i));}
 std::uint32_t word(std::uint32_t address){auto raw=view(address,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;}
};
}
int main(int argc,char** argv)try{
 need(argc==3,"Supply original wet voice and wet sample fixtures");const auto fixture=Json::parse(std::ifstream(argv[1]));const auto wet=Json::parse(std::ifstream(argv[2]));
 need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("original_instruction_patches").empty() && fixture.at("supplied_allocator_device_or_playback_results").empty(),"Original authority/boundaries");
 need(fixture.at("original_default_format_hex")=="0100010080bb000000ee0200040018000000","Original default format differs");
 const auto constants=providers(fixture.at("providers"));const unsigned voice=fixture.at("voice"),data=fixture.at("data"),alternate=fixture.at("alternate"),pointer=fixture.at("source_pointer");
 unsigned cases{},calls{},store_count{},guards{},source_words{},refusals{};
 for(const auto& row:fixture.at("cases")){
  Memory memory;for(const auto& r:row.at("regions"))memory.regions.emplace(r.at("address"),unhex(r.at("hex")));
  for(const auto& [_,raw]:memory.regions)need(reinterpret_cast<std::uintptr_t>(raw.data())>0xffffffffu,"Actual native source storage below4GiB");
  const SoundSdkDeviceMemory access=[&](auto at,auto bytes){return memory.view(at,bytes);};std::recursive_mutex mutex;
  Json actual_guards=Json::array(),actual_stores=Json::array();
  const auto guard=[&](const char* kind,const auto& body){std::lock_guard lock(mutex);actual_guards.push_back({kind,"enter"});try{body();}catch(...){actual_guards.push_back({kind,"leave"});throw;}actual_guards.push_back({kind,"leave"});};
  SoundSdkVoiceInitializationServices initialization;initialization.memory=access;
  initialization.under_state_lock=[&](const auto& body){guard("state",body);};initialization.under_irql_guard=[&](const auto& body){guard("device",body);};
  initialization.allocate=[](auto,auto,auto)->std::uint32_t{throw std::runtime_error("Unexpected wet source allocation");};
  initialization.release_allocation=[](auto){throw std::runtime_error("Unexpected wet source free");};
  SoundSdkDeviceServices device;device.memory=access;device.under_device_lock=[&](const auto& body){guard("device",body);};
  device.fifo_available=[]()->std::uint32_t{throw std::runtime_error("Unexpected wet source FIFO/device readiness request");};
  device.device_write=[](auto,auto){throw std::runtime_error("Unexpected wet source device command");};
  SoundSdkVoiceAttachmentServices attachment;attachment.initialization=initialization;attachment.pages.device=device;
  const SoundDriverStartupObserver observer=[&](const SoundDriverStartupStore& store){actual_stores.push_back({store.instruction,store.address,store.bytes,store.before,store.after});};
  const SoundDriverFloatingPointContext fp{row.at("control"),row.at("mxcsr")};unsigned phase{};
  for(const auto& selected:row.at("calls")){
   actual_guards.clear();actual_stores.clear();const auto self=selected.at("self").get<unsigned>();const auto kind=selected.at("kind").get<std::string>();unsigned result{};
   if(kind=="initialize")result=source_sound_sdk_initialize_voice_data(self,selected.at("arguments")[0],constants,fp,initialization,observer);
   else if(kind=="format")result=source_sound_sdk_voice_format_apply(self,device,observer);
   else {need(kind=="attach","Unknown original phase");source_sound_sdk_attach_voice_stream(self,{pointer},constants,attachment,observer);}
   if(kind!="attach")need(result==selected.at("result"),"Original typed return differs");
   need(actual_stores==selected.at("stores"),row.at("name").get<std::string>()+" "+kind+": Original ordered stores differ");
   need(actual_guards==selected.at("guards"),"Original guard boundary order differs");store_count+=actual_stores.size();guards+=actual_guards.size();++calls;
   for(const auto& action:row.at("actions"))if(action.at("after_phase")==phase){const auto raw=unhex(action.at("hex"));std::copy(raw.begin(),raw.end(),memory.view(action.at("address"),raw.size()).begin());}++phase;
  }
  for(const auto& region:row.at("final_regions")){const auto expected=unhex(region.at("hex"));need(std::ranges::equal(memory.view(region.at("address"),expected.size()),expected),"Original complete retained source bytes differ");}
  const auto layout=source_sound_wet_voice_layout_tu15(voice,{pointer},access);const auto& expected=row.at("layout");
  need(layout.input_bin()==expected.at("input_bin") && layout.byte_offset()==expected.at("byte_offset") && layout.byte_extent()==expected.at("byte_extent") && layout.cursor_word()==expected.at("cursor_word") && layout.hardware_format()==expected.at("hardware_format"),"Original prepared wet layout differs");
  // Logical GP values come from an independently qualified original wet frame.
  // No packed DMA bytes or voice-to-device sample conversion is supplied.
  std::array<tu15_sound::Word24,tu15_gp_output::owned_words> zero{};auto output=tu15_gp_output::OutputOwner::cold_cursor_phase(zero);
  std::array<tu15_sound::Word24,1024> mix{};const auto& saved=wet.at("cases").back().at("frames")[11];
  for(unsigned i=0;i<8;++i)for(unsigned j=0;j<32;++j)mix[(15+i)*32+j]=tu15_sound::Word24(saved.at("reverb")[i<4?1:0].at("outputs")[(i%4)*32+j]);
  tu15_sound_wet_return::Sources sources(output->copy_values(mix));const auto view=sources.logical_view(layout);
  need(view.route.input_bin==layout.input_bin(),"Actual SDK layout lost its return source identity");
  for(unsigned j=0;j<32;++j){need(view.samples[j]==mix[layout.input_bin()*32+j],"Actual SDK layout selected the wrong native GP words");++source_words;}
  const auto source=memory.word(voice+0x80u);
  const std::vector<std::array<unsigned,3>> invalid{
   {data+0xcu,0x69,2},{data+0xeu,2,1},{data+0xfu,32,1},{data+0x10u,44100,4},{data+0x14u,2,4},
   {source+8u,0x100000,4},{source+0xd4u,14,4},{source+0xd4u,23,4},{source+0xc0u,127,4},
   {source+0xbcu,memory.word(source+0xbcu)+4u,4},{voice+0x12u,0x40,2},{voice+0x154u,0,4},
   {voice+0x14u,memory.word(voice+0x14u)|0x08000000u,4}};
  for(const auto& bad:invalid){auto span=memory.view(bad[0],bad[2]);std::vector<std::byte> before(span.begin(),span.end());memory.put(bad[0],bad[1],bad[2]);const auto snapshot=memory.regions;
   bool rejected=false;try{(void)source_sound_wet_voice_layout_tu15(voice,{pointer},access);}catch(const std::invalid_argument&){rejected=true;}
   need(rejected && memory.regions==snapshot,"Unsupported wet layout accepted or mutated storage");std::copy(before.begin(),before.end(),span.begin());++refusals;}
  ++cases;
 }
 need(cases==64 && calls==288 && source_words==2048 && refusals==832,"Original wet voice matrix incomplete");
 std::cout<<"PASS "<<cases<<" original TU1.5 wet voice profiles, "<<calls<<" complete CPU calls, "<<store_count<<" ordered stores, "<<guards<<" guard boundaries; "<<source_words
  <<" actual native GP source words, "<<refusals<<" rejected layouts; mono48000/24-in-four-byte SDK format and attachment cursor, no physical DMA/device/clock admission\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
