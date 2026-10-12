#include "halo2/vista_lobby_initializer.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t screen_id=0x1a00000,child_id=screen_id+0x8000;
std::vector<std::byte> unhex(std::string_view s) {
  if(s.size()%2)throw FormatError("Odd initializer fixture hex");
  const auto digit=[](char c)->unsigned {
    if(c>='0'&&c<='9')return unsigned(c-'0');
    if(c>='a'&&c<='f')return unsigned(c-'a')+10;
    throw FormatError("Invalid initializer fixture hex");
  };
  std::vector<std::byte> result(s.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(std::uint8_t(16*digit(s[2*i])+digit(s[2*i+1])));
  return result;
}
std::string hex(std::span<const std::byte> s) {
  constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(2*s.size());
  for(const auto b:s){const auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
  return result;
}
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(at+n>b.size())throw FormatError("Fixture read out of bounds");
  std::uint32_t result{};for(unsigned i=0;i<n;++i)result|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);
  return result;
}
void write(std::span<std::byte> b,std::size_t at,std::uint32_t value,unsigned n=4) {
  if(at+n>b.size())throw FormatError("Fixture write out of bounds");
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(std::uint8_t(value>>(8*i)));
}
void compare(std::span<const std::byte> actual,const Json& expected,std::size_t index,std::string_view name) {
  const auto bytes=unhex(expected.get<std::string>());
  if(bytes.size()!=actual.size())throw FormatError("Initializer fixture bank size mismatch");
  const auto mismatch=std::mismatch(actual.begin(),actual.end(),bytes.begin());
  if(mismatch.first!=actual.end()) {
    std::cerr<<"initializer case "<<index<<' '<<name<<" offset 0x"<<std::hex<<(mismatch.first-actual.begin())
             <<" actual="<<std::to_integer<unsigned>(*mismatch.first)<<" original="<<std::to_integer<unsigned>(*mismatch.second)<<std::dec<<'\n';
    throw FormatError("Original initializer bank mismatch");
  }
}
template<class F>void rejects(F&& f) {
  try{f();}catch(const FormatError&){return;}
  throw FormatError("Invalid initializer service boundary admitted");
}
struct Fixture {
  const Json& row;const Json& q;std::map<std::string,std::vector<std::byte>> banks;
  Json events=Json::array();std::set<std::size_t> applied;std::map<std::string,unsigned> occurrences;
  std::size_t factory{},hosts{},variants{},selected{};std::vector<std::byte> variant;
  explicit Fixture(const Json& value):row(value),q(value.at("input")) {
    for(auto it=row.at("initial").begin();it!=row.at("initial").end();++it)banks[it.key()]=unhex(it.value().get<std::string>());
  }
  std::span<std::byte> bank(VistaWidgetIdentity id) {
    if(id.value>=screen_id&&std::uint64_t(id.value)-screen_id<banks.at("screen").size())return std::span(banks.at("screen")).subspan(id.value-screen_id);
    if(id.value>=child_id&&std::uint64_t(id.value)-child_id<banks.at("child").size())return std::span(banks.at("child")).subspan(id.value-child_id);
    throw FormatError("Undeclared fixture widget identity");
  }
  void mutate(const std::string& name) {
    const auto occurrence=++occurrences[name];
    if(!q.contains("mutations"))return;
    for(std::size_t i=0;i<q.at("mutations").size();++i) {
      const auto& m=q.at("mutations").at(i);
      if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;
      applied.insert(i);
      if(m.contains("chat"))write(banks.at("screen"),0x1c6a,m.at("chat"),1);
      if(m.contains("descriptor"))write(banks.at("descriptor"),0,m.at("descriptor"));
      if(m.contains("style"))for(unsigned j=0;j<5;++j)write(banks.at("child"),0x70+4*j,m.at("style").at(j));
      if(m.contains("animation"))write(banks.at("child"),0x68,m.at("animation"),2);
      if(m.contains("manager_slot"))write(banks.at("manager"),0x10+4*m.at("manager_slot").at(0).get<unsigned>(),m.at("manager_slot").at(1));
      if(m.contains("screen_byte"))write(banks.at("screen"),m.at("screen_byte").at(0),m.at("screen_byte").at(1),1);
      if(m.contains("child_word"))write(banks.at("child"),m.at("child_word").at(0),m.at("child_word").at(1));
    }
  }
  template<class... T>void event(const std::string& name,T&&... values) {
    Json entry=Json::array();entry.push_back(name);(entry.push_back(std::forward<T>(values)),...);events.push_back(std::move(entry));mutate(name);
  }
  static Json text(std::u16string_view value) {Json result=Json::array();for(auto c:value)result.push_back(std::uint16_t(c));return result;}
  VistaLobbyInitializerState state() {
    return {{screen_id},banks.at("descriptor"),banks.at("manager"),banks.at("globals"),row.at("registration_table")};
  }
  VistaLobbyInitializerServices services() {
    VistaLobbyInitializerServices s;
    s.bank=[this](auto id){return bank(id);};
    s.definition=[this](auto id){event("definition",id);return 0x12345678u;};
    s.context=[this]{event("context");return 0x23456789u;};
    s.chat_available=[this](auto context){const auto value=q.at("chat").get<std::uint8_t>();event("chat",context,value);return value;};
    s.factory_flag=[this]{const auto value=q.at("factory").at(factory++).get<std::uint8_t>();event("factory",value);return value;};
    s.register_children=[this](auto definition,const auto& registration){
      Json table=Json::array();for(const auto id:registration.buttons)table.push_back(id.value);
      event("register",definition,registration.words,table,hex(registration.tail));
    };
    s.language=[this]{const auto value=q.at("language").get<unsigned>();event("language",value);return value;};
    s.style=[this](auto id){
      const auto offset=id.value==screen_id+0xd5c?0x100u:0x70u;
      event("style",id.value,id.value+offset);
      return VistaLobbyInitializerStyle{id.value+offset,bank({id.value+offset})};
    };
    s.assign_string=[this](auto id,auto value){
      event("assign",id,text(value));auto owner=bank({id});
      for(std::size_t i=0;i<value.size();++i)write(owner,0x44+2*i,std::uint16_t(value[i]),2);
      write(owner,0x44+2*value.size(),0,2);
    };
    s.identifier_observed=[this](auto id){event("identifier",id.value);};
    s.identifier_child=[this](auto id){assign_ids(id);};
    s.host=[this]{const auto value=q.at("hosts").at(hosts++).get<std::uint8_t>();event("host",value);return value;};
    s.activate_focus=[this](auto id){event("activate",id.value);};
    s.focus=[this](auto id,auto first,auto second){event("focus",id.value,first,second);};
    s.variant=[this]()->std::optional<std::span<const std::byte>> {
      const auto& value=q.at("variants").at(variants++);event("variant",value);
      if(value.is_null())return std::nullopt;
      variant=unhex(value.template get<std::string>());return std::span<const std::byte>(variant);
    };
    s.selected_map=[this](auto& outputs){
      const auto& value=q.at("maps").at(selected++);event("selected",value);
      if(!value.at("first").is_null())outputs.first=value.at("first").template get<std::uint32_t>();
      if(!value.at("second").is_null())outputs.second=value.at("second").template get<std::uint32_t>();
      if(!value.at("third").is_null())outputs.third=value.at("third").template get<std::uint32_t>();
      return value.at("ok").template get<std::uint8_t>();
    };
    s.favorite=[this]{const auto value=q.at("favorite").get<std::uint8_t>();event("favorite",value);return value;};
    s.prepare_text=[this](auto value,auto font){event("prepare_text",text(value),font);return q.at("prepare_result").get<std::uint8_t>();};
    s.release_owned_string=[this](auto id){event("release",id);};
    s.text=[this](auto index){event("text",index);return VistaWidgetIdentity{q.at("child").get<unsigned>()?child_id:0u};};
    s.animated_bounds=[this](auto id){event("animated_bounds",id.value);VistaLobbyInitializerBounds b{};const auto raw=unhex(q.at("child_bounds").get<std::string>());std::copy(raw.begin(),raw.end(),b.begin());return b;};
    s.append=[this](auto id){
      event("append",id.value);
      // Declared lower61208E binding, limited to the actual fixture's retained
      //linked tree; the production orchestration does not implement append.
      auto current=read(bank({screen_id}),0x14);
      if(!current)write(bank({screen_id}),0x14,id.value);
      else{while(read(bank({current}),0x18))current=read(bank({current}),0x18);write(bank({current}),0x18,id.value);write(bank(id),0x1c,current);}
      write(bank(id),0x10,screen_id);mutate("after_append");
    };
    s.initialize_clone=[this](auto id,auto flags,auto animation,const auto& vector,auto font,const auto& bounds){event("clone_initialize",id.value,flags,animation,vector,font,hex(bounds));};
    return s;
  }
  void assign_ids(VistaWidgetIdentity id) {
    VistaLobbyIdentifierServices s;
    s.bank=[this](auto key){return bank(key);};s.observed=[this](auto key){event("identifier",key.value);};
    s.child_method=[this](auto key){assign_ids(key);};vista_lobby_assign_widget_identifiers(id,s);
  }
};
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;
  std::size_t count{};
  for(const auto& row:fixture.at("cases")) {
    Fixture f(row);auto state=f.state();const auto services=f.services();vista_lobby_initialize(state,services);
    if(f.events!=row.at("events")) {
      std::size_t i{};const auto& expected=row.at("events");
      while(i<std::min(f.events.size(),expected.size())&&f.events.at(i)==expected.at(i))++i;
      std::cerr<<"initializer case "<<count<<" event "<<i<<" actual="<<(i<f.events.size()?f.events.at(i):Json())
               <<" original="<<(i<expected.size()?expected.at(i):Json())<<'\n';throw FormatError("Original initializer call order mismatch");
    }
    for(const auto& [name,bytes]:f.banks)compare(bytes,row.at("final").at(name),count,name);
    if(f.factory!=2)throw FormatError("Independent source factory samples collapsed");
    ++count;
  }
  std::size_t births{};
  for(const auto& row:fixture.at("manager_birth_cases")) {
    auto b=unhex(row.at("initial").get<std::string>());vista_lobby_chat_manager_construct(b,row.at("identity"));compare(b,row.at("final"),births++,"manager birth");
  }
  const auto& first=fixture.at("cases").at(0);
  {Fixture f(first);auto state=f.state();auto s=f.services();s.prepare_text={};rejects([&]{vista_lobby_initialize(state,s);});}
  {Fixture f(first);auto state=f.state();state.registration_table=0;const auto s=f.services();rejects([&]{vista_lobby_initialize(state,s);});}
  {Fixture f(first);auto state=f.state();state.globals=state.globals.first(0x144);const auto s=f.services();rejects([&]{vista_lobby_initialize(state,s);});}
  {std::vector<std::byte> manager(vista_lobby_chat_manager_bank_size,std::byte{0xa5});rejects([&]{vista_lobby_chat_manager_construct(manager,0xfffffff0);});}
  {std::vector<std::byte> manager(vista_lobby_chat_manager_bank_size);vista_lobby_chat_manager_clear(manager,{});write(manager,0x10,1);rejects([&]{vista_lobby_chat_manager_clear(manager,{});});}
  {Fixture f(first);write(f.banks.at("screen"),0x10,screen_id);rejects([&]{f.assign_ids({screen_id});});}
  std::cout<<"PASS "<<count<<" original Vista initializer cases and "<<births<<" manager births; exact full banks, callbacks, separate queries, ID recursion and declared lower-service boundaries\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
