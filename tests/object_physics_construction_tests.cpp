#include "halo2/object_physics_construction.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &text){return std::stoul(text,nullptr,16);}
std::vector<std::byte> bytes(const std::string &text){
  require(text.size()%2==0,"Odd original field bytes");std::vector<std::byte> result;
  for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return result;
}
struct Region {
  std::uint32_t address;std::vector<std::byte> data;
  friend bool operator==(const Region &,const Region &)=default;
};
struct LowerBoundary {
  halo2::ObjectPhysicsConstructionCall call;unsigned before,after;std::uint32_t result;
};
}
int main(int argc,char **argv){
  try{
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    std::vector<std::vector<Region>> states;std::vector<halo2::ObjectPhysicsConstructionStore> expected;
    std::vector<LowerBoundary> lower;unsigned current_state=0,entry=0,first=0,second=0,request=0;
    unsigned in_state=0,out_state=0,result=0,cases=0,stores=0,lower_count=0;std::size_t full=0;
    halo2::SourceObjectPhysicsConstructionState state{};bool reading_state=false,reading_case=false;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string extra;require(bool(row>>kind),"Empty physical-construction row");
      if(kind=='S'){
        require(bool(row>>current_state)&&!reading_state&&!reading_case&&current_state==states.size(),"Incorrect original state order");
        states.emplace_back();reading_state=true;
      }else if(kind=='M'){
        std::string address,raw;require(bool(row>>address>>raw)&&reading_state,"Original memory outside state");
        states.back().push_back({word(address),bytes(raw)});
      }else if(kind=='K'){
        unsigned id;std::string target,a,b,c,pool,components,context,counter,returned;
        require(bool(row>>id>>target>>a>>b>>c>>pool>>components>>context>>counter>>in_state>>out_state>>returned)&&!reading_state&&!reading_case&&id==cases+1,"Incorrect original case order");
        require(in_state<states.size()&&out_state<states.size(),"Missing original case states");
        entry=word(target);first=word(a);second=word(b);request=word(c);result=word(returned);
        state={word(pool),word(components),word(context),word(counter)};expected.clear();lower.clear();reading_case=true;
      }else if(kind=='L'){
        std::string pc,target,obj,component,returned;unsigned before,after;
        require(bool(row>>pc>>target>>obj>>component>>before>>after>>returned)&&reading_case,"Incomplete labeled lower boundary");
        require(before<states.size()&&after<states.size(),"Missing lower boundary state");
        lower.push_back({{word(pc),word(target),word(obj),word(component)},before,after,word(returned)});
      }else if(kind=='W'){
        std::string pc,address,before,after;unsigned count;
        require(bool(row>>pc>>address>>count>>before>>after)&&reading_case,"Incomplete owning store");
        expected.push_back({word(pc),word(address),word(before),word(after),count});
      }else if(kind=='E'){
        if(reading_state){require(!states.back().empty(),"Empty original state");reading_state=false;}
        else{
          require(reading_case,"Unexpected case end");auto backing=states[in_state];std::size_t invoked=0;
          halo2::SourceObjectPhysicsConstructionServices services;
          services.memory=[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
            for(auto &region:backing)if(address>=region.address&&std::uint64_t(address)-region.address<=region.data.size()&&size<=region.data.size()-(std::uint64_t(address)-region.address))
              return std::span(region.data).subspan(address-region.address,size);
            throw std::runtime_error("Physical-construction read escaped actual retained backing");
          };
          // Test-only replay of five actual lower boundary results/memory.
          // This verifies the upper caller; it is never a runtime provider.
          services.invoke=[&](const auto &call){
            require(invoked<lower.size(),"Unexpected missing complete lower owner");const auto &boundary=lower[invoked++];
            require(call==boundary.call,"Actual lower call ABI/order differs");
            require(backing==states[boundary.before],"Full backing before lower boundary differs");
            backing=states[boundary.after];return boundary.result;
          };
          std::vector<halo2::ObjectPhysicsConstructionStore> actual;
          const auto observe=[&](const auto &store){actual.push_back(store);};
          switch(entry){
          case 0x1c7050:halo2::source_object_physics_construct_tree(state,first,second,services,observe);break;
          case 0x1c6f50:halo2::source_object_physics_construct(state,first,services,observe);break;
          case 0x1c72e0:halo2::source_object_physics_mark(state,first,services,observe);break;
          case 0xc0d10:halo2::source_object_physics_word_bit(first,second,request,services,observe);break;
          case 0xbbe70:require(halo2::source_object_physics_root(state,first,services)==result,"Original root-chain defined EAX differs");break;
          default:throw std::runtime_error("Unknown physical-construction owner");
          }
          require(invoked==lower.size(),"Expected lower boundary omitted");require(actual==expected,"Owning store sequence differs");
          require(backing==states[out_state],"Full retained original output differs");
          for(const auto &region:backing)full+=region.data.size();
          stores+=actual.size();lower_count+=invoked;++cases;reading_case=false;
        }
      }else throw std::runtime_error("Unknown physical-construction fixture row");
      require(!(row>>extra),"Extra physical-construction operand");
    }
    require(!reading_state&&!reading_case&&cases==133&&stores==147&&lower_count==5&&full==4556774,"Incomplete physical-construction owner coverage");
    std::cout<<"PASS object physical construction: "<<cases<<" original cases, "<<stores<<" stores, "<<full<<" full compared bytes, "<<lower_count<<" explicitly replayed lower boundaries for upper-caller-only case\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
