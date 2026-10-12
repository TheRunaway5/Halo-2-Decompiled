// Composition test: frozen bitmap hardware observations through the real
// retained mutable cache owner. Expected values remain the independent
// unchanged-original fixture; the frozen standalone bitmap test is untouched.
#include "halo2/vista_widget_bitmap_update.hpp"
#include "halo2/native_vista_widget_cache.hpp"
#include <memory>
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <span>
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
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in||in.tellg()<0)throw FormatError("Actual Vista bitmap cache missing");
  std::vector<std::byte>b(std::size_t(in.tellg()));in.seekg(0);if(!in.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))throw FormatError("Short actual Vista cache");return b;
}
struct Segment {std::uint32_t base{};std::vector<std::byte>bytes;};
#if defined(__i386__) || defined(__x86_64__)
struct FloatContext {
  unsigned mxcsr=_mm_getcsr();std::uint16_t control{};
  FloatContext(unsigned m,std::uint16_t c){asm volatile("fnstcw %0":"=m"(control));_mm_setcsr(m);asm volatile("fldcw %0"::"m"(c));}
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
  if(f.at("schema")!="H2_VISTA_WIDGET_BITMAP_UPDATE_ORIGINAL_V1" || f.at("map").at("sha256")!="eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44")throw FormatError("Bitmap source authority changed");
  const auto input=load(argv[1]);std::uint64_t fingerprint=0xcbf29ce484222325ull;
  for(const auto b:input)fingerprint=(fingerprint^std::to_integer<std::uint8_t>(b))*0x100000001b3ull;
  if(input.size()!=f.at("map").at("bytes").get<std::size_t>() || fingerprint!=std::stoull(f.at("map").at("fnv64").get<std::string>(),nullptr,16))throw FormatError("Actual bitmap cache bytes changed");
  const auto obj=f.at("object").get<std::uint32_t>();const auto base=f.at("cache_base").get<std::uint32_t>();
  const auto index=scalar(std::span<const std::byte>(input).subspan(16,4));const auto table=scalar(std::span<const std::byte>(input).subspan(index+8,4));
  auto assets=std::make_shared<const VistaWidgetCacheBank>(input);
  const auto original_bytes=std::vector<std::byte>(assets->region().begin(),assets->region().end());
  for(const auto mapping:{0u,0x20000000u,0xa0000000u,0xffac0000u}) {
    NativeVistaWidgetCache lifetime(assets,mapping);
    if(lifetime.storage_size()!=original_bytes.size() ||
       !std::equal(original_bytes.begin(),original_bytes.end(),lifetime.bytes(mapping,original_bytes.size()).begin()))
      throw FormatError("Mutable native cache did not retain the complete source region");
    const auto held=lifetime.bytes(mapping,8);const auto identity=held.data();
    lifetime.write(mapping,0x12345678,4);
    if(lifetime.read(mapping,4)!=0x12345678 || lifetime.bytes(mapping,8).data()!=identity ||
       !std::equal(original_bytes.begin(),original_bytes.end(),assets->region().begin()))
      throw FormatError("Mutable native cache changed its identity or original assets");
    bool bad_width{},bad_extent{};
    try{lifetime.read(mapping,3);}catch(const FormatError&){bad_width=true;}
    try{lifetime.write(mapping+std::uint32_t(lifetime.storage_size()),0,1);}catch(const FormatError&){bad_extent=true;}
    if(!bad_width||!bad_extent)throw FormatError("Mutable cache accepted an unowned source extent");
  }
  bool missing_asset{},wrap{};
  try{NativeVistaWidgetCache invalid({},base);}catch(const FormatError&){missing_asset=true;}
  try{NativeVistaWidgetCache invalid(assets,0xffffff00u);}catch(const FormatError&){wrap=true;}
  if(!missing_asset||!wrap)throw FormatError("Mutable cache accepted absent/wrapping original bank");
  std::size_t cases{},checks{},faults{},actual{},calls{},reads{},writes{};
  for(const auto&q:f.at("cases")) {
    NativeVistaWidgetCache cache(assets,base);
    const auto held=cache.bytes(base,cache.storage_size());const auto identity=held.data();
    std::vector<Segment>segments{{obj,std::vector<std::byte>(0x10000)},{0xe3f608u,std::vector<std::byte>(32)}};
    for(const auto&a:f.at("globals"))segments.push_back({a.get<std::uint32_t>(),std::vector<std::byte>(4)});
    for(const auto&[at,raw]:f.at("constants").items())segments.push_back({std::uint32_t(std::stoul(at,nullptr,16)),unhex(raw.get<std::string>())});
    const auto writable=[&](std::uint32_t at,std::size_t n)->std::span<std::byte>{
      if(at>=base&&std::uint64_t(at-base)+n<=cache.storage_size())return cache.bytes(at,n);
      for(auto&g:segments)if(at>=g.base&&std::uint64_t(at-g.base)+n<=g.bytes.size())return std::span<std::byte>(g.bytes).subspan(at-g.base,n);
      throw FormatError("Unmapped bitmap registry");
    };
    const auto store=[&](std::uint32_t at,std::uint32_t v,std::size_t n){auto b=writable(at,n);for(std::size_t i=0;i<n;++i)b[i]=std::byte{std::uint8_t(v>>(i*8))};};
    const auto patches=[&](const Json&rows){for(const auto&p:rows){const auto raw=unhex(p.at(1).get<std::string>());auto b=writable(p.at(0).get<std::uint32_t>(),raw.size());std::copy(raw.begin(),raw.end(),b.begin());}};
    store(0x87cd50u,base+table,4);store(0x87cd54u,base,4);store(0x882290u,base,4);patches(q.at("patches"));
    if(!q.at("source").is_null()) {
      const auto&source=q.at("source");const auto raw=unhex(source.at("raw_hex").get<std::string>());const auto at=source.at("file_offset").get<std::size_t>();
      if(at>input.size()||raw.size()>input.size()-at||!std::equal(raw.begin(),raw.end(),input.begin()+std::ptrdiff_t(at))||raw!=std::vector<std::byte>(writable(obj+0x200,56).begin(),writable(obj+0x200,56).end()))throw FormatError("Actual authored bitmap definition changed");
      ++actual;checks+=2;
    }
    Json trace=Json::array();VistaWidgetBitmapUpdateServices s;
    s.read=[&](std::uint32_t at,std::size_t n){try{const auto v=scalar(writable(at,n));trace.push_back(Json::array({"r",at,n,v}));return v;}catch(const FormatError&){trace.push_back(Json::array({"fault","r",at,n}));throw;}};
    s.write=[&](std::uint32_t at,std::uint32_t v,std::size_t n){try{store(at,v,n);trace.push_back(Json::array({"w",at,n,v}));}catch(const FormatError&){trace.push_back(Json::array({"fault","w",at,n}));throw;}};
    std::map<std::string,std::size_t> ordinals;
    const auto service=[&](const std::string&name,const Json&args)->std::uint32_t{
      trace.push_back(Json::array({"call",name,args}));const auto ordinal=ordinals[name]++;
      if(!q.at("providers").contains(name))throw FormatError("missing-provider/"+name);
      const auto&list=q.at("providers").at(name);const auto&e=list.at(std::min(ordinal,list.size()-1));
      if(e.value("throw",false))throw FormatError("provider-fault/"+name);
      if(e.contains("patches"))patches(e.at("patches"));
      return e.value("return",0u);
    };
    s.ambient_update=[&](auto v){(void)service("ambient",Json::array({v}));};
    s.resource_size=[&](auto v){return service("size",Json::array({v}));};s.allocate=[&](auto v){return service("allocate",Json::array({v}));};
    s.read_async=[&](const auto&v){return service("async",Json(v));};s.read_sync=[&](const auto&v){(void)service("sync",Json(v));};
    s.reprioritize=[&](auto a,auto b){(void)service("priority",Json::array({a,b}));};s.wait=[&](auto a,auto b,auto c){(void)service("wait",Json::array({a,b,c}));};
    s.prepare=[&](auto a,auto b){return service("prepare",Json::array({a,b}));};s.finalize=[&](auto a){(void)service("finalize",Json::array({a}));};
    s.free=[&](auto a){(void)service("free",Json::array({a}));};s.touch=[&](auto a){(void)service("touch",Json::array({a}));};
    Json fault=nullptr;std::uint32_t returned{};unsigned mxcsr{};std::uint16_t cw{};
    {FloatContext context(q.at("mxcsr").get<unsigned>(),q.at("fpcw").get<std::uint16_t>());
      try {
        const auto frame=q.at("frame").get<std::uint32_t>();
        if(q.at("owner")=="update")vista_widget_bitmap_update(obj,s);
        else if(q.at("owner")=="frame")returned=vista_widget_bitmap_frame(obj,s);
        else if(q.at("owner")=="resource")returned=vista_widget_bitmap_resource(frame,q.at("flags").get<std::uint32_t>(),q.at("out").get<std::uint32_t>(),s);
        else if(q.at("owner")=="request")returned=vista_widget_bitmap_resource_request(frame,q.at("z").get<std::uint32_t>(),s);
        else throw FormatError("Unknown bitmap owner");
      }catch(const FormatError&e){const std::string message=e.what();fault=message.find("does not progress")!=std::string::npos?"nonprogress":message.find("DIV faults")!=std::string::npos?"integer-divide":message.find("provider-fault/")!=std::string::npos||message.find("missing-provider/")!=std::string::npos?message:"memory-fault";}
      mxcsr=_mm_getcsr();asm volatile("fnstcw %0":"=m"(cw));
    }
    const auto&r=q.at("result");const auto name=q.at("name").get<std::string>();
    if(fault!=r.at("fault"))throw FormatError("Bitmap completion differs: "+name+" actual="+fault.dump()+" expected="+r.at("fault").dump());
    if(fault.is_null()&&q.at("owner")!="update"&&returned!=r.at("returned"))throw FormatError("Borrowed bitmap result differs: "+name);
    if(mxcsr!=r.at("mxcsr")||cw!=r.at("fpcw"))throw FormatError("Bitmap FP context differs: "+name+" actual="+std::to_string(mxcsr)+" expected="+r.at("mxcsr").dump());
    if(trace!=r.at("trace")){std::cerr<<"expected "<<r.at("trace")<<"\nactual "<<trace<<'\n';throw FormatError("Bitmap read/write/provider order differs: "+name);}
    for(const auto&row:r.at("final")){const auto raw=unhex(row.at(1).get<std::string>());if(hex(writable(row.at(0).get<std::uint32_t>(),raw.size()))!=row.at(1).get<std::string>())throw FormatError("Bitmap aliased bank differs: "+name);++checks;}
    for(const auto&t:trace){reads+=t.at(0)=="r";writes+=t.at(0)=="w";calls+=t.at(0)=="call";}
    if(cache.bytes(base,cache.storage_size()).data()!=identity ||
       !std::equal(original_bytes.begin(),original_bytes.end(),assets->region().begin()))
      throw FormatError("Bitmap update lost retained cache identity or changed source assets");
    faults+=!fault.is_null();++cases;checks+=6;
  }
  std::size_t boundaries{};
  const auto reject=[&](const VistaWidgetBitmapUpdateServices&s){
    bool rejected{};try{vista_widget_bitmap_update(obj,s);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("Unproved bitmap service/FP boundary admitted");
    ++boundaries;
  };
  reject({});
  VistaWidgetBitmapUpdateServices absent;
  absent.read=[](std::uint32_t,std::size_t)->std::uint32_t{throw std::runtime_error("Unexpected bitmap registry read");};
  reject(absent);
  absent.write=[](std::uint32_t,std::uint32_t,std::size_t){throw std::runtime_error("Unexpected bitmap registry write");};
  {FloatContext context(0x1f00,0x37f);reject(absent);}
  asm volatile("fnclex");
  {FloatContext context(0x1f80,0x37e);reject(absent);}
  unsigned first_clock_reads{};absent.read=[&](std::uint32_t at,std::size_t n)->std::uint32_t{
    if(at!=0xd71900u||n!=4)throw std::runtime_error("Absent ambient service reached later source read");
    ++first_clock_reads;return 321;
  };
  reject(absent);
  if(first_clock_reads!=1)throw FormatError("Cached clock did not precede mandatory ambient provider");
  // A resident handle with the source recent-timestamp predicate needs no
  // allocator/file/GPU provider. Admission remains lazy for those services.
  VistaWidgetBitmapUpdateServices resident;resident.write=absent.write;
  resident.read=[](std::uint32_t at,std::size_t n)->std::uint32_t{
    if(n!=4)throw std::runtime_error("Unexpected resident resource width");
    if(at==0x150u)return 42;
    if(at==0x164u||at==0x168u||at==0xe3e430u||at==0xe3e434u)return 0;
    throw std::runtime_error("Unexpected resident resource read");
  };
  if(vista_widget_bitmap_resource(0x100u,3,0,resident)!=42)throw FormatError("Resident resource unexpectedly needs external provider");
  ++boundaries;
  std::cout<<"PASS: "<<cases<<" hardware-original cases through retained mutable cache, "<<actual<<" actual authored cases, "<<checks<<" bank/trace/full-MXCSR/CW checks, "<<reads<<" reads, "<<writes<<" writes, "<<calls<<" declared provider calls, "<<faults<<" matched fault/nonprogress prefixes, "<<boundaries<<" explicit service/FP boundaries\n";return 0;
#endif
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
