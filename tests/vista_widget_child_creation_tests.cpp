#include "halo2/vista_widget_child_creation.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool v,const char* message){++checks;if(!v)throw std::runtime_error(message);}
std::vector<std::byte> decode(const std::string& s){
  std::vector<std::byte> b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;
}
std::string hex(std::span<const std::byte> b){
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto v:b){auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
struct Banks {
  std::map<std::uint32_t,std::vector<std::byte>> storage;
  std::span<std::byte> bytes(std::uint32_t at,std::size_t n){
    auto it=storage.upper_bound(at);
    if(it==storage.begin())throw FormatError("Unmapped child creation source");
    --it;const auto offset=std::uint64_t(at)-it->first;
    if(offset>it->second.size()||n>it->second.size()-std::size_t(offset)||std::uint64_t(at)+n>0x100000000ull)
      throw FormatError("Unmapped child creation extent");
    return std::span(it->second).subspan(std::size_t(offset),n);
  }
  std::span<std::byte> bank(std::uint32_t at){
    auto it=storage.find(at);if(it==storage.end())throw FormatError("Unmapped retained child creation bank");return it->second;
  }
  std::uint32_t read(std::uint32_t at,std::size_t n){
    auto b=bytes(at,n);std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
  }
  void put(std::uint32_t at,std::uint32_t v,std::size_t n){auto b=bytes(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte(v>>(8*i));}
  Json snapshot(){Json out=Json::object();for(const auto& [a,b]:storage)out[std::to_string(a)]=hex(b);return out;}
};
}
int main(int argc,char** argv){
  try {
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);check(bool(input),"missing fixture");const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2_VISTA_WIDGET_CHILD_CREATION_ORIGINAL_V1","fixture schema");unsigned index{},faults{};
    constexpr std::uint32_t parent=0x01a00000u,child=parent+0x2000u,point=parent+0xa000u,definition=0x03200000u;
    for(const auto& expected:fixture.at("cases")){
      try {
        Banks banks;for(const auto& [a,b]:expected.at("initial").items())banks.storage.emplace(std::stoul(a),decode(b.get<std::string>()));
        const auto& q=expected.at("input");Json events=Json::array(),reads=Json::array();bool mutated{};unsigned clocks{};
        auto event=[&](const char* name,const Json& args){
          events.push_back({{"name",name},{"args",args},{"banks",banks.snapshot()}});
          if(q.contains("mutation")&&!mutated&&q.at("mutation").at("at")==name){
            const auto& m=q.at("mutation");banks.put(m.at("address"),m.at("value"),m.value("size",4u));mutated=true;
          }
        };
        VistaWidgetChildCreationServices s;
        s.birth.bank=[&](auto id){return banks.bank(id.value);};
        s.birth.read=[&](auto at,auto n){const auto v=banks.read(at,n);reads.push_back(Json::array({at,n,v}));return v;};
        s.birth.cached_ui_clock=[&]{event("clock",Json::array());return q.at("clock").get<std::uint32_t>()+clocks++;};
        s.birth.bitmap_residency=[&](auto datum){event("residency",Json::array({datum}));};s.birth.mxcsr=0x1f80;
        s.allocate=[&](auto size,auto flags){event("allocate",Json::array({size,flags}));return VistaWidgetIdentity{q.at("null").get<bool>()?0u:child};};
        s.grid_initialize=[&](auto id,auto position){check(position.data()==banks.bytes(point,4).data(),"borrowed source point");event("grid-initialize",Json::array({id.value,point}));};
        s.text_descriptor=[&](auto id,auto ordinal,auto record){event("text-descriptor",Json::array({id.value,ordinal,record}));};
        s.assign_text=[&](auto id,auto text){event("assign-text",Json::array({id.value,text}));};
        const auto owner=q.at("owner").get<std::string>();VistaWidgetIdentity result;bool rejected{};
        try {
          if(owner=="bitmap")result=vista_widget_create_bitmap({parent},definition,s);
          else if(owner=="kind9")result=vista_widget_create_kind9({parent},definition,s);
          else if(owner=="grid")result=vista_widget_create_grid({parent},std::span<const std::byte,4>(banks.bytes(point,4)),q.at("row"),definition,s);
          else if(owner=="text")result=vista_widget_create_text({parent},q.at("ordinal"),definition,s);
          else if(owner=="kind9-birth")result=vista_widget_kind9_construct({child},definition,s.birth);
          else result=vista_widget_skin_text_construct({child},q.at("controller"),owner=="short-birth",s.birth);
        }catch(const FormatError&){rejected=true;}
        check(rejected==!expected.at("fault").is_null(),"fault prefix");if(rejected)++faults;
        if(!rejected)check(result.value==expected.at("result"),"returned source identity");
        check(events==expected.at("events"),"callbacks/banks/order");check(reads==expected.at("reads"),"raw asset read order");
        check(banks.snapshot()==expected.at("final"),"complete final bank");check(clocks==expected.at("clocks"),"cached-clock count");
        check(mutated==expected.at("mutated"),"callback mutation reached");
      }catch(const std::exception& e){throw std::runtime_error("case "+std::to_string(index)+": "+e.what());}++index;
    }
    check(index==421,"case inventory");std::cout<<"PASS "<<checks<<" checks / "<<index<<" original cases / "<<faults<<" fault prefixes\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
