#include "halo2/sound_sdk_device.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &value){return std::stoul(value.get<std::string>(),nullptr,16);}
std::vector<std::byte> unhex(const nlohmann::json &value){
  const auto text=value.get<std::string>();std::vector<std::byte> raw;
  check(text.size()%2==0,"Invalid original SDK device fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
std::uint32_t word(std::span<const std::byte> raw,std::size_t offset=0){
  std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[offset+i])<<(i*8);return value;
}
void put(std::span<std::byte> raw,std::uint32_t value){
  for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(i*8));
}
using Writes=std::vector<std::pair<std::uint32_t,std::uint32_t>>;
struct Harness {
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  unsigned depth=0,locks=0,fifo_reads=0,required=0;
  std::uint32_t available=0,reserved=0;
  std::vector<std::uint32_t> voice_addresses;
  Writes writes;std::vector<SoundDriverStartupStore> stores;
  const nlohmann::json &row;
  Harness(const nlohmann::json &input):row(input){
    for(const auto &region:row.at("regions")){
      const auto address=number(region.at("address"));retained.emplace(address,unhex(region.at("before_hex")));
      const auto name=region.at("name").get<std::string>();
      if(name=="voice"||name.starts_with("child_voice_"))voice_addresses.push_back(address);
    }
    const auto voice=number(row.at("voice_identity"));
    const auto count=std::to_integer<unsigned>(retained.at(voice).at(0x64));
    const auto operation=row.at("operation").get<std::string>();
    required=count*(operation=="pitch"?2u:6u);
  }
  SoundSdkDeviceMemory memory(bool require_lock){
    return [this,require_lock](std::uint32_t address,std::size_t size){
      check(!require_lock||depth,"SDK device retained access outside lock");
      const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown SDK device retained address");
      auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
      check(offset+size<=found->second.size(),"SDK device retained extent exceeded");
      if(size==1&&offset==0x64&&std::find(voice_addresses.begin(),voice_addresses.end(),found->first)!=voice_addresses.end())
        required=std::to_integer<unsigned>(found->second[offset])*(row.at("operation")=="pitch"?2u:6u);
      return std::span<std::byte>(found->second).subspan(offset,size);
    };
  }
  SoundSdkDeviceServices services(){
    SoundSdkDeviceServices result;result.memory=memory(true);
    // These are declared test synchronization/FIFO/command models. Expected
    // words and retained stores come only from the original CPU/captures.
    result.under_device_lock=[this](const std::function<void()> &body){
      ++depth;++locks;try{body();}catch(...){--depth;throw;}--depth;
    };
    result.fifo_available=[this]{
      check(depth,"SDK device FIFO outside lock");++fifo_reads;
      reserved=required;
      if(!required)return 0u;
      const std::array<std::uint32_t,3> declared{0,required*4u-1u,required*4u+3u};
      available=declared[std::min(fifo_reads-1u,2u)];return available;
    };
    result.device_write=[this](std::uint32_t address,std::uint32_t value){
      check(depth&&fifo_reads&&(available>>2)>=reserved,"SDK command before available FIFO");writes.emplace_back(address,value);
      for(const auto &edit:row.at("external_edits"))if(edit.at("after_command")==writes.size()){
        const auto bytes=unhex(edit.at("hex"));const auto target=memory(true)(edit.at("address"),bytes.size());
        std::copy(bytes.begin(),bytes.end(),target.begin());
      }
    };
    return result;
  }
};
std::uint32_t apply(const std::string &operation,std::uint32_t voice,std::uint32_t override,
    const SoundSdkDeviceServices &services,const SoundDriverStartupObserver &observer={}){
  if(operation=="gain")return source_sound_sdk_gain_apply(voice,services);
  if(operation=="pitch")return source_sound_sdk_pitch_apply(voice,services);
  check(operation=="filter","Unknown original device operation");
  return source_sound_sdk_filter_apply(voice,override,services,observer);
}
}
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original SDK device CPU fixture");
    nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_DEVICE_CPU_V1","Wrong SDK device fixture schema");
    const auto voice=fixture.at("aliases").at("voice").get<std::uint32_t>();
    const auto filter=fixture.at("aliases").at("filter_override").get<std::uint32_t>();
    unsigned actual=0,declared=0,commands=0,store_count=0;
    for(const auto &row:fixture.at("cases")){
      Harness harness(row);const auto operation=row.at("operation").get<std::string>();
      const auto selected_voice=number(row.at("voice_identity"));
      if(operation=="gain_helper"||operation=="pitch_helper"){
        auto output=unhex(row.at("output_before_hex"));
        if(operation=="gain_helper"){
          std::array<std::uint32_t,9> words{};
          for(unsigned i=0;i<9;++i)words[i]=word(output,i*4);
          source_sound_sdk_gain_commands(selected_voice,words,harness.memory(false));
          for(unsigned i=0;i<9;++i)put(std::span<std::byte>(output).subspan(i*4,4),words[i]);
        }else put(output,source_sound_sdk_pitch_command(selected_voice,harness.memory(false)));
        if(output!=unhex(row.at("output_after_hex"))){
          std::cerr<<row.at("label").get<std::string>()<<'\n';throw std::runtime_error("Original SDK device CPU output differs");
        }
      }else{
        const auto services=harness.services();
        check(apply(operation,selected_voice,row.at("filter_override").get<bool>()?filter:0,services,
          [&](const SoundDriverStartupStore &store){harness.stores.push_back(store);})==0,"SDK device return differs");
        Writes expected;for(const auto &entry:row.at("writes"))expected.emplace_back(number(entry.at("address")),number(entry.at("value")));
        std::vector<SoundDriverStartupStore> stores;
        for(const auto &entry:row.at("stores"))stores.push_back({number(entry.at("instruction")),number(entry.at("address")),
          entry.at("bytes").get<unsigned>(),number(entry.at("before")),number(entry.at("after"))});
        if(harness.writes!=expected||harness.stores!=stores){
          std::cerr<<row.at("label").get<std::string>()<<'\n';throw std::runtime_error("Original SDK device CPU output differs");
        }
        check(!harness.depth&&harness.locks==row.at("expected_lock_count"),"SDK device synchronization differs");
        commands+=harness.writes.size();store_count+=harness.stores.size();
      }
      for(const auto &region:row.at("regions")){
        if(harness.retained.at(number(region.at("address")))!=unhex(region.at("after_hex"))){
          std::cerr<<row.at("label").get<std::string>()<<'\n';throw std::runtime_error("Original SDK device CPU output differs");
        }
      }
      row.at("actual_whole_capture").get<bool>()?++actual:++declared;
    }
    unsigned failure_controls=0;
    for(const auto &row:fixture.at("cases")){
      if(!row.at("actual_whole_capture").get<bool>())continue;
      const auto operation=row.at("operation").get<std::string>();
      for(unsigned missing=0;missing<6;++missing){
        Harness harness(row);auto services=harness.services();
        switch(missing){case 0:services.memory={};break;case 1:services.under_device_lock={};break;
          case 2:services.fifo_available={};break;case 3:services.device_write={};break;
          case 4:services.under_device_lock=[](const std::function<void()> &){};break;
          case 5:services.under_device_lock=[&harness](const std::function<void()> &body){
            ++harness.depth;try{body();body();}catch(...){--harness.depth;throw;}--harness.depth;};break;}
        bool stopped=false;try{apply(operation,voice,0,services);}catch(const std::runtime_error &){stopped=true;}
        check(stopped&&!harness.depth,"Missing SDK service or invalid lock silently succeeded");++failure_controls;
      }
    }
    const auto &gain=fixture.at("cases").at(0);
    for(unsigned count:{0u,4u}){
      Harness harness(gain);harness.retained.at(voice).at(0x64)=std::byte(count);
      std::array<std::uint32_t,9> output{};bool stopped=false;
      try{source_sound_sdk_gain_commands(voice,output,harness.memory(false));}catch(const std::runtime_error &){stopped=true;}
      check(stopped,"Unsupported original gain scratch/count silently succeeded");++failure_controls;
    }
    check(actual==3&&declared>2400,"Original SDK device cases missing");
    std::cout<<"PASS original SDK device CPU: "<<actual<<" genuine whole command calls, "<<declared
      <<" declared cases, "<<commands<<" command words, "<<store_count<<" ordered stores, "<<failure_controls
      <<" required-service/lock/count stops. Declared native lock/FIFO providers; recursive caller fragments checked, live DSP/audio unverified.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
