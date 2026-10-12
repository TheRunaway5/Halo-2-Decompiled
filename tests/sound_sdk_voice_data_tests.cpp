#include "halo2/sound_sdk_voice_data.hpp"
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
  const auto text=value.get<std::string>();std::vector<std::byte> raw;
  check(text.size()%2==0,"Invalid original voice-data fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
std::uint32_t word(std::span<const std::byte> raw,std::size_t offset=0){
  check(offset+4<=raw.size(),"Voice-data word exceeds retained extent");std::uint32_t value=0;
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[offset+i])<<(i*8);
  return value;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
SoundSdkVoiceConstants constants(const nlohmann::json &fixture){
  const auto &l=fixture.at("original_literal_hex");
  const auto get=[&](const char *address){return word(unhex(l.at(address)));};
  const auto one=unhex(fixture.at("original_double_literal_hex").at("0045dcc0"));
  return {get("0045dbd8"),get("0045dbc0"),get("0045dbb8"),get("004171e4"),get("0045dde8"),
    get("00385a44"),get("004171f4"),get("004171f0"),get("004122dc"),get("004172b4"),
    get("004123e8"),get("003869e4"),get("0045e4f0"),std::uint64_t(word(one))|(std::uint64_t(word(one,4))<<32)};
}
SoundSdkSpatialConstants spatial_constants(const nlohmann::json &fixture){
  const auto &ranges=fixture.at("ranges");SoundSdkSpatialConstants c{};
  const std::array<std::uint32_t *,10> fields{&c.zero_bits,&c.one_bits,&c.half_bits,&c.position_sentinel_bits,
    &c.angle_round_bias_bits,&c.half_turn_bits,&c.azimuth_scale_bits,&c.octant_bits,&c.quarter_turn_bits,&c.volume_scale_bits};
  for(unsigned i=0;i<fields.size();++i)*fields[i]=word(unhex(ranges.at(i).at("hex")));
  for(unsigned i=0;i<3;++i){c.mode_one_side[i]=word(unhex(ranges.at(10).at("hex")),i*4);
    c.mode_one_forward[i]=word(unhex(ranges.at(11).at("hex")),i*4);}
  return c;
}
struct Harness {
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::vector<SoundDriverStartupStore> stores;
  bool locked=false;unsigned invocations=0;
  Harness(const nlohmann::json &fixture,const nlohmann::json &row){
    for(unsigned i=0;i<5;++i)retained.emplace(fixture.at("aliases").at(i),unhex(row.at("before_hex").at(i)));
    const auto curve=unhex(row.at("curve_hex"));
    if(!curve.empty())retained.emplace(fixture.at("curve_alias"),curve);
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    check(locked,"SDK voice-data access outside state lock");
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown SDK voice-data identity");
    auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"SDK voice-data backing extent exceeds owner");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  SoundSdkVoiceDataServices services(const SoundSdkVoiceConstants &c,const SoundSdkSpatialConstants &spatial,
                                    const SoundDriverFloatingPointContext &fp){
    SoundSdkVoiceDataServices result;
    result.memory=[this](auto address,auto size){return memory(address,size);};
    // Declared native test lock and LightHRTF algorithm-dispatch models only.
    // Original expected data comes from the independently executed Xbox code.
    result.under_state_lock=[this](const std::function<void()> &body){
      check(!locked,"Nested SDK voice-data lock");locked=true;++invocations;
      try{body();}catch(...){locked=false;throw;}locked=false;
    };
    result.distance=[this,&c,fp](const SoundSdkDistanceParameters &p){
      check(locked,"SDK distance algorithm outside state lock");
      return source_sound_sdk_distance(p,c,fp,[this](auto address){return word(memory(address,4));});
    };
    result.cone_volume=[this,&c,fp](auto inner,auto outer,auto volume,auto angle){
      check(locked,"SDK cone-volume algorithm outside state lock");
      return source_sound_sdk_cone_volume(inner,outer,volume,angle,c,fp);
    };
    result.front_rear=[this,&spatial,fp](auto factor,auto distance,auto azimuth,auto,auto enabled){
      check(locked,"SDK front/rear algorithm outside state lock");
      return source_sound_sdk_front_rear(factor,distance,azimuth,enabled,spatial,fp);
    };
    result.rear_widen=[this](auto azimuth,auto elevation){
      check(locked,"SDK rear-widening algorithm outside state lock");return source_sound_sdk_rear_widen(azimuth,elevation);
    };
    result.doppler=[this,&c,fp](auto factor,auto scale,auto velocity){
      check(locked,"SDK Doppler algorithm outside state lock");return source_sound_sdk_doppler(factor,scale,velocity,c,fp);
    };
    return result;
  }
};
}
int main(int argc,char **argv){
  try{
    check(argc==3,"Supply original retained voice-data and immutable SDK fixtures");
    nlohmann::json fixture,immutable;std::ifstream(argv[1])>>fixture;std::ifstream(argv[2])>>immutable;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_VOICE_DATA_V1","Wrong original voice-data fixture schema");
    const auto c=constants(fixture);const auto spatial=spatial_constants(immutable);
    unsigned helpers=0,actual=0,declared=0,ordered=0;
    const auto invoke=[&](const nlohmann::json &row,const SoundSdkVoiceDataServices &services,
                          const SoundDriverStartupObserver &observer={}){
      std::array<std::uint32_t,5> addresses;
      for(unsigned i=0;i<5;++i)addresses[i]=fixture.at("aliases").at(i);
      for(const auto &absent:row.at("absent_arguments"))addresses.at(absent.get<unsigned>())=0;
      source_sound_sdk_voice_data(addresses[0],addresses[1],addresses[2],addresses[3],addresses[4],c,
          {row.at("x87_control_word"),row.at("mxcsr")},services,observer);
    };
    for(const auto &row:fixture.at("cases")){
      const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      Harness harness(fixture,row);const auto services=harness.services(c,spatial,fp);const FloatingPointState before;
      invoke(row,services,[&](const SoundDriverStartupStore &store){check(harness.locked,"Voice-data observer outside lock");harness.stores.push_back(store);});
      check(FloatingPointState{}==before,"SDK voice-data owner leaked floating-point context");
      std::vector<SoundDriverStartupStore> expected;
      for(const auto &entry:row.at("stores"))expected.push_back({number(entry.at("instruction")),number(entry.at("address")),
          entry.at("bytes"),number(entry.at("before")),number(entry.at("after"))});
      bool equal=expected==harness.stores;
      for(unsigned i=0;i<5;++i)equal=equal&&harness.retained.at(fixture.at("aliases").at(i))==unhex(row.at("after_hex").at(i));
      if(!equal){std::cerr<<row.at("label").get<std::string>()<<'\n';
        for(std::size_t i=0;i<std::max(expected.size(),harness.stores.size());++i){
          if(i<expected.size()){const auto &v=expected[i];std::cerr<<"expected "<<std::hex<<v.instruction<<' '<<v.address<<' '<<v.bytes<<' '<<v.before<<' '<<v.after<<'\n';}
          if(i<harness.stores.size()){const auto &v=harness.stores[i];std::cerr<<"got      "<<std::hex<<v.instruction<<' '<<v.address<<' '<<v.bytes<<' '<<v.before<<' '<<v.after<<'\n';}
        }
        throw std::runtime_error("Original retained SDK voice-data/store output differs");}
      check(!harness.locked&&harness.invocations==1,"Voice-data lock retained or invocation count differs");
      row.at("actual_whole_capture").get<bool>()?++actual:++declared;ordered+=expected.size();
    }
    for(const auto &row:fixture.at("helpers")){
      const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      std::vector<std::uint32_t> args;for(const auto &value:row.at("argument_words"))args.push_back(number(value));
      const auto owner=row.at("owner").get<std::string>();std::map<unsigned,std::uint32_t> result;
      const FloatingPointState before;
      if(owner=="distance"){
        const auto input=row.at("inputs_hex");SoundSdkCurveReader curve;
        std::vector<std::byte> backing;
        if(input.contains("3")){
          backing=unhex(input.at("3"));
          curve=[&](std::uint32_t address){check(address>=args[3],"Unknown SDK curve identity");return word(backing,address-args[3]);};
        }
        result[7]=source_sound_sdk_distance({args[0],args[1],args[2],args[3],args[4],args[5],args[6]},c,fp,curve);
      }else if(owner=="cone_volume")result[4]=source_sound_sdk_cone_volume(args[0],args[1],args[2],args[3],c,fp);
      else if(owner=="doppler")result[3]=source_sound_sdk_doppler(args[0],args[1],args[2],c,fp);
      else if(owner=="rear_widen"){
        const auto pair=source_sound_sdk_rear_widen(args[0],args[1]);result[2]=pair[0];result[3]=pair[1];
      }else if(owner=="environment"){
        const auto backing=unhex(row.at("inputs_hex").at("3"));std::array<std::uint32_t,10> words;
        for(unsigned i=0;i<words.size();++i)words[i]=word(backing,i*4);
        const auto output=source_sound_sdk_environment(args[0],args[1],args[2],words,c,fp);
        for(unsigned i=0;i<output.size();++i)result[4+i]=output[i];
      }else if(owner=="gain")result[0]=source_sound_sdk_gain(args[0],c,fp);
      else if(owner=="volume")result[0]=source_sound_sdk_volume(args[0],c,fp);
      else throw std::runtime_error("Unknown original voice-data helper owner");
      check(FloatingPointState{}==before,"SDK voice helper leaked floating-point context");
      std::map<unsigned,std::uint32_t> expected;
      if(owner=="gain"||owner=="volume")expected[0]=number(row.at("return_eax"));
      else for(const auto &[index,raw]:row.at("output_hex").items())expected[std::stoul(index)]=word(unhex(raw));
      if(result!=expected){
        std::cerr<<row.at("label").get<std::string>()<<'\n';
        for(const auto &[i,value]:result)std::cerr<<i<<" got "<<std::hex<<value<<" expected "<<expected.at(i)<<'\n';
        throw std::runtime_error("Original complete voice-data helper output differs");
      }
      ++helpers;
    }
    check(helpers==3555,"Original complete voice-data helpers missing");
    const auto found=std::find_if(fixture.at("cases").begin(),fixture.at("cases").end(),[](const auto &row){
      return row.at("label")=="mode_flags_00000000_ffffffff_2";});
    check(found!=fixture.at("cases").end(),"Original reached-dispatch control missing");const auto &row=*found;
    const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
    for(unsigned missing=0;missing<7;++missing){
      Harness harness(fixture,row);auto services=harness.services(c,spatial,fp);
      switch(missing){case 0:services.under_state_lock={};break;case 1:services.memory={};break;case 2:services.distance={};break;
        case 3:services.cone_volume={};break;case 4:services.front_rear={};break;case 5:services.rear_widen={};break;case 6:services.doppler={};break;}
      bool stopped=false;const FloatingPointState before;
      try{invoke(row,services);}catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!harness.locked,"Missing voice-data service silently succeeded or retained lock");
      check(FloatingPointState{}==before,"Voice-data service failure leaked floating-point context");
    }
    for(const bool repeat:{false,true}){
      Harness harness(fixture,row);auto services=harness.services(c,spatial,fp);
      services.under_state_lock=[&](const std::function<void()> &body){
        harness.locked=true;try{if(repeat){body();body();}}catch(...){harness.locked=false;throw;}harness.locked=false;
      };
      bool stopped=false;const FloatingPointState before;
      try{invoke(row,services);}catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!harness.locked,"Invalid voice-data lock provider silently succeeded or retained lock");
      check(FloatingPointState{}==before,"Voice-data lock failure leaked floating-point context");
    }
    bool curve_stopped=false;
    try{(void)source_sound_sdk_distance({0x3f800000,0x3f800000,0x42000000,0x1006000,4,0x40000000,0},c,fp);}
    catch(const std::runtime_error &){curve_stopped=true;}
    check(curve_stopped,"Missing reached curve owner silently succeeded");
    check(actual==1&&declared==2825,"Original retained voice-data cases missing");
    std::cout<<"PASS original voice-data CPU: "<<actual<<" genuine no-op capture, "<<declared<<" declared states, "
      <<ordered<<" ordered voice stores, "<<helpers<<" complete helpers and10 required-service/lock stops. "
        "Test lock/dispatch models only; no live DSP or audible claim.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
