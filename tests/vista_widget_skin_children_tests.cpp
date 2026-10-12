#include "halo2/vista_widget_skin_children.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool value,const char* text){++checks;if(!value)throw std::runtime_error(text);}
std::vector<std::byte> decode(const std::string& text){
  std::vector<std::byte> out;
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
std::string hex(std::span<const std::byte> bytes){
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Banks {
  std::map<std::uint32_t,std::vector<std::byte>> storage;
  std::span<std::byte> bytes(std::uint32_t at,std::size_t count){
    auto it=storage.upper_bound(at);if(it==storage.begin())throw FormatError("unmapped skin storage");
    --it;const auto offset=std::uint64_t(at)-it->first;
    if(offset>it->second.size() || count>it->second.size()-std::size_t(offset) || std::uint64_t(at)+count>0x100000000ull)
      throw FormatError("unmapped skin extent");
    return std::span(it->second).subspan(std::size_t(offset),count);
  }
  std::uint32_t read(std::uint32_t at,std::size_t count){
    std::uint32_t result{};const auto b=bytes(at,count);
    for(std::size_t i=0;i<count;++i)result|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return result;
  }
  void put(std::uint32_t at,std::uint32_t value,std::size_t count){auto b=bytes(at,count);for(std::size_t i=0;i<count;++i)b[i]=std::byte(value>>(i*8));}
  Json snapshot(){Json out=Json::object();for(const auto& [a,b]:storage)out[std::to_string(a)]=hex(b);return out;}
};
}
int main(int argc,char** argv){
  try{
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);check(bool(input),"missing fixture");const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2_VISTA_WIDGET_SKIN_CHILDREN_ORIGINAL_V1","fixture schema");unsigned index{},faults{};
    for(const auto& expected:fixture.at("cases")){
      try{
        Banks banks;for(const auto& [a,b]:expected.at("initial").items())banks.storage.emplace(std::stoul(a),decode(b.get<std::string>()));
        const auto& q=expected.at("input");const auto skin=expected.at("skin").get<std::uint32_t>();const auto point=expected.at("point").get<std::uint32_t>();
        const auto parent=expected.at("parent").get<std::uint32_t>();Json events=Json::array(),reads=Json::array();bool applied{};unsigned ordinal{};
        const auto event=[&](const char* name,const Json& args){
          events.push_back({{"name",name},{"args",args},{"banks",banks.snapshot()}});
          if(q.contains("mutation") && !applied && q.at("mutation").at("at")==name){
            const auto& m=q.at("mutation");const auto at=(m.at("target")=="point"?point:(m.at("target")=="nested"?0x03203000u:(m.at("target")=="global"?0x882290u:skin)))+m.at("offset").get<std::uint32_t>();
            banks.put(at,m.at("value"),m.value("size",4));applied=true;
          }
        };
        const auto create=[&](const char* name,const Json& args){
          event(name,args);VistaWidgetIdentity id;
          const auto skip=q.value("null_every",0u);
          if(!skip || ordinal%skip){
            id.value=parent+0x8000u+ordinal*0x100u;auto& b=banks.storage[id.value];b.resize(0x80);
            for(unsigned i=0;i<b.size();++i)b[i]=std::byte(i*17+ordinal*7);
            banks.put(id.value,parent+0x6000,4);
            const auto bounds=q.value("bounds",std::vector<std::int32_t>{31,-45,-19,83});
            for(unsigned i=0;i<4;++i)banks.put(id.value+0x20+i*2,std::uint32_t(bounds[i]),2);
          }
          ++ordinal;return id;
        };
        VistaWidgetSkinServices s;
        s.bank=[&](auto id){return banks.bytes(id.value,0x80);};s.read=[&](auto at,auto n){const auto value=banks.read(at,n);reads.push_back(Json::array({at,n,value}));return value;};
        s.bitmap=[&](auto id,auto record){return create("bitmap",Json::array({id.value,record}));};
        s.kind9=[&](auto id,auto record){return create("kind9",Json::array({id.value,record}));};
        s.grid=[&](auto id,auto borrowed,auto row,auto record){check(borrowed.data()==banks.bytes(point,4).data(),"borrowed point identity");return create("grid",Json::array({id.value,point,row,record}));};
        s.text=[&](auto id,auto row,auto record){return create("text",Json::array({id.value,row,record}));};
        s.bounds=[&](auto id){event("bounds",Json::array({id.value}));VistaWidgetSkinBounds b;auto src=banks.bytes(id.value+0x20,8);std::copy(src.begin(),src.end(),b.begin());return b;};
        bool rejected{};try{vista_widget_skin_children({parent},std::span<const std::byte,4>(banks.bytes(point,4)),skin,s);}catch(const FormatError&){rejected=true;}
        check(rejected==!expected.at("fault").is_null(),"fault prefix");if(rejected)++faults;
        check(events==expected.at("events"),"callback ordering/banks");check(reads==expected.at("reads"),"raw resource read order");check(banks.snapshot()==expected.at("final"),"full retained banks");
        check(applied==expected.at("mutation_applied"),"source mutation reached");
      }catch(const std::exception& e){throw std::runtime_error("case "+std::to_string(index)+": "+e.what());}++index;
    }
    check(index==164,"case inventory");std::cout<<"PASS "<<checks<<" checks / "<<index<<" original cases / "<<faults<<" fault prefixes\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
