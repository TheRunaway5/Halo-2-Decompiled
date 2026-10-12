#include "halo2/vista_widget_manager_lifetime.hpp"
#include "halo2/native_vista_widget_managers.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string& value) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid manager fixture hex");};
  if(value.size()%2)throw FormatError("Odd manager fixture hex");
  std::vector<std::byte> out(value.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(std::uint8_t(16*digit(value[2*i])+digit(value[2*i+1])));
  return out;
}
std::string hex(std::span<const std::byte> bank) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto b:bank){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}return out;
}
std::uint32_t get(std::span<const std::byte> bank) {
  std::uint32_t v{};for(std::size_t i=0;i<bank.size();++i)v|=std::uint32_t(std::to_integer<unsigned>(bank[i]))<<(8*i);return v;
}
void put(std::span<std::byte> bank,std::uint32_t value) {
  for(std::size_t i=0;i<bank.size();++i)bank[i]=std::byte(std::uint8_t(value>>(8*i)));
}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Unbound manager lifetime service admitted");}
} //namespace
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json fixture;input>>fixture;std::size_t count{},read_count{},write_count{},failures{},mutations{},unsupported{},reentries{};
  for(const auto& row:fixture.at("cases")) {
    if(!row.at("unsupported").is_null()) {
      if(row.at("unsupported")!="emulator_partial_cross_page_store")throw FormatError("Unknown unsupported manager observation");
      ++unsupported;continue;
    }
    const auto& q=row.at("input");const auto base=q.at("base").get<std::uint32_t>();
    auto bytes=unhex(fixture.at("banks").at(row.at("initial").get<std::size_t>()));
    Json trace=Json::array(),events=Json::array();bool applied{},failed{},reentered{};std::string failure;
    VistaWidgetManagerLifetimeServices s;
    const auto view=[&](std::uint32_t address,std::size_t n,char access)->std::span<std::byte> {
      if(address<base||std::uint64_t(address)-base>bytes.size()||n>bytes.size()-(std::uint64_t(address)-base)) {
        trace.push_back(Json::array({"fault",std::string(1,access),address,n}));failure="memory";
        throw FormatError("Unmapped original manager source address");
      }
      return std::span(bytes).subspan(address-base,n);
    };
    std::size_t disposals{},closes{};
    const auto provider=[&](const std::string& name,VistaWidgetIdentity child) {
      auto& index=name=="dispose"?disposals:closes;const auto occurrence=index++;
      events.push_back({{"name",name},{"child",child.value},{"bank",hex(bytes)}});
      if(q.contains("mutation")) {
        const auto& m=q.at("mutation");
        if(m.at("at")==name&&m.value("occurrence",std::size_t{0})==occurrence) {
          applied=true;
          for(const auto& w:m.at("writes"))put(view(base+w.at(0).get<std::uint32_t>(),w.at(2),'w'),w.at(1));
        }
      }
      if(q.contains("fail")&&q.at("fail")==Json::array({name,occurrence})) {
        failure="provider";throw FormatError("Declared original manager provider failure");
      }
      if(q.contains("reentry")) {
        const auto& r=q.at("reentry");
        if(!reentered&&r.at("at")==name&&r.value("occurrence",std::size_t{0})==occurrence) {
          reentered=true;
          switch(r.at("entry").get<std::uint32_t>()) {
          case 0x637bd7:vista_widget_manager_reset({base},VistaWidgetManagerReset::base637BD7,s);break;
          case 0x637f34:vista_widget_manager_reset({base},VistaWidgetManagerReset::extended637F34,s);break;
          case 0x6390e2:vista_widget_manager_reset({base},VistaWidgetManagerReset::global6390E2,s);break;
          default:throw FormatError("Unowned nested manager reset");
          }
        }
      }
    };
    s.read=[&](std::uint32_t address,std::size_t width) {const auto value=get(view(address,width,'r'));trace.push_back(Json::array({"r",address,width,value}));return value;};
    s.write=[&](std::uint32_t address,std::uint32_t value,std::size_t width) {auto target=view(address,width,'w');const auto mask=width==4?0xffffffffu:(1u<<(8*width))-1;trace.push_back(Json::array({"w",address,width,value&mask}));put(target,value);};
    s.dispose=[&](auto child) {provider("dispose",child);};s.extended_close=[&](auto manager) {provider("extended_close",manager);};
    const VistaWidgetIdentity id{base};std::uint32_t result{};
    try {
      switch(q.at("entry").get<std::uint32_t>()) {
      case 0x637ba8:result=vista_widget_manager_construct(id,VistaWidgetManagerBirth::base637BA8,s).value;break;
      case 0x637ef6:result=vista_widget_manager_construct(id,VistaWidgetManagerBirth::extended637EF6,s).value;break;
      case 0x639051:result=vista_widget_manager_construct(id,VistaWidgetManagerBirth::derived639051,s).value;break;
      case 0x63907e:result=vista_widget_manager_construct(id,VistaWidgetManagerBirth::derived63907E,s).value;break;
      case 0x639090:result=vista_widget_manager_construct(id,VistaWidgetManagerBirth::derived639090,s).value;break;
      case 0x6390a2:result=vista_widget_manager_construct(id,VistaWidgetManagerBirth::global6390A2,s).value;break;
      case 0x6092e3:result=vista_widget_ui_construct(id,s).value;break;
      case 0x637bd7:vista_widget_manager_reset(id,VistaWidgetManagerReset::base637BD7,s);break;
      case 0x637f34:vista_widget_manager_reset(id,VistaWidgetManagerReset::extended637F34,s);break;
      case 0x6390e2:vista_widget_manager_reset(id,VistaWidgetManagerReset::global6390E2,s);break;
      case 0x637628:vista_widget_manager_shutdown(id,VistaWidgetManagerShutdown::base637628,s);break;
      case 0x638211:vista_widget_manager_shutdown(id,VistaWidgetManagerShutdown::extended638211,s);break;
      case 0x63910e:vista_widget_manager_shutdown(id,VistaWidgetManagerShutdown::global63910E,s);break;
      case 0x63785b:vista_widget_manager_retire(id,s);break;
      default:throw FormatError("Unowned original manager case");
      }
    }catch(const FormatError&){if(row.at("failure").is_null())throw;failed=true;}
    auto expected_events=row.at("events");for(auto& event:expected_events)event["bank"]=fixture.at("banks").at(event.at("bank").get<std::size_t>());
    const bool return_ok=row.at("returned").is_null()||row.at("returned")==result;
    const auto expected_bank=fixture.at("banks").at(row.at("final").get<std::size_t>()).get<std::string>();
    if(q.value("pe_initial",false)) {
      NativeVistaWidgetManagers native;
      if(hex(native.bytes(NativeVistaWidgetManagers::source_base,NativeVistaWidgetManagers::storage_size))!=expected_bank)
        throw FormatError("Retained native UI bank differs from original fresh PE construction");
      const auto held=native.bytes(0xd71ebc,0x3c);const auto address=held.data();
      native.write(0xd71ec8,0x12345678,4);
      if(native.read(0xd71ec8,4)!=0x12345678||native.bytes(0xd71ebc,0x3c).data()!=address)
        throw FormatError("Native UI bank identity/storage did not remain retained");
      rejects([&]{native.read(0xd718df,1);});rejects([&]{native.write(0xd758d0,1,1);});
      rejects([&]{native.read(0xffffffffu,4);});rejects([&]{native.write(0xd71900,0,3);});
    }
    if(trace!=row.at("trace")||events!=expected_events||hex(bytes)!=expected_bank||applied!=row.at("mutation")||reentered!=row.at("reentry")||failed!=!row.at("failure").is_null()||(!row.at("failure").is_null()&&failure!=row.at("failure").get<std::string>())||!return_ok) {
      std::cerr<<"Manager case "<<count<<" entry "<<std::hex<<q.at("entry").get<std::uint32_t>()<<std::dec<<" trace "<<(trace==row.at("trace"))<<" events "<<(events==expected_events)<<" bank "<<(hex(bytes)==expected_bank)<<'\n';
      std::ofstream("/tmp/vista-manager-lifetime-mismatch.json")<<Json({{"trace",trace},{"expected",row.at("trace")},{"events",events},{"expected_events",expected_events}}).dump(2)<<'\n';
      throw FormatError("Original manager lifetime mismatch");
    }
    ++count;failures+=failed;mutations+=applied;reentries+=reentered;
    for(const auto& t:trace){read_count+=t.at(0)=="r";write_count+=t.at(0)=="w";}
  }
  VistaWidgetManagerLifetimeServices empty;
  rejects([&]{vista_widget_manager_construct({1},static_cast<VistaWidgetManagerBirth>(-1),empty);});
  rejects([&]{vista_widget_manager_reset({1},static_cast<VistaWidgetManagerReset>(-1),empty);});
  rejects([&]{vista_widget_manager_shutdown({1},static_cast<VistaWidgetManagerShutdown>(-1),empty);});
  rejects([&]{vista_widget_manager_construct({1},VistaWidgetManagerBirth::base637BA8,empty);});
  rejects([&]{vista_widget_ui_construct({1},empty);});
  rejects([&]{vista_widget_manager_reset({1},VistaWidgetManagerReset::base637BD7,empty);});
  rejects([&]{vista_widget_manager_shutdown({1},VistaWidgetManagerShutdown::base637628,empty);});
  rejects([&]{vista_widget_manager_retire({1},empty);});
  empty.read=[](auto,auto){return 0u;};vista_widget_manager_retire({1},empty);
  rejects([&]{vista_widget_manager_shutdown({1},VistaWidgetManagerShutdown::extended638211,empty);});
  empty.extended_close=[](auto){};
  rejects([&]{vista_widget_manager_shutdown({1},VistaWidgetManagerShutdown::base637628,empty);});
  empty.write=[](auto,auto,auto){};
  vista_widget_manager_shutdown({1},VistaWidgetManagerShutdown::base637628,empty);
  empty.read=[](auto,auto){return 1u;};rejects([&]{vista_widget_manager_retire({1},empty);});
  std::cout<<"PASS "<<count<<" original UI/manager cases, "<<read_count<<" reads, "<<write_count<<" writes, "<<failures<<" partial failures, "<<mutations<<" callback mutations, "<<reentries<<" nested reset reentries; "<<unsupported<<" retained emulator partial-store observations excluded\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
