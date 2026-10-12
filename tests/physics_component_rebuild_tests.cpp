#include "halo2/physics_component_rebuild.hpp"
#include "halo2/physics_component_construction.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
void require(bool v,const char *message){if(!v)throw std::runtime_error(message);}
std::uint32_t word(const std::string &s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string &s){
  require(s.size()%2==0,"Odd original rebuild backing");std::vector<std::byte> b;
  for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
  return b;
}
struct Region {std::uint32_t address;std::vector<std::byte> data;friend bool operator==(const Region &,const Region &)=default;};
struct Lower {halo2::PhysicsComponentRebuildCall call;unsigned before,after;std::uint32_t result;};
struct RequiredLower {};
}
int main(int argc,char **argv){
  try {
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    std::vector<std::vector<Region>> states;std::vector<Lower> lower;
    std::vector<halo2::ObjectPhysicsConstructionStore> expected;
    halo2::SourcePhysicsComponentRebuildState state{};halo2::PhysicsComponentRebuildBranches branches{};
    halo2::PhysicsComponentConstructionFloatingPointContext context{};
    unsigned datum=0,in_state=0,out_state=0,counter=0,clock=0,flag=0,scale=0,mode=0;
    unsigned cases=0,stores=0,replayed=0,stops=0,composed=0;std::size_t full=0;
    bool reading_state=false,reading_case=false,summary=false;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string extra;require(bool(row>>kind),"Empty original rebuild row");
      if(kind=='S'){
        unsigned id;require(bool(row>>id)&&!reading_state&&!reading_case&&id==states.size(),"Incorrect original rebuild state order");
        states.emplace_back();reading_state=true;
      }else if(kind=='M'){
        std::string p,b;require(bool(row>>p>>b)&&reading_state,"Memory outside original rebuild state");states.back().push_back({word(p),bytes(b)});
      }else if(kind=='K'){
        unsigned id;std::string d,p,o,r,t,u,i,g,b,c,count,tick,f,s,cw,mx;
        require(bool(row>>id>>mode>>d>>p>>o>>r>>t>>u>>i>>g>>b>>c>>in_state>>out_state>>count>>tick>>f>>s>>cw>>mx)
            &&!reading_case&&!reading_state&&id==cases+1,"Incorrect original rebuild case order");
        datum=word(d);state={word(p),word(o),word(r),word(t)};branches={word(u),word(i),word(g),word(b),word(c)};
        counter=word(count);clock=word(tick);flag=word(f);scale=word(s);context={std::uint16_t(word(cw)),word(mx)};
        require(in_state<states.size()&&out_state<states.size(),"Missing original rebuild state");lower.clear();expected.clear();reading_case=true;
      }else if(kind=='L'){
        std::string pc,e,c,o,l,a,result;std::array<std::string,4> args;unsigned n,before,after;
        require(bool(row>>pc>>e>>c>>o>>l>>a>>n>>args[0]>>args[1]>>args[2]>>args[3]>>before>>after>>result)&&reading_case&&n<=4,"Incomplete original rebuild lower ABI");
        require(before<states.size()&&after<states.size(),"Missing original lower state");
        lower.push_back({{word(pc),word(e),word(c),word(o),word(l),word(a),{word(args[0]),word(args[1]),word(args[2]),word(args[3])},n},before,after,word(result)});
      }else if(kind=='W'){
        std::string pc,p,b,a;unsigned n;require(bool(row>>pc>>p>>n>>b>>a)&&reading_case,"Incomplete original rebuild store");
        expected.push_back({word(pc),word(p),word(b),word(a),n});
      }else if(kind=='E'){
        if(reading_state){require(!states.back().empty(),"Empty original rebuild backing");reading_state=false;}
        else {
          require(reading_case&&!lower.empty(),"Incomplete original rebuild case");auto backing=states[in_state];unsigned invoked=0;
          halo2::SourcePhysicsComponentRebuildServices services;
          services.memory=[&](std::uint32_t address,std::size_t n)->std::span<std::byte>{
            for(auto &r:backing)if(address>=r.address&&std::uint64_t(address)-r.address<=r.data.size()&&n<=r.data.size()-(std::uint64_t(address)-r.address))
              return std::span(r.data).subspan(address-r.address,n);
            throw std::runtime_error("Original component rebuild escaped retained backing");
          };
          services.invoke=[&](const auto &call)->std::uint32_t{
            require(invoked<lower.size(),"Unexpected original rebuild lower owner");const auto &l=lower[invoked++];
            require(call==l.call,"Original rebuild lower ABI differs");require(backing==states[l.before],"Original full lower input differs");
            if(mode!=0)throw RequiredLower{};
            // Only this one upper-caller test replays two measured original
            // lower results/storage. It is never a live physics adapter.
            backing=states[l.after];++replayed;return l.result;
          };
          std::vector<halo2::ObjectPhysicsConstructionStore> actual;const auto observe=[&](const auto &s){actual.push_back(s);};bool stopped=false;
          try {
            if(mode==2){
              unsigned allocations=0,rebuilds=0;
              halo2::SourceObjectPhysicsConstructionServices outer{services.memory,[&](const auto &call)->std::uint32_t{
                if(call.entry==0x1d2aa0){
                  require(call.instruction==0x1c6fc7&&call.object==datum&&allocations++==0,"Original composed allocation ABI differs");
                  return halo2::source_physics_component_allocate({state.component_pool_slot,state.object_pool_slot,counter,clock},datum,{scale},context,{services.memory},observe);
                }
                require(call.instruction==0x1c6fd2&&call.entry==0x1d2b10&&call.component==0xe1680000u&&allocations==1&&rebuilds++==0,"Original composed rebuild ABI differs");
                halo2::source_physics_component_rebuild(state,call.component,branches,services,observe);return 0;
              }};
              halo2::source_object_physics_construct({state.object_pool_slot,state.component_pool_slot,flag,counter},datum,outer,observe);
            }else halo2::source_physics_component_rebuild(state,datum,branches,services,observe);
          }catch(const RequiredLower &){stopped=true;}
          require(stopped==(mode!=0)&&invoked==lower.size(),"Required original rebuild lower boundary omitted");
          require(actual==expected,"Original rebuild owning stores differ");require(backing==states[out_state],"Full original rebuild output differs");
          for(const auto &r:backing)full+=r.data.size();
          stores+=actual.size();stops+=stopped;composed+=mode==2;++cases;reading_case=false;
        }
      }else if(kind=='Z'){
        unsigned c,s,r,p;std::size_t n;require(bool(row>>c>>s>>r>>p>>n)&&!reading_case&&!reading_state&&!summary,"Invalid original rebuild summary");
        require(c==cases&&s==stores&&r==replayed&&p==stops&&n==full&&composed==1,"Incomplete original rebuild coverage");summary=true;
      }else throw std::runtime_error("Unknown original rebuild row");
      require(!(row>>extra),"Extra original rebuild field");
    }
    require(summary&&!reading_case&&!reading_state,"Missing original rebuild summary");
    std::cout<<"PASS component rebuild: "<<cases<<" original upper cases/required stops, "<<stores<<" owning stores, "<<full<<" full compared bytes; "<<replayed
      <<" test-only upper-caller lower replays, "<<stops<<" no-replay required stops, "<<composed<<" actual native allocator/init/rebuild prefix\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
