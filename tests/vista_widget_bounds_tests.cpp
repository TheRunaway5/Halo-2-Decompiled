#include "halo2/vista_widget_bounds.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
using namespace halo2;
using Json=nlohmann::json;
namespace {
std::vector<std::byte> unhex(const std::string&raw) {
  const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};std::vector<std::byte>b(raw.size()/2);
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte{std::uint8_t(digit(raw[i*2])*16+digit(raw[i*2+1]))};
  return b;
}
std::string hex(std::span<const std::byte>b) {
  constexpr char d[]="0123456789abcdef";std::string out(b.size()*2,'0');
  for(std::size_t i=0;i<b.size();++i){const auto v=std::to_integer<unsigned>(b[i]);out[2*i]=d[v>>4];out[2*i+1]=d[v&15];}return out;
}
std::uint32_t scalar(std::span<const std::byte>b) {std::uint32_t v{};for(std::size_t i=0;i<b.size();++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;}
std::vector<std::byte> load(const char*path) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in||in.tellg()<0)throw FormatError("Actual Vista source cache missing");
  std::vector<std::byte>b(std::size_t(in.tellg()));in.seekg(0);if(!in.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))throw FormatError("Short actual Vista cache");return b;
}
struct Segment {std::uint32_t base{};std::vector<std::byte>bytes;};
#if defined(__i386__) || defined(__x86_64__)
struct FloatContext {
  unsigned mxcsr=_mm_getcsr();std::uint16_t control{};
  FloatContext(unsigned mxcsr_in,std::uint16_t control_in){asm volatile("fnstcw %0":"=m"(control));_mm_setcsr(mxcsr_in);asm volatile("fldcw %0"::"m"(control_in));}
  ~FloatContext(){_mm_setcsr(mxcsr);asm volatile("fldcw %0"::"m"(control));}
};
#endif
}
int main(int argc,char**argv)try {
  if(argc!=3)return 77;
#if !defined(__i386__) && !defined(__x86_64__)
  return 77;
#else
  std::ifstream in(argv[2]);if(!in)return 77;Json f;in>>f;
  if(f.at("schema")!="H2_VISTA_WIDGET_BOUNDS_ORIGINAL_V1" || f.at("map").at("sha256")!="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44")throw FormatError("Bounds source authority changed");
  const auto input=load(argv[1]);std::uint64_t fingerprint=0xcbf29ce484222325ull;
  for(const auto byte:input)fingerprint=(fingerprint^std::to_integer<std::uint8_t>(byte))*0x100000001b3ull;
  if(input.size()!=f.at("map").at("bytes").get<std::size_t>() || fingerprint!=std::stoull(f.at("map").at("fnv64").get<std::string>(),nullptr,16))throw FormatError("Actual bounds cache bytes changed");
  const auto obj=f.at("object").get<std::uint32_t>();std::size_t cases{},checks{},faults{},actual{};
  for(const auto&q:f.at("cases")) {
    std::vector<Segment>segments{{obj,std::vector<std::byte>(0x20000)}};
    for(const auto&a:f.at("globals"))segments.push_back({a.get<std::uint32_t>(),std::vector<std::byte>(a==0xe3e424u?16:4)});
    const auto writable=[&](std::uint32_t at,std::size_t n)->std::span<std::byte>{
      for(auto&g:segments)if(at>=g.base && std::uint64_t(at-g.base)+n<=g.bytes.size())return std::span<std::byte>(g.bytes).subspan(at-g.base,n);
      throw FormatError("Unmapped bounds registry");
    };
    for(const auto&[at,raw]:f.at("constants").items()){const auto b=unhex(raw.get<std::string>());auto d=writable(std::uint32_t(std::stoul(at,nullptr,16)),b.size());std::copy(b.begin(),b.end(),d.begin());}
    for(const auto&p:q.at("patches")){const auto b=unhex(p.at(1).get<std::string>());auto d=writable(p.at(0).get<std::uint32_t>(),b.size());std::copy(b.begin(),b.end(),d.begin());}
    if(!q.at("source_frame").is_null()) {
      const auto&frame=q.at("source_frame");const auto raw=unhex(frame.at("raw_hex").get<std::string>());const auto offset=frame.at("file_offset").get<std::size_t>();
      if(offset>input.size()||raw.size()>input.size()-offset||!std::equal(raw.begin(),raw.end(),input.begin()+std::ptrdiff_t(offset)))throw FormatError("Actual raw Vista key differs");
      if(!std::equal(raw.begin()+8,raw.end(),writable(obj+0x58,12).begin()))throw FormatError("Declared widget does not consume exact raw Vista XYZ");
      ++actual;checks+=2;
    }
    Json events=Json::array(),reads=Json::array(),writes=Json::array();VistaWidgetBoundsServices s;
    s.read=[&](std::uint32_t at,std::size_t n){try{const auto v=scalar(writable(at,n));reads.push_back(Json::array({at,n,v}));return v;}catch(const FormatError&){events.push_back(Json::array({"memory-fault",at,n}));throw;}};
    s.write=[&](std::uint32_t at,std::uint32_t v,std::size_t n){try{auto b=writable(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte{std::uint8_t(v>>(i*8))};writes.push_back(Json::array({at,n,v}));}catch(const FormatError&){events.push_back(Json::array({"memory-fault",at,n}));throw;}};
    bool rejected=false;std::uint32_t returned{};unsigned mxcsr{};std::uint16_t control{};
    {
      FloatContext context(q.at("mxcsr").get<unsigned>(),q.at("fpcw").get<std::uint16_t>());
      const auto output=q.at("output").get<std::uint32_t>();const auto viewport=q.at("viewport").get<std::uint32_t>();
      try {
        if(q.at("owner")=="short")returned=vista_widget_short_bounds(obj,output,s);
        else if(q.at("owner")=="float")returned=vista_widget_float_bounds(obj,output,s);
        else if(q.at("owner")=="projected")returned=vista_widget_projected_bounds(obj,viewport,output,s);
        else if(q.at("owner")=="rectangle")returned=vista_widget_project_float_rectangle(output,q.at("z").get<std::uint32_t>(),viewport,s);
        else if(q.at("owner")=="translate")returned=vista_widget_translate_short_rectangle(output,q.at("dx").get<std::uint32_t>(),q.at("dy").get<std::uint32_t>(),s);
        else throw FormatError("Unknown original bounds owner");
      } catch(const FormatError&){rejected=true;}
      mxcsr=_mm_getcsr();asm volatile("fnstcw %0":"=m"(control));
    }
    const auto&expected=q.at("result");const auto name=q.at("name").get<std::string>();
    if((!expected.at("fault").is_null())!=rejected)throw FormatError("Original bounds completion differs: "+name);
    if(rejected)++faults;else if(returned!=expected.at("returned"))throw FormatError("Original borrowed bounds output differs: "+name);
    const auto csr_mask=f.value("hardware_oracle",false)?0xffffffffu:~63u;
    if((mxcsr&csr_mask)!=(expected.at("mxcsr").get<unsigned>()&csr_mask)||control!=expected.at("fpcw"))throw FormatError("Original bounds FP context differs: "+name);
    if(events!=expected.at("events"))throw FormatError("Original bounds memory-fault prefix differs: "+name);
    if(expected.contains("reads")&&reads!=expected.at("reads")) {
      std::cerr<<"expected "<<expected.at("reads")<<"\nactual "<<reads<<'\n';
      throw FormatError("Original bounds read order/value differs: "+name);
    }
    if(writes!=expected.at("writes")) {
      std::cerr<<"expected "<<expected.at("writes")<<"\nactual "<<writes<<'\n';
      throw FormatError("Original bounds write order/value differs: "+name);
    }
    for(const auto&v:expected.at("snapshots")){const auto raw=unhex(v.at(1).get<std::string>());if(hex(writable(v.at(0).get<std::uint32_t>(),raw.size()))!=v.at(1).get<std::string>())throw FormatError("Original bounds aliased bank differs: "+name);++checks;}
    ++cases;checks+=expected.contains("reads")?6:5;
  }
  std::size_t boundaries{};
  const auto rejected_boundary=[&](const VistaWidgetBoundsServices&s){
    bool rejected=false;try{vista_widget_short_bounds(obj,obj+0x100,s);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("Unproved bounds service/FP context was admitted");
    ++boundaries;
  };
  rejected_boundary({});
  rejected_boundary({[](std::uint32_t,std::size_t)->std::uint32_t{throw FormatError("Unexpected resident read");},{}});
  VistaWidgetBoundsServices untouched{
    [](std::uint32_t,std::size_t)->std::uint32_t{throw std::runtime_error("Boundary reached resident read");},
    [](std::uint32_t,std::uint32_t,std::size_t){throw std::runtime_error("Boundary reached resident write");}};
  {FloatContext context(0x1f00,0x37f);rejected_boundary(untouched);}
  // The corpus deliberately sets masked x87 sticky exceptions. Clear those
  // test-only pending flags before temporarily unmasking this rejected input.
  asm volatile("fnclex");
  {FloatContext context(0x1f80,0x37e);rejected_boundary(untouched);}
  std::cout<<"PASS: "<<cases<<" original bounds cases, "<<actual<<" actual raw-key cases, "<<checks<<" output/read/write/bank/FP-context checks, "<<faults<<" matched memory-fault prefixes, "<<boundaries<<" explicit rejected boundaries\n";return 0;
#endif
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
