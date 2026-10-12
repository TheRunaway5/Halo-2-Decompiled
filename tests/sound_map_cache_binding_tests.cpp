#include "halo2/native_sound_storage.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using namespace halo2;
void check(bool condition,const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t number(const std::string &value) { return std::stoul(value,nullptr,16); }
std::vector<std::byte> bytes(const std::string &raw) {
  check(!raw.empty() && raw.size()%2==0,"Incomplete original map owner payload");
  std::vector<std::byte> result(raw.size()/2);
  for (std::size_t i=0;i<result.size();++i) result[i]=std::byte(number(raw.substr(i*2,2)));
  return result;
}
std::array<std::uint32_t,8> fields(const SoundMapActivationState &s) {
  return {s.current_matg_address,s.sound_system_address,s.current_ugh_address,s.driver_address,
          s.environment_address,s.options_address,s.runtime_environment_parameters_address,s.queue_budget};
}
struct Boundary { std::uint32_t address;std::size_t extent; };
struct Region { std::uint32_t address;bool writable;std::vector<std::byte> data; };
struct Fixture {
  SoundMapActivationConstants constants;
  SoundMapActivationState entry,stopped;
  std::vector<Region> regions;
  std::vector<std::byte> output;
  std::vector<SoundMapActivationStore> stores;
  std::vector<std::uint32_t> clocks;
  std::uint32_t stop_address;std::size_t stop_extent;
};
Fixture fixture(const char *path) {
  std::ifstream input(path);check(bool(input),"Original map fixture missing");
  Fixture result;
  for (std::string line;std::getline(input,line);) {
    if (line.empty() || line[0]=='#') continue;
    std::vector<std::string> row;std::stringstream stream(line);
    for (std::string item;std::getline(stream,item,'\t');) row.push_back(item);
    if (row[0]=="constants") {result.constants={number(row[1]),number(row[2])};continue;}
    if (row.size()<2 || row[1]!="literal_lockout") continue;
    if (row[0]=="state") {
      check(row.size()==11,"Incomplete original map state");
      SoundMapActivationState state{number(row[3]),number(row[4]),number(row[5]),number(row[6]),
          number(row[7]),number(row[8]),number(row[9]),number(row[10])};
      if (row[2]=="entry") result.entry=state;
      else if (row[2]=="stopped") result.stopped=state;
      else throw std::runtime_error("Unexpected original map state phase");
    } else if (row[0]=="region") result.regions.push_back({number(row[2]),row[3]=="1",bytes(row[4])});
    else if (row[0]=="output") result.output=bytes(row[2]);
    else if (row[0]=="store") result.stores.push_back({number(row[2]),number(row[3]),unsigned(std::stoul(row[4])),
                                                     number(row[5]),number(row[6])});
    else if (row[0]=="clock") result.clocks.push_back(number(row[2]));
    else if (row[0]=="stop") {result.stop_address=number(row[3]);result.stop_extent=std::stoul(row[4]);}
  }
  check(result.regions.size()==3 && result.output.size()==0x23000 && result.clocks.size()==1,
        "Incomplete unchanged original map prefix fixture");
  return result;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==5,"Supply unchanged map fixture, owned Lockout, verified1.1 and u5 images");
    for (int i=1;i<argc;++i) if (!std::filesystem::exists(argv[i])) return 77;
    const auto original=fixture(argv[1]);
    auto cache=std::make_shared<Cache>(argv[2]);
    SourceTagMetadataStore store(cache,{"matg","sncl","snmx","ugh!"});
    check(store.tag_count()==4,"Actual sound map groups are incomplete");
    const auto metadata=store.providers();
    const auto tag_address=store.index_address_provider();
    for (const auto &region:original.regions) if (!region.writable) {
      const auto actual=metadata.read(region.address,region.data.size());
      check(std::equal(actual.begin(),actual.end(),region.data.begin(),region.data.end()),
            "Actual Cache metadata differs from unchanged original map input");
    }

    for (int image=3;image<5;++image) {
      const auto data=OriginalData::load_xbe(argv[image]);
      check(bool(data->sound_map_activation_constants),"Verified map operands unavailable");
      const auto &constants=*data->sound_map_activation_constants;
      check(constants.one_bits==original.constants.one_bits &&
            constants.ambient_transition_bits==original.constants.ambient_transition_bits,
            "Original map words differ from imported executable");
      // Only the actual previously source-created arena is a retained fixture
      // input. Metadata/lookup callbacks read the real owned Cache directly.
      // The allocator identity/alias namespace below are explicit native-only
      // controls; this test performs no source raw allocator request.
      NativeSoundStorage storage({0x11223344,0x10000000,0x10100000});
      for (const auto &region:original.regions) if (region.writable) {
        storage.map_zeroed(region.address,region.data.size());
        auto owned=storage.memory(region.address,region.data.size());
        std::copy(region.data.begin(),region.data.end(),owned.begin());
      }
      auto state=original.entry;
      std::vector<SoundMapActivationStore> writes;std::size_t clocks=0;
      SoundMapActivationServices services;
      services.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
        try { return storage.memory(address,extent); }
        catch (const std::out_of_range &) {}
        try { return metadata.read(address,extent); }
        catch (const FormatError &) { throw Boundary{address,extent}; }
      };
      services.writable=[&](std::uint32_t address,std::size_t extent) {
        try { return storage.memory(address,extent); }
        catch (const std::out_of_range &) { throw Boundary{address,extent}; }
      };
      services.tag_address=tag_address;
      services.tick_milliseconds=[&] {
        check(clocks<original.clocks.size(),"Extra original map clock request");
        return original.clocks[clocks++];
      };
      // No plugin/device callbacks exist. The genuine absent driver stops
      // earlier; an admitted platform return is never supplied by this test.
      bool stopped=false;
      try {source_sound_map_activation(state,constants,services,[&](const auto &write){writes.push_back(write);});}
      catch (const Boundary &boundary) {
        stopped=true;
        check(boundary.address==original.stop_address && boundary.extent==original.stop_extent,
              "Real Cache/storage composition stopped at a different boundary");
      }
      check(stopped && clocks==original.clocks.size() && fields(state)==fields(original.stopped) &&
            writes==original.stores,"Original complete retained map prefix differs with actual Cache/OS storage");
      const auto output=storage.memory(state.sound_system_address,original.output.size());
      check(std::equal(output.begin(),output.end(),original.output.begin(),original.output.end()),
            "Whole original arena output differs with actual Cache/OS storage");
      check(state.driver_address==0 && storage.memory(state.sound_system_address+0x78,1)[0]==std::byte{},
            "Map binding supplied driver readiness");
      std::cout<<"PASS actual Cache low16/matg/snmx/ugh reads + actual native arena; importedImage="<<image-2
               <<" exactArenaBytes="<<output.size()<<" orderedStores="<<writes.size()<<'\n';
    }
    std::cout<<"sourceOriginalExecutionImage=1.1 metadataAndTagLookupFixtureCallbacks=0 "
                 "retainedSourceStartupArenaAndClock=explicitInputs wholeMapAdmission=unverified\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
