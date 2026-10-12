#include "halo2/sound_sdk_construction.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &v){return std::stoul(v.get<std::string>(),nullptr,16);}
std::vector<std::byte> unhex(const nlohmann::json &v){
  const auto text=v.get<std::string>();std::vector<std::byte> out;
  check(text.size()%2==0,"Invalid original construction bytes");
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
SoundSdkConstructionData import(const nlohmann::json &j){
  SoundSdkConstructionData d{};
  const std::array<std::pair<const char *,std::uint32_t *>,12> fields{{
      {"singleton_address",&d.singleton_address},{"speaker_override_address",&d.speaker_override_address},
      {"speaker_config_address",&d.speaker_config_address},{"busy_address",&d.busy_address},
      {"device_vtable",&d.device_vtable},{"device_data_vtable",&d.device_data_vtable},
      {"buffer_base_vtable",&d.buffer_base_vtable},{"buffer_vtable",&d.buffer_vtable},
      {"voice_base_vtable",&d.voice_base_vtable},{"voice_vtable",&d.voice_vtable},
      {"plain_reference_method",&d.plain_reference_method},{"synchronized_reference_method",&d.synchronized_reference_method}}};
  for(const auto &[name,target]:fields)*target=number(j.at(name));
  for(unsigned i=0;i<8;++i)d.references[i]={number(j.at("references").at(i).at("vtable_identity")),number(j.at("references").at(i).at("method_identity"))};
  for(unsigned i=0;i<15;++i)d.listener_defaults[i]=j.at("listener_defaults").at(i);
  for(unsigned i=0;i<12;++i)d.environment_defaults[i]=j.at("environment_defaults").at(i);
  return d;
}
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::vector<SoundDriverStartupStore> stores;
  unsigned acquisitions=0,releases=0,settings=0;
  explicit Harness(const nlohmann::json &r):row(r){
    for(const auto &region:r.at("regions"))retained.emplace(number(region.at("address")),unhex(region.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown construction identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Construction extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void observe(const SoundDriverStartupStore &store){
    stores.push_back(store);
    for(const auto &action:row.at("actions"))if(number(action.at("pc"))==store.instruction){
      const auto raw=unhex(action.at("hex"));const auto target=memory(action.at("address"),raw.size());
      std::copy(raw.begin(),raw.end(),target.begin());
    }
  }
  SoundSdkConstructionServices services(){
    SoundSdkConstructionServices s;s.memory=[this](auto a,auto n){return memory(a,n);};
    s.acquire_state_access=[this]{++acquisitions;check(!releases,"Reference acquire after release");return row.at("state_access_input").get<unsigned>()!=0;};
    s.release_state_access=[this]{++releases;check(releases<=acquisitions,"Reference release without access");};
    s.saved_audio_setting=[this]{++settings;return number(row.at("saved_setting_input"));};
    return s;
  }
};
std::uint32_t apply(const nlohmann::json &row,const SoundSdkConstructionData &d,Harness &h,
    const SoundSdkConstructionServices &s,const SoundDriverStartupObserver &observer){
  const auto owner=number(row.at("owner"));const auto kind=row.at("kind").get<std::string>();
  std::vector<std::uint32_t> a;for(const auto &v:row.at("arguments"))a.push_back(number(v));
  if(kind=="device_object")return source_sound_sdk_construct_device(owner,d,s,observer);
  if(kind=="device_data")return source_sound_sdk_construct_device_data(owner,d,s,observer);
  if(kind=="buffer_object")return source_sound_sdk_construct_buffer(owner,a.at(0),d,s,observer);
  if(kind=="voice_object")return source_sound_sdk_construct_voice(owner,a.at(0),a.at(1),d,s,observer);
  if(kind=="buffer_binding"){source_sound_sdk_bind_buffer(a.at(0),a.at(1),a.at(2),d,s,observer);return 0;}
  if(kind=="pair"){source_sound_sdk_initialize_pair(owner,a.at(0),s,observer);return 0;}
  if(kind=="reference")return source_sound_sdk_retain(a.at(0),d,s,observer);
  if(kind=="add_reference"||kind=="synchronized_reference")return source_sound_sdk_add_reference(a.at(0),d,s,observer);
  (void)h;throw std::runtime_error("Unknown construction fixture kind");
}
} // namespace
int main(int argc,char **argv){
  try {
    check(argc==2,"Supply original SDK construction fixture");nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_CONSTRUCTION_CPU_V1","Wrong construction fixture schema");
    const auto data=import(fixture.at("data"));unsigned cases=0,stores=0,genuine=0,stops=0;
    for(const auto &row:fixture.at("cases")){
      try{
        Harness h(row);const auto services=h.services();
        const auto result=apply(row,data,h,services,[&](const auto &s){h.observe(s);});
        for(const auto &region:row.at("regions"))
          check(h.retained.at(number(region.at("address")))==unhex(region.at("after_hex")),"Original SDK construction output differs");
        check(h.stores.size()==row.at("stores").size(),"Original SDK construction store count differs");
        for(unsigned i=0;i<h.stores.size();++i){const auto &actual=h.stores[i];const auto &expected=row.at("stores").at(i);
          check(actual.instruction==number(expected.at("pc"))&&actual.address==number(expected.at("address"))&&
              actual.bytes==expected.at("size")&&actual.before==number(expected.at("before"))&&actual.after==number(expected.at("after")),
              "Original SDK construction store order differs");
        }
        const auto kind=row.at("kind").get<std::string>();
        if(kind!="buffer_binding"&&kind!="pair")check(result==number(row.at("result")),"Original SDK construction return differs");
        check(h.settings==row.at("settings")&&h.releases==row.at("releases"),"Original SDK construction service count differs");
        check(h.acquisitions==row.at("acquisitions"),"Original SDK reference acquisition count differs");
        ++cases;stores+=h.stores.size();genuine+=row.at("genuine_capture_state").get<bool>();
      }catch(const std::exception &error){throw std::runtime_error(row.at("label").get<std::string>()+": "+error.what());}
    }
    const auto find=[&](const char *label)->const nlohmann::json &{
      for(const auto &r:fixture.at("cases"))if(r.at("label")==label)return r;
      throw std::runtime_error("Required construction control missing");
    };
    const auto reject=[&](const char *label,const auto &change,bool released=false){
      const auto &row=find(label);Harness h(row);auto d=data;auto s=h.services();change(h,d,s);
      bool stopped=false;try{(void)apply(row,d,h,s,{});}catch(const std::runtime_error &){stopped=true;}
      check(stopped,"Unavailable SDK construction owner admitted");
      if(released)check(h.releases==1,"Reference failure did not release actual access");
      ++stops;
    };
    reject("device_object",[](auto &,auto &,auto &s){s.memory={};});
    reject("device_object",[](auto &,auto &,auto &s){s.memory=[](auto,auto){return std::span<std::byte>{};};});
    reject("device_data",[](auto &,auto &,auto &s){s.saved_audio_setting={};});
    reject("buffer_object",[](auto &,auto &,auto &s){s.acquire_state_access={};});
    reject("buffer_object",[](auto &,auto &,auto &s){s.release_state_access={};});
    reject("buffer_object",[](auto &,auto &d,auto &){d.references[0].vtable_identity=0;});
    reject("buffer_object",[](auto &,auto &d,auto &){d.references[0].method_identity=0;});
    reject("buffer_object",[](auto &,auto &d,auto &){d.references[1]=d.references[0];});
    reject("buffer_busy_00000000_access1",[](auto &h,auto &,auto &){h.retained.erase(0x386b0c);},true);
    reject("buffer_binding",[](auto &h,auto &,auto &){h.retained.erase(0x1008000);});
    std::cout<<"PASS "<<genuine<<" genuine states, "<<cases<<" original CPU cases, "<<stores
             <<" ordered stores, "<<stops<<" missing-owner stops; platform boundaries modeled, no live DSP claim\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
