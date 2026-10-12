#include "halo2/original_data.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace {
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
std::uint32_t number(const std::string &s) {return std::stoul(s,nullptr,16);}
std::array<std::byte,16> raw(const std::string &s) {
  check(s.size()==32,"Incomplete original descriptor");
  std::array<std::byte,16> out;
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(number(s.substr(i*2,2)));
  return out;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==5,"Supply original table fixture and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    check(!halo2::OriginalData::load_xbe(argv[2])->unit_action_tables,
          "Unverified base action tables invented");
    std::ifstream input(argv[1]);
    std::shared_ptr<const halo2::OriginalData> owned;
    std::array<bool,60> seen{};
    std::array<bool,3> seen_types{};
    std::size_t images=0,actions=0,types=0;
    auto release=[&] {
      if(!owned)return;
      for(bool value:seen)check(value,"Missing original action descriptor");
      for(bool value:seen_types)check(value,"Missing verified object descriptor");
      const auto &table=*owned->unit_action_tables;
      check(!table.base_extent(1) && !table.base_extent(3) && !table.base_extent(0xffffffffu),
            "Unverified object allocation type invented");
      bool rejected=false;
      try{(void)table.descriptor(60);}catch(const std::out_of_range &){rejected=true;}
      check(rejected,"Out-of-bank action descriptor invented");
      std::weak_ptr<const halo2::OriginalData> lifetime=owned;
      std::function<std::span<const std::byte>(std::uint32_t)> provider=
          [retained=owned](std::uint32_t type){return retained->unit_action_tables->descriptor(type);};
      const auto expected=table.descriptors[0];
      owned.reset();
      check(!lifetime.expired(),"Action provider lost its owned executable tables");
      check(std::equal(provider(0).begin(),provider(0).end(),expected.begin()),
            "Retained action provider changed descriptor");
      provider={};
      check(lifetime.expired(),"Action provider leaked owned executable tables");
    };
    for(std::string line;std::getline(input,line);) {
      std::istringstream row(line);char kind;row>>kind;
      if(kind=='M') {
        release();unsigned image;std::string action_bank,type_bank;
        check(bool(row>>image>>action_bank>>type_bank) && image==images+1 && image<=2,
              "Incomplete or unordered source image");
        owned=halo2::OriginalData::load_xbe(argv[image+2]);
        check(bool(owned->unit_action_tables),"Verified original action tables unavailable");
        const auto &actual=*owned->unit_action_tables;
        check(actual.action_bank==number(action_bank) && actual.object_type_bank==number(type_bank),
              "Original source bank identities differ");
        seen.fill(false);seen_types.fill(false);++images;
      } else if(kind=='A') {
        unsigned type;std::string address,bytes;
        check(owned && bool(row>>type>>address>>bytes) && type<seen.size() && !seen[type],
              "Incomplete or duplicate action record");
        const auto &actual=*owned->unit_action_tables;
        check(actual.descriptor_addresses[type]==number(address) && actual.descriptors[type]==raw(bytes),
              "Full original action descriptor differs");
        check(std::equal(actual.descriptor(type).begin(),actual.descriptor(type).end(),raw(bytes).begin()),
              "Action provider span differs");
        seen[type]=true;++actions;
      } else if(kind=='T') {
        unsigned type;std::string address,bytes;int extent;
        check(owned && bool(row>>type>>address>>bytes>>extent),"Incomplete original type record");
        const auto &actual=*owned->unit_action_tables;
        std::size_t index=0;
        while(index<actual.object_types.size() && actual.object_types[index].type!=type)++index;
        check(index<actual.object_types.size() && !seen_types[index],"Unverified or repeated object type");
        const auto &record=actual.object_types[index];
        check(record.source_address==number(address) && record.bytes==raw(bytes) &&
              actual.base_extent(type)==extent,"Original object record or signed base extent differs");
        seen_types[index]=true;++types;
      } else throw std::runtime_error("Unknown original action-table row");
    }
    release();
    check(images==2 && actions==120 && types==6,"Incomplete original table coverage");
    std::cout<<"supportedImages=2 fullActionRecords=120 fullTypeRecords=6 exactBytes=2016 "
                "retainedProviders=passed unavailableBaseAndTypes=passed liveConstruction=unverified\n";
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
