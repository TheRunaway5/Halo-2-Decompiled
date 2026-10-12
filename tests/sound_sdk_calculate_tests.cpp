#include "halo2/sound_sdk_calculate.hpp"
#include "halo2/sound_sdk_spatial_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &value){return std::stoul(value.get<std::string>(),nullptr,16);}
std::vector<std::byte> unhex(const nlohmann::json &value){
  const auto text=value.get<std::string>();std::vector<std::byte> bytes;
  check(text.size()%2==0,"Invalid original SDK calculation fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return bytes;
}
std::uint32_t word(std::span<const std::byte> bytes,std::size_t at=0){
  std::uint32_t result=0;
  for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(bytes[at+i])<<(i*8);
  return result;
}
std::array<std::uint32_t,3> vector(const nlohmann::json &value){
  const auto bytes=unhex(value);check(bytes.size()==12,"Original SDK vector extent differs");
  return {word(bytes),word(bytes,4),word(bytes,8)};
}
std::vector<std::byte> bytes(std::span<const std::uint32_t> words){
  std::vector<std::byte> result;
  for(const auto value:words)for(unsigned i=0;i<4;++i)result.push_back(std::byte(value>>(i*8)));
  return result;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
struct Harness {
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::vector<SoundDriverStartupStore> stores;
  bool locked=false;unsigned invocations=0;
  Harness(const nlohmann::json &fixture,const nlohmann::json &row){
    retained.emplace(fixture.at("listener_alias"),unhex(row.at("listener_before_hex")));
    if(!row.at("source_before_hex").is_null())
      retained.emplace(fixture.at("source_alias"),unhex(row.at("source_before_hex")));
  }
  SoundSdkCalculateServices services(const SoundSdkSpatialData &data,const SoundDriverFloatingPointContext &fp){
    SoundSdkCalculateServices result;
    result.memory=[this](std::uint32_t address,std::size_t size){
      check(locked,"SDK retained access outside state lock");
      const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown SDK retained address");
      auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
      check(offset+size<=found->second.size(),"SDK retained extent exceeds owner");
      return std::span<std::byte>(found->second).subspan(offset,size);
    };
    // Declared native test lock and current LightHRTF dispatch models. They do
    // not establish original kernel synchronization or a live native device.
    result.under_state_lock=[this](const std::function<void()> &body){
      check(!locked,"Nested SDK state lock");locked=true;++invocations;
      try{body();}catch(...){locked=false;throw;}locked=false;
    };
    result.position=[this,&data,fp](auto mode,const auto &listener,const auto &source){
      check(locked,"SDK position dispatch outside state lock");
      return source_sound_sdk_position(mode,listener,source,data.constants,fp);
    };
    result.angles=[this,&data,fp](auto mode,const auto &forward,const auto &,const auto &side,
                                const auto &direction,auto distance){
      check(locked,"SDK angle dispatch outside state lock");
      return source_sound_sdk_angles(mode,forward,side,direction,distance,data.constants,fp);
    };
    result.cone=[this,&data,fp](const auto &orientation,const auto &direction){
      check(locked,"SDK cone dispatch outside state lock");
      return source_sound_sdk_cone(orientation,direction,data.cone_constants,fp);
    };
    result.velocity=[this,fp](auto mode,const auto &listener,const auto &source,const auto &direction){
      check(locked,"SDK velocity dispatch outside state lock");
      return source_sound_sdk_velocity(mode,listener,source,direction,fp);
    };
    return result;
  }
};
}
int main(int argc,char **argv){
  try{
    check(argc==3,"Supply original retained SDK calculation and immutable data fixtures");
    nlohmann::json fixture,immutable;std::ifstream(argv[1])>>fixture;std::ifstream(argv[2])>>immutable;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_CALCULATE_V1","Wrong original retained SDK fixture schema");
    const auto &ranges=immutable.at("ranges");SoundSdkSpatialData data{};auto &c=data.constants;
    const std::array<std::uint32_t *,10> fields{&c.zero_bits,&c.one_bits,&c.half_bits,&c.position_sentinel_bits,
      &c.angle_round_bias_bits,&c.half_turn_bits,&c.azimuth_scale_bits,&c.octant_bits,&c.quarter_turn_bits,&c.volume_scale_bits};
    for(unsigned i=0;i<fields.size();++i)*fields[i]=word(unhex(ranges.at(i).at("hex")));
    for(unsigned i=0;i<3;++i){c.mode_one_side[i]=word(unhex(ranges.at(10).at("hex")),i*4);
      c.mode_one_forward[i]=word(unhex(ranges.at(11).at("hex")),i*4);}
    data.cone_constants={c.octant_bits,c.quarter_turn_bits,word(unhex(ranges.at(15).at("hex")))};
    check(data.cone_constants.angle_scale_bits==word(unhex(fixture.at("cone_angle_scale_hex"))),
          "Original cone operand differs between calculation and immutable fixtures");
    const auto listener=fixture.at("listener_alias").get<std::uint32_t>();
    unsigned actual=0,declared=0,ordered=0,helpers=0;
    for(const auto &row:fixture.at("cases")){
      const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      Harness harness(fixture,row);const auto services=harness.services(data,fp);
      const FloatingPointState before;
      source_sound_sdk_calculate(listener,row.at("source_before_hex").is_null()?0u:
          fixture.at("source_alias").get<std::uint32_t>(),fp,services,
          [&](const SoundDriverStartupStore &store){check(harness.locked,"SDK observation outside lock");harness.stores.push_back(store);});
      check(FloatingPointState{}==before,"SDK calculation did not restore caller floating-point state");
      std::vector<SoundDriverStartupStore> expected;
      for(const auto &entry:row.at("stores"))expected.push_back({number(entry.at("instruction")),number(entry.at("address")),
          entry.at("bytes"),number(entry.at("before")),number(entry.at("after"))});
      bool equal=harness.stores==expected&&harness.retained.at(listener)==unhex(row.at("listener_after_hex"));
      if(!row.at("source_after_hex").is_null())
        equal=equal&&harness.retained.at(fixture.at("source_alias").get<std::uint32_t>())==unhex(row.at("source_after_hex"));
      if(!equal){
        std::cerr<<row.at("label").get<std::string>()<<'\n';
        throw std::runtime_error("Original retained SDK calculation/store output differs");
      }
      check(!harness.locked&&harness.invocations==1,"SDK state lock retained or body invocation count differs");
      row.at("actual_whole_capture").get<bool>()?++actual:++declared;ordered+=expected.size();
    }
    for(const auto &row:fixture.at("helpers")){
      const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      const auto &input=row.at("inputs_hex");std::vector<std::byte> result;
      const FloatingPointState before;
      const auto owner=row.at("owner").get<std::string>();
      if(owner=="cross")result=bytes(source_sound_sdk_cross(vector(input.at("0")),vector(input.at("1")),fp));
      else{
        const auto output=owner=="cone"?source_sound_sdk_cone(vector(input.at("0")),vector(input.at("1")),data.cone_constants,fp):
          source_sound_sdk_velocity(number(row.at("argument_words").at(0)),vector(input.at("1")),vector(input.at("2")),vector(input.at("3")),fp);
        result=bytes(std::span<const std::uint32_t>(&output,1));
      }
      check(FloatingPointState{}==before,"SDK helper did not restore caller floating-point state");
      if(result!=unhex(row.at("output_hex"))){std::cerr<<row.at("label").get<std::string>()<<'\n';
        throw std::runtime_error("Original complete SDK helper output differs");}
      ++helpers;
    }
    const auto &row=fixture.at("cases").at(1);
    const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
    for(unsigned missing=0;missing<6;++missing){
      Harness harness(fixture,row);auto services=harness.services(data,fp);
      switch(missing){case 0:services.under_state_lock={};break;case 1:services.memory={};break;
        case 2:services.position={};break;case 3:services.angles={};break;case 4:services.cone={};break;case 5:services.velocity={};break;}
      bool stopped=false;const FloatingPointState before;
      try{source_sound_sdk_calculate(listener,fixture.at("source_alias"),fp,services);}
      catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!harness.locked,"Missing SDK service silently succeeded or retained state lock");
      check(FloatingPointState{}==before,"SDK service error did not restore floating-point state");
    }
    for(const bool repeat:{false,true}){
      Harness harness(fixture,row);auto services=harness.services(data,fp);
      services.under_state_lock=[&](const std::function<void()> &body){
        harness.locked=true;
        try{if(repeat){body();body();}}catch(...){harness.locked=false;throw;}
        harness.locked=false;
      };
      bool stopped=false;const FloatingPointState before;
      try{source_sound_sdk_calculate(listener,fixture.at("source_alias"),fp,services);}
      catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!harness.locked,"SDK lock provider skipped or repeated body without stopping");
      check(FloatingPointState{}==before,"SDK lock provider error did not restore floating-point state");
    }
    check(actual==2&&declared==1072&&helpers==1445,"Original SDK calculation fixture cases missing");
    std::cout<<"PASS original retained SDK calculation: "<<actual<<" genuine captures, "<<declared
      <<" declared states, "<<ordered<<" ordered stores, "<<helpers<<" complete helpers,6 required-service and2 lock-provider stops. "
        "Native test lock/dispatch models only; no live DSP or audible claim.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
