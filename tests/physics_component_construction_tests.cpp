#include "halo2/physics_component_construction.hpp"
#include <cfenv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <xmmintrin.h>
#endif

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &text){return std::stoul(text,nullptr,16);}
std::vector<std::byte> bytes(const std::string &text){
  require(text.size()%2==0,"Odd original component field length");std::vector<std::byte> result;
  for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return result;
}
struct Region {std::uint32_t address;std::vector<std::byte> data,expected;};
struct RequiredComponentRebuild {};
}
int main(int argc,char **argv){
  try{
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    halo2::SourcePhysicsComponentConstructionState state{};
    halo2::PhysicsComponentConstructionConstants constants{};
    halo2::PhysicsComponentConstructionFloatingPointContext context{};
    std::vector<Region> backing;std::vector<halo2::ObjectPhysicsConstructionStore> expected;
    std::vector<std::uint32_t> returns;unsigned entry=0,argument=0,object=0,context_slot=0,cases=0,calls=0,stores=0,prefixes=0,native_precision_status=0;std::size_t full=0;bool active=false;
    require(std::fesetround(FE_DOWNWARD)==0,"Host test rounding context unavailable");
    require(std::feraiseexcept(FE_INVALID|FE_INEXACT)==0,"Host test sticky context unavailable");
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string extra;require(bool(row>>kind),"Empty component-construction fixture row");
      if(kind=='K'){
        unsigned id;std::string target,arg,obj,pool,objects,counter,clock,scale,cw,mx;
        require(bool(row>>id>>target>>arg>>obj>>pool>>objects>>counter>>clock>>scale>>cw>>mx)&&!active&&id==cases+1,"Incorrect component case order");
        entry=word(target);argument=word(arg);object=word(obj);state={word(pool),word(objects),word(counter),word(clock)};
        constants={word(scale)};context={std::uint16_t(word(cw)),word(mx)};backing.clear();expected.clear();returns.clear();active=true;
      }else if(kind=='P'){
        unsigned id;std::string obj,pool,objects,counter,clock,flag,scale,cw,mx;
        require(bool(row>>id>>obj>>pool>>objects>>counter>>clock>>flag>>scale>>cw>>mx)&&!active&&id==cases+1,"Incorrect composed prefix order");
        entry=0x1c6f50;argument=object=word(obj);state={word(pool),word(objects),word(counter),word(clock)};context_slot=word(flag);
        constants={word(scale)};context={std::uint16_t(word(cw)),word(mx)};backing.clear();expected.clear();returns={0};active=true;
      }else if(kind=='R'){
        require(active&&returns.empty(),"Repeated original return vector");std::string value;
        while(row>>value)returns.push_back(word(value));
        row.clear();
        require(!returns.empty(),"Missing original return ABI");
      }else if(kind=='M'){
        std::string address,before,after;require(bool(row>>address>>before>>after)&&active,"Incomplete component backing");
        auto data=bytes(before),output=bytes(after);require(data.size()==output.size(),"Component backing extent changed");
        backing.push_back({word(address),std::move(data),std::move(output)});
      }else if(kind=='W'){
        std::string pc,address,before,after;unsigned size;
        require(bool(row>>pc>>address>>size>>before>>after)&&active,"Incomplete component owning store");
        expected.push_back({word(pc),word(address),word(before),word(after),size});
      }else if(kind=='E'){
        require(active&&!backing.empty()&&!returns.empty(),"Incomplete original component case");
        halo2::SourcePhysicsComponentConstructionServices services{[&](std::uint32_t address,std::size_t size)->std::span<std::byte>{
          for(auto &region:backing)if(address>=region.address&&std::uint64_t(address)-region.address<=region.data.size()&&size<=region.data.size()-(std::uint64_t(address)-region.address))
            return std::span(region.data).subspan(address-region.address,size);
          throw std::runtime_error("Component construction escaped genuine retained backing");
        }};
        std::vector<halo2::ObjectPhysicsConstructionStore> actual;const auto observe=[&](const auto &store){actual.push_back(store);};
        const auto rounding=std::fegetround(),flags=std::fetestexcept(FE_ALL_EXCEPT);
        const auto source_mxcsr_before=context.mxcsr;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
        const auto mxcsr=_mm_getcsr();std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
#endif
        for(const auto returned:returns){
          if(entry==0x16d2c0){
            const auto value=halo2::source_raw_datum_allocate(argument,{services.memory},[&](const auto &store){
              actual.push_back({store.instruction,store.address,store.before,store.after,store.bytes});
            });require(value==returned,"Original raw pool return datum differs");
          }else if(entry==0x1d2aa0){
            require(halo2::source_physics_component_allocate(state,argument,constants,context,services,observe)==returned,"Original component return datum differs");
          }else if(entry==0x1d3340){
            halo2::source_physics_component_initialize(state,argument,object,constants,context,services,observe);
          }else if(entry==0x1c6f50){
            unsigned allocations=0;bool stopped=false;
            halo2::SourceObjectPhysicsConstructionServices outer{services.memory,[&](const auto &call)->std::uint32_t{
              if(call.entry==0x1d2aa0){
                require(call.instruction==0x1c6fc7&&call.object==argument&&allocations++==0,"Original allocating callback ABI differs");
                return halo2::source_physics_component_allocate(state,argument,constants,context,services,observe);
              }
              require(call.instruction==0x1c6fd2&&call.entry==0x1d2b10&&call.component==0xe1680000u&&allocations==1,"Unexpected missing world rebuild boundary");
              throw RequiredComponentRebuild{};
            }};
            try{halo2::source_object_physics_construct({state.object_pool_slot,state.component_pool_slot,context_slot,state.counter_slot},argument,outer,observe);}
            catch(const RequiredComponentRebuild &){stopped=true;}
            require(stopped&&allocations==1,"Required complete physical rebuild did not stop");++prefixes;
          }else throw std::runtime_error("Unknown component-construction entry");
        }
        require(std::fegetround()==rounding&&std::fetestexcept(FE_ALL_EXCEPT)==flags,"Host floating environment changed");
        require((context.mxcsr&~0x3fu)==(source_mxcsr_before&~0x3fu)&&
            (context.mxcsr&source_mxcsr_before&0x3fu)==(source_mxcsr_before&0x3fu),"Source SSE controls/status were discarded");
        if(context.mxcsr&0x20u)++native_precision_status;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
        std::uint16_t after_control;asm volatile("fnstcw %0":"=m"(after_control));
        require(_mm_getcsr()==mxcsr&&after_control==control,"Host source FP controls leaked");
#endif
        require(actual==expected,"Original component/pool owning store sequence differs");
        for(const auto &region:backing){require(region.data==region.expected,"Full original component/pool backing differs");full+=region.data.size();}
        calls+=returns.size();stores+=actual.size();++cases;active=false;
      }else throw std::runtime_error("Unknown component fixture row");
      require(!(row>>extra),"Extra component fixture operand");
    }
    require(!active&&cases==511&&calls==1561&&stores==59789&&prefixes==1&&full==3775976&&native_precision_status>0,"Incomplete original component/pool coverage");
    std::cout<<"PASS component/pool construction: "<<cases<<" original cases and required stops, "<<calls<<" calls, "<<stores<<" stores, "<<full<<" full compared bytes, no lower replay, host FP context restored; native SSE status retained (emulator status excluded)\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
