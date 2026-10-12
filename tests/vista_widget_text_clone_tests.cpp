#include "halo2/vista_widget_text_clone.hpp"
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
constexpr std::uint32_t widget_id=0x1a00000,style0_id=widget_id+0x4000,style1_id=widget_id+0x5000;
std::vector<std::byte> unhex(std::string_view raw) {
  if(raw.size()%2)throw FormatError("Invalid text clone fixture hex");
  const auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid hex digit");};
  std::vector<std::byte> output(raw.size()/2);
  for(std::size_t i=0;i<output.size();++i)output[i]=std::byte(std::uint8_t(16*digit(raw[2*i])+digit(raw[2*i+1])));
  return output;
}
std::string hex(std::span<const std::byte> raw) {
  constexpr char digits[]="0123456789abcdef";std::string output;output.reserve(raw.size()*2);
  for(auto b:raw){const auto c=std::to_integer<unsigned>(b);output+=digits[c>>4];output+=digits[c&15];}return output;
}
std::uint32_t read(std::span<const std::byte> bank,std::size_t at,unsigned n=4) {
  if(at>bank.size()||n>bank.size()-at)throw FormatError("Fixture read exceeds bank");
  std::uint32_t output{};for(unsigned i=0;i<n;++i)output|=std::to_integer<std::uint32_t>(bank[at+i])<<(8*i);return output;
}
void write(std::span<std::byte> bank,std::size_t at,std::uint32_t value,unsigned n=4) {
  if(at>bank.size()||n>bank.size()-at)throw FormatError("Fixture write exceeds bank");
  for(unsigned i=0;i<n;++i)bank[at+i]=std::byte(std::uint8_t(value>>(8*i)));
}
Json utf16(std::span<const std::byte> bytes) {
  Json result=Json::array();
  for(std::size_t i=0;i+1<bytes.size();i+=2){const auto c=read(bytes,i,2);if(!c)return result;result.push_back(c);}
  throw FormatError("Fixture text lacks terminator");
}
void write_text(std::span<std::byte> target,const Json& text) {
  for(std::size_t i=0;i<text.size();++i)write(target,2*i,text.at(i),2);
  write(target,2*text.size(),0,2);
}
void compare(std::span<const std::byte> bank,const Json& value,std::size_t row,std::string_view name) {
  const auto expected=unhex(value.get<std::string>());
  if(bank.size()!=expected.size())throw FormatError("Text clone bank size differs");
  const auto difference=std::mismatch(bank.begin(),bank.end(),expected.begin());
  if(difference.first!=bank.end()) {
    std::cerr<<"clone case "<<row<<' '<<name<<" offset 0x"<<std::hex<<(difference.first-bank.begin())<<" actual="<<std::to_integer<unsigned>(*difference.first)<<" original="<<std::to_integer<unsigned>(*difference.second)<<std::dec<<'\n';
    throw FormatError("Original clone byte mismatch");
  }
}
template<class F>void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Invalid text clone boundary admitted");}
struct Fixture {
  const Json& row;const Json& q;std::map<std::string,std::vector<std::byte>> banks;
  std::vector<std::byte> raw_descriptor,raw_vector,raw_bounds,text_scratch;
  std::span<std::byte> descriptor;Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;std::size_t styles{};
  explicit Fixture(const Json& value):row(value),q(row.at("input")) {
    for(auto it=row.at("initial").begin();it!=row.at("initial").end();++it)banks[it.key()]=unhex(it.value().get<std::string>());
    raw_descriptor=unhex(q.at(q.at("mode")=="bridge"?"bridge_scratch":"descriptor").get<std::string>());
    raw_vector=unhex(q.at("vector").get<std::string>());raw_bounds=unhex(q.at("bounds").get<std::string>());text_scratch=unhex(q.at("text_scratch").get<std::string>());
    descriptor=raw_descriptor;
    if(q.at("mode")=="consume"&&!q.at("descriptor_alias").is_null())descriptor=std::span(banks.at("widget")).subspan(q.at("descriptor_alias").get<std::size_t>(),0x2c);
  }
  std::span<std::byte> bank(VistaWidgetIdentity id) {
    for(const auto& [identity,name]:std::array<std::pair<std::uint32_t,const char*>,7>{{{widget_id,"widget"},{widget_id+0x1000,"parent"},{widget_id+0x2000,"root"},{style0_id,"style0"},{style1_id,"style1"},{widget_id+0x6000,"context"},{widget_id+0xb000,"tag"}}})
      if(id.value==identity)return banks.at(name);
    throw FormatError("Unknown clone fixture identity");
  }
  void mutate(const std::string& name) {
    const auto occurrence=++occurrences[name];if(!q.contains("mutations"))return;
    for(std::size_t i=0;i<q.at("mutations").size();++i) {
      const auto& m=q.at("mutations").at(i);
      if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;
      applied.insert(i);
      for(const auto& key:{"descriptor","widget","context","style0","style1"})if(m.contains(key)) {
        const auto& action=m.at(key);write(std::string_view(key)=="descriptor"?descriptor:std::span(banks.at(key)),action.at(0),action.at(1),action.at(2));
      }
    }
  }
  template<class... T>void event(const std::string& name,T&&... values) {
    Json entry=Json::array();entry.push_back(name);(entry.push_back(std::forward<T>(values)),...);events.push_back(std::move(entry));mutate(name);
  }
  VistaWidgetTextCloneServices services() {
    VistaWidgetTextCloneServices s;
    s.consuming=[this](auto,auto id,auto data){event("consume",id,hex(data));};
    s.bank=[this](auto id){return bank(id);};
    s.definition=[this](auto definition){event("definition",definition);return q.at("definition").get<std::uint32_t>();};
    s.tag=[this](auto definition)->std::optional<std::span<const std::byte>>{event("tag",definition);if(!q.at("tag").get<unsigned>())return std::nullopt;return banks.at("tag");};
    s.localized=[this](auto tag,auto id,auto output){event("localized",tag,id);write_text(output,q.at("localized"));};
    s.style=[this](auto){const auto index=q.at("styles").at(styles++%q.at("styles").size()).get<unsigned>();const auto id=index?style1_id:style0_id;event("style",id);return VistaWidgetIdentity{id};};
    s.assign_string=[this](auto id,auto text){const auto chars=utf16(text);event("assign",id.value,chars);write_text(bank(id).subspan(0x44),chars);};
    s.context=[this]()->std::optional<std::span<const std::byte>>{event("context",q.at("context"));if(!q.at("context").get<unsigned>())return std::nullopt;return banks.at("context");};
    s.text=[this](auto id)->std::span<const std::byte>{event("text",id.value);return bank(id).subspan(0x44);};
    s.prepare_text=[this](auto text,auto font){event("prepare",utf16(text),font);return q.at("prepare").get<std::uint8_t>();};
    return s;
  }
  void run(const VistaWidgetTextCloneServices& services) {
    const std::span<std::byte,0x400> scratch{text_scratch.data(),0x400};
    if(q.at("mode")=="bridge") {
      const auto vector=q.at("vector_alias").is_null()?std::span<const std::byte>(raw_vector):std::span<const std::byte>(descriptor).subspan(q.at("vector_alias").get<std::size_t>(),16);
      const auto bounds=q.at("bounds_alias").is_null()?std::span<const std::byte>(raw_bounds):std::span<const std::byte>(descriptor).subspan(q.at("bounds_alias").get<std::size_t>(),8);
      vista_widget_text_clone_initialize({widget_id},q.at("flags"),q.at("animation"),std::span<const std::byte,16>{vector.data(),16},q.at("font"),std::span<const std::byte,8>{bounds.data(),8},std::span<std::byte,0x2c>{descriptor.data(),0x2c},scratch,services);
    } else vista_widget_text_descriptor({widget_id},q.at("identifier"),std::span<const std::byte,0x2c>{descriptor.data(),0x2c},scratch,services);
    if(q.at("flag").get<unsigned>()){event("flag");vista_widget_text_clone_flag({widget_id},services);}
  }
};
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;std::size_t count{};
  for(const auto& row:fixture.at("cases")) {
    Fixture f(row);const auto services=f.services();f.run(services);
    if(f.events!=row.at("events")) {
      const auto& expected=row.at("events");std::size_t i{};while(i<std::min(f.events.size(),expected.size())&&f.events.at(i)==expected.at(i))++i;
      std::cerr<<"clone case "<<count<<" event "<<i<<" actual="<<(i<f.events.size()?f.events.at(i):Json())<<" original="<<(i<expected.size()?expected.at(i):Json())<<'\n';throw FormatError("Original clone call order differs");
    }
    for(const auto& [name,bytes]:f.banks)compare(bytes,row.at("final").at(name),count,name);
    compare(f.descriptor,row.at("descriptor"),count,"descriptor");compare(f.text_scratch,row.at("text_buffer"),count,"text scratch");++count;
  }
  const auto& first=fixture.at("cases").at(0);
  {Fixture f(first);auto s=f.services();s.prepare_text={};rejects([&]{f.run(s);});}
  {Fixture f(first);auto s=f.services();s.assign_string={};rejects([&]{f.run(s);});}
  {Fixture f(first);auto s=f.services();s.definition={};s.tag={};s.localized={};f.run(s);}
  {Fixture f(first);auto s=f.services();write(f.banks.at("parent"),0x10,widget_id+0x1000);rejects([&]{f.run(s);});}
  std::cout<<"PASS "<<count<<" original clone/text cases; exact descriptor padding, font/flags/bounds, alias/read order, retained banks and mandatory lower services\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
