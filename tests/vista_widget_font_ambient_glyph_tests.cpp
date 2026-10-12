#include "halo2/vista_widget_glyph_layout.hpp"
#include "halo2/vista_widget_font_ambient.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
using U=std::uint32_t;
namespace {
constexpr U obj=0x1a00000,entry=obj+0x18000,global=0x8cdf00;
std::vector<std::byte> unhex(const std::string& s) {
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid glyph bank hex");};
  std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::uint8_t(16*digit(s[2*i])+digit(s[2*i+1])));return b;
}
std::string hex(std::span<const std::byte> b) {constexpr char d[]="0123456789abcdef";std::string out;for(auto c:b){const auto v=std::to_integer<unsigned>(c);out+=d[v>>4];out+=d[v&15];}return out;}
U get(std::span<const std::byte> b,std::size_t n) {U v{};for(std::size_t i=0;i<n;++i)v|=U(std::to_integer<unsigned>(b[i]))<<(8*i);return v;}
void put(std::span<std::byte> b,U v,std::size_t n=4) {for(std::size_t i=0;i<n;++i)b[i]=std::byte(std::uint8_t(v>>(8*i)));}
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;
  Json fixture;input>>fixture;
  const auto state=[&](const Json& index) {Json out=Json::object();for(const auto& [address,bank]:fixture.at("states").at(index.get<std::size_t>()).items()){std::string data;for(const auto& id:bank)data+=fixture.at("banks").at(id.get<std::size_t>()).get<std::string>();out[address]=std::move(data);}return out;};
  std::size_t count{},reads{},writes{},services{},failures{};
  for(const auto& r:fixture.at("cases")) {
    const auto& q=r.at("input");const auto op=q.at("owner").get<std::string>();
    std::map<U,std::vector<std::byte>> regions;
    const auto initial=state(r.at("initial"));for(const auto& [key,value]:initial.items())regions.emplace(std::stoul(key),unhex(value));
    for(const auto& [key,value]:fixture.at("constants").items()) {std::vector<std::byte> b(4);put(b,value.get<U>());regions.emplace(std::stoul(key),std::move(b));}
    const auto view=[&](U a,std::size_t n)->std::span<std::byte> {auto it=regions.upper_bound(a);if(it==regions.begin())throw FormatError("Unbound glyph address");--it;const auto off=std::size_t(a-it->first);if(off>it->second.size()||n>it->second.size()-off)throw FormatError("Unbound glyph address "+std::to_string(a)+" size "+std::to_string(n));return std::span(it->second).subspan(off,n);};
    U current_state=op=="width"?q.at("eax").get<U>():0;
    const auto& args=q.at("args");for(std::size_t i=0;i<args.size();++i)put(view(entry+4+U(i)*4,4),args.at(i).get<U>());
    const auto snapshot=[&](bool published=false) {Json out=Json::object();for(auto [a,n]:std::vector<std::pair<U,std::size_t>>{{obj,0x4000},{global,0x3000},{entry+4,args.size()*4}})out[std::to_string(a)]=hex(view(a,n));if(published&&current_state)out[std::to_string(current_state)]=hex(view(current_state,0x44));return out;};
    bool nested{},applied{};Json trace=Json::array(),events=Json::array();std::map<std::string,std::size_t> counters;
    const auto tracked=[&](U a,std::size_t n) {return (a>=obj&&std::uint64_t(a)+n<=obj+0x10000)||(a>=global&&std::uint64_t(a)+n<=global+0x3000)||(a>=entry+4&&std::uint64_t(a)+n<=entry+4+args.size()*4)||fixture.at("constants").contains(std::to_string(a));};
    const auto mutate=[&](const std::string& name,U address=0) {if(q.contains("mutation")&&!applied){const auto& m=q.at("mutation");if(m.at("at")==name&&m.value("call",std::size_t{0})==counters[name]&&(name!="read"||m.at("address")==address)){applied=true;const auto bytes=unhex(m.at("hex"));auto target=view(m.at("write"),bytes.size());std::copy(bytes.begin(),bytes.end(),target.begin());}}};
    const auto read=[&](U a,std::size_t n) {if(!nested&&tracked(a,n))mutate("read",a);const U v=get(view(a,n),n);if(!nested&&tracked(a,n))trace.push_back(Json::array({"r",a,n,v}));return v;};
    const auto write=[&](U a,U v,std::size_t n) {v=n==1?v&255:n==2?v&65535:v;if(!nested&&tracked(a,n))trace.push_back(Json::array({"w",a,n,v}));put(view(a,n),v,n);};
    const auto provider=[&](const std::string& name,Json a) {events.push_back({{"name",name},{"args",a},{"state",snapshot(true)}});mutate(name);if(q.contains("fail")&&q.at("fail")==name)throw FormatError("Declared glyph provider failure");++counters[name];};
    VistaGlyphLayoutServices s;s.read=read;s.write=write;
    VistaWidgetFontServices lower;lower.read=read;lower.write=write;
    lower.fallback_font=[&](U font){provider("fallback",Json::array({font}));return q.value("fallback",font);};
    lower.font_definition=[&](U font){provider("definition",Json::array({font}));return q.value("definition",obj+0x1000);};
    s.iterator_birth=[&](U id,U vector,std::array<U,6> a) {
      current_state=id;Json values=Json::array({id,vector});for(U v:a)values.push_back(v);provider("birth",values);
      nested=true;
      const auto result=vista_font_iterator_birth_ambient(id,std::span<std::byte,0x44>(view(id,0x44)),vector,a,lower);
      nested=false;return result;
    };
    lower.pair_classify=[&](U a,U b){provider("pair",Json::array({a,b}));return std::uint8_t(q.value("pair",1u));};
    s.iterator_next=[&](U id) {current_state=id;provider("next",Json::array({id}));nested=true;const U result=vista_font_iterator_next_ambient(id,std::span<std::byte,0x44>(view(id,0x44)),lower);nested=false;return result;};
    s.glyph=[&](U font,U ch){provider("glyph",Json::array({font,ch}));const auto missing=q.value("missing",Json::array());return std::find(missing.begin(),missing.end(),ch)!=missing.end()?0u:obj+0x2000;};
    s.font_definition=[&](U font){provider("definition",Json::array({font}));return q.value("definition",obj+0x1000);};
    s.color_classify=[&](U ch){provider("color",Json::array({ch}));return std::uint8_t(q.value("color",0u));};
    s.ellipsis=[&](U dst,U e){provider("ellipsis",Json::array({dst,e}));const auto list=q.value("ellipsis",Json::array({46,46,46,0}));for(std::size_t i=0;i<list.size();++i)put(view(dst+U(i)*2,2),list.at(i),2);};
    s.convert_one=[&](U a,U b){provider("one",Json::array({a,b}));return get(view(a,2),2);};
    s.copy=[&](U dst,U src,U n){provider("copy",Json::array({dst,src,n}));std::vector<std::byte> bytes(view(src,n).begin(),view(src,n).end());std::copy(bytes.begin(),bytes.end(),view(dst,n).begin());};
    s.convert=[&](U src,U dst,U cap){provider("convert",Json::array({src,dst,cap}));U c=0;for(U i=0;i<cap;++i){c=i+1<cap?get(view(src+i*2,2),2):0;put(view(dst+i*4,4),c);if(!c)break;}return c;};
    s.raster=[&](U target,std::array<U,14> a){Json values=Json::array({target});for(U v:a)values.push_back(v);provider("raster",values);};
    const auto saved=_mm_getcsr();std::uint16_t saved_cw;asm volatile("fnstcw %0":"=m"(saved_cw));
    const auto cw=std::uint16_t(q.value("fpcw",0x37fu));asm volatile("fninit; fldcw %0"::"m"(cw));_mm_setcsr(q.value("mxcsr",0x1f80u));
    bool failed{};U returned{},cursor{};
    try {
      if(op=="classify")returned=vista_glyph_character_classify(args.at(0));
      else if(op=="kerning")returned=vista_glyph_kerning(args.at(0),args.at(1),args.at(2),s);
      else if(op=="collect")vista_glyph_collect_bounds(entry,s);
      else if(op=="width"){auto result=vista_glyph_line_width(q.at("eax"),entry,s);returned=result.width_bits;cursor=result.cursor;}
      else if(op=="draw")vista_glyph_draw(q.at("eax"),q.at("ecx").get<std::uint16_t>(),entry,s);
      else if(op=="layout")vista_glyph_layout(entry,s);
      else if(op=="dwords")vista_glyph_measure_dwords(entry,s);
      else if(op=="measure")vista_glyph_measure(entry,s);
      else throw FormatError("Unknown glyph owner");
    }catch(const FormatError& error){if(r.at("failure").is_null()){std::cerr<<"case "<<count<<" "<<op<<" "<<q.dump()<<" error "<<error.what()<<'\n';throw;}failed=true;}
    const U mxcsr=_mm_getcsr();std::uint16_t x87cw,x87status;asm volatile("fnstcw %0; fnstsw %1":"=m"(x87cw),"=m"(x87status));
    _mm_setcsr(saved);asm volatile("fldcw %0"::"m"(saved_cw));
    Json expected_events=r.at("events");for(auto& event:expected_events)event["state"]=state(event.at("state"));
    const bool returns=failed||(op!="width"&&op!="kerning"&&op!="classify")||(returned==(r.at("returned").at(0).get<U>()&(op=="classify"?255u:0xffffffffu))&&(op!="width"||cursor==r.at("returned").at(1)));
    const bool fp=failed||(mxcsr==r.at("mxcsr")&&x87cw==r.at("x87_control")&&x87status==r.at("x87_status"));
    if(trace!=r.at("trace")||events!=expected_events||snapshot()!=state(r.at("final"))||applied!=r.at("mutated")||failed!=!r.at("failure").is_null()||!returns||!fp) {
      std::cerr<<"glyph case "<<count<<" "<<op<<" trace "<<(trace==r.at("trace"))<<" events "<<(events==expected_events)<<" banks "<<(snapshot()==state(r.at("final")))<<" returns "<<returns<<" fp "<<fp<<" mxcsr "<<mxcsr<<" expected "<<r.at("mxcsr")<<" x87 "<<x87status<<" expected "<<r.at("x87_status")<<'\n';
      Json debug={{"input",q},{"actual_trace",trace},{"expected_trace",r.at("trace")},{"actual_events",events},{"expected_events",expected_events},{"actual_final",snapshot()},{"expected_final",state(r.at("final"))}};std::ofstream("/tmp/vista-font-ambient-glyph-mismatch.json")<<debug.dump(2)<<'\n';throw FormatError("Original glyph layout differential mismatch");
    }
    ++count;services+=events.size();if(failed)++failures;for(const auto& t:trace){if(t.at(0)=="r")++reads;if(t.at(0)=="w")++writes;}
  }
  VistaGlyphLayoutServices missing;
  if(vista_glyph_kerning(0,1,1,missing)!=0)throw FormatError("NULL font definition changed");
  const auto rejects=[&](auto&& action){try{action();}catch(const FormatError&){return;}throw FormatError("Missing genuine glyph provider admitted");};
  rejects([&]{vista_glyph_kerning(1,1,1,missing);});
  rejects([&]{vista_glyph_collect_bounds(1,missing);});
  rejects([&]{vista_glyph_line_width(1,2,missing);});
  rejects([&]{vista_glyph_draw(0,0,1,missing);});
  rejects([&]{vista_glyph_layout(1,missing);});
  rejects([&]{vista_glyph_measure_dwords(1,missing);});
  rejects([&]{vista_glyph_measure(1,missing);});
  missing.read=[](auto,auto){return 0u;};missing.write=[](auto,auto,auto){};
  rejects([&]{vista_glyph_collect_bounds(1,missing);});
  rejects([&]{vista_glyph_line_width(1,2,missing);});
  rejects([&]{vista_glyph_draw(0,0,1,missing);});
  rejects([&]{vista_glyph_layout(1,missing);});
  rejects([&]{vista_glyph_measure_dwords(1,missing);});
  rejects([&]{vista_glyph_measure(1,missing);});
  std::cout<<"PASS "<<count<<" physical original glyph/ambient-iterator composition cases, "<<reads<<" exact reads, "<<writes<<" exact writes, "<<services<<" exact provider calls, "<<failures<<" partial failures; full MXCSR/x87 control/status compared\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
