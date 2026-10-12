#include "halo2/original_data.hpp"
#include "halo2/sound_metadata_store.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F &&f) {
  bool stopped=false;
  try {f();} catch(const halo2::FormatError &) {stopped=true;}
  check(stopped,"Unavailable source input was replaced by a default");
}
std::uint32_t number(const std::string &text) {return std::stoul(text,nullptr,16);}
template<std::size_t N> std::array<std::byte,N> bytes(const std::string &text) {
  check(text.size()==N*2,"Incomplete original byte record");
  std::array<std::byte,N> out;
  for(std::size_t n=0;n<N;++n)out[n]=std::byte(number(text.substr(n*2,2)));
  return out;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==6,"Supply sound metadata fixture, root and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    rejects([]{halo2::SoundMetadataStore absent(nullptr);});
    std::ifstream input(argv[1]);
    halo2::SourceSoundMetadataProviders provider;
    std::weak_ptr<const halo2::Cache> lifetime;
    std::size_t maps=0,sounds=0,classes=0,expected_sounds=0,expected_classes=0,map_sounds=0,map_classes=0;
    auto release=[&] {
      if(!maps)return;
      check(map_sounds==expected_sounds && map_classes==expected_classes,"Incomplete original map records");
      rejects([&]{(void)provider.definition(0xffffffffu);});
      rejects([&]{(void)provider.sound_class(-1);});
      if(expected_classes<=127)rejects([&]{(void)provider.sound_class(std::int8_t(expected_classes));});
      provider={};
      check(lifetime.expired(),"Released source providers leaked map backing");
    };
    for(std::string line;std::getline(input,line);) {
      std::istringstream row(line);std::string kind;row>>kind;
      if(kind=="M") {
        release();std::string path;
        check(bool(row>>std::quoted(path)>>expected_sounds>>expected_classes),"Incomplete original map row");
        const auto file=std::filesystem::path(path).is_absolute()?std::filesystem::path(path):std::filesystem::path(argv[2])/path;
        if(!std::filesystem::exists(file))return 77;
        {
          auto cache=std::make_shared<halo2::Cache>(file);lifetime=cache;
          halo2::SoundMetadataStore owned(cache);
          check(owned.sound_count()==expected_sounds && owned.class_count()==expected_classes,"Original resident record counts differ");
          provider=owned.providers();cache.reset();
        }
        check(!lifetime.expired(),"Source callback lost cache after facade destruction");
        map_sounds=map_classes=0;++maps;
      } else if(kind=="S") {
        std::string datum,raw;check(bool(row>>datum>>raw),"Incomplete original sound row");
        const auto expected=bytes<20>(raw);auto copy=provider.definition(number(datum));
        check(copy==expected,"Original full sound definition differs");
        copy[0]^=std::byte{1};
        check(provider.definition(number(datum))==expected,"Caller copy changed owned sound metadata");
        ++sounds;++map_sounds;
      } else if(kind=="C") {
        int index;std::string raw;check(bool(row>>index>>raw) && index>=0 && index<=127,"Incomplete original class row");
        check(provider.sound_class(std::int8_t(index))==bytes<92>(raw),"Original full class definition differs");
        ++classes;++map_classes;
      } else if(kind=="V") {
        int image;std::string position,forward,ordinary,special;
        check(bool(row>>image>>position>>forward>>ordinary>>special) && (image==1 || image==2),"Incomplete original immutable operands");
        const auto imported=halo2::OriginalData::load_xbe(argv[image+3]);
        check(bool(imported->first_person_sound_constants),"Verified sound operands unavailable");
        const auto &actual=*imported->first_person_sound_constants;
        check(actual.marker_position==bytes<12>(position) && actual.marker_forward==bytes<12>(forward) &&
              actual.ordinary_callbacks==number(ordinary) && actual.weapon_callbacks==number(special),"Immutable source marker/callback identity differs");
      } else throw std::runtime_error("Unknown original metadata record");
    }
    release();
    check(!halo2::OriginalData::load_xbe(argv[3])->first_person_sound_constants,"Unverified base sound constants invented");
    check(maps && sounds && classes,"No original metadata coverage");
    std::cout<<"ownedMaps="<<maps<<" fullSoundDefinitions="<<sounds<<" fullClassRecords="<<classes
             <<" providerLifetime=passed callerCopies=passed supportedImages=2 unavailableBase=passed liveAudio=unverified\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
