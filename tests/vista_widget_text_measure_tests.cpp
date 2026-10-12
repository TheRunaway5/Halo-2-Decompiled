#include "halo2/vista_widget_text_measure.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using Json=nlohmann::json;
namespace {
const Json* pool{};
std::vector<std::byte> unhex(const Json& value){const auto text=(value.is_number_unsigned()?pool->at(value.get<std::size_t>()):value).get<std::string>();auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid measurement hex");};std::vector<std::byte>b(text.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(digit(text[i*2])*16+digit(text[i*2+1]));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string v;for(auto c:b){const auto n=std::to_integer<unsigned>(c);v.push_back(d[n>>4]);v.push_back(d[n&15]);}return v;}
void compare(std::span<const std::byte>a,const Json& e,std::size_t row,const std::string& name){const auto b=unhex(e);if(a.size()!=b.size())throw FormatError("Measurement extent mismatch");const auto m=std::mismatch(a.begin(),a.end(),b.begin());if(m.first!=a.end()){std::cerr<<"measurement "<<row<<' '<<name<<" off="<<std::hex<<(m.first-a.begin())<<" actual="<<std::to_integer<unsigned>(*m.first)<<" original="<<std::to_integer<unsigned>(*m.second)<<std::dec<<'\n';throw FormatError("Original measurement byte mismatch");}}
struct Fixture {
 const Json& q;std::map<std::uint32_t,std::vector<std::byte>> banks;std::vector<std::byte> scratch;std::uint32_t id;Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;
 explicit Fixture(const Json& row):q(row.at("input")),scratch(unhex(q.at("scratch"))),id(q.at("scratch_id")){for(const auto& b:q.at("banks"))banks.emplace(b.at(0).get<std::uint32_t>(),unhex(b.at(1)));}
 std::span<std::byte> bytes(std::uint32_t at,std::size_t n){if(at>=id&&std::uint64_t(at)+n<=std::uint64_t(id)+scratch.size())return std::span(scratch).subspan(at-id,n);auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;const auto off=std::uint64_t(at)-it->first;if(off+n<=it->second.size())return std::span(it->second).subspan(std::size_t(off),n);}throw FormatError("Missing measured source bank "+std::to_string(at));}
 std::uint32_t read(std::uint32_t at,std::size_t n=4){auto b=bytes(at,n);std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n=4){auto b=bytes(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte(v>>(8*i));}
 void raw(std::uint32_t at,const Json& value){const auto b=unhex(value);auto out=bytes(at,b.size());std::copy(b.begin(),b.end(),out.begin());}
 void mutate(const std::string& name){const auto n=++occurrences[name];for(std::size_t i=0;i<q.at("mutations").size();++i){const auto& m=q.at("mutations").at(i);if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=n)continue;applied.insert(i);for(const auto& w:m.at("writes"))raw(w.at(0),w.at(1));}}
 template<class... T>void event(const std::string& name,T... values){Json e=Json::array();e.push_back(name);(e.push_back(values),...);e.push_back(hex(scratch));events.push_back(std::move(e));mutate(name);}
 VistaTextMeasureServices services(){VistaTextMeasureServices s;s.x87_control=q.at("x87_control");s.mxcsr=q.at("mxcsr");s.read=[this](auto at,auto n){return read(at,n);};s.write=[this](auto at,auto v,auto n){write(at,v,n);};
  s.text=[this](auto style){const auto& a=q.at("texts");const auto v=a.at(std::min<std::size_t>(occurrences["text"],a.size()-1)).get<std::uint32_t>();event("text",style,v);return v;};
  s.core=[this](auto a){event("core",a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11]);const auto& values=q.at("core_results");const auto& v=values.at(std::min<std::size_t>(occurrences["core"]-1,values.size()-1));raw(a[11],v.at(0));raw(a[10],v.at(1));};
  s.copy_wide=[this](auto dest,auto input,auto cap){event("copy",dest,input,cap);for(std::uint32_t i=0;i<cap;++i){const auto c=i<cap-1?read(input+2*i,2):0u;write(dest+2*i,c,2);if(!c)break;}};
  s.rewrite=[this](auto text,auto cap,auto previous){const auto& a=q.at("rewrite");const auto v=a.at(std::min<std::size_t>(occurrences["rewrite"],a.size()-1)).get<std::uint8_t>();event("rewrite",text,cap,previous,v);return v;};
  s.font_definition=[this](auto font){event("definition",font);return q.at("definition").get<std::uint32_t>();};
  s.project_point=[this](auto point,auto viewport){const auto index=occurrences["project"];event("project",point,viewport);const auto& a=q.at("points").at(index);for(unsigned i=0;i<3;++i)write(point+i*4,a.at(i));};
  s.layout=[this](auto a){event("layout",a[0],a[1],a[2],a[3],a[4]);raw(a[2],q.at("layout_metrics"));raw(a[3],q.at("layout_result"));};return s;}
 void run(const VistaTextMeasureServices& s){if(q.at("mode")=="style")vista_text_style_measure(0x1a00000,0x1918004,id,std::span<std::byte,0x28>(scratch.data(),scratch.size()),s);else vista_text_core_measure(0x1918004,id,std::span<std::byte,0x83c>(scratch.data(),scratch.size()),s);}
};
template<class F>void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Invalid measured owner admitted");}
}
int main(int argc,char** argv)try{
 if(argc!=2)return 77;
 std::ifstream in(argv[1]);if(!in)return 77;Json doc;in>>doc;pool=&doc.at("byte_banks");std::size_t count{},checks{};
 for(const auto& row:doc.at("cases")){Fixture f(row);f.run(f.services());if(f.events!=row.at("events")){std::size_t i{};const auto& e=row.at("events");while(i<f.events.size()&&i<e.size()&&f.events.at(i)==e.at(i))++i;std::cerr<<"measurement case "<<count<<" event "<<i<<" counts="<<f.events.size()<<'/'<<e.size()<<'\n';if(i<f.events.size())std::cerr<<"actual="<<f.events.at(i)<<'\n';if(i<e.size())std::cerr<<"original="<<e.at(i)<<'\n';throw FormatError("Original measurement trace mismatch");}compare(f.scratch,row.at("scratch"),count,"scratch");for(const auto& b:row.at("banks"))compare(f.banks.at(b.at(0).get<std::uint32_t>()),b.at(1),count,std::to_string(b.at(0).get<std::uint32_t>()));checks+=1+f.banks.size()+f.events.size();++count;}
 {Fixture f(doc.at("cases").at(0));auto s=f.services();s.mxcsr=0;rejects([&]{f.run(s);});if(!f.events.empty())throw FormatError("Invalid measured FP progressed");}
 for(const auto& row:doc.at("cases"))if(row.at("input").at("mode")=="core"){Fixture f(row);auto s=f.services();s.layout={};rejects([&]{f.run(s);});if(f.occurrences["layout"])throw FormatError("Missing layout synthesized widths");break;}
 for(const auto& row:doc.at("cases"))if(row.at("input").at("mode")=="core"&&row.at("input").at("args").at(0)!=0){
  for(unsigned missing=0;missing<5;++missing){Fixture f(row);auto s=f.services();if(missing==0)s.copy_wide={};if(missing==1)s.rewrite={};if(missing==2)s.font_definition={};if(missing==3)s.project_point={};if(missing==4)s.layout={};rejects([&]{f.run(s);});}break;
 }
 {const auto& row=doc.at("cases").at(0);for(unsigned missing=0;missing<2;++missing){Fixture f(row);auto s=f.services();if(missing==0)s.text={};else s.core={};rejects([&]{f.run(s);});}}
 std::cout<<"PASS "<<count<<" original style/core measurement cases; "<<checks<<" checks of borrowed banks, scratch, call order and FP results\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
