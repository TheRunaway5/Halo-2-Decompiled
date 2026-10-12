#include "halo2/sound_sdk_commit.hpp"
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
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t number(const nlohmann::json &v){return std::stoul(v.get<std::string>(),nullptr,16);}
std::vector<std::byte> unhex(const nlohmann::json &v){
  const auto text=v.get<std::string>();std::vector<std::byte> raw;
  check(text.size()%2==0,"Invalid SDK commit fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)raw.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return raw;
}
std::uint32_t word(std::span<const std::byte> raw,std::size_t offset=0){
  check(offset+4<=raw.size(),"SDK commit word exceeds backing");std::uint32_t result=0;
  for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(raw[offset+i])<<(i*8);
  return result;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
SoundSdkSpatialData immutable_data(const nlohmann::json &fixture){
  const auto &ranges=fixture.at("ranges");SoundSdkSpatialData data{};auto &c=data.constants;
  const auto get=[&](unsigned i){return word(unhex(ranges.at(i).at("hex")));};
  const std::array<std::uint32_t *,10> fields{&c.zero_bits,&c.one_bits,&c.half_bits,&c.position_sentinel_bits,
    &c.angle_round_bias_bits,&c.half_turn_bits,&c.azimuth_scale_bits,&c.octant_bits,&c.quarter_turn_bits,&c.volume_scale_bits};
  for(unsigned i=0;i<fields.size();++i)*fields[i]=get(i);
  for(unsigned i=0;i<3;++i){c.mode_one_side[i]=word(unhex(ranges.at(10).at("hex")),i*4);
    c.mode_one_forward[i]=word(unhex(ranges.at(11).at("hex")),i*4);}
  c.kernel_table_a=number(ranges.at(12).at("base_address"));c.kernel_table_b=number(ranges.at(13).at("base_address"));
  data.null_kernel_identity=number(ranges.at(14).at("base_address"));
  const auto kernels=unhex(ranges.at(12).at("hex")),null=unhex(ranges.at(14).at("hex"));
  check(kernels.size()==data.kernels.size()&&null.size()==data.null_kernel.size(),"SDK commit immutable extent differs");
  std::copy(kernels.begin(),kernels.end(),data.kernels.begin());std::copy(null.begin(),null.end(),data.null_kernel.begin());
  data.cone_constants={c.octant_bits,c.quarter_turn_bits,get(15)};
  const auto one=unhex(ranges.at(27).at("hex"));
  data.voice_constants={get(0),get(1),get(16),get(17),get(18),get(19),get(20),get(21),get(22),get(23),
    get(24),get(25),get(26),std::uint64_t(word(one))|(std::uint64_t(word(one,4))<<32)};
  return data;
}
struct Call {
  std::string owner;std::uint32_t voice;std::vector<std::uint32_t> arguments;
  friend bool operator==(const Call &,const Call &)=default;
};
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::vector<SoundDriverStartupStore> stores;std::vector<Call> calls;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> writes;
  unsigned state_depth=0,device_depth=0,state_locks=0,device_locks=0,fifo_reads=0;
  explicit Harness(const nlohmann::json &r):row(r){
    for(const auto &region:row.at("regions"))retained.emplace(number(region.at("address")),unhex(region.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    const auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown SDK commit identity");
    auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"SDK commit retained extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void entered(std::string owner,std::uint32_t voice,std::vector<std::uint32_t> arguments={}){
    calls.push_back({std::move(owner),voice,std::move(arguments)});
    for(const auto &action:row.at("actions"))if(action.at("after_call")==calls.size()){
      const auto bytes=unhex(action.at("hex"));const auto target=memory(action.at("address"),bytes.size());
      std::copy(bytes.begin(),bytes.end(),target.begin());
    }
  }
  void state_lock(const std::function<void()> &body){
    ++state_depth;++state_locks;try{body();}catch(...){--state_depth;throw;}--state_depth;
  }
  SoundSdkDispatchServices dispatch(){
    SoundSdkDispatchServices s;s.memory=[this](auto a,auto n){return memory(a,n);};
    s.gain=[this](auto voice){entered("gain",voice);};s.pitch=[this](auto voice){entered("pitch",voice);};
    s.hrtf=[this](auto voice,auto source,auto parameters){entered("hrtf",voice,{source,parameters});};
    s.filter=[this](auto voice,auto filter){entered("filter",voice,{filter});};return s;
  }
  SoundSdkUpdateServices update(){
    SoundSdkUpdateServices s;s.memory=[this](auto a,auto n){return memory(a,n);};
    s.under_state_lock=[this](const auto &body){state_lock(body);};
    s.calculate=[this](auto listener,auto source){check(state_depth,"Calculation outside commit lock");entered("calculate",0,{listener,source});};
    s.voice_data=[this](auto l,auto source,auto le,auto se,auto p){check(state_depth,"Voice data outside commit lock");entered("voice_data",0,{l,source,le,se,p});};
    s.dispatch=[this](auto voice){check(state_depth,"Dispatch outside commit lock");entered("dispatch",voice);};return s;
  }
  SoundSdkPipelineServices pipeline(const SoundSdkSpatialData &data,const SoundDriverFloatingPointContext &fp){
    SoundSdkPipelineServices s;s.memory=[this](auto a,auto n){
      check(state_depth,"Pipeline retained access outside state lock");return memory(a,n);};
    s.under_state_lock=[this](const auto &body){state_lock(body);};
    // Declared native test lock/FIFO providers and current LightHRTF dispatch.
    // All source CPU implementations execute. They are checked against the
    // independently executed original, not against these service models.
    s.calculate.position=[this,&data,fp](auto mode,const auto &listener,const auto &source){
      check(state_depth,"Position outside lock");return source_sound_sdk_position(mode,listener,source,data.constants,fp);};
    s.calculate.angles=[this,&data,fp](auto mode,const auto &forward,const auto &,const auto &side,const auto &direction,auto distance){
      check(state_depth,"Angles outside lock");return source_sound_sdk_angles(mode,forward,side,direction,distance,data.constants,fp);};
    s.calculate.cone=[this,&data,fp](const auto &orientation,const auto &direction){
      check(state_depth,"Cone outside lock");return source_sound_sdk_cone(orientation,direction,data.cone_constants,fp);};
    s.calculate.velocity=[this,fp](auto mode,const auto &listener,const auto &source,const auto &direction){
      check(state_depth,"Velocity outside lock");return source_sound_sdk_velocity(mode,listener,source,direction,fp);};
    s.voice_data.distance=[this,&data,fp](const auto &p){
      check(state_depth,"Distance outside lock");return source_sound_sdk_distance(p,data.voice_constants,fp,[this](auto a){return word(memory(a,4));});};
    s.voice_data.cone_volume=[this,&data,fp](auto inner,auto outer,auto volume,auto angle){
      check(state_depth,"Cone volume outside lock");return source_sound_sdk_cone_volume(inner,outer,volume,angle,data.voice_constants,fp);};
    s.voice_data.front_rear=[this,&data,fp](auto factor,auto distance,auto azimuth,auto,auto enabled){
      check(state_depth,"Front/rear outside lock");return source_sound_sdk_front_rear(factor,distance,azimuth,enabled,data.constants,fp);};
    s.voice_data.rear_widen=[this](auto azimuth,auto elevation){
      check(state_depth,"Rear widening outside lock");return source_sound_sdk_rear_widen(azimuth,elevation);};
    s.voice_data.doppler=[this,&data,fp](auto factor,auto scale,auto velocity){
      check(state_depth,"Doppler outside lock");return source_sound_sdk_doppler(factor,scale,velocity,data.voice_constants,fp);};
    s.device.under_device_lock=[this](const auto &body){
      check(state_depth,"Device update outside commit state lock");++device_depth;++device_locks;
      try{body();}catch(...){--device_depth;throw;}--device_depth;};
    s.device.fifo_available=[this]{check(device_depth,"FIFO outside device lock");++fifo_reads;
      return fifo_reads==1?0u:0x100u;};
    s.device.device_write=[this](auto address,auto value){
      check(device_depth&&fifo_reads>=2,"Device command before declared FIFO credits");writes.emplace_back(address,value);};
    s.hrtf.select_kernel=[this,&data,fp](auto azimuth,auto,auto fold){
      check(device_depth,"Kernel selection outside device lock");return source_sound_sdk_kernel(azimuth,fold,data.constants,fp);};
    s.hrtf.kernel_bytes=[this,&data](auto identity){
      check(device_depth,"Kernel bytes outside device lock");return data.kernel(identity);};
    return s;
  }
};
}
int main(int argc,char **argv){
  try{
    check(argc==3,"Supply original commit and immutable SDK fixtures");nlohmann::json fixture,immutable;
    std::ifstream(argv[1])>>fixture;std::ifstream(argv[2])>>immutable;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_COMMIT_CPU_V1","Wrong original SDK commit schema");
    const auto data=immutable_data(immutable);const auto voice=fixture.at("aliases").at("voice").get<std::uint32_t>();
    const SoundSdkHrtfBindings bindings{0x386b04,data.null_kernel_identity};
    unsigned actual=0,declared=0,command_count=0,store_count=0,pipeline_count=0;
    for(const auto &row:fixture.at("cases")){
      Harness h(row);const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      const auto observe=[&](const SoundDriverStartupStore &store){h.stores.push_back(store);};
      const FloatingPointState before;const auto mode=row.at("mode").get<std::string>();bool identity_equal=true;
      if(mode=="pipeline"){
        source_sound_sdk_spatial_commit(voice,data.constants,data.voice_constants,bindings,fp,h.pipeline(data,fp),observe);++pipeline_count;
      }else if(mode=="dispatch"){
        identity_equal=source_sound_sdk_dispatch(voice,h.dispatch(),observe)==number(row.at("return_eax"));
      }else{check(mode=="update","Unknown SDK commit mode");source_sound_sdk_update(voice,h.update(),observe);}
      check(FloatingPointState{}==before,"SDK commit leaked floating-point context");
      std::vector<SoundDriverStartupStore> stores;
      for(const auto &e:row.at("stores"))stores.push_back({number(e.at("instruction")),number(e.at("address")),e.at("bytes"),number(e.at("before")),number(e.at("after"))});
      std::vector<std::pair<std::uint32_t,std::uint32_t>> writes;
      for(const auto &e:row.at("writes"))writes.emplace_back(number(e.at("address")),number(e.at("value")));
      bool equal=identity_equal&&h.stores==stores&&h.writes==writes;
      for(const auto &r:row.at("regions"))equal=equal&&h.retained.at(number(r.at("address")))==unhex(r.at("after_hex"));
      if(mode!="pipeline"){
        std::vector<Call> expected;
        for(const auto &c:row.at("calls")){
          const auto owner=c.at("owner").get<std::string>();std::vector<std::uint32_t> arguments;
          for(const auto &a:c.at("arguments"))arguments.push_back(number(a));
          expected.push_back({owner,owner=="calculate"||owner=="voice_data"?0u:number(c.at("this")),arguments});
        }
        equal=equal&&h.calls==expected;
      }
      if(!equal){
        std::cerr<<row.at("label").get<std::string>()<<'\n';
        for(std::size_t i=0;i<std::max(stores.size(),h.stores.size());++i){
          if(i<stores.size()){const auto &v=stores[i];std::cerr<<"expected "<<std::hex<<v.instruction<<' '<<v.address<<' '<<v.before<<' '<<v.after<<'\n';}
          if(i<h.stores.size()){const auto &v=h.stores[i];std::cerr<<"got      "<<std::hex<<v.instruction<<' '<<v.address<<' '<<v.before<<' '<<v.after<<'\n';}
        }
        throw std::runtime_error("Original combined SDK commit output differs");
      }
      check(!h.state_depth&&!h.device_depth,"SDK commit retained a test lock");
      if(mode=="pipeline"){
        const auto &v=h.retained.at(voice);check(h.state_locks==((std::to_integer<unsigned>(v[0x12])&3u)==3u?3u:1u),"Pipeline state-lock nesting differs");
      }
      row.at("actual_whole_capture").get<bool>()?++actual:++declared;command_count+=writes.size();store_count+=stores.size();
    }
    unsigned stops=0;
    const auto selected=[&](const char *label)->const nlohmann::json &{
      const auto &cases=fixture.at("cases");auto i=std::find_if(cases.begin(),cases.end(),[&](const auto &r){return r.at("label")==label;});
      check(i!=cases.end(),"Required SDK commit control missing");return *i;};
    for(unsigned missing=0;missing<5;++missing){
      Harness h(selected("dispatch_flags_ff"));auto s=h.dispatch();
      switch(missing){case 0:s.memory={};break;case 1:s.gain={};break;case 2:s.pitch={};break;case 3:s.hrtf={};break;case 4:s.filter={};break;}
      bool stopped=false;try{source_sound_sdk_dispatch(voice,s);}catch(const std::runtime_error &){stopped=true;}
      check(stopped,"Missing dispatcher owner silently succeeded");++stops;
    }
    for(unsigned missing=0;missing<7;++missing){
      Harness h(selected("update_flags_03"));auto s=h.update();
      switch(missing){case 0:s.memory={};break;case 1:s.under_state_lock={};break;case 2:s.calculate={};break;
        case 3:s.voice_data={};break;case 4:s.dispatch={};break;
        case 5:s.under_state_lock=[](const auto &){};break;
        case 6:s.under_state_lock=[&](const auto &body){h.state_lock([&]{body();body();});};break;}
      bool stopped=false;try{source_sound_sdk_update(voice,s);}catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!h.state_depth,"Missing update owner or invalid lock silently succeeded");++stops;
    }
    for(unsigned missing=0;missing<16;++missing){
      const auto &row=selected("pipeline_all_dirty");Harness h(row);
      const SoundDriverFloatingPointContext fp{row.at("x87_control_word"),row.at("mxcsr")};
      auto s=h.pipeline(data,fp);
      switch(missing){case 0:s.memory={};break;case 1:s.under_state_lock={};break;
        case 2:s.device.under_device_lock={};break;case 3:s.device.fifo_available={};break;
        case 4:s.device.device_write={};break;case 5:s.calculate.position={};break;
        case 6:s.calculate.angles={};break;case 7:s.calculate.cone={};break;
        case 8:s.calculate.velocity={};break;case 9:s.voice_data.distance={};break;
        case 10:s.voice_data.cone_volume={};break;case 11:s.voice_data.front_rear={};break;
        case 12:s.voice_data.rear_widen={};break;case 13:s.voice_data.doppler={};break;
        case 14:s.hrtf.select_kernel={};break;case 15:s.hrtf.kernel_bytes={};break;}
      bool stopped=false;const FloatingPointState before;
      try{source_sound_sdk_spatial_commit(voice,data.constants,data.voice_constants,bindings,fp,s);}
      catch(const std::runtime_error &){stopped=true;}
      check(stopped&&!h.state_depth&&!h.device_depth,"Missing pipeline owner silently succeeded or retained lock");
      check(FloatingPointState{}==before,"Failed SDK pipeline leaked floating-point context");++stops;
    }
    check(actual==2&&declared>700&&pipeline_count>200,"Original combined SDK commit cases missing");
    std::cout<<"PASS combined SDK commit CPU: "<<actual<<" genuine whole captures, "<<declared<<" declared cases, "
      <<pipeline_count<<" complete CPU pipelines, "<<command_count<<" device words, "<<store_count<<" ordered stores, "
      <<stops<<" required-owner/lock stops. Test lock/FIFO services; no live DSP or audible claim.\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
