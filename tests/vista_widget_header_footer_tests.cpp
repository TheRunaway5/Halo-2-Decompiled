#include "halo2/vista_widget_header_footer.hpp"
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
constexpr std::uint32_t screen_id=0x1a00000,style_id=0x1a04000,pool_id=0x2800000;
constexpr std::uint32_t authored_id=screen_id+0x1100,vtable_id=screen_id+0x16000;
std::vector<std::byte> unhex(std::string_view text) {
  if(text.size()%2)throw FormatError("Invalid footer fixture hex");
  const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a')+10;throw FormatError("Invalid footer hex digit");};
  std::vector<std::byte> result(text.size()/2);
  for(std::size_t i=0;i<result.size();++i)result[i]=std::byte(16*digit(text[2*i])+digit(text[2*i+1]));
  return result;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digit[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
  for(const auto b:bytes){const auto value=std::to_integer<unsigned>(b);result.push_back(digit[value>>4]);result.push_back(digit[value&15]);}return result;
}
void compare(std::span<const std::byte> actual,const Json& value,std::size_t row,const std::string& name) {
  const auto expected=unhex(value.get<std::string>());
  if(actual.size()!=expected.size())throw FormatError("Footer bank extent differs");
  const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
  if(mismatch.first!=actual.end()) {
    std::cerr<<"footer case "<<row<<' '<<name<<" offset 0x"<<std::hex<<(mismatch.first-actual.begin())<<" actual="<<std::to_integer<unsigned>(*mismatch.first)<<" original="<<std::to_integer<unsigned>(*mismatch.second)<<std::dec<<'\n';
    throw FormatError("Original footer byte mismatch");
  }
}
template<class F>void rejects(F&& f) {try{f();}catch(const FormatError&){return;}throw FormatError("Invalid footer boundary admitted");}
struct Fixture {
  const Json& q;std::map<std::uint32_t,std::vector<std::byte>> banks;
  std::vector<std::byte> scratch;std::uint32_t scratch_id{},error=0x12345678;
  Json events=Json::array();std::map<std::string,unsigned> occurrences;std::set<std::size_t> applied;
  bool low_fault{};std::uint32_t fault_address{};
  explicit Fixture(const Json& row):q(row.at("input")),scratch(unhex(q.at("scratch").get<std::string>())),scratch_id(q.at("scratch_identity")) {
    for(const auto& b:q.at("banks"))banks.emplace(b.at(0).get<std::uint32_t>(),unhex(b.at(1).get<std::string>()));
  }
  std::span<std::byte> bytes(std::uint32_t at,std::size_t n) {
    if(at>=scratch_id&&std::uint64_t(at)+n<=std::uint64_t(scratch_id)+scratch.size())return std::span(scratch).subspan(at-scratch_id,n);
    auto it=banks.upper_bound(at);
    if(it!=banks.begin()) {--it;const auto offset=std::uint64_t(at)-it->first;if(offset+n<=it->second.size())return std::span(it->second).subspan(std::size_t(offset),n);}
    if(at<4096){low_fault=true;fault_address=at;}
    throw FormatError("Missing actual footer source bank at "+std::to_string(at));
  }
  std::uint32_t read(std::uint32_t at,std::size_t n=4) {
    const auto bank=bytes(at,n);std::uint32_t result{};for(std::size_t i=0;i<n;++i)result|=std::to_integer<std::uint32_t>(bank[i])<<(8*i);return result;
  }
  void write(std::uint32_t at,std::uint32_t value,std::size_t n=4) {
    auto bank=bytes(at,n);for(std::size_t i=0;i<n;++i)bank[i]=std::byte(value>>(8*i));
  }
  void copy(std::uint32_t to,std::uint32_t from,std::size_t n) {
    const auto source=bytes(from,n);const std::vector<std::byte> cached(source.begin(),source.end());
    auto target=bytes(to,n);std::copy(cached.begin(),cached.end(),target.begin());
  }
  std::uint32_t style(std::uint32_t widget)const {
    if(widget==screen_id+0x80)return style_id;
    if(widget==screen_id+0x53c)return style_id+0x1000;
    if(widget==authored_id)return style_id+0x2000;
    if(widget<pool_id||widget>=pool_id+0x4000||(widget-pool_id)%0x1000)throw FormatError("Unknown declared text style");
    return style_id+0x3000+(widget-pool_id);
  }
  Json text(std::uint32_t at) {
    if(!at)return nullptr;
    Json result=Json::array();for(unsigned i=0;i<2048;++i){const auto value=read(at+i*2,2);if(!value)return result;result.push_back(value);}throw FormatError("Unterminated declared text");
  }
  void mutate(const std::string& name) {
    const auto occurrence=++occurrences[name];
    for(std::size_t i=0;i<q.at("mutations").size();++i) {
      const auto& m=q.at("mutations").at(i);if(applied.contains(i)||m.at("at")!=name||m.value("occurrence",1u)!=occurrence)continue;
      applied.insert(i);for(const auto& action:m.at("writes")) {const auto value=unhex(action.at(1).get<std::string>());auto target=bytes(action.at(0),value.size());std::copy(value.begin(),value.end(),target.begin());}
    }
  }
  template<class... T>void event(const std::string& name,T... values) {
    Json item=Json::array();item.push_back(name);(item.push_back(values),...);item.push_back(hex(scratch));events.push_back(std::move(item));mutate(name);
  }
  VistaWidgetHeaderFooterServices services() {
    VistaWidgetHeaderFooterServices s;s.mxcsr=q.at("mxcsr");
    s.read=[this](auto at,auto n){return read(at,n);};s.write=[this](auto at,auto value,auto n){write(at,value,n);};
    s.context=[this]{event("context");return q.at("context").get<bool>()?screen_id+0x17000:0u;};
    s.definition=[this](auto id){const auto& values=q.at("definitions");const auto result=values.at(std::min<std::size_t>(occurrences["definition"],values.size()-1)).get<std::uint32_t>();event("definition",id,result);return result;};
    s.tag=[this](auto datum){const auto& values=q.at("tags");const auto result=values.at(std::min<std::size_t>(occurrences["tag"],values.size()-1)).get<std::uint32_t>();event("tag",datum,result);return result;};
    s.append=[this](auto screen,auto child){event("append",screen.value,child.value);};
    s.header_locale=[this](auto widget,auto id){event("header_locale",widget.value,id);};
    s.footer_locale=[this](auto widget,auto id){event("footer_locale",widget.value,id);};
    s.style=[this](auto widget){const auto result=style(widget.value);event("style",widget.value,result);return result;};
    s.text=[this](auto widget){const auto result=q.at("null_text").get<bool>()?0u:widget+0x44;event("text",widget,result);return result;};
    s.bounds=[this](auto widget,auto output){event("bounds",widget.value,output);copy(output,widget.value+0x20,8);};
    s.initialize_text=[this](auto widget,auto flags,auto animation,auto color,auto font,auto bounds){event("initialize",widget.value,flags,animation,color,font,bounds);copy(widget.value+0x20,bounds,8);};
    s.assign_text=[this](auto widget,auto pointer){const auto values=text(pointer);event("assign",widget.value,pointer,values);auto dest=style(widget.value)+0x44;for(const auto& value:values){write(dest,value.template get<std::uint32_t>(),2);dest+=2;}write(dest,0,2);};
    s.text_child=[this](auto screen,auto index){const auto result=q.at("text_child").get<std::uint32_t>();event("text_child",screen.value,index,result);return VistaWidgetIdentity{result};};
    s.animation_descriptor=[this](auto screen,auto index,auto reason,auto output){event("descriptor",screen.value,index,reason,output);const auto value=unhex(q.at("descriptor").get<std::string>());auto dest=bytes(output,value.size());std::copy(value.begin(),value.end(),dest.begin());return q.at("descriptor_result").get<std::uint8_t>();};
    s.detach_kinds=[this](auto screen,auto first,auto last){event("detach",screen.value,first,last);};
    s.measure=[this](auto widget,auto rect,auto prefix,auto output,auto flag){event("measure",widget.value,rect,prefix,output,flag);const auto width=prefix*q.at("measure_width").get<std::uint32_t>();write(output,q.at("measure_top").get<std::int32_t>(),2);write(output+2,width,2);write(output+4,q.at("measure_bottom").get<std::int32_t>(),2);write(output+6,width,2);};
    s.allocate=[this](auto size,auto zero){const auto occurrence=occurrences["allocate"];const auto result=std::int64_t(occurrence)==q.at("allocation_failure").get<std::int64_t>()?0u:pool_id+occurrence*0x1000;event("allocate",size,zero,result);return VistaWidgetIdentity{result};};
    s.construct_text=[this](auto id,auto controller){event("construct",id.value,controller);write(id.value,vtable_id);return id;};
    s.failure.store_errno=[this](auto value){error=value;event("errno",value);};s.failure.invalid_parameter=[this](auto values){event("invalid",values[0],values[1],values[2],values[3],values[4]);};
    return s;
  }
  void run(const VistaWidgetHeaderFooterServices& s) {
    const auto mode=q.at("mode").get<std::string>();
    if(mode=="header")vista_widget_header_initialize({screen_id},scratch_id,std::span<std::byte,0x430>(scratch.data(),scratch.size()),s);
    else if(mode=="footer")vista_widget_footer_initialize({screen_id},q.at("append"),scratch_id,std::span<std::byte,0x428>(scratch.data(),scratch.size()),s);
    else vista_widget_footer_layout({screen_id},scratch_id,std::span<std::byte,0x4ac>(scratch.data(),scratch.size()),s);
  }
};
}
int main(int argc,char** argv)try {
  if(argc!=2)return 77;
  std::ifstream input(argv[1]);if(!input)return 77;Json fixture;input>>fixture;std::size_t count{},checks{};
  for(const auto& row:fixture.at("cases")) {
    Fixture f(row);bool failed=false;try{f.run(f.services());}catch(const FormatError&){if(!f.low_fault)throw;failed=true;}
    if(failed!=!row.at("fault").is_null())throw FormatError("Original footer fault-prefix mismatch");
    if(failed&&f.fault_address!=std::stoul(row.at("fault").get<std::string>().substr(11,8),nullptr,16))throw FormatError("Original footer fault address mismatch");
    if(f.events!=row.at("events")) {
      const auto& expected=row.at("events");std::size_t index=0;while(index<f.events.size()&&index<expected.size()&&f.events.at(index)==expected.at(index))++index;
      std::cerr<<"footer case "<<count<<" event "<<index<<" actual count="<<f.events.size()<<" original count="<<expected.size()<<'\n';
      if(index<f.events.size()&&index<expected.size()) {
        auto a=f.events.at(index),b=expected.at(index);const auto ah=a.back(),bh=b.back();a.erase(a.end()-1);b.erase(b.end()-1);
        std::cerr<<"actual="<<a<<" original="<<b<<'\n';compare(unhex(ah.get<std::string>()),bh,count,"event scratch");
      }
      throw FormatError("Original footer service trace mismatch");
    }
    compare(f.scratch,row.at("scratch"),count,"scratch");
    for(const auto& bank:row.at("banks"))compare(f.banks.at(bank.at(0).get<std::uint32_t>()),bank.at(1),count,std::to_string(bank.at(0).get<std::uint32_t>()));
    if(f.error!=row.at("errno").get<std::uint32_t>())throw FormatError("Original footer errno mismatch");
    checks+=f.events.size()+f.banks.size()+3;++count;
  }
  const auto& first=fixture.at("cases").at(0);
  {Fixture f(first);auto s=f.services();s.context={};rejects([&]{f.run(s);});if(!f.events.empty())throw FormatError("Absent context callback progressed");}
  {Fixture f(first);auto s=f.services();s.definition={};rejects([&]{f.run(s);});if(f.events.size()!=1)throw FormatError("Absent definition callback progressed");}
  {Fixture f(first);auto s=f.services();f.scratch_id=0xffffffff;rejects([&]{f.run(s);});if(!f.events.empty())throw FormatError("Overflow scratch callback progressed");}
  for(const auto& row:fixture.at("cases"))if(row.at("input").at("mode")=="layout") {
    Fixture f(row);auto s=f.services();s.mxcsr=0;rejects([&]{f.run(s);});if(!f.events.empty())throw FormatError("Unmasked SSE context progressed");break;
  }
  for(const auto& row:fixture.at("cases"))if(row.at("input").at("mode")=="header") {
    Fixture f(row);if((f.read(screen_id+0x15000)&4)==0)continue;
    auto s=f.services();s.append={};s.initialize_text={};s.style={};s.text={};s.assign_text={};s.header_locale={};f.run(s);break;
  }
  for(const auto& row:fixture.at("cases"))if(row.at("input").at("mode")=="layout") {
    Fixture f(row);if(!f.read(style_id+0x1044,2)||(f.read(screen_id+0x15000)&4)||!row.at("fault").is_null())continue;
    auto s=f.services();s.measure={};rejects([&]{f.run(s);});if(f.occurrences["measure"])throw FormatError("Missing measurement fabricated callback");break;
  }
  std::cout<<"PASS "<<count<<" original header/footer/layout cases; "<<checks<<" checks of full banks, scratch, service order, partial faults and masked SSE controls\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
