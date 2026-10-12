#include "halo2/vista_widget_pool.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& text) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid pool fixture hex");};
  if(text.size()%2)throw FormatError("Odd pool fixture hex");
  std::vector<std::byte> result(text.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(std::uint8_t(16*digit(text[2*i])+digit(text[2*i+1])));
  return result;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string result;
  for(const auto b:bytes){const auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}return result;
}
std::uint32_t get(std::span<const std::byte> bytes) {
  std::uint32_t v{};for(std::size_t i=0;i<bytes.size();++i)v|=std::uint32_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);return v;
}
void put(std::span<std::byte> bytes,std::uint32_t v) {
  for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(std::uint8_t(v>>(8*i)));
}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Unbound pool service admitted");}
} //namespace
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;
  std::size_t cases{},reads{},writes{},failures{},mutations{},reentries{},providers{},checks{};
  for(const auto& row:fixture.at("cases")) {
    const auto& q=row.at("input");std::map<std::uint32_t,std::vector<std::byte>> banks,private_banks;
    for(const auto& [address,index]:row.at("initial").items())banks.emplace(std::stoul(address),unhex(fixture.at("banks").at(index.get<std::size_t>())));
    Json trace=Json::array(),events=Json::array(),results=Json::array();std::string failure;bool mutated{},reentered{},failed{};
    const auto snapshot=[&] {Json result=Json::object();for(const auto& [address,bytes]:banks)result[std::to_string(address)]=hex(bytes);return result;};
    const auto view=[&](std::uint32_t address,std::size_t n,char access)->std::span<std::byte> {
      for(auto* collection:{&banks,&private_banks})for(auto& [base,bytes]:*collection)
        if(address>=base&&std::uint64_t(address)-base<=bytes.size()&&n<=bytes.size()-(std::uint64_t(address)-base))return std::span(bytes).subspan(address-base,n);
      trace.push_back(Json::array({"fault",std::string(1,access),address,n}));failure="memory";throw FormatError("Unmapped pool source32 address");
    };
    VistaWidgetPoolServices s;std::map<std::string,std::size_t> occurrences;
    std::function<Json(const Json&)> operation;
    const auto provider=[&](const std::string& name,const Json& args) {
      const auto occurrence=occurrences[name]++;events.push_back({{"name",name},{"args",args},{"banks",snapshot()}});
      if(q.contains("mutation")) {
        const auto& m=q.at("mutation");
        if(m.at("at")==name&&m.value("occurrence",std::size_t{0})==occurrence) {
          mutated=true;for(const auto& w:m.at("writes"))put(view(w.at(0),w.at(2),'w'),w.at(1));
        }
      }
      if(q.contains("fail")&&q.at("fail")==Json::array({name,occurrence})) {failure="provider";throw FormatError("Declared genuine pool provider failure");}
      if(q.contains("reentry")) {
        const auto& r=q.at("reentry");
        if(!reentered&&r.at("at")==name&&r.value("occurrence",std::size_t{0})==occurrence) {reentered=true;(void)operation(r.at("op"));}
      }
    };
    s.read=[&](std::uint32_t address,std::size_t width){const auto value=get(view(address,width,'r'));trace.push_back(Json::array({"r",address,width,value}));return value;};
    s.write=[&](std::uint32_t address,std::uint32_t value,std::size_t width){auto target=view(address,width,'w');const auto mask=width==4?0xffffffffu:(1u<<(8*width))-1;trace.push_back(Json::array({"w",address,width,value&mask}));put(target,value);};
    s.backing_allocate=[&](auto allocator,auto target,auto size){provider("backing_allocate",Json::array({allocator.value,target,size}));return VistaWidgetIdentity{q.value("backing",0x1a00000u)};};
    s.backing_release=[&](auto allocator,auto target,auto heap){provider("backing_release",Json::array({allocator.value,target,heap.value}));};
    s.dispose=[&](auto child){provider("dispose",Json::array({child.value}));};
    s.timestamp=[&] {provider("timestamp",Json::array());return q.value("timestamp",0xfedcba98u);};
    operation=[&](const Json& op)->Json {
      const auto entry=op.at("entry").get<std::uint32_t>();const auto heap=op.value("heap",0x1a00000u);const auto output=op.value("output",0x1a08000u);
      switch(entry) {
      case 0x60d196:vista_widget_pool_birth(op.value("kind",0u),s);break;
      case 0x60d21c:vista_widget_pool_shutdown(s);break;
      case 0x609616:vista_widget_pool_retire(s);break;
      case 0x48c08e:return vista_widget_pool_allocate({heap},output,op.value("size",0u),op.value("metadata1",0u),op.value("metadata2",0u),s)?1:0;
      case 0x48bdbf:return vista_widget_pool_length(output,s);
      case 0x48bd80:vista_widget_pool_unlink({heap},output,s);break;
      case 0x60d2d8:case 0x60d239: {
        const auto scratch=op.at("scratch").get<std::uint32_t>();private_banks[scratch]=std::vector<std::byte>(4);
        if(entry==0x60d2d8)return vista_widget_ui_allocate(op.value("size",0u),op.value("ignored",0u),scratch,s).value;
        return vista_widget_ui_allocate_raw(op.value("size",0u),op.value("ignored",0u),0,scratch,s).value;
      }
      case 0x60d2ea: {
        const auto argument=op.at("argument").get<std::uint32_t>();private_banks[argument]=std::vector<std::byte>(4);put(private_banks[argument],op.value("data",0x1a00110u));
        vista_widget_ui_release(argument,s);break;
      }
      default:throw FormatError("Unowned original pool entry");
      }
      return nullptr;
    };
    for(const auto& op:q.at("operations")) {
      try{results.push_back(operation(op));}catch(const FormatError&){if(row.at("failure").is_null())throw;failed=true;results.push_back(nullptr);break;}
    }
    auto expected_events=row.at("events");
    for(auto& event:expected_events)for(auto& [address,index]:event.at("banks").items())index=fixture.at("banks").at(index.get<std::size_t>());
    auto expected_final=row.at("final");for(auto& [address,index]:expected_final.items())index=fixture.at("banks").at(index.get<std::size_t>());
    const bool okay=trace==row.at("trace")&&events==expected_events&&snapshot()==expected_final&&results==row.at("results")&&mutated==row.at("mutation")&&reentered==row.at("reentry")&&failed==!row.at("failure").is_null()&&(!failed||failure==row.at("failure").get<std::string>());
    if(!okay) {
      std::cerr<<"Pool case "<<cases<<" trace "<<(trace==row.at("trace"))<<" events "<<(events==expected_events)<<" bank "<<(snapshot()==expected_final)<<" result "<<(results==row.at("results"))<<'\n';
      std::ofstream("/tmp/vista-pool-mismatch.json")<<Json({{"q",q},{"trace",trace},{"expected_trace",row.at("trace")},{"events",events},{"expected_events",expected_events},{"result",results},{"expected_result",row.at("results")},{"final",snapshot()},{"expected_final",expected_final}}).dump(2)<<'\n';
      throw FormatError("Original pool mismatch");
    }
    ++cases;checks+=8+trace.size()+events.size();failures+=failed;mutations+=mutated;reentries+=reentered;providers+=events.size();
    for(const auto& t:trace){reads+=t.at(0)=="r";writes+=t.at(0)=="w";}
  }
  VistaWidgetPoolServices empty;
  rejects([&]{vista_widget_pool_birth(0,empty);});rejects([&]{vista_widget_pool_shutdown(empty);});
  rejects([&]{vista_widget_pool_allocate({1},2,3,4,5,empty);});rejects([&]{vista_widget_pool_length(1,empty);});
  rejects([&]{vista_widget_pool_unlink({1},2,empty);});rejects([&]{vista_widget_pool_retire(empty);});
  rejects([&]{vista_widget_ui_allocate(1,0,2,empty);});rejects([&]{vista_widget_ui_release(1,empty);});
  empty.read=[](auto,auto){return 0u;};vista_widget_pool_shutdown(empty);
  rejects([&]{vista_widget_pool_birth(0,empty);});
  std::cout<<"PASS "<<cases<<" original pool cases, "<<checks<<" trace/bank/order checks, "<<reads<<" reads, "<<writes<<" writes, "<<providers<<" genuine provider calls, "<<failures<<" fault prefixes, "<<mutations<<" mutations, "<<reentries<<" nested source reentries; 9 unbound service boundaries\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
