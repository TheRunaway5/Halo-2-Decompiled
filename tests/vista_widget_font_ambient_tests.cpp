#include "halo2/vista_widget_font_ambient.hpp"
#include <immintrin.h>
#include <cstring>
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
const Json* byte_banks{};
constexpr std::uint32_t stack=0x1918000,screen=0x1a00000,style=screen+0xb100;
std::vector<std::byte> unhex(std::string_view text) {
 const auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid font fixture hex");};
 if(text.size()%2)throw FormatError("Odd font hex extent");
 std::vector<std::byte> b(text.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(digit(text[i*2])*16+digit(text[i*2+1]));return b;
}
std::vector<std::byte> decode(const Json& value){return unhex((value.is_number_unsigned()?byte_banks->at(value.get<std::size_t>()):value).get<std::string>());}
std::string hex(std::span<const std::byte> bytes){constexpr char d[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);for(const auto b:bytes){const auto n=std::to_integer<unsigned>(b);out.push_back(d[n>>4]);out.push_back(d[n&15]);}return out;}
void compare(std::span<const std::byte> actual,const Json& expected,std::size_t row,const std::string& name) {
 const auto b=decode(expected);if(actual.size()!=b.size())throw FormatError("Font bank extent mismatch");const auto m=std::mismatch(actual.begin(),actual.end(),b.begin());
 if(m.first!=actual.end()){std::cerr<<"font case "<<row<<' '<<name<<" offset="<<std::hex<<(m.first-actual.begin())<<" actual="<<std::to_integer<unsigned>(*m.first)<<" original="<<std::to_integer<unsigned>(*m.second)<<std::dec<<'\n';throw FormatError("Original font byte mismatch");}
}
struct Fixture {
 const Json& q;std::map<std::uint32_t,std::vector<std::byte>> banks;std::vector<std::byte> state,scratch;
 std::uint32_t state_id;Json events=Json::array(),fp_events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;
 explicit Fixture(const Json& row):q(row.at("input")),state(unhex(q.at("state").get<std::string>())),scratch(unhex(q.at("scratch").get<std::string>())),state_id(q.at("state_id")){for(const auto& b:q.at("banks"))banks.emplace(b.at(0).get<std::uint32_t>(),decode(b.at(1)));}
 std::span<std::byte> bytes(std::uint32_t at,std::size_t n) {
  if(at>=state_id&&std::uint64_t(at)+n<=std::uint64_t(state_id)+state.size())return std::span(state).subspan(at-state_id,n);
  if(q.at("mode")=="measure"&&at>=stack-0x24&&std::uint64_t(at)+n<=stack-4)return std::span(scratch).subspan(at-(stack-0x24),n);
  auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;const auto off=std::uint64_t(at)-it->first;if(off+n<=it->second.size())return std::span(it->second).subspan(std::size_t(off),n);}
  throw FormatError("Missing font bank at "+std::to_string(at));
 }
 std::uint32_t read(std::uint32_t at,std::size_t n=4){const auto b=bytes(at,n);std::uint32_t v{};for(std::size_t i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
 void write(std::uint32_t at,std::uint32_t v,std::size_t n=4){auto b=bytes(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte(v>>(8*i));}
 void mutate(const std::string& name){const auto n=++occurrences[name];for(std::size_t i=0;i<q.at("mutations").size();++i){const auto& m=q.at("mutations").at(i);if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=n)continue;applied.insert(i);for(const auto& w:m.at("writes")){const auto b=unhex(w.at(1).get<std::string>());auto out=bytes(w.at(0),b.size());std::copy(b.begin(),b.end(),out.begin());}}}
 template<class... T>void event(const std::string& name,T... args){std::uint16_t cw,sw;asm volatile("fnstcw %0; fnstsw %1":"=m"(cw),"=m"(sw));const auto mx=_mm_getcsr();fp_events.push_back(Json::array({name,mx,cw,sw}));Json e=Json::array();e.push_back(name);(e.push_back(args),...);e.push_back(q.at("mode")=="prepare"&&name=="clock"?"":hex(state));e.push_back(q.at("mode")=="measure"?hex(scratch):"");events.push_back(std::move(e));mutate(name);}
 VistaWidgetFontServices services(){
  VistaWidgetFontServices s;s.x87_control=q.at("x87_control");s.mxcsr=q.at("mxcsr");
  s.read=[this](auto at,auto n){return read(at,n);};s.write=[this](auto at,auto v,auto n){write(at,v,n);};
  s.fallback_font=[this](auto font){event("fallback",font);return q.at("fallback").get<std::uint32_t>();};
  s.font_definition=[this](auto font){event("definition",font);return q.at("definition").get<std::uint32_t>();};
  s.pair_classify=[this](auto first,auto second){event("pair",first,second);return q.at("pair").get<std::uint8_t>();};
  s.glyph_request=[this](auto font,auto c){const auto& v=q.at("glyphs");const auto result=v.at(std::min<std::size_t>(occurrences["glyph"],v.size()-1)).get<std::uint8_t>();event("glyph",font,c,result);return result;};
  s.clock=[this]{event("clock");return q.at("clock").get<std::uint32_t>();};
  s.bounds=[this](auto widget,auto output){event("bounds",widget.value,output);const auto b=bytes(widget.value+0x20,8);std::vector<std::byte> copy(b.begin(),b.end());auto dest=bytes(output,8);std::copy(copy.begin(),copy.end(),dest.begin());};
  s.style=[this](auto widget){event("style",widget.value);return style;};
  s.style_measure=[this](auto owner,auto args){event("measure",owner,args[0],args[1],args[2],args[3],args[4],args[5]);const auto v=unhex(q.at("measure_output").get<std::string>());auto dest=bytes(args[4],v.size());std::copy(v.begin(),v.end(),dest.begin());};return s;
 }
 std::uint32_t run(const VistaWidgetFontServices& s){
  const auto mode=q.at("mode").get<std::string>();auto state_span=std::span<std::byte,0x44>(state.data(),state.size());
  if(mode=="convert")return vista_font_convert(q.at("input"),q.at("output"),q.at("capacity"),s);
  if(mode=="birth")return vista_font_iterator_birth_ambient(state_id,state_span,screen+0xa000,q.at("args").get<std::array<std::uint32_t,6>>(),s);
  if(mode=="next")return vista_font_iterator_next_ambient(state_id,state_span,s);
  if(mode=="prepare32")return vista_font_prepare_dwords(q.at("input"),q.at("font"),state_id,state_span,s);
  if(mode=="prepare"){const auto id=q.at("output").get<std::uint32_t>();auto b=bytes(id,0x2000);return vista_font_prepare(q.at("input"),q.at("font"),id,std::span<std::byte,0x2000>(b.data(),b.size()),state_id,state_span,s);}
  vista_widget_measure({screen},screen+0xc000,q.at("prefix"),screen+0xc100,q.at("flag"),stack-0x24,std::span<std::byte,0x20>(scratch.data(),scratch.size()),s);return 0;
 }
};
template<class F>void rejects(F&& f){try{f();}catch(const FormatError&){return;}throw FormatError("Invalid font boundary admitted");}
}
int main(int argc,char** argv)try{
 if(argc!=2)return 77;
 std::ifstream in(argv[1]);if(!in)return 77;Json document;in>>document;byte_banks=&document.at("byte_banks");std::size_t count{},checks{};
 for(const auto& row:document.at("cases")){
  Fixture f(row);auto services=f.services();
  alignas(16) std::array<std::byte,28> saved{},incoming{};
  const auto host_mxcsr=_mm_getcsr();asm volatile("fnstenv %0":"=m"(saved));
  asm volatile("fninit; fnstenv %0":"=m"(incoming));
  const auto cw=f.q.at("x87_control").get<std::uint16_t>();const auto sw=f.q.value("x87_status",std::uint16_t(0));
  std::memcpy(incoming.data(),&cw,2);std::memcpy(incoming.data()+4,&sw,2);
  asm volatile("fldenv %0"::"m"(incoming));_mm_setcsr(f.q.at("mxcsr").get<unsigned>());
  const auto result=f.run(services);
  std::uint16_t final_cw,final_sw;asm volatile("fnstcw %0; fnstsw %1":"=m"(final_cw),"=m"(final_sw));const auto final_mxcsr=_mm_getcsr();
  asm volatile("fldenv %0"::"m"(saved));_mm_setcsr(host_mxcsr);
  if(result!=row.at("result"))throw FormatError("Ambient font return mismatch");
  if(f.events!=row.at("events"))throw FormatError("Ambient font event mismatch");
  if(f.fp_events!=row.at("fp_events")){
   std::cerr<<"font ambient case "<<count<<" FP provider actual "<<f.fp_events<<" original "<<row.at("fp_events")<<'\n';throw FormatError("Ambient font provider FP mismatch");
  }
  if(final_mxcsr!=row.at("mxcsr")||final_cw!=row.at("x87_control")||final_sw!=row.at("x87_status")){
   std::cerr<<"font ambient case "<<count<<" final FP actual "<<final_mxcsr<<' '<<final_cw<<' '<<final_sw<<" original "<<row.at("mxcsr")<<' '<<row.at("x87_control")<<' '<<row.at("x87_status")<<'\n';throw FormatError("Ambient font final FP mismatch");
  }
  compare(f.state,row.at("state"),count,"state");compare(f.scratch,row.at("scratch"),count,"scratch");for(const auto& b:row.at("banks"))compare(f.banks.at(b.at(0).get<std::uint32_t>()),b.at(1),count,std::to_string(b.at(0).get<std::uint32_t>()));checks+=7+f.banks.size()+f.events.size();++count;
 }
 std::cout<<"PASS "<<count<<" physical original ambient birth/next cases; "<<checks<<" checks of banks, ordered providers and full ambient FP\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
