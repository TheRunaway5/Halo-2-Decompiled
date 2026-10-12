#include "halo2/source_mutable_tag_store.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/sound_promotion.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
template<class F>void rejects(F &&function) {
  bool stopped=false;try{function();}catch(const std::exception &){stopped=true;}
  require(stopped,"Unavailable mutable owner succeeded");
}
std::uint32_t word(const std::string &text) {return std::uint32_t(std::stoul(text,nullptr,16));}
std::vector<std::string> fields(const std::string &line) {
  std::istringstream input(line);std::vector<std::string> result;
  for(std::string field;std::getline(input,field,'\t');)result.push_back(field);
  return result;
}
std::vector<std::byte> decode(const std::string &text) {
  require(!(text.size()%2),"Incomplete original region");std::vector<std::byte> result(text.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(word(text.substr(i*2,2)));
  return result;
}
struct Region {
  std::uint32_t address;
  std::vector<std::byte> actual,expected;
  bool map_owned;
};
}
int main(int argc,char **argv) {
  try {
    require(argc==2,"Supply original promotion fixture");
    if(!std::filesystem::exists(argv[1]))return 77;
#if defined(__SSE__)
    const auto previous_mxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#endif
    std::ifstream input(argv[1]);std::string line;
    require(bool(std::getline(input,line))&&line=="SOUND_PROMOTION_NATIVE_V1","Wrong original schema");
    halo2::SoundPromotionConstants constants;
    halo2::SoundPromotionState state;
    halo2::SoundPromotionServices services;
    halo2::SourceMutableTagProviders tags;
    std::weak_ptr<const halo2::Cache> lifetime;
    std::shared_ptr<const halo2::Cache> cache;
    std::vector<Region> regions;
    std::vector<halo2::SoundStartupStore> expected;
    std::uint32_t sound=0,tick=0,returned=0,selector=0;
    bool clock=false,active=false;
    std::size_t maps=0,calls=0,writes=0,bytes=0,owned_bytes=0;
    auto memory=[&](std::uint32_t at,std::size_t extent)->std::span<std::byte> {
      Region *owner=nullptr;
      for(auto &region:regions)
        if(at>=region.address&&std::uint64_t(at)-region.address<=region.actual.size()&&
           extent<=region.actual.size()-(std::uint64_t(at)-region.address)) {
          require(!owner,"Ambiguous original input interval");owner=&region;
        }
      require(owner&&extent,"Unprovided original input interval");
      return owner->map_owned?tags.memory(at,extent):std::span(owner->actual).subspan(at-owner->address,extent);
    };
    services.memory=memory;services.tick_milliseconds=[&]{return tick;};
    for(;std::getline(input,line);) {
      const auto row=fields(line);if(row.empty())continue;
      if(row[0]=="K") {
        require(row.size()==4,"Incomplete original constants");constants={word(row[1]),word(row[2]),word(row[3])};
      }else if(row[0]=="M") {
        require(!active&&row.size()==7,"Incomplete original map");regions.clear();
        require(lifetime.expired(),"Released mutable map owner leaked cache");
        if(!std::filesystem::exists(row[2]))return 77;
        cache=std::make_shared<halo2::Cache>(row[2]);lifetime=cache;
        rejects([&]{halo2::SourceMutableTagStore invalid(cache,{});});
        rejects([&]{halo2::SourceMutableTagStore invalid(cache,{"sbsp"});});
        halo2::SourceMutableTagStore owner(cache,{"ugh!","snd!"});tags=owner.providers();
        for(std::size_t index=0;index<cache->tags().size();++index) {
          const auto &tag=cache->tags()[index];
          if(tag.address&&(tag.group=="ugh!"||tag.group=="snd!"))
            require(tags.address(tag.datum)==tag.address&&tags.index_address(std::uint16_t(index))==tag.address,
                    "Mutable payload lost actual table-slot identity");
        }
        state={word(row[3]),word(row[4]),word(row[5]),word(row[6])};
      }else if(row[0]=="R") {
        require(cache&&!active&&row.size()==3,"Incomplete original raw region");
        const auto at=word(row[1]);auto raw=decode(row[2]);bool owned=false;
        const auto found=std::ranges::find_if(cache->tags(),[&](const auto &tag) {
          return tag.address==at&&(tag.group=="ugh!"||tag.group=="snd!");
        });
        if(found!=cache->tags().end()) {
          require(std::ranges::equal(tags.memory(at,raw.size()),raw),
                  "Cache-owned serialized bank differs from original input");
          owned=true;owned_bytes+=raw.size();
        }
        regions.push_back({at,raw,std::move(raw),owned});
      }else if(row[0]=="C") {
        require(!active&&row.size()==6,"Incomplete original call");cache.reset();
        require(!lifetime.expired(),"Providers lost mutable cache ownership");active=true;expected.clear();
        clock=row[1]=="clock";sound=word(row[2]);tick=word(row[3]);returned=word(row[4]);selector=word(row[5]);
      }else if(row[0]=="S") {
        require(active&&row.size()==6,"Incomplete original store");
        expected.push_back({word(row[1]),word(row[2]),unsigned(std::stoul(row[3])),word(row[4]),word(row[5])});
      }else if(row[0]=="E") {
        require(active,"Unexpected source return");active=false;
        for(const auto &store:expected) {
          const auto found=std::ranges::find_if(regions,[&](const auto &region) {
            return store.address>=region.address&&std::uint64_t(store.address)+store.bytes<=
                   std::uint64_t(region.address)+region.expected.size();
          });
          require(found!=regions.end(),"Original store escapes inputs");
          const auto offset=store.address-found->address;std::uint32_t before=0;
          for(unsigned i=0;i<store.bytes;++i)before|=std::to_integer<std::uint32_t>(found->expected[offset+i])<<(8*i);
          require(before==store.before,"Original before bytes differ");
          for(unsigned i=0;i<store.bytes;++i)found->expected[offset+i]=std::byte(store.after>>(8*i));
        }
        std::vector<halo2::SoundStartupStore> actual;
        const auto observe=[&](const auto &store){actual.push_back(store);};
        if(clock)halo2::source_sound_promotion_clock(state,constants,services,observe);
        else require(std::uint32_t(halo2::source_sound_promote(state,sound,0xa400000,constants,services,observe))==returned,
                     "Original promotion return differs");
        require(actual==expected,"Original ordered mutable-bank stores differ");
        if(!clock) {
          const auto raw=memory(0xa400000,4);std::uint32_t output=0;
          for(unsigned i=0;i<4;++i)output|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
          require(output==selector,"Original promotion selector differs");
        }
        ++calls;writes+=actual.size();
      }else if(row[0]=="ENDMAP") {
        require(!active&&!cache,"Incomplete source map execution");
        for(const auto &region:regions) {
          require(std::ranges::equal(memory(region.address,region.actual.size()),region.expected),
                  "Complete original mutable map output differs");bytes+=region.expected.size();
        }
        {
          const auto original=lifetime.lock();
          require(bool(original),"Original cache backing unavailable");
          for(const auto &region:regions)if(region.map_owned) {
            const auto found=std::ranges::find_if(original->tags(),[&](const auto &tag) {
              return tag.address==region.address&&(tag.group=="ugh!"||tag.group=="snd!");
            });
            require(found!=original->tags().end(),"Original resident payload unavailable");
            const auto payload=original->load_source_tag_metadata(found->datum);
            require(std::ranges::equal(payload.bytes_at(region.address,region.actual.size()),region.actual),
                    "Mutable source writes modified the immutable original cache");
          }
        }
        rejects([&]{(void)tags.memory(0,1);});
        rejects([&]{(void)tags.memory(state.current_ugh,~std::size_t{});});
        rejects([&]{(void)tags.address(0xffffffffu);});
        rejects([&]{(void)tags.index_address(0xffffu);});
        tags={};require(lifetime.expired(),"Final mutable provider leaked cache");++maps;
      }else throw std::runtime_error("Unknown original fixture row");
    }
    require(!active&&maps==46&&calls==74318&&writes==249740&&bytes==19211104,
            "Incomplete original map-owner coverage");
#if defined(__SSE__)
    _mm_setcsr(previous_mxcsr);
#endif
    std::cout<<"ownedMapBanks="<<maps<<" sourceCalls="<<calls<<" orderedStores="<<writes
             <<" fullRegionBytes="<<bytes<<" importedMutableTagBytes="<<owned_bytes
             <<" persistentCountersAndProviderLifetime=passed liveAdmission=unverified\n";
    return 0;
  }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
