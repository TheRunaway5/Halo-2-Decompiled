#include "halo2/sound_driver_stream.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
using namespace halo2;
struct Region {std::uint32_t base;std::vector<std::byte> data;};
struct Image {std::uint32_t driver;SoundDriverStreamConstants constants;std::vector<Region> regions;};
struct Control {std::uint32_t address,before,after;unsigned bytes;};
struct Case {
  std::string name,image,operation,outcome;std::uint32_t category,index;
  SoundDriverStreamScratch scratch;
  std::vector<Control> controls;std::vector<SoundDriverStartupStore> stores;
  std::optional<SoundDriverStreamCreateRequest> request;
};
struct PlatformStop {};
void require(bool condition,const char *message) {
  if(!condition)throw std::runtime_error(message);
}
std::uint32_t hex(const std::string &text){return std::stoul(text,nullptr,16);}
std::vector<std::byte> bytes(const std::string &text) {
  require(text.size()%2==0,"Incomplete source bytes");std::vector<std::byte> result;
  for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(hex(text.substr(i,2))));
  return result;
}
std::span<std::byte> memory(std::vector<Region> &regions,std::uint32_t address,std::size_t size) {
  for(auto &region:regions)if(address>=region.base&&std::uint64_t(address)+size<=std::uint64_t(region.base)+region.data.size())
    return std::span(region.data).subspan(address-region.base,size);
  throw std::runtime_error("Actual source stream backing unavailable");
}
std::uint32_t word(std::span<const std::byte> data) {
  require(data.size()<=4,"Source scalar exceeds DWORD extent");
  std::uint32_t value=0;for(unsigned i=0;i<data.size();++i)value|=std::to_integer<std::uint32_t>(data[i])<<(8*i);return value;
}
void apply(std::vector<Region> &regions,const Control &store) {
  auto data=memory(regions,store.address,store.bytes);
  require(word(data)==store.before,"Original stream control/store input differs");
  for(unsigned i=0;i<store.bytes;++i)data[i]=std::byte(store.after>>(8*i));
}
}
int main(int argc,char **argv) {
  try {
    require(argc==2,"Supply original source stream fixture");std::ifstream input(argv[1]);
    require(bool(input),"Original source stream fixture unavailable");
    std::map<std::string,Image> images;std::vector<Case> cases;std::map<std::string,std::size_t> by_name;
    for(std::string line;std::getline(input,line);) {
      std::istringstream row(line);std::string kind,name,text;row>>kind>>name;
      if(kind=="state") {
        Image image{};row>>text;image.driver=hex(text);
        const std::array fields{&image.constants.one_bits,&image.constants.stereo_bits,
          &image.constants.pcm_queue,&image.constants.adpcm_queue,&image.constants.pcm_initialize_method,
          &image.constants.adpcm_initialize_method,&image.constants.completion_callback};
        for(auto field:fields){require(bool(row>>text),"Missing actual stream operand");*field=hex(text);}
        require(images.emplace(name,std::move(image)).second,"Duplicate source stream image");
      }else if(kind=="region") {
        std::string raw;row>>text>>raw;images.at(name).regions.push_back({hex(text),bytes(raw)});
      }else if(kind=="case") {
        Case c;c.name=name;std::string wave,descriptor,slot;
        row>>c.image>>c.operation>>c.category>>c.index>>wave>>descriptor>>slot>>c.outcome;
        require(bool(row),"Incomplete source stream case");c.scratch={hex(wave),hex(descriptor),hex(slot)};
        require(by_name.emplace(name,cases.size()).second,"Duplicate source stream case");cases.push_back(std::move(c));
      }else if(kind=="control") {
        auto &c=cases.at(by_name.at(name));Control store;std::string before,after;
        row>>text>>store.bytes>>before>>after;store.address=hex(text);store.before=hex(before);store.after=hex(after);c.controls.push_back(store);
      }else if(kind=="store") {
        auto &c=cases.at(by_name.at(name));SoundDriverStartupStore store;
        std::string address,before,after;row>>text>>address>>store.bytes>>before>>after;
        store.instruction=hex(text);store.address=hex(address);store.before=hex(before);store.after=hex(after);c.stores.push_back(store);
      }else if(kind=="request") {
        auto &c=cases.at(by_name.at(name));auto &request=c.request.emplace();
        const std::array fields{&request.device_identity,&request.descriptor_address,&request.output_address,&request.outer_identity};
        for(auto field:fields){row>>text;*field=hex(text);}
        std::string descriptor,format;row>>descriptor>>format;
        auto descriptor_bytes=bytes(descriptor),format_bytes=bytes(format);
        require(descriptor_bytes.size()==24&&format_bytes.size()==20,"Incomplete source stream platform request");
        for(unsigned i=0;i<6;++i)request.descriptor[i]=word(std::span(descriptor_bytes).subspan(4*i,4));
        std::copy(format_bytes.begin(),format_bytes.end(),request.format.begin());
      }else throw std::runtime_error("Unknown source stream fixture row");
      require(!(row>>text),"Unexpected source stream fixture field");
    }
    require(images.size()==2&&cases.size()==174,"Incomplete paired original stream coverage");
    std::size_t stores=0,whole=0,requests=0;
    for(const auto &c:cases) {
      try {
        const auto &image=images.at(c.image);auto actual=image.regions,expected=image.regions;
        for(const auto &store:c.controls){apply(actual,store);apply(expected,store);}
        for(const auto &store:c.stores)apply(expected,{store.address,store.before,store.after,store.bytes});
        std::vector<SoundDriverStartupStore> observed;bool stopped=false;unsigned calls=0;
        SoundDriverStreamServices services;
        services.memory=[&](auto address,auto size){return memory(actual,address,size);};
        services.native_create_stream=[&](const SoundDriverStreamCreateRequest &request)->std::int32_t {
          require(c.request.has_value(),"Unexpected native stream construction");++calls;
          const auto &original=*c.request;
          require(request.device_identity==original.device_identity&&request.descriptor_address==original.descriptor_address&&
                  request.output_address==original.output_address&&request.outer_identity==original.outer_identity&&
                  request.descriptor==original.descriptor&&request.format==original.format,
                  "Actual source stream platform request differs");
          throw PlatformStop();
        };
        const auto observe=[&](const SoundDriverStartupStore &store){observed.push_back(store);};
        try {
          if(c.operation=="format")source_sound_driver_stream_format(c.category,c.scratch.format,c.scratch.queue_slot,image.constants,services,observe);
          else if(c.operation=="stream")
            (void)source_sound_driver_stream_construct(image.driver,c.index,c.category,c.scratch,image.constants,services,observe);
          else throw std::runtime_error("Unknown source stream operation");
        }catch(const PlatformStop &){stopped=true;}
        require(stopped==(c.outcome=="platform_stop")&&calls==unsigned(c.request.has_value()),
                "Source stream platform boundary differs");
        require(observed==c.stores,"Ordered original stream stores differ");
        for(std::size_t i=0;i<actual.size();++i){require(actual[i].data==expected[i].data,"Whole original stream backing differs");whole+=actual[i].data.size();}
        stores+=observed.size();requests+=calls;
      }catch(const std::exception &error){throw std::runtime_error(c.name+": "+error.what());}
    }
    std::cout<<"PASS "<<cases.size()<<" original stream cases, "<<stores<<" ordered stores, "<<whole
             <<" full region bytes, "<<requests<<" exact create requests without results; no device/pause/playback claim.\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
