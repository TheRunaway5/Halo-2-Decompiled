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
  std::size_t definitions=0,tracks=0,references=0,negative=0,palette=0;
  const Tag* fixture=nullptr;
  for(const auto& tag:cache.select_tags({.group=tag_groups::sound_looping,.require_address=true})){
    const unsigned prefix=cache.is_mcc()?8:0;
    const Bytes root(cache.read_tag_bytes(tag.address,0x2c+prefix));
    const auto count=root.u32(prefix+0x1c),first=root.u32(prefix+0x20);
    require(count<=32,"Source schema track limit");
    const auto definition=cache.load_looping_sound(tag.datum);
    require(definition.name==tag.name && definition.flags==root.u32(prefix),"Root flags/name projection");
    require(definition.tracks.size()==count,"Source track count projection");
    require(cache.find_tag(tag.handle(),tag_groups::sound_looping)==&tag,"Full salted owner roundtrip");
    for(unsigned i=0;i<count;++i){
      const Bytes row(cache.read_tag_bytes(first+i*0x58,0x58));
      const auto& t=definition.tracks[i];
      require(t.flags==row.u32(4),"Track flags");
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
  }
  for(unsigned i=0;i<cache.bsps().size();++i){
    const auto audio=cache.load_multiplayer_audio_bsp(i);
    std::set<std::uint32_t> unique;
    for(const auto&p:audio.palette){if(p.sound!=null_datum)unique.insert(p.sound);if(p.inside_sound!=null_datum)unique.insert(p.inside_sound);}
    for(auto sound:unique){require(!cache.load_looping_sound(sound).name.empty(),"Actual ambience palette read");++palette;}
  }
  require(initial==fingerprint(cache.source_runtime_cache_bytes()),"Resident metadata changed by loader/fixtures");
  std::cout<<"PASS "<<cache.name()<<" mcc="<<cache.is_mcc()<<" definitions="<<definitions
    <<" tracks="<<tracks<<" references="<<references<<" palette="<<palette<<" negative="<<negative<<'\n';
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
