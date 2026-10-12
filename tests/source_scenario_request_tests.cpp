#include "halo2/source_scenario_request.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
void require(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::uint32_t number(const std::string &s){return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::byte> raw(const std::string &s) {
  require(s.size()%2==0,"Odd request fixture extent");std::vector<std::byte> out;
  for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(number(s.substr(i,2))));
  return out;
}
std::uint32_t word(std::span<const std::byte> data) {
  require(data.size()<=4,"Oversized fixture word");std::uint32_t out=0;
  for(unsigned i=0;i<data.size();++i)out|=std::to_integer<std::uint32_t>(data[i])<<(8*i);
  return out;
}
struct Region {std::uint32_t address;std::vector<std::byte> bytes;};
bool same(const halo2::ObjectConstructionCall &a,const halo2::ObjectConstructionCall &b) {
  return a.instruction==b.instruction && a.entry==b.entry && a.registers==b.registers &&
      a.register_mask==b.register_mask && a.arguments==b.arguments && a.argument_count==b.argument_count && !a.xmm0_zero;
}
template<class F> void unavailable(F f){bool stopped=false;try{f();}catch(const std::exception &){stopped=true;}require(stopped,"Missing request owner did not stop");}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2 && argc!=6)return 77;
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    std::array<std::shared_ptr<const halo2::OriginalData>,2> images{};
    std::optional<halo2::SourceTagMetadataProviders> cache;
    if(argc==6) {
      const auto base=halo2::OriginalData::load_xbe(argv[2]);
      require(!base->scenario_request_operands && !base->object_creation_request_operands &&
          !base->object_construction_tables,"Unverified base request imports present");
      images={halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
      cache=halo2::SourceTagMetadataStore(std::make_shared<halo2::Cache>(argv[5]),{"scnr"}).providers();
    }
    std::ifstream input(argv[1]);std::string line;std::getline(input,line);
    require(line=="SCENARIO_REQUEST_V1","Request fixture schema");
    halo2::ObjectCreationRequestOperands initializer{};
    halo2::SourceScenarioRequestOperands constants{};
    std::array<std::uint32_t,13> pointers{};
    std::array<std::uint16_t,13> extents{};
    halo2::SourceScenarioRequestInput in{};in.request=0x02000000;
    std::string name;std::uint32_t scenario=0,comparison=0;unsigned returned=0,side=0;
    int extent=-1;bool type_null=false;
    std::vector<Region> before;std::vector<std::byte> expected;
    std::vector<halo2::ObjectConstructionCall> calls;
    std::vector<halo2::ObjectConstructionStore> stores;
    unsigned cases=0,writes=0,lower=0,authored=0,imports=0;
    while(std::getline(input,line)) {
      std::istringstream row(line);char kind;row>>kind;
      if(kind=='D') {std::string d;row>>d;scenario=number(d);}
      else if(kind=='I') {
        std::string digest,one,forward,up,zero,scale;row>>side>>digest>>one>>forward>>up>>zero>>scale;
        require(side<2 && digest.size()==64,"Request selected-image identity");
        initializer.one_bits=word(raw(one));const auto f=raw(forward),u=raw(up);
        require(f.size()==12 && u.size()==12,"Request basis extent");
        std::copy(f.begin(),f.end(),initializer.basis_forward.begin());
        std::copy(u.begin(),u.end(),initializer.basis_up.begin());
        constants={word(raw(zero)),initializer.one_bits,word(raw(scale))};
        if(images[side]) {
          const auto &image=*images[side];require(image.object_creation_request_operands &&
              image.scenario_request_operands && image.object_construction_tables,"Actual request operands missing");
          const auto &a=*image.object_creation_request_operands;const auto &b=*image.scenario_request_operands;
          require(a.one_bits==initializer.one_bits && a.basis_forward==initializer.basis_forward &&
              a.basis_up==initializer.basis_up && b.zero_bits==constants.zero_bits &&
              b.one_bits==constants.one_bits && b.color_scale_bits==constants.color_scale_bits,"Imported request operands differ");
        }
      } else if(kind=='T') {
        unsigned typ;std::string address,data;row>>typ>>address>>data;
        require(typ<13,"Fixture type escapes bank");pointers[typ]=number(address);const auto fields=raw(data);
        require(fields.size()==6,"Type descriptor field extent");
        extents[typ]=std::uint16_t(word(std::span(fields).subspan(4,2)));
        if(images[side]) {
          const auto &tables=*images[side]->object_construction_tables;
          require(tables.type_addresses[typ]==pointers[typ] && tables.placement_extent(typ)==extents[typ],"Imported type request field differs");
          const auto offsets=tables.scenario_blocks(typ);
          require(offsets[0]==std::bit_cast<std::int16_t>(std::uint16_t(word(std::span(fields).subspan(0,2)))) &&
              offsets[1]==std::bit_cast<std::int16_t>(std::uint16_t(word(std::span(fields).subspan(2,2)))),"Imported scenario block offset differs");
          unavailable([&]{tables.read(pointers[typ]+0xa,2);});++imports;
        }
      } else if(kind=='N') {
        unsigned force,null;std::string placement,palette,mxcsr;
        row>>name>>in.type>>in.ordinal>>force>>placement>>palette>>returned>>mxcsr>>extent>>null;
        in.force=std::uint8_t(force);in.placement=number(placement);in.palette=number(palette);
        comparison=number(mxcsr);type_null=null!=0;before.clear();expected.clear();calls.clear();stores.clear();
      } else if(kind=='B') {
        std::string address,data;row>>address>>data;before.push_back({number(address),raw(data)});
      } else if(kind=='C') {
        halo2::ObjectConstructionCall c{};std::string pc,entry,mask,text;
        row>>pc>>entry>>mask;c.instruction=number(pc);c.entry=number(entry);c.register_mask=number(mask);
        for(auto &r:c.registers){row>>text;r=number(text);}
        row>>c.argument_count;require(c.argument_count<=5,"Original lower stack extent");
        for(unsigned i=0;i<c.argument_count;++i){row>>text;c.arguments[i]=number(text);}calls.push_back(c);
      } else if(kind=='S') {
        std::string pc,at,b,a;unsigned n;row>>pc>>at>>n>>b>>a;
        stores.push_back({number(pc),number(at),n,word(raw(b)),word(raw(a))});
      } else if(kind=='A') {std::string data;row>>data;expected=raw(data);}
      else if(kind=='E') {
        auto regions=before;
        halo2::SourceScenarioRequestServices p;
        p.read=[&](std::uint32_t at,std::size_t n)->std::span<const std::byte> {
          for(const auto &r:regions)if(at>=r.address && std::uint64_t(at)+n<=std::uint64_t(r.address)+r.bytes.size())
            return std::span<const std::byte>(r.bytes).subspan(at-r.address,n);
          throw std::runtime_error(name+": Read escapes actual fixture backing");
        };
        p.memory=[&](std::uint32_t at,std::size_t n)->std::span<std::byte> {
          require(at>=in.request && std::uint64_t(at)+n<=std::uint64_t(in.request)+0xc4,"Write escapes original request");
          return std::span(regions[0].bytes).subspan(at-in.request,n);
        };
        p.type_address=[&](std::int32_t t){require(t==in.type && t>=0 && t<13,"Type index ABI differs");return type_null?0:pointers[t];};
        p.placement_extent=[&](std::int32_t t){return extent<0?extents.at(t):std::uint16_t(extent);};
        p.game_options=[] {return 0x02001000u;};
        std::vector<halo2::ObjectConstructionStore> actual_stores;
        const halo2::ObjectConstructionObserver observer=[&](const auto &s){actual_stores.push_back(s);};
        auto bound=halo2::source_scenario_request_byte_owners(p,initializer,constants,0x027f,observer);
        const auto invoke=bound.invoke;std::vector<halo2::ObjectConstructionCall> actual_calls;
        bound.invoke=[&](const auto &c,auto &fp){actual_calls.push_back(c);return invoke(c,fp);};
        halo2::ObjectConstructionFloatingPointContext fp{0x1f80};
        const auto result=halo2::source_scenario_creation_request(in,constants,fp,bound,observer);
        require(unsigned(result)==returned,name+": Defined original AL differs");
        require(regions[0].bytes==expected,name+": Complete original C4 bytes differ");
        if(actual_stores!=stores) {
          const auto n=std::min(actual_stores.size(),stores.size());
          for(std::size_t i=0;i<n;++i)if(actual_stores[i]!=stores[i]) {
            const auto &a=actual_stores[i],&b=stores[i];
            std::cerr<<name<<" store "<<std::dec<<i<<" actual/expected pc="<<std::hex
                <<a.instruction<<'/'<<b.instruction<<" address="<<a.address<<'/'<<b.address
                <<" before="<<a.before<<'/'<<b.before<<" after="<<a.after<<'/'<<b.after<<'\n';break;
          }
          throw std::runtime_error(name+": Ordered original request stores differ");
        }
        require(actual_calls.size()==calls.size() && std::equal(actual_calls.begin(),actual_calls.end(),calls.begin(),same),name+": Original lower call ABI differs");
        require(fp.mxcsr==comparison,name+": COMISS boundary state differs");
        for(std::size_t i=1;i<regions.size();++i)require(regions[i].bytes==before[i].bytes,name+": Authored input was mutated");
        if(name.starts_with("authored_")) {
          ++authored;
          if(cache) {
            std::array<std::byte,0xc4> prior{};std::copy(before[0].bytes.begin(),before[0].bytes.end(),prior.begin());
            const auto fresh=halo2::source_cache_scenario_creation_request(*cache,*images[side],scenario,
                in.type,in.ordinal,in.force,prior,halo2::SourceScenarioGameOptions{2,1},0x027f,{0x1f80});
            require(unsigned(fresh.admitted)==returned && std::equal(fresh.bytes.begin(),fresh.bytes.end(),expected.begin()),name+": Production request from actual cache differs");
          }
        }
        // Lower-owner failure keeps the exact source prefix, including scale.
        if(name=="type_11") {
          regions=before;actual_stores.clear();auto missing=bound;
          missing.invoke=[&](const auto &c,auto &state)->std::uint32_t {
            if(c.entry==0x11f680)throw std::runtime_error("Euler owner absent");
            return invoke(c,state);
          };
          fp={0x1f80};unavailable([&]{halo2::source_scenario_creation_request(in,constants,fp,missing,observer);});
          auto first=std::find_if(stores.begin(),stores.end(),[](auto s){return s.instruction==0x11f69c;});
          require(first!=stores.end() && actual_stores==std::vector(stores.begin(),first),"Missing Euler lost original partial writes");
          regions=before;actual_stores.clear();auto unsupported=halo2::source_scenario_request_byte_owners(p,initializer,constants,0x037f,observer);
          fp={0x1f80};unavailable([&]{halo2::source_scenario_creation_request(in,constants,fp,unsupported,observer);});
          require(actual_stores==std::vector(stores.begin(),first),"Unsupported math silently supplied request basis");
          regions=before;actual_stores.clear();std::fenv_t host;
          require(std::fegetenv(&host)==0 && std::fesetround(FE_DOWNWARD)==0,"Host rounding control unavailable");
          fp={0x1f80};const auto again=halo2::source_scenario_creation_request(in,constants,fp,bound,observer);
          const auto rounding=std::fegetround();std::fesetenv(&host);
          require(again && regions[0].bytes==expected && rounding==FE_DOWNWARD,"Byte math leaked or inherited host rounding");
        }
        ++cases;writes+=unsigned(stores.size());lower+=unsigned(calls.size());
      } else throw std::runtime_error("Unknown scenario request fixture row");
      require(!row.fail(),"Malformed request fixture row");
      std::string extra;require(!(row>>extra),"Unexpected request fixture operand");
    }
    require(cases==1246 && authored==48 && writes==97324 && lower==6524,"Scenario request coverage changed");
    require(!cache || imports==26,"Selected-image import coverage truncated");
    std::cout<<"PASS scenario request: "<<cases<<" cases, "<<authored<<" authored, "<<writes<<" stores, "<<lower<<" complete lower calls; cacheBinding="<<bool(cache)<<'\n';
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
