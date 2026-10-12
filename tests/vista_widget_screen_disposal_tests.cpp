#include "halo2/vista_widget_screen_disposal.hpp"
#include "halo2/vista_widget_locale.hpp"
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
std::vector<std::byte> unhex(const Json& value){const auto t=(value.is_number_unsigned()?pool->at(value.get<std::size_t>()):value).get<std::string>();auto d=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid disposal hex");};std::vector<std::byte>b(t.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(d(t[i*2])*16+d(t[i*2+1]));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto c:b){const auto v=std::to_integer<unsigned>(c);s.push_back(d[v>>4]);s.push_back(d[v&15]);}return s;}
struct Fixture {
 const Json& q;std::map<std::uint32_t,std::vector<std::byte>> banks;Json events=Json::array(),trace=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;std::string failure;bool reentered{};
 explicit Fixture(const Json& row):q(row.at("input")){for(const auto& b:q.at("banks"))banks.emplace(b.at(0).get<std::uint32_t>(),unhex(b.at(1)));}
 std::span<std::byte> bytes(std::uint32_t at,std::size_t n){auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;const auto off=std::uint64_t(at)-it->first;if(off+n<=it->second.size())return std::span(it->second).subspan(std::size_t(off),n);}throw FormatError("Missing disposal source bank");}
 std::uint32_t read(std::uint32_t at,std::size_t n){std::span<std::byte>b;try{b=bytes(at,n);}catch(const FormatError&){failure="memory";trace.push_back(Json::array({"fault","r",at,n}));throw;}std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);trace.push_back(Json::array({"r",at,n,v}));return v;}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n){std::span<std::byte>b;try{b=bytes(at,n);}catch(const FormatError&){failure="memory";trace.push_back(Json::array({"fault","w",at,n}));throw;}for(std::size_t i=0;i<n;++i)b[i]=std::byte(v>>(8*i));trace.push_back(Json::array({"w",at,n,n==1?v&255:n==2?v&65535:v}));}
 Json snapshot(){Json out=Json::array();for(const auto& b:q.at("banks"))out.push_back(Json::array({b.at(0),hex(banks.at(b.at(0).get<std::uint32_t>()))}));return out;}
 void raw(std::uint32_t at,const Json& value){const auto b=unhex(value);auto output=bytes(at,b.size());std::copy(b.begin(),b.end(),output.begin());}
 template<class... T>void event(const std::string& name,T... values){Json args=Json::array();(args.push_back(values),...);events.push_back(Json{{"name",name},{"args",args},{"banks",snapshot()}});const auto n=++occurrences[name];for(std::size_t i=0;i<q.at("mutations").size();++i){const auto& m=q.at("mutations").at(i);if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=n)continue;applied.insert(i);for(const auto& w:m.at("writes"))raw(w.at(0),w.at(1));}if(q.contains("fail")&&q.at("fail")==Json::array({name,n})){failure="provider";throw FormatError("Declared genuine disposal provider failure");}}
 VistaWidgetScreenDisposalServices services(){VistaWidgetScreenDisposalServices s;s.read=[this](auto at,auto n){return read(at,n);};s.write=[this](auto at,auto v,auto n){write(at,v,n);};s.screen_query=[this](auto target,auto screen){const auto v=q.at(target==0x1800200?"index":"kind").template get<std::uint32_t>();event("query",target,screen.value,v);return v;};s.manager_remove=[this](auto target,auto manager,auto screen){event("remove",target,manager.value,screen.value);if(target!=0x637d9b)throw FormatError("Unbound actual manager virtual20");vista_widget_manager_unlink(manager,screen,services());};s.root=[this](auto child){event("root",child.value);VistaWidgetLocaleServices l;l.read=[this](auto at,auto n){return read(at,n);};return vista_widget_locale_root(child,l);};s.cleanup=[this](auto target,auto screen){event("cleanup",target,screen.value);if(q.value("reentry",false)&&!reentered){reentered=true;vista_widget_screen_dispose({0x1a06000},services());}};s.destructor=[this](auto target,auto screen,auto flag){event("destructor",target,screen.value,flag);};s.release=[this](auto screen){event("release",screen.value);};return s;}
 Json run(){const auto mode=q.at("mode");try{if(mode=="mapping")return vista_widget_manager_mapping(q.at("index"),q.at("kind")).value;const auto s=services();if(mode=="dispose")vista_widget_screen_dispose({q.at("screen")},s);else vista_widget_manager_unlink({q.at("manager")},{q.at("screen")},s);}catch(const FormatError&){if(failure.empty())throw;}return nullptr;}
};
Json expanded(Json events){for(auto& e:events)for(auto& b:e.at("banks"))b.at(1)=hex(unhex(b.at(1)));return events;}
void compare(const Json&a,const Json&b,std::size_t row,const char* label){if(a!=b){std::cerr<<"disposal case "<<row<<' '<<label<<" differs\n";if(std::string(label)=="trace"){std::size_t i{};while(i<a.size()&&i<b.size()&&a.at(i)==b.at(i))++i;std::cerr<<"at "<<i<<" counts "<<a.size()<<'/'<<b.size()<<'\n';if(i<a.size())std::cerr<<"actual "<<a.at(i)<<'\n';if(i<b.size())std::cerr<<"original "<<b.at(i)<<'\n';}throw FormatError("Original disposal comparison failed");}}
}
int main(int argc,char**argv)try{
 if(argc!=2)return 77;
 std::ifstream in(argv[1]);if(!in)return 77;Json doc;in>>doc;pool=&doc.at("byte_banks");std::size_t rows{},checks{};
 for(const auto& row:doc.at("cases")){Fixture f(row);compare(f.run(),row.at("result"),rows,"result");compare(f.failure.empty()?Json(nullptr):Json(f.failure),row.at("failure"),rows,"failure");compare(f.trace,row.at("trace"),rows,"trace");compare(f.events,expanded(row.at("events")),rows,"events");Json banks=row.at("banks");for(auto& b:banks)b.at(1)=hex(unhex(b.at(1)));compare(f.snapshot(),banks,rows,"banks");checks+=5+f.trace.size()+f.events.size()+f.banks.size();++rows;}
 std::cout<<"PASS "<<rows<<" original disposal/unlink/mapping cases; "<<checks<<" checks of live reads/writes, fresh vtables, reentry and failure prefixes\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
