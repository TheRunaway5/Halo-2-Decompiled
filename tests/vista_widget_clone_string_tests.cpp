#include "halo2/vista_widget_clone_string.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t style_id=0x1a01000,input_id=0x1a04000,alt_id=0x1a08000,long_id=0x2200000;
std::vector<std::byte> unhex(std::string_view text) {
  if(text.size()%2)throw FormatError("Invalid string fixture hex");
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid string fixture digit");};
  std::vector<std::byte> result(text.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(16*digit(text[2*i])+digit(text[2*i+1]));
  return result;
}
void write(std::span<std::byte> bank,std::size_t at,std::uint32_t value,unsigned count=4) {
  if(at>bank.size()||count>bank.size()-at)throw FormatError("Fixture write exceeds bank");
  for(unsigned i=0;i<count;++i)bank[at+i]=std::byte(value>>(8*i));
}
void compare(std::span<const std::byte> actual,const Json& value,std::size_t row,const std::string& name) {
  const auto expected=unhex(value.get<std::string>());
  if(actual.size()!=expected.size())throw FormatError("Clone string bank extent differs");
  const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
  if(mismatch.first!=actual.end()) {
    std::cerr<<"string case "<<row<<' '<<name<<" offset 0x"<<std::hex<<(mismatch.first-actual.begin())<<" actual="<<std::to_integer<unsigned>(*mismatch.first)<<" original="<<std::to_integer<unsigned>(*mismatch.second)<<std::dec<<'\n';
    throw FormatError("Original clone string byte mismatch");
  }
}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Invalid clone string boundary admitted");}
struct Fixture {
  const Json& q;std::map<std::string,std::vector<std::byte>> banks;
  Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;
  std::uint32_t error=0x12345678;
  explicit Fixture(const Json& row):q(row.at("input")) {
    for(const auto* name:{"style","input","alt"})banks[name]=unhex(q.at(name).get<std::string>());
    auto& text=banks["long"];const auto count=q.at("long_units").get<std::size_t>();text.resize((count+1)*2);
    constexpr std::array<std::uint16_t,4> pattern{65,0xd83d,0xde00,0xffff};
    for(std::size_t i=0;i<count;++i)write(text,i*2,pattern[i%4],2);
    write(text,count*2,0,2);
  }
  std::uint32_t address(const std::string& name,std::uint32_t offset=0) const {
    if(name=="null")return offset;
    return std::map<std::string,std::uint32_t>{{"style",style_id},{"input",input_id},{"alt",alt_id},{"long",long_id}}.at(name)+offset;
  }
  std::span<std::byte> bytes(const std::string& name,std::size_t offset=0) {
    if(name=="null")return {};
    return std::span(banks.at(name)).subspan(offset);
  }
  VistaUtf16View view(const std::string& name,std::uint32_t offset=0) {return {address(name,offset),bytes(name,offset)};}
  void mutate(const std::string& name) {
    const auto occurrence=++occurrences[name];
    for(std::size_t i=0;i<q.at("mutations").size();++i) {
      const auto& m=q.at("mutations").at(i);
      if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;
      applied.insert(i);
      for(const auto& action:m.at("writes"))write(banks.at(action.at(0).get<std::string>()),action.at(1),action.at(2),action.at(3));
    }
  }
  template<class... T>void event(const std::string& name,T... values) {
    Json item=Json::array();item.push_back(name);(item.push_back(values),...);events.push_back(std::move(item));mutate(name);
  }
  VistaUtf16FailureServices failure() {
    VistaUtf16FailureServices s;
    s.store_errno=[this](auto value){error=value;event("errno",value);};
    s.invalid_parameter=[this](auto values){event("invalid",values[0],values[1],values[2],values[3],values[4]);};
    return s;
  }
  VistaCloneStringServices services() {
    VistaCloneStringServices s;
    s.bank=[this](auto identity)->std::span<std::byte>{if(identity.value!=style_id)throw FormatError("Unknown clone style identity");return bytes("style");};
    s.text=[this](auto identity){
      event("text",identity.value);
      if(q.at("getter_actual").get<bool>())return VistaUtf16View{vista_widget_clone_text_identity(identity),bytes("style",0x44)};
      return view(q.at("getter_bank"),q.at("getter_offset"));
    };
    s.failure=failure();return s;
  }
  std::uint32_t run() {
    const auto mode=q.at("mode").get<std::string>();
    auto source=view(q.at("source_bank"),q.at("source_offset"));
    VistaUtf16Bank destination{address(q.at("dest_bank"),q.at("dest_offset")),bytes(q.at("dest_bank"),q.at("dest_offset"))};
    if(mode=="assign")return vista_widget_clone_string_assign({style_id},source,services());
    if(mode=="copy")return vista_utf16_copy(destination,source,q.at("capacity"),failure());
    if(mode=="secure")return vista_utf16_copy_truncate(destination,source,q.at("capacity"),failure());
    if(mode=="length")return vista_utf16_length(source);
    if(mode=="style")return vista_widget_clone_style_identity({q.at("getter_this").get<std::uint32_t>()}).value;
    event("text",q.at("getter_this"));return vista_widget_clone_text_identity({q.at("getter_this").get<std::uint32_t>()});
  }
};
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;std::size_t count{};
  for(const auto& row:fixture.at("cases")) {
    Fixture f(row);const auto result=f.run();
    if(result!=row.at("result").get<std::uint32_t>()) {
      std::cerr<<"string case "<<count<<" return actual="<<result<<" original="<<row.at("result")<<'\n';throw FormatError("Original clone string return mismatch");
    }
    if(f.events!=row.at("events")||f.error!=row.at("errno").get<std::uint32_t>()) {
      std::cerr<<"string case "<<count<<" trace actual="<<f.events<<" original="<<row.at("events")<<'\n';throw FormatError("Original clone string service order mismatch");
    }
    for(const auto* name:{"style","input","alt"})compare(f.banks.at(name),row.at("final").at(name),count,name);
    ++count;
  }
  const auto& first=fixture.at("cases").at(0);
  {Fixture f(first);auto s=f.services();s.text={};const auto before=f.banks;rejects([&]{vista_widget_clone_string_assign({style_id},f.view("input"),s);});if(f.banks!=before)throw FormatError("Missing text service changed banks");}
  {Fixture f(first);auto s=f.services();s.bank={};rejects([&]{vista_widget_clone_string_assign({style_id},f.view("input"),s);});}
  {Fixture f(first);f.banks.at("style").resize(0x843);auto s=f.services();rejects([&]{vista_widget_clone_string_assign({style_id},f.view("input"),s);});if(!f.events.empty())throw FormatError("Short style reached callback");}
  {Fixture f(first);auto s=f.services();s.failure={};vista_widget_clone_string_assign({style_id},f.view("input"),s);}
  {std::array<std::byte,2> bytes{std::byte{1},std::byte{2}};rejects([&]{vista_utf16_length({1,bytes});});rejects([&]{vista_utf16_length({0,bytes});});rejects([&]{vista_utf16_length({0xffffffff,bytes});});}
  {std::array<std::byte,2> bytes{};rejects([&]{vista_utf16_copy({1,bytes},{1,bytes},2);});rejects([&]{vista_utf16_copy({0xffffffff,bytes},{1,bytes},1);});rejects([&]{vista_utf16_copy({0,{}},{1,bytes},1);});}
  std::cout<<"PASS "<<count<<" original clone string cases; full banks, UTF16 truncation/terminators, forward overlap, signed counters, callback order and source32 getters\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
