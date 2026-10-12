#include "halo2/vista_loading_audio.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool v,const std::string& message){if(!v)throw std::runtime_error(message);}
std::vector<std::uint8_t> read(const std::string& path){std::ifstream f(path,std::ios::binary);check(bool(f),"Cannot read "+path);return {std::istreambuf_iterator<char>(f),{}};}
Json words(const VistaLoadingAudioState& s){Json a=Json::array();for(auto v:s.cached_volumes)a.push_back(std::bit_cast<std::uint32_t>(v));for(auto v:s.buffers)a.push_back(v);for(auto v:{s.device,std::bit_cast<std::uint32_t>(s.attenuation),std::bit_cast<std::uint32_t>(s.maximum_volume),s.event,s.worker})a.push_back(v);return a;}
std::vector<std::uint8_t> hex(const std::string& s){std::vector<std::uint8_t> out;for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::stoul(s.substr(i,2),nullptr,16));return out;}
std::uint32_t little(const std::vector<std::uint8_t>& b,unsigned at,unsigned n){std::uint32_t result=0;for(unsigned i=0;i<n;++i)result|=std::uint32_t(b.at(at+i))<<(i*8);return result;}
VistaLoadingAudioState initial(){VistaLoadingAudioState s;for(unsigned i=0;i<4;++i){s.buffers[i]=25233408+i*16;s.cached_volumes[i]=-10000;}s.event=53249;return s;}
Json normalize(const Json& events){Json out=Json::array();unsigned created=0;for(auto e:events){const std::string name=e[0];if(name=="security_cookie_check")continue;
 if(name=="DirectSoundCreate8")e=Json::array({name});
 else if(name=="CreateSoundBuffer")e=Json::array({name,e[1],created++});
 else if(name=="buffer_description")continue;
 else if(name=="Lock")e=Json::array({name,e[1],e[2],e[3],e[8]});
 else if(name=="CreateEventA" || name=="CreateThread")e=Json::array({name});
 out.push_back(e);}return out;}
}
int main(int argc,char** argv){try {
 check(argc==4,"Supply execution.json, encoded bank, original PCM bank");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
 check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","Wrong original PE");
 const auto encoded=read(argv[2]),pcm=read(argv[3]);check(encoded.size()==18396 && pcm.size()==65408,"Wrong original bank sizes");check(vista_loading_audio_decode(encoded)==pcm,"Decoded PCM differs from unchanged original49A688");
 const VistaLoadingFloatingPoint context{0x027f,0x1f80};unsigned gains=0,envelopes=0,setups=0,workers=0,shutdowns=0;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 std::uint16_t before_cw;asm volatile("fnstcw %0":"=m"(before_cw));const auto before_mxcsr=_mm_getcsr();const auto before_exceptions=std::fetestexcept(FE_ALL_EXCEPT);
#endif
 for(const auto& row:fixture.at("gain_cases")){VistaLoadingAudioState s;vista_loading_audio_gain(s,row.at("gain_bits"),context);check(s.attenuation==row.at("attenuation"),"Gain case "+std::to_string(gains));++gains;}
 for(const auto& row:fixture.at("envelope_cases")){
  auto s=initial();s.attenuation=row.at("attenuation");const unsigned mask=row.value("active_mask",15u);for(unsigned i=0;i<4;++i)if(!(mask&(1u<<i)))s.buffers[i]=0;
  Json events=Json::array();VistaLoadingAudioServices api;api.volume=[&](auto handle,auto value){events.push_back(Json::array({"SetVolume",handle,std::bit_cast<std::uint32_t>(value)}));return std::int32_t{-1};};
  vista_loading_audio_envelope(s,row.at("seconds_bits"),api,context);
  check(Json(s.cached_volumes)==row.at("cached_volumes"),"Final integer envelope case "+std::to_string(envelopes)+": "+Json(s.cached_volumes).dump()+" != "+row.at("cached_volumes").dump());
  check(events==row.at("events"),"Envelope SetVolume trace "+std::to_string(envelopes));events.clear();vista_loading_audio_envelope(s,row.at("seconds_bits"),api,context);check(events.empty(),"Repeated cached envelope emitted call");++envelopes;
 }
 for(const auto& row:fixture.at("setup_cases")){
  VistaLoadingAudioState s;const auto& q=row.at("supplied");s.event=q.at("event");s.worker=q.at("thread");const std::string failure=q.at("failed_service");Json events=Json::array();unsigned created=0;std::array<std::vector<std::uint8_t>,4> banks;
  auto record=[&](auto&&... v){events.push_back(Json::array({v...}));};auto result=[&](const char* name){return failure==name?std::int32_t{-2147467259}:0;};
  VistaLoadingAudioServices api;
  api.setting2=[&]{const unsigned v=q.at("setting2_query");record("setting2_query",v);return v!=0;};
  api.create_device=[&](auto& device){record("DirectSoundCreate8");if(result("DirectSoundCreate8")>=0)device=25231872;return result("DirectSoundCreate8");};
  api.cooperative_level=[&](auto d,auto w,auto level){record("SetCooperativeLevel",d,w,level);return result("SetCooperativeLevel");};
  api.allocate_pcm=[&](auto bytes){check(bytes==65408,"Allocation bytes");record("allocate_pcm",bytes,25235456);return 25235456u;};
  api.free_pcm=[&](auto allocation){record("free_pcm",allocation);};
  api.create_buffer=[&](auto d,const auto& desc,auto& buffer){const Json* original=nullptr;for(const auto& event:row.at("events"))if(event[0]=="buffer_description" && event[1]==created)original=&event;check(original,"Original buffer description missing");const auto raw=hex((*original)[2]),wave=hex((*original)[3]);check(desc.size==little(raw,0,4) && desc.flags==little(raw,4,4) && desc.bytes==little(raw,8,4) && desc.reserved==little(raw,12,4),"Original buffer descriptor differs");for(unsigned i=0;i<4;++i)check(desc.algorithm[i]==little(raw,20+i*4,4),"Original 3D algorithm differs");check(desc.format==little(wave,0,2) && desc.channels==little(wave,2,2) && desc.samples_per_second==little(wave,4,4) && desc.bytes_per_second==little(wave,8,4) && desc.block_alignment==little(wave,12,2) && desc.bits_per_sample==little(wave,14,2) && desc.extra_bytes==little(wave,16,2),"Original PCM format differs");record("CreateSoundBuffer",d,created);if(result("CreateSoundBuffer")>=0)buffer=25233408+created*16;++created;return result("CreateSoundBuffer");};
  api.lock=[&](auto buffer,auto offset,auto bytes,auto flags,auto& lock){record("Lock",buffer,offset,bytes,flags);if(result("Lock")>=0)lock={25362432+(buffer-25233408)/16*65536,65408,0,0};return result("Lock");};
  api.copy_pcm=[&](auto token,auto bytes){check(bytes.size()==65408,"Copy PCM bytes");const auto i=(token-25362432)/65536;check(i<4,"Copy PCM token");banks[i]={bytes.begin(),bytes.end()};};
  api.unlock=[&](auto buffer,const auto& lock){record("Unlock",buffer,lock.first_token,lock.first_bytes,lock.second_token,lock.second_bytes);return result("Unlock");};
  api.frequency=[&](auto buffer,auto frequency){record("SetFrequency",buffer,frequency);return result("SetFrequency");};
  api.create_event=[&]{record("CreateEventA");return failure=="CreateEventA"?0u:53249u;};
  api.create_worker=[&]{record("CreateThread");return failure=="CreateThread"?0u:53250u;};
  vista_loading_audio_setup(s,q.at("window"),encoded,api);
  check(words(s)==row.at("after"),"Setup owner "+std::to_string(setups)+": "+words(s).dump());
  check(events==normalize(row.at("events")),"Setup services "+std::to_string(setups)+": "+events.dump()+" != "+normalize(row.at("events")).dump());
  if(failure.empty() && q.at("window")!=0 && q.at("setting2_query")==0 && q.at("thread")==0)for(const auto& bank:banks)check(bank==pcm,"Setup copied PCM differs");
  ++setups;
 }
 for(const auto& row:fixture.at("worker_cases")){
  auto s=initial();Json events=Json::array();const auto& q=row.at("supplied");unsigned clocks=0,waits=0;auto record=[&](auto&&... v){events.push_back(Json::array({v...}));};VistaLoadingAudioServices api;
  api.volume=[&](auto b,auto v){record("SetVolume",b,std::bit_cast<std::uint32_t>(v));return -1;};
  api.play=[&](auto b,auto a,auto c,auto d){record("Play",b,a,c,d);return -1;};
  api.clock=[&]{record("GetTickCount");check(clocks<q.at("clocks").size(),"Unexpected clock");return q.at("clocks")[clocks++].get<std::uint32_t>();};
  api.wait=[&](auto event,auto ms){record("WaitForSingleObject",event,ms);check(waits<q.at("wait_results").size(),"Unexpected wait");return q.at("wait_results")[waits++].get<std::uint32_t>();};
  vista_loading_audio_worker(s,api,context);check(Json(s.cached_volumes)==row.at("after"),"Worker final volumes");check(events==row.at("events"),"Worker trace "+std::to_string(workers));check(clocks==q.at("clocks").size() && waits==q.at("wait_results").size(),"Worker inputs not consumed");++workers;
 }
 for(const auto& row:fixture.at("teardown_cases")){
  auto s=initial();for(unsigned i=0;i<4;++i)s.cached_volumes[i]=fixture.at("worker_cases").back().at("after")[i];const bool active=row.at("supplied").at("active");s.device=active?25231872:0;s.worker=active?53250:0;if(!active)s.buffers={};Json events=Json::array();auto record=[&](auto&&... v){events.push_back(Json::array({v...}));};VistaLoadingAudioServices api;
  api.signal=[&](auto h){record("SetEvent",h);};api.wait=[&](auto h,auto ms){record("WaitForSingleObject",h,ms);return 0;};api.close=[&](auto h){record("CloseHandle",h);};api.stop=[&](auto h){record("Stop",h);};api.release=[&](auto h){record("Release",h);};
  vista_loading_audio_shutdown(s,api);check(words(s)==row.at("after"),"Shutdown retained owner");check(events==row.at("events"),"Shutdown service trace");++shutdowns;
 }
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 std::uint16_t after_cw;asm volatile("fnstcw %0":"=m"(after_cw));check(before_cw==after_cw && before_mxcsr==_mm_getcsr() && before_exceptions==std::fetestexcept(FE_ALL_EXCEPT),"Audio helper leaked FP environment");
#endif
 std::cout<<"PASS original PCM bytes65408 gain"<<gains<<" envelopes"<<envelopes<<" setup"<<setups<<" worker"<<workers<<" shutdown"<<shutdowns<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
