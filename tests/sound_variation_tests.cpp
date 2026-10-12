#include "halo2/sound_variation.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
using Row=std::vector<std::string>;
std::uint32_t word(const std::string &s,unsigned base=16) {return std::stoul(s,nullptr,base);}
void check(bool value,const std::string &context) {if(!value)throw FormatError("Original sound variation differs: "+context);}
std::vector<std::byte> bytes(const std::string &s) {
  if(s.size()%2)throw FormatError("Odd fixture hex length");
  std::vector<std::byte> out;for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(word(s.substr(i,2))));return out;
}
template<class T>void same_bytes(const T &actual,const std::string &expected,const std::string &context) {
  const auto raw=bytes(expected);check(actual.size()==raw.size() && std::equal(actual.begin(),actual.end(),raw.begin()),context);
}
void same_state(const SoundRangeState &actual,const std::string &range,const std::string &used,const std::string &context) {
  const auto r=bytes(range),u=bytes(used);
  check(actual.last==std::bit_cast<std::int8_t>(r.at(4)) && actual.forced==std::bit_cast<std::int8_t>(r.at(5)),context+" last/forced");
  std::uint32_t bank=0;for(unsigned i=0;i<u.size();++i)bank|=std::to_integer<std::uint32_t>(u[i])<<(8*i);
  check(actual.used==bank,context+" used bank");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply project and original sound variation fixture");
    const std::filesystem::path root(argv[1]),map=root/"assets/disc/maps/lockout.map";
    if(!std::filesystem::exists(map) || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(map);std::vector<Row> calls,metadata;std::map<std::string,std::vector<Row>> stores;
    std::ifstream file(argv[2]);
    for(std::string line;std::getline(file,line);) {
      std::istringstream stream(line);Row r;for(std::string s;stream>>s;)r.push_back(s);
      if(r.empty())continue;
      if(r[0]=="M")metadata.push_back(r);
      else if(r[0]=="S" || r[0]=="C")calls.push_back(r);
      else if(r[0]=="R")stores[r.at(1)].push_back(r);
      else throw FormatError("Unknown sound fixture row");
    }
    unsigned imports=0,permutations=0,selections=0,creations=0,draws=0;
    for(const auto &m:metadata) {
      const auto d=cache.load_sound_variation(word(m.at(1)),word(m.at(2),10));
      same_bytes(d.sound,m.at(3),"snd definition");same_bytes(d.playback,m.at(4),"playback parameters");
      same_bytes(d.sound_class,m.at(5),"source sound class");same_bytes(d.range,m.at(6),"resident pitch range");
      std::istringstream p(m.at(7));unsigned i=0;
      for(std::string raw;std::getline(p,raw,',');)same_bytes(d.permutations.at(i++),raw,"permutation metadata");
      check(i==d.permutations.size(),"complete permutation import");++imports;permutations+=i;
    }
    struct Chain {
      SoundRangeState state;
      SoundLocalRandom random;
      Chain(SoundRangeState s,std::uint32_t seed):state(s),random(seed) {}
    };
    std::map<std::uint32_t,Chain> chains;
    for(const auto &c:calls) {
      const auto &id=c.at(1);const auto datum=word(c.at(2));const auto d=cache.load_sound_variation(datum);
      const auto expected=stores[id];std::vector<SoundRngStore> actual;
      if(c[0]=="S") {
        if(word(c.at(3),10)==0)check(chains.try_emplace(datum,imported_sound_range_state(d),word(c.at(4))).second,id+" initial chain");
        auto &chain=chains.at(datum);same_state(chain.state,c.at(5),c.at(6),id+" before");
        check(chain.random.seed==word(c.at(4)),id+" before seed");chain.random.stores.clear();
        check(source_sound_permutation(d,chain.state,chain.random)==std::int16_t(word(c.at(7),10)),id+" selected permutation");
        same_state(chain.state,c.at(9),c.at(10),id+" after");check(chain.random.seed==word(c.at(8)),id+" after seed");
        actual=chain.random.stores;++selections;
      }else {
        SoundLocalRandom random(word(c.at(3)));auto state=imported_sound_range_state(d);
        check(std::bit_cast<std::uint32_t>(source_sound_gain(d,random))==word(c.at(4)),id+" source+94 gain");
        check(std::bit_cast<std::uint32_t>(float(source_sound_pitch(d,random)))==word(c.at(5)),id+" source+90 pitch");
        check(random.seed==word(c.at(6)),id+" source+98 seed snapshot");
        check(source_sound_permutation(d,state,random)==std::int16_t(word(c.at(7),10)),id+" source+9D selection");
        check(random.seed==word(c.at(8)),id+" final local seed");actual=random.stores;++creations;
      }
      check(actual.size()==expected.size(),id+" random draw count");
      for(unsigned i=0;i<actual.size();++i) {
        const auto &r=expected[i];check(actual[i].instruction==word(r.at(2)) && actual[i].before==word(r.at(3)) && actual[i].after==word(r.at(4)),id+" ordered LCG store "+std::to_string(i));
      }
      draws+=actual.size();
    }
    check(imports==17 && permutations==23 && selections==32 && creations==3 && draws==83,"complete original suite");
    std::cout<<imports<<" authored sound imports / "<<permutations<<" permutations; "<<selections<<" original selector calls; "<<creations<<" gain/pitch/source selection cases; "<<draws<<" exact ordered LCG stores matched\n";
  }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
