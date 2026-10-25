#include "halo2/content.hpp"
#include "halo2/multiplayer_audio.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <set>

namespace {
using namespace halo2;
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
std::uint64_t fingerprint(std::span<const std::byte> bytes){
  std::uint64_t h=14695981039346656037ull;
  for(auto b:bytes){h^=std::to_integer<unsigned char>(b);h*=1099511628211ull;}return h;
}
template<class F>void rejects(F f,const char* message){
  bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}require(rejected,message);
}
// A private Cache fixture is modified temporarily. Imported files are never
// written, and the complete resident source fingerprint must be restored.
struct RestoreWord {
  std::span<const std::byte> bytes;std::array<std::byte,4> old;
  RestoreWord(std::span<const std::byte>b,std::uint32_t value):bytes(b){
    require(b.size()==4,"Fixture word extent");std::copy(b.begin(),b.end(),old.begin());
    auto*p=const_cast<std::byte*>(b.data());for(unsigned i=0;i<4;++i)p[i]=std::byte(value>>(i*8));
  }
  ~RestoreWord(){std::copy(old.begin(),old.end(),const_cast<std::byte*>(bytes.data()));}
};
}
int main(int argc,char**argv)try{
  using namespace halo2;
  if(argc<2 || !*argv[1])return 77;
  Cache cache(argv[1],argc>2?argv[2]:"");
  const auto initial=fingerprint(cache.source_runtime_cache_bytes());
  std::size_t definitions=0,tracks=0,details=0,detail_only=0,references=0,negative=0,palette=0;
  const Tag* fixture=nullptr;
  for(const auto& tag:cache.select_tags({.group=tag_groups::sound_looping,.require_address=true})){
    const unsigned prefix=cache.is_mcc()?8:0;
    const Bytes root(cache.read_tag_bytes(tag.address,0x2c+prefix));
    const auto count=root.u32(prefix+0x1c),first=root.u32(prefix+0x20);
    require(count<=32,"Source schema track limit");
    const auto definition=cache.load_looping_sound(tag.datum);
    require(definition.name==tag.name && definition.flags==root.u32(prefix),"Root flags/name projection");
    require(definition.tracks.size()==count,"Source track count projection");
    [&]<class Definition>(const Definition &owned){
      if constexpr(requires{owned.music_time;owned.distance;owned.continuous_damage;}) {
        for(unsigned n=0;n<2;++n)require(std::bit_cast<std::uint32_t>(owned.music_time[n])==root.u32(prefix+4+n*4)&&
            std::bit_cast<std::uint32_t>(owned.distance[n])==root.u32(prefix+0xc+n*4),"Loop root scalar input bits");
        require(owned.continuous_damage==root.u32(prefix+0x18),"Continuous damage reference was not preserved");
      }
    }(definition);
    const auto detail_count=root.u32(prefix+0x24),detail_first=root.u32(prefix+0x28);
    if(!count && detail_count)++detail_only;
    [&]<class Definition>(const Definition &owned){
      if constexpr(requires{owned.details;}) {
        require(owned.details.size()==detail_count,"Resident looping detail sounds were omitted");
        for(unsigned i=0;i<detail_count;++i) {
          const Bytes row(cache.read_tag_bytes(detail_first+i*0x34,0x34));const auto &detail=owned.details[i];
          require(detail.name==row.u32(0)&&detail.sound==row.u32(8)&&detail.flags==row.u32(0x18),"Detail identity/flags");
          require(detail.unknown14_bits==row.u32(0x14),"Unknown detail scalar was not preserved");
          for(unsigned n=0;n<2;++n)require(
              std::bit_cast<std::uint32_t>(detail.period[n])==row.u32(0xc+n*4)&&
              std::bit_cast<std::uint32_t>(detail.yaw[n])==row.u32(0x1c+n*4)&&
              std::bit_cast<std::uint32_t>(detail.pitch[n])==row.u32(0x24+n*4)&&
              std::bit_cast<std::uint32_t>(detail.distance[n])==row.u32(0x2c+n*4),"Detail timing/spatial input bits");
          if(detail.sound!=null_datum){require(cache.find_tag({detail.sound},tag_groups::sound),"Detail full salted snd identity");++references;}
          ++details;
        }
      }else require(detail_count==0,"Resident looping detail sounds were omitted");
    }(definition);
    require(cache.find_tag(tag.handle(),tag_groups::sound_looping)==&tag,"Full salted owner roundtrip");
    for(unsigned i=0;i<count;++i){
      const Bytes row(cache.read_tag_bytes(first+i*0x58,0x58));
      const auto& t=definition.tracks[i];
      require(t.flags==row.u32(4),"Track flags");
      [&]<class Track>(const Track &owned){
        if constexpr(requires{owned.alternate_transition_in;}) {
          require(owned.name==row.u32(0)&&owned.output_effect==row.u16(0x3c)&&owned.unknown3e==row.u16(0x3e),"Track name/output input");
          require(owned.alternate_transition_in==row.u32(0x44)&&owned.alternate_transition_out==row.u32(0x4c),"Alternate transition references");
          require(std::bit_cast<std::uint32_t>(owned.alternate_crossfade)==row.u32(0x50)&&
              std::bit_cast<std::uint32_t>(owned.alternate_fade_out)==row.u32(0x54),"Alternate transition scalar bits");
        }
      }(t);
      const std::array<std::uint32_t,5> refs{t.start,t.loop,t.end,t.alternate,t.alternate_end};
      for(unsigned j=0;j<refs.size();++j){
        require(refs[j]==row.u32(0x18+j*8),"Track exact sound reference operand");
        if(refs[j]!=null_datum){
          require(cache.find_tag({refs[j]},tag_groups::sound),"Track full salted snd identity");++references;
        }
      }
      require(std::bit_cast<std::uint32_t>(t.gain_db)==row.u32(8) &&
              std::bit_cast<std::uint32_t>(t.fade_in)==row.u32(12) &&
              std::bit_cast<std::uint32_t>(t.fade_out)==row.u32(16),"Track float bits");
      ++tracks;
    }
    if(!fixture)fixture=&tag;
    ++definitions;
  }
  require(definitions!=0 && fixture,"Actual looping owners unavailable");
  rejects([&]{cache.load_looping_sound(fixture->datum^0x10000u);},"Stale salt accepted");++negative;
  for(const auto& tag:cache.tags())if(tag.address && tag.type()!=tag_groups::sound_looping){
    rejects([&]{cache.load_looping_sound(tag.datum);},"Foreign group accepted");++negative;break;
  }
  if(cache.is_mcc()){
    for(auto count:{33u,0xffffffffu}){
      RestoreWord restore(cache.read_tag_bytes(fixture->address+0x24,4),count);
      rejects([&]{cache.load_looping_sound(fixture->datum);},"Invalid track count accepted");++negative;
    }
    for(auto pointer:{0u,0xffffffffu}){
      RestoreWord count(cache.read_tag_bytes(fixture->address+0x24,4),1);
      RestoreWord address(cache.read_tag_bytes(fixture->address+0x28,4),pointer);
      rejects([&]{cache.load_looping_sound(fixture->datum);},"Foreign track extent accepted");++negative;
    }
    {
      RestoreWord count(cache.read_tag_bytes(fixture->address+0x2c,4),0xffffffffu);
      rejects([&]{cache.load_looping_sound(fixture->datum);},"Invalid detail count accepted");++negative;
    }
    for(auto pointer:{0u,0xffffffffu}){
      RestoreWord count(cache.read_tag_bytes(fixture->address+0x2c,4),1);
      RestoreWord address(cache.read_tag_bytes(fixture->address+0x30,4),pointer);
      rejects([&]{cache.load_looping_sound(fixture->datum);},"Foreign detail extent accepted");++negative;
    }
  }
  for(unsigned i=0;i<cache.bsps().size();++i){
    const auto audio=cache.load_multiplayer_audio_bsp(i);
    std::set<std::uint32_t> unique;
    for(const auto&p:audio.palette){if(p.sound!=null_datum)unique.insert(p.sound);if(p.inside_sound!=null_datum)unique.insert(p.inside_sound);}
    for(auto sound:unique){require(!cache.load_looping_sound(sound).name.empty(),"Actual ambience palette read");++palette;}
  }
  require(initial==fingerprint(cache.source_runtime_cache_bytes()),"Resident metadata changed by loader/fixtures");
  std::cout<<"PASS "<<cache.name()<<" mcc="<<cache.is_mcc()<<" definitions="<<definitions
    <<" tracks="<<tracks<<" details="<<details<<" detail_only="<<detail_only<<" references="<<references<<" palette="<<palette<<" negative="<<negative<<'\n';
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
