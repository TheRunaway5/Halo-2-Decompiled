#include "halo2/sound_sdk_setters.hpp"
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
  const auto text=v.get<std::string>();std::vector<std::byte> out;
  check(text.size()%2==0,"Invalid original setter fixture bytes");
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
struct FloatingPointState {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  unsigned short control;unsigned mxcsr;
  FloatingPointState():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0":"=m"(control));}
#endif
  friend bool operator==(const FloatingPointState &,const FloatingPointState &)=default;
};
struct Call {
  std::string owner;std::uint32_t voice;std::vector<std::uint32_t> arguments;
  friend bool operator==(const Call &,const Call &)=default;
};
struct Harness {
  const nlohmann::json &row;
  std::map<std::uint32_t,std::vector<std::byte>> retained;
  std::vector<SoundDriverStartupStore> stores;std::vector<Call> calls;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> writes;
  unsigned acquisitions=0,releases=0,device_depth=0,fifo_reads=0;
  explicit Harness(const nlohmann::json &r):row(r){
    for(const auto &region:r.at("regions"))retained.emplace(number(region.at("address")),unhex(region.at("before_hex")));
  }
  std::span<std::byte> memory(std::uint32_t address,std::size_t size){
    check(acquisitions==1,"Setter retained access before platform state-access owner");
    auto after=retained.upper_bound(address);check(after!=retained.begin(),"Unknown setter identity");
    const auto found=std::prev(after);const auto offset=std::uint64_t(address)-found->first;
    check(offset+size<=found->second.size(),"Setter retained extent differs");
    return std::span<std::byte>(found->second).subspan(offset,size);
  }
  void store(const SoundDriverStartupStore &s){
    stores.push_back(s);
    for(const auto &action:row.at("actions"))if(action.at("after_store")==stores.size()){
      const auto raw=unhex(action.at("hex"));const auto target=memory(action.at("address"),raw.size());
      std::copy(raw.begin(),raw.end(),target.begin());
    }
  }
  SoundSdkDeviceServices device(){
    SoundSdkDeviceServices s;s.memory=[this](auto a,auto n){return memory(a,n);};
    s.under_device_lock=[this](const auto &body){
      check(acquisitions==1&&!releases,"Filter device owner outside setter state access");++device_depth;
      try{body();}catch(...){--device_depth;throw;}--device_depth;};
    s.fifo_available=[this]{check(device_depth,"Setter filter FIFO outside lock");++fifo_reads;return fifo_reads==1?0u:0x100u;};
    s.device_write=[this](auto a,auto v){check(device_depth&&fifo_reads>=2,"Setter filter command without FIFO credits");writes.emplace_back(a,v);};
    return s;
  }
  SoundSdkSetterServices services(){
    SoundSdkSetterServices s;s.memory=[this](auto a,auto n){return memory(a,n);};
    // Explicit test state/FIFO services. Acquisition1 matches actual captured
    // calls; acquisition0 is the separately measured original IRQ-byte branch.
    s.acquire_state_access=[this]{++acquisitions;check(acquisitions==1,"State access repeated");return row.at("acquired").get<unsigned>()!=0;};
    s.release_state_access=[this]{++releases;check(releases==1,"State release repeated");};
    s.commit=[this](auto voice){check(!releases,"Commit after state release");calls.push_back({"commit",voice,{}});};
    s.filter=[this](auto voice,auto payload){
      calls.push_back({"filter",voice,{payload}});const auto backend=device();
      check(source_sound_sdk_filter_apply(voice,payload,backend,[this](const auto &s){store(s);})==0,"Source filter result differs");};
    return s;
  }
};
std::uint32_t apply(const nlohmann::json &row,Harness &h,const SoundSdkSetterServices &s,bool through_driver=false){
  std::vector<std::uint32_t> a;for(const auto &v:row.at("arguments"))a.push_back(number(v));
  const SoundSdkSetterBindings bindings{0x386b0c};
  const SoundDriverFloatingPointContext fp{row.at("context").at(0).get<std::uint16_t>(),row.at("context").at(1).get<std::uint32_t>()};
  const auto observer=[&](const auto &store){h.store(store);};
  if(through_driver){
    std::vector<std::uint32_t> words;
    const auto kind=row.at("kind").get<std::string>();
    const auto operation=kind=="position"?SoundDriverSpatialOperation::Position:
        kind=="environment"?SoundDriverSpatialOperation::I3dl2Source:SoundDriverSpatialOperation::Filter;
    if(kind=="position")words={a.at(1),a.at(2),a.at(3)};
    else for(unsigned i=0;i<(kind=="environment"?9u:6u);++i){
      const auto address=a.at(1)+i*4u;const auto found=std::prev(h.retained.upper_bound(address));
      const auto offset=address-found->first;check(offset+4<=found->second.size(),"Driver input payload is unbacked");
      std::uint32_t value=0;for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(found->second[offset+n])<<(n*8);
      words.push_back(value);
    }
    unsigned publications=0;
    const SoundSdkPayloadPublisher publish=[&](auto raw,const auto &body){
      check(std::equal(raw.begin(),raw.end(),words.begin(),words.end()),"Driver changed authored SDK payload words");
      ++publications;body(a.at(1));
    };
    const auto result=source_sound_sdk_driver_operation({operation,a.at(0),kind=="filter"?std::optional<std::uint32_t>{}:a.back(),words},
        bindings,fp,s,publish,observer);
    check(publications==(kind=="position"?0u:1u),"Driver SDK payload publication count differs");return result;
  }
  if(row.at("kind")=="position")return source_sound_sdk_set_position(a.at(0),{a.at(1),a.at(2),a.at(3)},a.at(4),bindings,fp,s,observer);
  if(row.at("kind")=="environment")return source_sound_sdk_set_environment(a.at(0),a.at(1),a.at(2),bindings,s,observer);
  return source_sound_sdk_set_filter(a.at(0),a.at(1),bindings,s);
}
}
int main(int argc,char **argv){
  try{
    check(argc==2,"Supply original SDK setter fixture");nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("schema")=="ORIGINAL_SOUND_SDK_SETTER_CPU_V1","Wrong SDK setter fixture schema");
    unsigned genuine=0,declared=0,store_count=0,command_count=0;
    for(const auto &row:fixture.at("cases"))for(bool through_driver:{false,true}){
      Harness h(row);const auto services=h.services();const FloatingPointState before;
      std::uint32_t result;
      try{result=apply(row,h,services,through_driver);}catch(...){std::cerr<<row.at("label").get<std::string>()<<"\n";throw;}
      bool same=result==number(row.at("result"));
      check(FloatingPointState{}==before,"Setter changed caller floating-point context");
      same&=h.acquisitions==1&&h.releases==row.at("releases").get<unsigned>()&&h.device_depth==0;
      for(const auto &region:row.at("regions"))same&=h.retained.at(number(region.at("address")))==unhex(region.at("after_hex"));
      std::vector<Call> expected_calls;
      for(const auto &call:row.at("calls")){
        Call c{call.at("owner").get<std::string>(),number(call.at("voice")),{}};
        for(const auto &v:call.at("arguments"))c.arguments.push_back(number(v));
        expected_calls.push_back(std::move(c));
      }
      same&=h.calls==expected_calls;
      same&=h.stores.size()==row.at("ordered_stores").size();
      for(std::size_t i=0;i<std::min(h.stores.size(),row.at("ordered_stores").size());++i){
        const auto &a=h.stores[i];const auto &b=row.at("ordered_stores").at(i);
        same&=a.instruction==number(b.at("pc"))&&a.address==number(b.at("address"))&&a.bytes==b.at("size").get<unsigned>()&&
          a.before==number(b.at("before"))&&a.after==number(b.at("after"));
      }
      std::vector<std::pair<std::uint32_t,std::uint32_t>> expected_writes;
      for(const auto &w:row.at("device_writes"))expected_writes.emplace_back(number(w.at("address")),number(w.at("value")));
      same&=h.writes==expected_writes;
      if(!same){std::cerr<<row.at("label").get<std::string>()<<"\n";throw std::runtime_error("Original SDK setter output differs");}
      if(!through_driver){row.at("genuine").get<bool>()?++genuine:++declared;store_count+=h.stores.size();command_count+=h.writes.size();}
    }
    const auto &position=fixture.at("cases").at(0);unsigned stops=0;
    for(unsigned mode=0;mode<8;++mode){
      auto row=position;
      if(mode==3)row["arguments"].back()="00000000";
      Harness h(row);auto s=h.services();
      if(mode==0)s.memory={};
      if(mode==1)s.acquire_state_access={};
      if(mode==2)s.release_state_access={};
      if(mode==3)s.commit={};
      if(mode==4)s.memory=[](auto,auto){return std::span<std::byte>{};};
      if(mode==5)row["context"][0]=0x027e;
      if(mode==6)row["context"][1]=0x10000;
      if(mode==7)row["context"][1]=0;
      const FloatingPointState before;bool failed=false;
      try{(void)apply(row,h,s);}catch(const std::exception &){failed=true;}
      check(failed&&FloatingPointState{}==before,"Missing setter owner/FP context did not stop safely");
      if(h.acquisitions)check(h.releases==1,"Setter error leaked state access");
      ++stops;
    }
    for(unsigned mode=0;mode<4;++mode){
      const auto &row=fixture.at("cases").at(2);Harness h(row);auto s=h.services();
      if(!mode)s.filter={};else{
        auto backend=h.device();if(mode==1)backend.under_device_lock={};if(mode==2)backend.fifo_available={};if(mode==3)backend.device_write={};
        s.filter=[&h,backend](auto voice,auto payload){source_sound_sdk_filter_apply(voice,payload,backend,[&h](const auto &v){h.store(v);});};
      }
      bool failed=false;try{(void)apply(row,h,s);}catch(const std::exception &){failed=true;}
      check(failed&&h.releases==1&&h.device_depth==0,"Setter filter error leaked access or did not stop");++stops;
    }
    for(unsigned mode=0;mode<5;++mode){
      const auto &row=fixture.at("cases").at(1);Harness h(row);const auto s=h.services();
      std::array<std::uint32_t,9> words{};
      const auto payload=number(row.at("arguments").at(1));const auto &raw=h.retained.at(payload);
      for(unsigned i=0;i<words.size();++i)for(unsigned n=0;n<4;++n)
        words[i]|=std::to_integer<std::uint32_t>(raw[i*4+n])<<(n*8);
      SoundSdkPayloadPublisher publish;
      if(mode)publish=[&](auto,const auto &body){
        if(mode==1)return;
        body(mode==4?0u:payload);if(mode==2)body(payload);
      };
      bool failed=false;
      try{source_sound_sdk_driver_operation({SoundDriverSpatialOperation::I3dl2Source,
          number(row.at("arguments").at(0)),1,std::span<const std::uint32_t>(words).first(mode==3?8:9)},
          {0x386b0c},{0x027f,0x1f80},s,publish);}catch(const std::exception &){failed=true;}
      check(failed&&(!h.acquisitions||h.releases==1),"SDK payload owner failed to stop or leaked state access");++stops;
    }
    std::cout<<"PASS SDK setters: "<<genuine<<" genuine whole calls, "<<declared<<" declared CPU cases, "<<store_count
      <<" ordered stores, "<<command_count<<" device words through both direct and driver operation paths, "<<stops
      <<" required-owner/FP stops. Declared state/device services; no live audio claim.\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<"\n";return 1;}
}
