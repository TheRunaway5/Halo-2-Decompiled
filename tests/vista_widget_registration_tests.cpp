#include "halo2/vista_widget_registration.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t obj=0x1a00000,heap=0x2400000,meta=0x2200000,vt=meta+0xf000;
unsigned checks{};
void check(bool value,const std::string& message){++checks;if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string& input) {
  std::vector<std::byte> b(input.size()/2);
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::stoul(input.substr(i*2,2),nullptr,16));
  return b;
}
std::string hex(std::span<const std::byte> b) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(at+n>b.size())throw std::runtime_error("fixture field extent");
  std::uint32_t v{};
  for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return v;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n=4) {
  if(at+n>b.size())throw std::runtime_error("fixture field extent");
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));
}
struct Pane {
  std::array<std::vector<VistaWidgetRecord>,5> records;
  std::vector<std::vector<std::byte>> data;
};
struct Run {
  const Json& q;
  std::map<std::uint32_t,std::vector<std::byte>> memory;
  std::vector<Pane> panes;
  Json events=Json::array();unsigned creates{};
  VistaWidgetLifetime lifetime{{obj},0};
  explicit Run(const Json& input):q(input) {
    for(const auto& r:q.at("regions"))memory.emplace(r.at("address").get<std::uint32_t>(),bytes(r.at("hex")));
    panes.resize(q.at("panes").size());
    const std::array<const char*,5> names{"bitmap","scene","grid","text","table"};
    for(std::size_t i=0;i<panes.size();++i)if(!q.at("panes")[i].is_null()) {
      auto& p=panes[i];const auto& raw=q.at("panes")[i];std::size_t n{};
      for(auto name:names)n+=raw.at(name).size();
      p.data.reserve(n);
      for(std::size_t k=0;k<names.size();++k)for(std::size_t j=0;j<raw.at(names[k]).size();++j) {
        p.data.push_back(bytes(raw.at(names[k])[j]));
        p.records[k].push_back({raw.at(std::string(names[k])+"_ids")[j],p.data.back()});
      }
    }
  }
  std::span<std::byte> bank(VistaWidgetIdentity id) {
    auto it=memory.upper_bound(id.value);if(it==memory.begin())throw std::runtime_error("unowned fixture identity");--it;
    if(std::size_t(id.value-it->first)>=it->second.size())throw std::runtime_error("unowned fixture bank");
    return std::span<std::byte>(it->second).subspan(id.value-it->first);
  }
  template<class... A> void event(const char* name,A&&... a) {
    auto v=Json::array();v.push_back(name);(v.push_back(std::forward<A>(a)),...);events.push_back(std::move(v));
  }
  VistaWidgetRegistrationServices services() {
    VistaWidgetRegistrationServices s;
    s.bank=[this](auto id){return bank(id);};
    s.screen=[this](auto id)->std::optional<VistaWidgetScreen>{
      event("screen",id);if(!q.at("metadata").get<bool>())return std::nullopt;
      return VistaWidgetScreen{q.at("flags"),q.at("pane_count"),q.at("shape")};
    };
    s.pane=[this](auto id)->std::optional<VistaWidgetPane>{
      const auto index=std::bit_cast<std::int16_t>(std::uint16_t(read(bank(id),0x9f8,2)));event("pane",id.value,index);
      if(index<0 || std::size_t(index)>=panes.size() || q.at("panes")[index].is_null())return std::nullopt;
      auto& p=panes[std::size_t(index)];const auto& raw=q.at("panes")[index];
      return VistaWidgetPane{raw.at("animation"),raw.at("buttons"),p.records[0],p.records[1],p.records[2],p.records[3],p.records[4]};
    };
    s.button=[this](auto id){event("button",id.value);};
    s.list=[this](auto id,auto arg){event("list",id.value,arg);};
    s.list_callback=[this](auto a,auto b){event("list_callback",a,b);};
    s.animate=[this](auto id,auto arg){event("animate",id.value,arg);};
    s.base_layout=[this](auto id){event("base_layout",id.value);};
    s.shape=[this](auto id,auto arg){event("shape",id.value,arg);};
    s.shutdown=[this](auto id){event("shutdown",id.value);if(q.at("shutdown_clears").get<bool>())write(bank(id),0x6c,0,1);};
    s.destructor=[this](auto id,auto arg){event("destructor",id.value,arg);};
    s.free=[this](auto id){event("free",id.value);};
    return s;
  }
  void execute(const Json& expected) {
    auto s=services();
    s.header=[this,&s](auto id){event("header",id.value);vista_widget_append(id,{id.value+0x80},s);};
    s.footer=[this,&s](auto id,auto arg){event("footer",id.value,arg);vista_widget_append(id,{id.value+0x53c},s);};
    s.create=[this,&s](auto parent,const VistaWidgetCreate& r){return create(parent,r,s);};
    if(q.at("mode")=="register") {
      const auto raw=q.at("buttons").get<std::vector<std::uint32_t>>();std::vector<VistaWidgetIdentity> ids;
      for(auto id:raw)ids.push_back({id});
      VistaWidgetRegistration registration;
      registration.words=q.at("words").get<std::array<std::uint32_t,6>>();registration.buttons=ids;
      vista_widget_register(lifetime,registration,s);
    } else if(q.at("mode")=="change")vista_widget_change_pane(lifetime,q.at("request"),s);
    else if(q.at("mode")=="append")vista_widget_append({q.at("target_parent")},{q.at("target_child")},s);
    else vista_widget_detach({q.at("target_parent")},{q.at("target_child")},s);
    check(events==expected.at("events"),"source ordered service trace differs: "+events.dump());
    for(const auto& r:expected.at("regions")) {
      const auto id=r.at("address").get<std::uint32_t>();
      check(hex(bank({id}).first(r.at("hex").get<std::string>().size()/2))==r.at("hex").get<std::string>(),"source retained bank differs at "+std::to_string(id));
    }
    const bool rebuilt=std::any_of(events.begin(),events.end(),[](const Json& e){return e[0]=="base_layout";});
    check(lifetime.pane_generation==unsigned(rebuilt),"generation must follow actual dynamic rebuild");
  }
  VistaWidgetIdentity create(VistaWidgetIdentity parent,const VistaWidgetCreate& request,const VistaWidgetRegistrationServices& s) {
    const std::array<const char*,5> names{"bitmap","scene","grid","text","table"};
    const auto kind=std::size_t(request.kind);const auto id=heap+0x4000+creates*0x100;
    const auto& failures=q.at("failures");const bool fail=std::find(failures.begin(),failures.end(),creates)!=failures.end();++creates;
    event("create",names[kind],request.record.source_identity,request.element,fail?0u:id);
    if(!fail) {
      memory[id].assign(0x100,std::byte{0xa5});auto b=bank({id});write(b,0,vt);
      write(b,4,std::array<unsigned,5>{8,10,2,6,7}[kind]);write(b,0xc,0x12345678);
      for(const auto at:{0x10,0x14,0x18,0x1c})write(b,std::size_t(at),0);
      write(b,0x6c,1,1);vista_widget_append(parent,{id},s);
      if(request.kind==VistaWidgetCreateKind::text)write(b,0xa,std::uint16_t(request.element),2);
    }
    if(q.at("mutate_time").get<bool>())write(bank(parent),0x34,read(bank(parent),0x34)+1);
    return {fail?0u:id};
  }
};
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Usage: vista_widget_registration_tests fixture.json");
    std::ifstream file(argv[1]);const auto fixture=Json::parse(file);
    check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","original source pin");
    unsigned cases{};
    for(const auto& row:fixture.at("cases")){Run run(row.at("input"));run.execute(row);++cases;}
    // Unknown reached owners fail closed instead of acquiring fixture results.
    for(unsigned variant=0;variant<4;++variant) {
      Run run(fixture.at("cases")[0].at("input"));auto s=run.services();
      s.header=[](auto){};s.footer=[](auto,auto){};
      VistaWidgetRegistration registration;registration.words={0,1,0,0,0,0};
      if(variant==0)s.header={};
      if(variant==1)registration.words[1]=0xffffffffu;
      if(variant==2)registration.words[0]=1;
      if(variant==3)s.bank=[](auto){return std::span<std::byte>{};};
      bool rejected{};
      try{vista_widget_register(run.lifetime,registration,s);}catch(const FormatError&){rejected=true;}
      check(rejected,"unowned consumed boundary must be rejected");
    }
    std::cout<<"Vista widget registration: "<<cases<<" original cases, "<<checks<<" checks PASS\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
