#include "halo2/vista_widget_text_assignment.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t widget=0x1a00000,style=widget+0x78,alt=widget+0x4000;
std::vector<std::byte> unhex(std::string_view text) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid assignment fixture digit");};
  if(text.size()%2)throw FormatError("Invalid assignment fixture hex");
  std::vector<std::byte> out(text.size()/2);for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(digit(text[2*i])*16+digit(text[2*i+1]));return out;
}
std::string hex(std::span<const std::byte> b) {
  constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(b.size()*2);
  for(auto v:b){const auto x=std::to_integer<unsigned>(v);out.push_back(digits[x>>4]);out.push_back(digits[x&15]);}return out;
}
struct Fixture {
  const Json& q;std::map<std::uint32_t,std::vector<std::byte>> banks;
  Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;
  unsigned style_calls{};std::uint32_t error=0x12345678;
  explicit Fixture(const Json& row):q(row.at("input")) {
    for(auto i=row.at("initial").begin();i!=row.at("initial").end();++i)banks.emplace(static_cast<std::uint32_t>(std::stoul(i.key())),unhex(i.value().get<std::string>()));
  }
  std::span<std::byte> bytes(std::uint32_t id) {
    if(!id)return {};
    auto i=banks.upper_bound(id);if(i==banks.begin())throw FormatError("Unmapped assignment bank");--i;
    const auto offset=std::uint64_t(id)-i->first;if(offset>=i->second.size())throw FormatError("Unmapped assignment bank");return std::span(i->second).subspan(static_cast<std::size_t>(offset));
  }
  std::uint32_t read(std::uint32_t id,unsigned n=4) {
    auto b=bytes(id);if(b.size()<n)throw FormatError("Unmapped assignment read");std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;
  }
  void write(std::uint32_t id,std::uint32_t value,unsigned n) {
    auto b=bytes(id);if(b.size()<n)throw FormatError("Unmapped assignment write");for(unsigned i=0;i<n;++i)b[i]=std::byte(value>>(i*8));
  }
  void mutate(const std::string& name) {
    const auto occurrence=++occurrences[name];
    for(std::size_t i=0;i<q.at("mutations").size();++i) {
      const auto& m=q.at("mutations").at(i);
      if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;
      applied.insert(i);for(const auto& w:m.at("writes"))write(w.at(0),w.at(1),w.at(2));
    }
  }
  template<class... A>void event(const std::string& name,A... args) {
    events.push_back(Json{{"name",name},{"args",Json::array({args...})},{"widget",hex(bytes(widget).first(0x100))},{"alt",hex(bytes(alt).first(0x100))}});mutate(name);
  }
  VistaUtf16View view(std::uint32_t id){return {id,bytes(id)};}
  VistaWidgetIdentity selected_style(VistaWidgetIdentity id) {
    event("style",id.value);++style_calls;
    if(q.at("actual").get<bool>())return vista_widget_skin_style_identity(id);
    const auto& choices=q.at("styles");return {choices.at(std::min<std::size_t>(style_calls-1,choices.size()-1)).get<std::uint32_t>()};
  }
  VistaCloneStringServices string_services() {
    VistaCloneStringServices s;s.bank=[this](auto id){return bytes(id.value);};
    s.text=[this](auto id){event("text",id.value);return view(vista_widget_skin_text_identity(id));};
    s.failure.store_errno=[this](auto value){error=value;event("errno",value);};
    s.failure.invalid_parameter=[this](auto v){event("invalid",v[0],v[1],v[2],v[3],v[4]);};return s;
  }
  std::uint32_t assignment(VistaWidgetIdentity id,VistaUtf16View input) {
    const auto vtable=read(id.value);const auto pc=vtable==0x7cf218?0x60f425u:vtable==0x7cf22c?0x60f4c9u:vtable==0x7d3eb0?0x643ddbu:0u;
    if(!pc)throw FormatError("Unbound original assignment class");
    event("assign",id.value,input.identity,pc);const auto s=string_services();
    if(pc==0x60f425)return vista_widget_short_string_assign(id,input,s);
    if(pc==0x60f4c9)return vista_widget_long_string_assign(id,input,s);
    return vista_widget_clone_string_assign(id,input,s);
  }
  VistaWidgetTextAssignmentServices services() {
    VistaWidgetTextAssignmentServices s;s.bank=[this](auto id){return bytes(id.value);};
    s.style=[this](auto id){return selected_style(id);};s.assign_string=[this](auto id,auto input){return assignment(id,input);};
    s.prepare_text=[this](auto input,auto font)->std::uint8_t{event("prepare",input.identity,font);return static_cast<std::uint8_t>(q.at("prepare").get<std::uint32_t>());};return s;
  }
  std::uint32_t run() {
    const auto mode=q.at("mode").get<std::string>();
    if(mode=="style")return vista_widget_skin_style_identity({q.at("getter_this").get<std::uint32_t>()}).value;
    if(mode=="text")return vista_widget_skin_text_identity({q.at("getter_this").get<std::uint32_t>()});
    const auto source=std::map<std::string,std::uint32_t>{{"input",widget+0x8000},{"style",style},{"alt",alt},{"null",0}}.at(q.at("source"))+q.at("offset").get<std::uint32_t>();
    const auto input=view(source);const auto s=services();
    if(mode=="assign")return vista_widget_text_assign({widget},input,s);
    if(mode=="direct")return assignment({style},input);
    if(mode=="prepare")vista_widget_text_prepare_assign({widget},input,s);
    else if(mode=="font")vista_widget_text_font({widget},q.at("new_font"),s);
    else throw FormatError("Unknown original assignment mode");
    return 0;
  }
};
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Unbound assignment service admitted");}
} //namespace
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;std::size_t count{};
  for(const auto& row:fixture.at("cases")) {
    Fixture f(row);const auto result=f.run();
    if(result!=row.at("result")||f.events!=row.at("events")||f.error!=row.at("errno")) {
      std::cerr<<"assignment case "<<count<<" result "<<result<<'/'<<row.at("result")<<" events "<<f.events<<" expected "<<row.at("events")<<'\n';throw FormatError("Original assignment trace mismatch");
    }
    for(const auto& [id,bank]:f.banks)if(hex(bank)!=row.at("final").at(std::to_string(id)).get<std::string>()) {
      const auto expected=unhex(row.at("final").at(std::to_string(id)).get<std::string>());const auto m=std::mismatch(bank.begin(),bank.end(),expected.begin());
      std::cerr<<"assignment case "<<count<<" bank "<<std::hex<<id<<" offset "<<(m.first-bank.begin())<<std::dec<<'\n';throw FormatError("Original assignment bank mismatch");
    }
    ++count;
  }
  {Fixture f(fixture.at("cases").at(0));auto s=f.services();s.style={};rejects([&]{vista_widget_text_assign({widget},f.view(widget+0x8000),s);});if(!f.events.empty())throw FormatError("Unbound style changed bytes");}
  {Fixture f(fixture.at("cases").at(0));auto s=f.services();s.assign_string={};rejects([&]{vista_widget_text_assign({widget},f.view(widget+0x8000),s);});if(f.events.size()!=1)throw FormatError("Assignment boundary lost style order");}
  {Fixture f(fixture.at("cases").at(0));auto s=f.services();s.prepare_text={};rejects([&]{vista_widget_text_prepare_assign({widget},f.view(widget+0x8000),s);});}
  std::cout<<"PASS "<<count<<" original text assignment cases; capacities32/512/1024, preparation gate, fresh styles, overlap, counters and full banks\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
