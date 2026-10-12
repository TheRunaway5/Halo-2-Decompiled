#include "halo2/sound_driver_owners.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
void check(bool v,const std::string &s){if(!v)throw std::runtime_error(s);}
std::uint32_t number(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::byte> bytes(const std::string &s){check(s.size()%2==0,"odd source hex");std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(number(s.substr(2*i,2)));return b;}
struct Request {std::uint32_t device,output,outer;std::vector<std::byte> descriptor;};
struct Case {std::string image,operation,outcome;std::uint32_t index;std::vector<SoundDriverOwnersStore> controls,stores;std::optional<std::pair<std::uint32_t,std::size_t>> invalid;std::optional<Request> request;};
struct Image {SoundDriverOwnersState state;std::map<std::uint32_t,std::vector<std::byte>> regions;};
struct PlatformBoundary {};
void apply(std::map<std::uint32_t,std::vector<std::byte>> &regions,const SoundDriverOwnersStore &q){
 auto i=regions.upper_bound(q.address);check(i!=regions.begin(),"source write has no region");--i;auto &b=i->second;const auto at=q.address-i->first;check(std::uint64_t(at)+q.bytes<=b.size(),"source write crosses region");std::uint32_t before=0;for(unsigned k=0;k<q.bytes;++k)before|=std::to_integer<std::uint32_t>(b[at+k])<<(8*k);check(before==q.before,"recorded source before-write mismatch");for(unsigned k=0;k<q.bytes;++k)b[at+k]=std::byte(q.after>>(8*k));
}
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: sound_driver_owners_tests original-fixtures.tsv");std::ifstream f(argv[1]);check(bool(f),"source fixtures unavailable");std::map<std::string,Image> images;std::map<std::string,Case> cases;std::string line;
 while(std::getline(f,line)){
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> q;std::stringstream row(line);std::string word;while(std::getline(row,word,'\t'))q.push_back(word);
  if(q[0]=="state")images[q[1]].state={number(q[2]),number(q[3]),number(q[4])};
  else if(q[0]=="region")images[q[1]].regions.emplace(number(q[2]),bytes(q[3]));
  else if(q[0]=="case")cases.emplace(q[1],Case{q[2],q[3],q[5],std::uint32_t(std::stoul(q[4])),{},{},{},{}});
  else if(q[0]=="control")cases.at(q[1]).controls.push_back({0,number(q[2]),unsigned(std::stoul(q[3])),number(q[4]),number(q[5])});
  else if(q[0]=="store")cases.at(q[1]).stores.push_back({number(q[2]),number(q[3]),unsigned(std::stoul(q[4])),number(q[5]),number(q[6])});
  else if(q[0]=="invalid")cases.at(q[1]).invalid={{number(q[2]),std::size_t(std::stoul(q[3]))}};
  else if(q[0]=="request")cases.at(q[1]).request=Request{number(q[2]),number(q[3]),number(q[4]),bytes(q[5])};
  else throw std::runtime_error("unknown source fixture row");
 }
 std::size_t stores_count=0,returned=0,invalid=0,platform=0;
 for(const auto &[name,c]:cases){
  const auto &image=images.at(c.image);auto input=image.regions;for(const auto &q:c.controls)apply(input,q);auto expected=input;for(const auto &q:c.stores)apply(expected,q);
  // The identity comes from the real constructor's pool allocator+30. The
  // unused future alias interval is explicit private host mapping space;
  // neither value initializes a source voice, cache pin or source arena.
  const auto &pool=input.at(image.state.cache_address);std::uint32_t allocator=0;for(unsigned i=0;i<4;++i)allocator|=std::to_integer<std::uint32_t>(pool[0x30+i])<<(8*i);
  NativeSoundStorage storage({allocator,0x0a200000,0x0b000000});const auto page=storage.page_size();
  for(const auto &[a,b]:input){const auto start=std::uint32_t(a-a%page);const auto size=std::size_t(a-start)+b.size();storage.map_zeroed(start,size);auto target=storage.memory(a,b.size());std::copy(b.begin(),b.end(),target.begin());}
  SoundDriverOwnersServices services;std::optional<std::pair<std::uint32_t,std::size_t>> bad;std::vector<SoundDriverOwnersStore> stores;unsigned calls=0;
  services.memory=[&](std::uint32_t a,std::size_t n){try{return storage.memory(a,n);}catch(const std::out_of_range&){if(!bad)bad={{a,n}};throw;}};
  services.native_buffer_create=[&](const SoundDriverBufferCreateRequest &r)->std::int32_t{
   ++calls;check(bool(c.request),"unowned platform request supplied");const auto &q=*c.request;
   std::vector<std::byte> descriptor;for(const auto v:r.descriptor)for(unsigned j=0;j<4;++j)descriptor.push_back(std::byte(v>>(8*j)));
   check(r.device_identity==q.device&&r.output_address==q.output&&r.outer_identity==q.outer&&descriptor==q.descriptor,name+" exact original buffer request differs");throw PlatformBoundary{};
  };
  std::string outcome="returned";
  try{
   auto observe=[&](const auto &q){stores.push_back(q);};const auto record=image.state.driver_address+0xc;
   if(c.operation=="flush")source_sound_driver_stream_flush(image.state,services,observe);
   else if(c.operation=="shutdown")source_sound_driver_queue_shutdown(image.state,services,observe);
   else if(c.operation=="release")source_sound_driver_release_packets(image.state,record,services,observe);
   else if(c.operation=="complete")source_sound_driver_complete_packet(image.state,record,0,services,observe);
   else if(c.operation=="static")source_sound_driver_static_buffer_construct(image.state,c.index,services,observe);
   else throw std::runtime_error("unknown owner operation");
  }catch(const PlatformBoundary&){outcome="platform_stop";}catch(const std::out_of_range&){outcome="invalid_storage";}
  check(outcome==c.outcome&&bad==c.invalid,name+" original return/storage boundary differs");check(stores==c.stores,name+" every ordered original write differs");check(calls==(c.request?1u:0u),"source platform boundary bypassed");
  for(const auto &[a,b]:expected){auto actual=storage.memory(a,b.size());check(std::equal(b.begin(),b.end(),actual.begin()),name+" complete source regions differ");}
  stores_count+=stores.size();returned+=outcome=="returned";invalid+=outcome=="invalid_storage";platform+=outcome=="platform_stop";
 }
 check(cases.size()==1138&&returned==328&&invalid==708&&platform==102&&stores_count==514,"source coverage changed");
 std::cout<<"PASS complete source flush/queue/packet/static-buffer CPU owners: "<<cases.size()<<" cases, "<<stores_count<<" exact ordered writes, "<<returned<<" natural/control returns, "<<invalid<<" unallocated storage stops, "<<platform<<" exact create-buffer requests; real NativeSoundStorage/full regions; no voice/device/DSP/platform return supplied\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
