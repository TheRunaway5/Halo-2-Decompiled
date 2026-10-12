#include "halo2/object_model_resources.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &text){return std::stoul(text,nullptr,16);}
std::vector<std::byte> bytes(const std::string &text){
  require(text.size()%2==0,"Odd raw model-resource length");std::vector<std::byte> out;
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
struct Region {std::uint32_t address;std::vector<std::byte> data,expected;};
}
int main(int argc,char **argv){
  try{
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    halo2::SourceObjectModelResourceState state{};std::uint32_t object=0,mode=0,collision=0;
    unsigned expected_al=0,cases=0,stores=0;std::size_t full=0;bool active=false;
    std::vector<Region> regions;std::vector<halo2::SourceObjectModelResourceStore> expected;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string text;require(bool(row>>kind),"Empty model-resource row");
      if(kind=='K'){
        unsigned id,image;std::string pool,table,obj,out_mode,out_collision;
        require(bool(row>>id>>image>>pool>>table>>obj>>out_mode>>out_collision>>expected_al)&&!active,"Incomplete original model-resource case");
        require(id==cases+1&&(image==1||image==2),"Original case ordering differs");
        state={word(pool),word(table)};object=word(obj);mode=word(out_mode);collision=word(out_collision);
        regions.clear();expected.clear();active=true;
      }else if(kind=='M'){
        std::string address,before,after;require(bool(row>>address>>before>>after)&&active,"Incomplete original backing");
        auto source=bytes(before),output=bytes(after);require(source.size()==output.size(),"Backing extent changed");
        regions.push_back({word(address),std::move(source),std::move(output)});
      }else if(kind=='W'){
        std::string pc,address,before,after;require(bool(row>>pc>>address>>before>>after)&&active,"Incomplete source store");
        expected.push_back({word(pc),word(address),word(before),word(after)});
      }else if(kind=='E'){
        require(active&&!regions.empty(),"Missing source-owned input");
        halo2::SourceObjectModelResourceServices services{[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
          for(auto &region:regions)if(address>=region.address&&std::uint64_t(address)-region.address<=region.data.size()&&
              size<=region.data.size()-(std::uint64_t(address)-region.address))
            return std::span(region.data).subspan(address-region.address,size);
          throw std::runtime_error("Model-resource CPU read escaped retained owning fields");
        }};
        std::vector<halo2::SourceObjectModelResourceStore> actual;
        const auto result=halo2::source_object_model_resources(state,object,mode,collision,services,
            [&](const auto &store){actual.push_back(store);});
        require(result==(expected_al!=0),"Original defined AL differs");require(actual==expected,"Original ordered output stores differ");
        for(const auto &region:regions){require(region.data==region.expected,"Full retained backing differs");full+=region.data.size();}
        ++cases;stores+=actual.size();active=false;
      }else throw std::runtime_error("Unknown model-resource fixture row");
      require(!(row>>text),"Unexpected model-resource fixture operand");
    }
    require(!active&&cases==60&&stores==96&&full==21120,"Incomplete original model-resource coverage");
    std::cout<<"PASS model resource query: "<<cases<<" original cases, "<<stores<<" stores, "<<full<<" full compared bytes\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
