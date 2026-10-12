#include "halo2/vista_widget_glyph_support.hpp"
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
std::vector<std::byte> unhex(const Json& value){const auto t=(value.is_number_unsigned()?pool->at(value.get<std::size_t>()):value).get<std::string>();auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid glyph support hex");};std::vector<std::byte>b(t.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(digit(t[i*2])*16+digit(t[i*2+1]));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s.push_back(d[n>>4]);s.push_back(d[n&15]);}return s;}
void compare(std::span<const std::byte>a,const Json& value,std::size_t row){const auto b=unhex(value);if(a.size()!=b.size())throw FormatError("Glyph support extent mismatch");auto m=std::mismatch(a.begin(),a.end(),b.begin());if(m.first!=a.end()){std::cerr<<"support case "<<row<<" byte "<<(m.first-a.begin())<<" actual "<<std::to_integer<unsigned>(*m.first)<<" original "<<std::to_integer<unsigned>(*m.second)<<'\n';throw FormatError("Original glyph support bank mismatch");}}
struct Fixture {
 const Json& q;std::map<std::uint32_t,std::vector<std::byte>> banks;VistaGlyphEllipsisScratch scratch{};std::uint32_t id;Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;
 explicit Fixture(const Json& row):q(row.at("input")),id(q.at("scratch_id")){for(const auto& b:q.at("banks"))banks.emplace(b.at(0).get<std::uint32_t>(),unhex(b.at(1)));const auto seed=unhex(q.at("scratch"));std::copy(seed.begin(),seed.end(),scratch.begin());}
 std::span<std::byte> bytes(std::uint32_t at,std::size_t n){if(q.at("mode")=="ellipsis"&&at>=id&&std::uint64_t(at)+n<=std::uint64_t(id)+scratch.size())return std::span(scratch).subspan(at-id,n);auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;const auto off=std::uint64_t(at)-it->first;if(off+n<=it->second.size())return std::span(it->second).subspan(std::size_t(off),n);}throw FormatError("Missing glyph support source bank");}
 std::uint32_t read(std::uint32_t at,std::size_t n){const auto b=bytes(at,n);std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n){auto b=bytes(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte(v>>(i*8));}
 void raw(std::uint32_t at,const Json& value){const auto b=unhex(value);auto output=bytes(at,b.size());std::copy(b.begin(),b.end(),output.begin());}
 void mutate(const std::string& name){const auto occurrence=++occurrences[name];for(std::size_t i=0;i<q.at("mutations").size();++i){const auto& m=q.at("mutations").at(i);if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;applied.insert(i);for(const auto& w:m.at("writes"))raw(w.at(0),w.at(1));}}
 template<class... T>void event(const std::string& name,T... values){Json e=Json::array();e.push_back(name);(e.push_back(values),...);e.push_back(q.at("mode")=="ellipsis"?hex(scratch):"");events.push_back(std::move(e));mutate(name);}
 VistaGlyphSupportServices services(){VistaGlyphSupportServices s;s.read=[this](auto at,auto n){return read(at,n);};s.write=[this](auto at,auto value,auto n){write(at,value,n);};s.context=[this]{const auto v=q.at("context").get<std::uint32_t>();event("context",v);return v;};s.localized=[this](auto list,auto sid,auto output){event("localized",list,sid,output);raw(output,q.at("localized"));};s.errno_address=[this]{const auto v=q.at("errno").get<std::uint32_t>();event("errno",v);return v;};s.invalid_parameter=[this](auto args){event("invalid",args[0],args[1],args[2],args[3],args[4]);};return s;}
 Json run(const VistaGlyphSupportServices& s){const auto& args=q.at("args");if(q.at("mode")=="color")return vista_glyph_color_classify(args.at(0));if(q.at("mode")=="copy")return vista_glyph_copy_wide(args.at(0),args.at(1),args.at(2),s);vista_glyph_ellipsis(q.at("destination"),id,scratch,s);return nullptr;}
};
template<class F>void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Missing glyph provider admitted");}
}
int main(int argc,char** argv)try{
 if(argc!=2)return 77;
 std::ifstream input(argv[1]);if(!input)return 77;Json doc;input>>doc;pool=&doc.at("byte_banks");std::size_t rows{},checks{};
 for(const auto& row:doc.at("cases")){Fixture f(row);const auto result=f.run(f.services());if(result!=row.at("result")||f.events!=row.at("events")){std::cerr<<"support case "<<rows<<" actual result "<<result<<" original "<<row.at("result")<<" actual events "<<f.events<<" original "<<row.at("events")<<'\n';throw FormatError("Original glyph support trace mismatch");}if(!row.at("scratch").is_null())compare(f.scratch,row.at("scratch"),rows);for(const auto& b:row.at("banks"))compare(f.banks.at(b.at(0).get<std::uint32_t>()),b.at(1),rows);checks+=1+f.banks.size()+f.events.size();++rows;}
 bool context=false,localized=false,errors=false;
 for(const auto& row:doc.at("cases")){const auto& q=row.at("input");if(q.at("mode")=="ellipsis"&&!context){Fixture f(row);auto s=f.services();s.context={};rejects([&]{f.run(s);});context=true;}if(q.at("mode")=="ellipsis"&&q.at("context")!=0&&!localized){Fixture f(row);auto s=f.services();s.localized={};rejects([&]{f.run(s);});localized=true;}if(q.at("mode")=="copy"&&q.at("args").at(1)==0&&!errors){for(unsigned which=0;which<2;++which){Fixture f(row);auto s=f.services();if(!which)s.errno_address={};else s.invalid_parameter={};rejects([&]{f.run(s);});}errors=true;}}
 if(!context||!localized||!errors)throw FormatError("Missing glyph error fixtures");
 std::cout<<"PASS "<<rows<<" original glyph-support cases; "<<checks<<" checks of literal classification, live banks, copy errors and callback order\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
