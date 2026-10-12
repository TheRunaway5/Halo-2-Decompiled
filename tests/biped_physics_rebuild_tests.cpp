#include "halo2/biped_physics_rebuild.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string&s){
  check(s.size()%2==0,"Original rebuild byte extent");std::vector<std::byte>v;
  for(std::size_t i=0;i<s.size();i+=2)v.push_back(std::byte(word(s.substr(i,2))));return v;
}
struct Memory:halo2::SourcePhysicsContactMemory{
  std::map<std::uint32_t,std::byte>data;
  void read(std::uint32_t at,std::span<std::byte>out)const override{
    for(unsigned i=0;i<out.size();++i)out[i]=data.at(at+i);
  }
  void write(std::uint32_t at,std::span<const std::byte>in)override{
    for(unsigned i=0;i<in.size();++i)data.at(at+i)=in[i];
  }
};
}
int main(int argc,char**argv){try{
  if(argc!=2)throw std::runtime_error("Supply original natural biped rebuild fixture");
  if(!std::filesystem::exists(argv[1]))return 77;
  Memory initial;std::map<std::uint32_t,std::vector<std::byte>>expected;
  std::vector<halo2::SourcePhysicsContactRecordStore>grouping,own;
  std::uint32_t component=0,result=0;std::ifstream input(argv[1]);
  for(std::string line;std::getline(input,line);){
    std::istringstream in(line);std::string kind,at,before,after,site;in>>kind;
    if(kind=="C"){in>>at>>after;component=word(at);result=word(after);}
    else if(kind=="M"||kind=="A"){
      in>>at>>after;const auto p=word(at);const auto b=bytes(after);
      if(kind=="M")for(unsigned i=0;i<b.size();++i)check(initial.data.emplace(p+i,b[i]).second,"Unique original source byte");
      else check(expected.emplace(p,b).second,"Unique original output span");
    }else if(kind=="S"){in>>at>>before>>after;grouping.push_back({0,word(at),bytes(before),bytes(after)});}
    else if(kind=="W"){in>>site>>at>>before>>after;own.push_back({word(site),word(at),bytes(before),bytes(after)});}
    else throw std::runtime_error("Unknown original biped rebuild record");
  }
  auto memory=initial;std::vector<halo2::SourcePhysicsContactRecordStore>actual;unsigned calls=0;
  halo2::SourceBipedPhysicsRebuildServices services;
  services.group=[&](auto input){
    check(input==component&&++calls==1,"Actual original grouping owner and order");
    // Independently executed required-owner side effects belong to this test
    // adapter alone. Production cannot manufacture a positive body bank.
    for(const auto&store:grouping){std::vector<std::byte>before(store.before.size());memory.read(store.address,before);
      check(before==store.before,"Retained source grouping store-before bytes");memory.write(store.address,store.written);}
    return std::uint8_t(result);
  };
  halo2::source_rebuild_biped_physics_1_1(memory,component,services,[&](const auto&store){actual.push_back(store);});
  check(calls==1&&actual.size()==own.size(),"Complete natural rebuild service/store sequence");
  for(unsigned i=0;i<actual.size();++i)check(actual[i].instruction==own[i].instruction&&actual[i].address==own[i].address&&
      actual[i].before==own[i].before&&actual[i].written==own[i].written,"Exact original biped rebuild store");
  std::size_t compared=0;
  for(const auto&[at,bytes]:expected){std::vector<std::byte>out(bytes.size());memory.read(at,out);
    check(out==bytes,"Complete original natural rebuild output");compared+=out.size();}
  bool stopped=false;memory=initial;
  try{halo2::source_rebuild_biped_physics_1_1(memory,component,{});}
  catch(const std::runtime_error&e){stopped=std::string(e.what())=="Required original biped rigid-body grouping owner unavailable";}
  check(stopped,"Natural positive path requires its genuine grouping owner");
  std::cout<<compared<<" natural biped rebuild bytes and "<<own.size()<<" native stores match; "
      <<grouping.size()<<" lower-owner stores independently replayed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
