#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F> void rejects(F &&f){
  bool rejected=false;
  try{f();}catch(const std::exception &){rejected=true;}
  check(rejected,"Unavailable source metadata replaced by a default");
}
std::uint32_t number(const std::string &s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> raw(const std::string &s){
  check(!s.empty() && s.size()%2==0,"Incomplete source payload");
  std::vector<std::byte> bytes(s.size()/2);
  for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(number(s.substr(i*2,2)));
  return bytes;
}
bool equal(std::span<const std::byte> actual,std::span<const std::byte> expected){
  return actual.size()==expected.size() && std::equal(actual.begin(),actual.end(),expected.begin());
}
}
int main(int argc,char **argv){
  try{
    check(argc==3,"Supply original complete tag fixture and repository root");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    rejects([]{halo2::SourceTagMetadataStore store(nullptr,{"hlmt"});});
    std::ifstream input(argv[1]);
    halo2::SourceTagMetadataProviders provider;
    std::weak_ptr<const halo2::Cache> lifetime;
    std::size_t maps=0,tags=0,bytes=0,queries=0,cross=0,expected_tags=0,map_tags=0;
    auto release=[&]{
      if(!maps)return;
      check(map_tags==expected_tags,"Incomplete source map tag coverage");
      rejects([&]{(void)provider.definition(0xffffffffu);});
      rejects([&]{(void)provider.address(0xffffffffu);});
      rejects([&]{(void)provider.read(0,1);});
      rejects([&]{(void)provider.read(0xffffffffu,1);});
      auto last=provider.read;provider={};
      check(!lifetime.expired(),"Last read callback lost original cache backing");
      last={};
      check(lifetime.expired(),"Released source metadata provider leaked cache");
    };
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;row>>kind;
      if(kind=='M'){
        release();std::string path;
        check(bool(row>>std::quoted(path)>>expected_tags),"Incomplete source map row");
        const auto map=std::filesystem::path(path).is_absolute()?std::filesystem::path(path):std::filesystem::path(argv[2])/path;
        if(!std::filesystem::exists(map))return 77;
        {
          auto cache=std::make_shared<halo2::Cache>(map);lifetime=cache;
          rejects([&]{halo2::SourceTagMetadataStore store(cache,{});});
          rejects([&]{halo2::SourceTagMetadataStore store(cache,{"sbsp"});});
          rejects([&]{(void)cache->load_source_tag_metadata(0xffffffffu);});
          halo2::SourceTagMetadataStore store(cache,{"bipd","hlmt","udlg"});
          check(store.tag_count()==expected_tags,"Original resident tag count differs");
          provider=store.providers();
          const auto outside=std::find_if(cache->tags().begin(),cache->tags().end(),[](const auto &tag){return tag.group=="scnr";});
          check(outside!=cache->tags().end(),"Original scenario tag unavailable");
          rejects([&]{(void)provider.definition(outside->datum);});
          cache.reset();
        }
        check(!lifetime.expired(),"Provider lost backing after cache/store facade destruction");
        map_tags=0;++maps;
      }else if(kind=='T'){
        std::string datum,group,address,hex;
        check(bool(row>>datum>>group>>address>>hex),"Incomplete full source tag record");
        const auto expected=raw(hex);const auto va=number(address),id=number(datum);
        const auto &actual=provider.definition(id);
        check(actual.datum()==id && actual.group()==group && actual.source_address()==va &&
              provider.address(id)==va && actual.size()==expected.size(),"Original tag identity/extent differs");
        check(equal(actual.bytes_at(va,expected.size()),expected) && equal(provider.read(va,expected.size()),expected),
              "Full original resident tag payload differs");
        check(actual.bytes_at(va+std::uint32_t(actual.size()),0).empty(),"Valid empty tag-end read rejected");
        rejects([&]{(void)actual.bytes_at(va-1,1);});
        rejects([&]{(void)actual.bytes_at(va+std::uint32_t(actual.size()),1);});
        rejects([&]{(void)provider.read(va,std::numeric_limits<std::size_t>::max());});
        ++tags;++map_tags;bytes+=expected.size();
      }else if(kind=='Q'){
        std::string datum,address,owner,hex;std::uint32_t count;
        check(bool(row>>datum>>count>>address>>owner>>hex),"Incomplete original model node bank");
        const auto &caller=provider.definition(number(datum));
        const halo2::Bytes header(caller.bytes_at(caller.source_address(),caller.size()));
        const auto expected=raw(hex);const auto va=number(address);
        check(caller.group()=="hlmt" && header.u32(0x78)==count && header.u32(0x7c)==va && expected.size()==std::size_t(count)*0x5c,
              "Original authored model node pointer/count differs");
        const auto &backing=provider.definition(number(owner));
        check(equal(backing.bytes_at(va,expected.size()),expected) && equal(provider.read(va,expected.size()),expected),
              "Original model forwarding pointer lost its real backing");
        ++queries;cross+=number(owner)!=number(datum);
      }else throw std::runtime_error("Unknown original metadata row");
    }
    release();check(maps==23 && tags && queries && cross,"Incomplete original resident coverage");
    std::cout<<"ownedMaps="<<maps<<" fullTagRecords="<<tags<<" exactBytes="<<bytes
             <<" modelNodeBanks="<<queries<<" crossTagBanks="<<cross
             <<" retainedProviders=passed boundsAndMissing=passed liveConstruction=unverified\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
