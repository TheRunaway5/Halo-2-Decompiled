#include "halo2/sound_sdk_hrtf.hpp"
#include "halo2/sound_sdk_spatial_data.hpp"
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
  check(text.size()%2==0,"Invalid original HRTF fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
std::uint32_t word(std::span<const std::byte> bytes,std::size_t at=0){
  std::uint32_t result=0;for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(bytes[at+i])<<(i*8);return result;
}
using Writes=std::vector<std::pair<std::uint32_t,std::uint32_t>>;
struct Harness {
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  bool locked=false;unsigned fifo_reads=0,dispatches=0;
  Writes writes;
  Harness(const nlohmann::json &fixture,const nlohmann::json &row){
    for(const auto &[name,field]:std::array<std::pair<const char *,const char *>,4>{{
        {"voice","voice_before_hex"},{"data","data_hex"},{"source","source_hex"},{"parameters","parameters_hex"}}})
      retained.emplace(fixture.at("aliases").at(name).get<std::uint32_t>(),unhex(row.at(field)));
    retained.emplace(0x386b04,unhex(row.at("speaker_config_hex")));
  }
  SoundSdkHrtfServices services(const SoundSdkSpatialData &data,const SoundDriverFloatingPointContext &fp){
    SoundSdkHrtfServices result;
    result.memory=[this](std::uint32_t address,std::size_t size){
      check(locked,"HRTF retained access outside device lock");
      const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown HRTF retained address");
      auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
      check(offset+size<=found->second.size(),"HRTF backing extent exceeds owner");
      return std::span<std::byte>(found->second).subspan(offset,size);
    };
    // Explicit native test service models. These do not claim actual Xbox lock
    // state, FIFO observations or DSP execution. Expected words remain solely
    // the separately measured original CPU/device-command fixtures.
    result.under_device_lock=[this](const std::function<void()> &body){
      check(!locked,"Nested HRTF device lock");locked=true;
      try{body();}catch(...){locked=false;throw;}locked=false;
    };
    result.select_kernel=[this,&data,fp](std::uint32_t azimuth,std::uint32_t,std::uint32_t fold){
      check(locked,"HRTF algorithm dispatch outside device lock");++dispatches;
      return source_sound_sdk_kernel(azimuth,fold,data.constants,fp);
    };
    result.kernel_bytes=[this,&data](std::uint32_t identity){
      check(locked,"HRTF kernel access outside device lock");return data.kernel(identity);
    };
    result.fifo_available=[this]{check(locked,"HRTF FIFO access outside device lock");
      const std::array<std::uint32_t,3> declared{0,75,79};return declared[std::min(fifo_reads++,2u)];};
    result.device_write=[this](std::uint32_t address,std::uint32_t value){
      check(locked&&fifo_reads>=3,"HRTF command before available device FIFO");writes.emplace_back(address,value);
    };
    return result;
  }
};
}
int main(int argc,char **argv){
  try{
    check(argc==3,"Supply original HRTF CPU and immutable SDK fixtures");
    nlohmann::json fixture,immutable;std::ifstream(argv[1])>>fixture;std::ifstream(argv[2])>>immutable;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_HRTF_CPU_V1","Wrong original HRTF fixture schema");
    const auto &ranges=immutable.at("ranges");SoundSdkSpatialData data{};auto &c=data.constants;
    const std::array<std::uint32_t *,10> fields{&c.zero_bits,&c.one_bits,&c.half_bits,&c.position_sentinel_bits,
      &c.angle_round_bias_bits,&c.half_turn_bits,&c.azimuth_scale_bits,&c.octant_bits,&c.quarter_turn_bits,&c.volume_scale_bits};
    for(unsigned i=0;i<fields.size();++i)*fields[i]=word(unhex(ranges.at(i).at("hex")));
    for(unsigned i=0;i<3;++i){c.mode_one_side[i]=word(unhex(ranges.at(10).at("hex")),i*4);
      c.mode_one_forward[i]=word(unhex(ranges.at(11).at("hex")),i*4);}
    c.kernel_table_a=number(ranges.at(12).at("base_address"));c.kernel_table_b=number(ranges.at(13).at("base_address"));
    data.null_kernel_identity=number(ranges.at(14).at("base_address"));
    const auto kernels=unhex(ranges.at(12).at("hex")),null=unhex(ranges.at(14).at("hex"));
    check(kernels.size()==data.kernels.size()&&null.size()==data.null_kernel.size(),"Wrong original HRTF data extent");
    std::copy(kernels.begin(),kernels.end(),data.kernels.begin());std::copy(null.begin(),null.end(),data.null_kernel.begin());
    unsigned actual=0,declared=0;
    const auto voice=fixture.at("aliases").at("voice").get<std::uint32_t>();
    for(const auto &row:fixture.at("cases")){
      const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      Harness harness(fixture,row);const auto services=harness.services(data,fp);
      source_sound_sdk_hrtf_apply(voice,number(row.at("source_override")),number(row.at("parameters_override")),
                                 {0x386b04,data.null_kernel_identity},c,fp,services);
      Writes expected;for(const auto &entry:row.at("writes"))expected.emplace_back(number(entry.at("address")),number(entry.at("value")));
      if(harness.writes!=expected||harness.retained.at(voice)!=unhex(row.at("voice_after_hex"))){
        std::cerr<<row.at("label").get<std::string>()<<'\n';throw std::runtime_error("Original HRTF CPU command/store output differs");}
      check(!harness.locked,"HRTF device lock retained after return");
      row.at("actual_command_capture").get<bool>()?++actual:++declared;
    }
    const auto &row=fixture.at("cases").at(0);
    const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
    for(unsigned missing=0;missing<6;++missing){
      Harness harness(fixture,row);auto services=harness.services(data,fp);
      switch(missing){case 0:services.under_device_lock={};break;case 1:services.memory={};break;
        case 2:services.select_kernel={};break;case 3:services.fifo_available={};break;
        case 4:services.device_write={};break;case 5:services.kernel_bytes={};break;}
      bool stopped=false;try{source_sound_sdk_hrtf_apply(voice,0,0,{0x386b04,data.null_kernel_identity},c,fp,services);}
      catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!harness.locked,"Missing HRTF provider silently succeeded or retained device lock");
    }
    check(actual==1&&declared>1000,"Original HRTF fixture cases missing");
    std::cout<<"PASS original HRTF CPU: "<<actual<<" genuine command capture, "<<declared
      <<" declared CPU cases, all19 command words/retained voice stores and6 required-service stops. "
        "Native test lock/FIFO services only; no live DSP or audible claim.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
