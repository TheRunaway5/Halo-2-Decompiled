#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
using namespace halo2;
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
template<class F> void unavailable(F action) {
  bool stopped = false;
  try { action(); } catch (const std::exception &) { stopped = true; }
  check(stopped, "Missing native sound storage was silently accepted");
}
std::uint32_t number(const std::string &value) {
  return std::uint32_t(std::stoul(value, nullptr, 16));
}
std::vector<std::byte> bytes(const std::string &value) {
  check(value.size()%2 == 0, "Incomplete original storage payload");
  std::vector<std::byte> output(value.size()/2);
  for (std::size_t i=0; i<output.size(); ++i) output[i]=std::byte(number(value.substr(2*i, 2)));
  return output;
}
std::vector<std::uint32_t> fields(const SoundCacheStartupState &state) {
  std::vector<std::uint32_t> output{state.general_allocator_identity,state.static_allocator_identity,
      state.entry_pool,state.page_allocator,state.physical_buffer,state.physical_arena_index};
  output.insert(output.end(),state.physical_lower_limits.begin(),state.physical_lower_limits.end());
  output.insert(output.end(),state.physical_upper_limits.begin(),state.physical_upper_limits.end());
  return output;
}
struct Region { std::uint32_t address; std::vector<std::byte> data; };
struct Stage { SoundCacheStartupState state; std::vector<Region> regions; };
struct Case {
  std::map<std::string,Stage> stages;
  std::vector<std::array<std::uint32_t,3>> protections;
  std::vector<SoundCacheStartupStore> stores;
  std::uint32_t first_alias=0;
};

void provider_checks() {
  // Explicit native alias controls. None of these are imported retail arena
  // fields or an owning game's allocator construction.
  NativeSoundStorage storage({0x11223344,0x10000000,0x10010000});
  const auto page=storage.alias_page_size();
  check(page==4096, "These retained original/native address controls require4KiB aliases");
  check(storage.page_size() && !(storage.page_size()&(storage.page_size()-1)),"Native host page size is invalid");
  auto services=storage.cache_services();
  storage.map_zeroed(0x20000000,1);
  check(storage.region_count()==1, "Explicit OS mapping missing");
  auto block=storage.memory(0x20000000,page);
  check(std::all_of(block.begin(),block.end(),[](auto b){return b==std::byte{};}), "Actual OS backing is not zeroed");
  block[17]=std::byte{0x53};
  check(services.memory(0x20000011,1)[0]==std::byte{0x53}, "Source alias failed to resolve actual owned backing");
  unavailable([&]{storage.map_zeroed(0x20000000,page);});
  unavailable([&]{storage.map_zeroed(0x20000001,page);});
  unavailable([&]{storage.map_zeroed(0xfffff000,2*page);});
  unavailable([&]{storage.memory(0x20000fff,2);});
  unavailable([&]{storage.memory(0xffffffff,2);});
  unavailable([&]{storage.memory(0x20000000,0);});
  unavailable([&]{services.raw_protect(0x30000000,page,4);});
  unavailable([&]{services.raw_protect(0x20000000,page,2);});
  unavailable([&]{services.raw_protect(0x20000001,page,4);});
  unavailable([&]{services.raw_protect(0x20000000,1,4);});
  unavailable([&]{services.raw_allocate(0x55667788,16);});
  check(storage.region_count()==1 && block[17]==std::byte{0x53}, "Rejected host requests changed source backing");
  services.raw_protect(0x20000000,page,4);
  block[18]=std::byte{0x76};
  check(services.memory(0x20000012,1)[0]==std::byte{0x76}, "Protected actual backing lost storage");

  // The private allocator skips explicit occupied backing rather than
  // aliasing or overwriting it. It does not infer a different source owner.
  storage.map_zeroed(0x10000000,2*page);
  const auto first=services.raw_allocate(0x11223344,17);
  const auto second=services.raw_allocate(0x11223344,page+1);
  check(first==0x10002000 && second==0x10003000 && storage.region_count()==4,
        "Actual raw allocation alias/rounded extent differs");
  check(std::all_of(services.memory(first,page).begin(),services.memory(first,page).end(),
                   [](auto b){return b==std::byte{};}), "Raw host allocation was not actually zeroed");
  services.memory(first+page-1,1)[0]=std::byte{0x7f};
  unavailable([&]{services.memory(first+page-1,2);});
  unavailable([&]{services.raw_allocate(0x11223344,0x20000);});
  check(services.memory(first+page-1,1)[0]==std::byte{0x7f}, "Alias exhaustion changed retained source bytes");
  unavailable([&]{NativeSoundStorage invalid({0,0x10000000,0x10010000});});
  unavailable([&]{NativeSoundStorage invalid({0x11223344,0,0x10000});});
  unavailable([&]{NativeSoundStorage invalid({0x11223344,0x10000001,0x10010000});});
  unavailable([&]{NativeSoundStorage invalid({0x11223344,0x10000000,0x10010001});});

  // All three callbacks retain real OS backing after the facade disappears.
  SoundCacheStartupServices retained;
  {
    NativeSoundStorage temporary({0xaabbccdd,0x30000000,0x30010000});
    temporary.map_zeroed(0x40000000,page);
    temporary.memory(0x40000031,1)[0]=std::byte{0xd5};
    retained=temporary.cache_services();
  }
  check(retained.memory(0x40000031,1)[0]==std::byte{0xd5}, "Callback lost actual backing owner");
  retained.raw_protect(0x40000000,page,4);
  check(retained.raw_allocate(0xaabbccdd,page)==0x30000000, "Retained callback lost actual allocator owner");
  retained={};
}

void host_granularity_checks() {
  // All three private aliases fit the same numeric 16 KiB interval. Each
  // still owns independent OS backing, including on a 16 KiB host.
  NativeSoundStorage storage({0x11223344,0x10001000,0x10009000});
  auto services=storage.cache_services();
  for (std::uint32_t i=1;i<=3;++i) {
    const auto address=0x20000000+i*4096;
    storage.map_zeroed(address,1);
    auto block=storage.memory(address,4096);
    check(std::all_of(block.begin(),block.end(),[](auto b){return b==std::byte{};}),
          "Independent host backing is not zeroed");
    block.front()=std::byte(i);block.back()=std::byte(0x30+i);
    services.raw_protect(address,4096,4);
  }
  unavailable([&]{services.memory(0x20000000,1);});
  unavailable([&]{services.memory(0x20001fff,2);});
  unavailable([&]{services.memory(0x20004000,1);});
  unavailable([&]{services.raw_protect(0x20001000,8192,4);});
  for (std::uint32_t i=1;i<=3;++i) {
    const auto block=services.memory(0x20000000+i*4096,4096);
    check(block.front()==std::byte(i) && block.back()==std::byte(0x30+i),
          "Subpage protection changed an independent alias");
  }
  const auto first=services.raw_allocate(0x11223344,0x208c);
  const auto second=services.raw_allocate(0x11223344,0x30fc);
  check(first==0x10001000 && second==0x10004000,
        "Private alias allocation changed with host granularity");
  services.memory(first,1)[0]=std::byte{0xa1};
  services.memory(first+0x2fff,1)[0]=std::byte{0xb2};
  services.memory(second,1)[0]=std::byte{0xc3};
  services.raw_protect(first+0x1000,0x1000,4);
  services.raw_protect(first+0x2000,0x1000,4);
  services.raw_protect(second+0x3000,0x1000,4);
  unavailable([&]{services.memory(first+0x2fff,2);});
  unavailable([&]{services.memory(second+0x4000,1);});
  check(services.memory(first,1)[0]==std::byte{0xa1} &&
        services.memory(first+0x2fff,1)[0]==std::byte{0xb2} &&
        services.memory(second,1)[0]==std::byte{0xc3},
        "Host-rounded protection changed source bytes");

  storage.map_zeroed(0xfffff000,1);
  services.raw_protect(0xfffff000,4096,4);
  services.memory(0xffffffff,1)[0]=std::byte{0xd4};
  unavailable([&]{services.memory(0xffffffff,2);});
  unavailable([&]{services.raw_protect(0xfffff000,8192,4);});
  check(services.memory(0xffffffff,1)[0]==std::byte{0xd4},
        "Last source-address page lost bounded backing");
}

struct ComparisonStats { std::size_t cases=0,bytes=0,stores=0; };
ComparisonStats original_comparison(const char *path) {
  std::ifstream input(path); check(bool(input), "Original cache constructor fixture missing");
  SoundCacheStartupConstants constants;
  std::map<std::string,Case> cases;
  for (std::string line;std::getline(input,line);) {
    if (line.empty() || line[0]=='#') continue;
    std::vector<std::string> row;
    std::stringstream stream(line);
    for (std::string field;std::getline(stream,field,'\t');) row.push_back(field);
    if (row[0]=="constants") constants={number(row[1]),number(row[2])};
    else if (row[0]=="case") cases.emplace(row[1],Case{});
    else if (row[0]=="stage") {
      check(row.size()==20,"Incomplete original cache state");
      SoundCacheStartupState state{number(row[3]),number(row[4]),number(row[5]),number(row[6]),
          number(row[7]),number(row[8]),{}, {}};
      for (unsigned i=0;i<5;++i) {
        state.physical_lower_limits[i]=number(row[9+i]);
        state.physical_upper_limits[i]=number(row[14+i]);
      }
      cases.at(row[1]).stages.emplace(row[2],Stage{state,{}});
    } else if (row[0]=="region")
      cases.at(row[1]).stages.at(row[2]).regions.push_back({number(row[3]),bytes(row[6])});
    else if (row[0]=="allocate") {
      auto &fixture=cases.at(row[1]);
      if (!fixture.first_alias) fixture.first_alias=number(row[4]);
    } else if (row[0]=="protect")
      cases.at(row[1]).protections.push_back({number(row[2]),number(row[3]),number(row[4])});
    else if (row[0]=="store")
      cases.at(row[1]).stores.push_back({number(row[2]),number(row[3]),unsigned(std::stoul(row[4])),
                                      number(row[5]),number(row[6])});
    else throw std::runtime_error("Unknown original cache fixture row");
  }
  check(cases.size()==6,"Incomplete original cache cases");
  std::size_t exact_bytes=0,exact_stores=0;
  for (const auto &[name,fixture]:cases) {
    auto state=fixture.stages.at("entry").state;
    NativeSoundStorage storage({state.general_allocator_identity,fixture.first_alias,
                               std::uint64_t(fixture.first_alias)+0x100000});
    for (const auto &request:fixture.protections) storage.map_zeroed(request[0],request[1]);
    const auto services=storage.cache_services();
    std::vector<SoundCacheStartupStore> writes;
    source_sound_cache_startup(state,constants,services,[&](const auto &write){writes.push_back(write);});
    const auto &expected=fixture.stages.at("returned");
    check(fields(state)==fields(expected.state),"Native host-backed constructor globals differ from original");
    check(storage.region_count()==2+fixture.protections.size(),"Real original raw allocation count differs");
    for (const auto &region:expected.regions) {
      const auto output=storage.memory(region.address,region.data.size());
      check(std::equal(output.begin(),output.end(),region.data.begin(),region.data.end()),
            "Whole original constructor output differs with native storage");
      exact_bytes+=output.size();
    }
    std::set<std::uint32_t> owners;
    for (const auto &write:writes) if (write.instruction) owners.insert(write.instruction);
    auto represented=[&](const auto &list) {
      std::vector<SoundCacheStartupStore> result;
      for (const auto &write:list) if (owners.contains(write.instruction)) result.push_back(write);
      return result;
    };
    const auto native=represented(writes);
    check(native==represented(fixture.stores),"Original ordered constructor writes differ with native storage");
    exact_stores+=native.size();

    if (!fixture.protections.empty()) {
      // Genuine missing physical backing stops after the original reservation
      // writes. The provider cannot publish a fake protected buffer.
      state=fixture.stages.at("entry").state;
      NativeSoundStorage absent({state.general_allocator_identity,fixture.first_alias,
                                std::uint64_t(fixture.first_alias)+0x100000});
      const auto missing=absent.cache_services();
      unavailable([&]{source_sound_cache_startup(state,constants,missing);});
      check(fields(state)==fields(fixture.stages.at("before_real_protection").state),
            "Missing real protection backing changed original stopped state");
    }
    std::cout<<"PASS original constructor with real native storage: "<<name<<'\n';
  }
  check(exact_bytes==172032 && exact_stores==4090,"Incomplete whole original cache constructor comparison");
  std::cout<<"originalCases=6 exactOutputBytes="<<exact_bytes<<" orderedOriginalStores="<<exact_stores
           <<" fixtureAllocationCallbacks=0 sourceDeviceMapAdmission=unverified\n";
  return {cases.size(),exact_bytes,exact_stores};
}
}

int main(int argc,char **argv) {
  int status=0;
  ComparisonStats stats;
  std::size_t host_page=0,alias_page=0;
  try {
    check(argc==2 || argc==3,"Supply original cache constructor fixture and optional bounded report");
    NativeSoundStorage storage({0x11223344,0x10001000,0x10009000});
    host_page=storage.page_size();alias_page=storage.alias_page_size();
    provider_checks();
    host_granularity_checks();
    stats=original_comparison(argv[1]);
    std::cout<<"PASS native allocation/protection/address bounds/callback ownership\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; status=1; }
  if (argc==3) {
    std::ofstream report(argv[2]);
    report<<"{\"passed\":"<<(status?"false":"true")<<",\"host_page_bytes\":"<<host_page
          <<",\"alias_page_bytes\":"<<alias_page<<",\"original_cases\":"<<stats.cases
          <<",\"exact_output_bytes\":"<<stats.bytes<<",\"ordered_original_stores\":"<<stats.stores
          <<",\"source_device_map_admission\":\"unverified\"}\n";
    if (!report) return 1;
  }
  return status;
}
