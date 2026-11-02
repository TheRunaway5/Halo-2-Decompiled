// Test-only bridge: real SDL services at an explicitly native ABI boundary.
// No Xbox device object, source cache admission or hardware result is made.
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include <cstring>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <map>
#include <memory>
#include <optional>
#include <openssl/evp.h>
using namespace halo2;
namespace {
using J=nlohmann::json;using Abi=SourcePhysicsContactRuntimeAbi;
struct Fault{std::uint32_t address,bytes;};
struct Memory final:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::array<std::byte,4096>> pages;
 void map(std::uint32_t a,std::uint32_t bytes){for(auto n=a>>12;n<=(a+bytes-1u)>>12;++n)pages.try_emplace(n);}
 void read(std::uint32_t a,std::span<std::byte> out)const override{const auto first=a,n=std::uint32_t(out.size());
  for(auto& b:out){const auto p=pages.find(a>>12);if(p==pages.end())throw Fault{first,n};b=p->second[a++&4095u];}}
 void write(std::uint32_t a,std::span<const std::byte> in)override{const auto first=a,n=std::uint32_t(in.size());
  for(auto b:in){const auto p=pages.find(a>>12);if(p==pages.end())throw Fault{first,n};p->second[a++&4095u]=b;}}
 std::uint32_t get(std::uint32_t a,unsigned n=4)const{std::array<std::byte,4>b{};read(a,std::span(b).first(n));std::uint32_t v{};
  for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(8*k);return v;}
 void put(std::uint32_t a,unsigned n,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8*k));write(a,std::span(b).first(n));}
};
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string digest(std::span<const std::byte> bytes){std::array<unsigned char,32> data{};unsigned n{};
 need(EVP_Digest(bytes.data(),bytes.size(),data.data(),&n,EVP_sha256(),nullptr)==1&&n==32,"Native bridge digest failed");
 constexpr char digits[]="0123456789abcdef";std::string out;for(auto c:data){out+=digits[c>>4];out+=digits[c&15];}return out;}
// Explicit native ABI at the two source call boundaries. These services
// open a real manual SDL device and create/change actual SDL voices. They do
// not execute or qualify the retail SDK, driver startup or Xbox DSP.
constexpr std::uint32_t create_entry=0x3875c2u,pause_entry=0x3858f8u,device_binding=0x07600000u,voice_base=0x07100000u;
struct Provider{
 std::unique_ptr<SdlSoundPlatform> platform;std::map<std::uint32_t,SdlSoundPlatform::Voice> voices;
 J trace=J::array();unsigned created{},pause_calls{},paused_successfully{};bool remove_before_create{};
 SdlSoundPlatform& output(){if(!platform)platform=std::make_unique<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});return *platform;}
 static SdlSoundVoiceParameters parameters(bool paused){return {std::bit_cast<std::uint32_t>(1.f),0,paused,{{0,0,1.f},{1,1,1.f}}};}
 std::uint32_t create(std::uint64_t context){const auto v=output().create_voice(context,parameters(true));need(v<0x100000u,"Native voice token extent exceeded");const auto token=voice_base+std::uint32_t(v)*16u;need(voices.emplace(token,v).second,"Native voice token repeated");return token;}
 explicit Provider(const J& row):remove_before_create(row.value("removed_before_create",false)){
  if(row.value("previous_voice",false))need(create(0x888u)==voice_base+16u,"Previous native voice token changed");
 }
 ~Provider(){if(!platform)return;try{for(const auto& [_,v]:voices){output().flush(v);while(const auto c=output().completion(v))output().acknowledge(v,c->submission);output().destroy_voice(v);}}catch(...) {}}
 J service(const J& request){const auto target=request.at("target").get<std::uint32_t>();const auto& args=request.at("args");J reply{{"eax",0u}};
  if(target==create_entry){
   const auto& d=request.at("descriptor");const auto f=request.at("format").get<std::vector<unsigned>>();
   need(args.size()==4&&args[0]==device_binding&&args[3]==0,"Native device/outer binding differs");
   need(d.size()==6&&d[1]==2&&d[3]==0x227140u&&d[5]==0&&(d[0].get<unsigned>()&0x20000000u),"Native stream descriptor unavailable");
   need(f.size()==20,"Native stream format extent differs");const auto word=[&](unsigned at,unsigned bytes){std::uint32_t v{};for(unsigned n=0;n<bytes;++n)v|=f[at+n]<<(8*n);return v;};
   const auto channels=word(2,2),tag=word(0,2);
   need(word(4,4)==44100&&((tag==0x69u&&(channels==1||channels==2)&&word(12,2)==36*channels&&word(14,2)==4&&word(16,2)==2&&word(18,2)==64)||(tag==1&&channels==2&&word(12,2)==4&&word(14,2)==16&&word(16,2)==0)),"Native provider refused the declared original format");
   if(remove_before_create){need(output().device_removed(output().device_id()),"Actual removal before creation missing");remove_before_create=false;}
   try{reply["output_value"]=create(d[4]);++created;}catch(const std::runtime_error&){reply["eax"]=0x80004005u;}
  }else if(target==pause_entry){
   need(args.size()==2&&args[1]==0,"Native constructor changed its pause request");++pause_calls;
   const auto p=voices.find(args[0].get<std::uint32_t>());
   if(p==voices.end())reply["eax"]=0x80070057u;
   else try{output().parameters(p->second,parameters(false));++paused_successfully;}catch(const std::runtime_error&){reply["eax"]=0x80004005u;}
  }else throw std::runtime_error("Unknown constructor native service");
  trace.push_back({{"request",request},{"result",reply}});return reply;
 }
 J receipt(){J status=J::array();for(const auto& [token,v]:voices){const auto s=output().status(v);status.push_back({token,s.accepts_input,s.paused,s.device_removed,s.pending_packets,s.completed_packets});}
  return {{"native_service_trace",trace},{"actual_created_voices",created},{"actual_pause_calls",pause_calls},{"actual_successful_pauses",paused_successfully},{"actual_voice_status",status}};
 }
};
J frame(const Memory& m,const Abi& a,std::uint32_t target){const auto sp=a.gp[4];
 const unsigned count=target==create_entry?4u:target==pause_entry?2u:target==0x224e20u?0u:1u;J args=J::array();for(unsigned n=0;n<count;++n)args.push_back(m.get(sp+4u+n*4u));
 J result{{"target",target},{"gp",a.gp},{"return",m.get(sp)},{"args",args}};
 if(target==create_entry){J descriptor=J::array();for(unsigned n=0;n<6;++n)descriptor.push_back(m.get(args[1].get<unsigned>()+n*4u));
  result["descriptor"]=descriptor;J format=J::array();for(unsigned n=0;n<20;++n)format.push_back(m.get(descriptor[2].get<unsigned>()+n,1));result["format"]=format;}
 return result;
}
std::unique_ptr<Provider> external;std::string result;
const native_tu15_sound::Kernel& kernel(std::uint32_t target){
 for(const auto table:{native_tu15_sound::startup_kernels(),native_tu15_sound::resource_kernels()}){
  const auto k=std::ranges::find(table,target,&native_tu15_sound::Kernel::entry);if(k!=table.end())return *k;}
 throw std::runtime_error("Required native constructor CPU owner unavailable");
}
}
extern "C" const char* h2_construct_provider(const char* text){try{const auto request=J::parse(text);const auto operation=request.at("op");
 if(operation=="reset"){external=std::make_unique<Provider>(request.at("row"));result=external->receipt().dump();}
 else if(operation=="service"){need(bool(external),"Native creation owner absent");result=external->service(request.at("request")).dump();}
 else if(operation=="receipt"){need(bool(external),"Native creation owner absent");result=external->receipt().dump();}
 else if(operation=="close"){external.reset();result="{}";}else throw std::runtime_error("Unknown native constructor operation");
 }catch(const std::exception& e){result=J{{"error",e.what()}}.dump();}return result.c_str();}
extern "C" const char* h2_construct_native(const char* text){try{
 const auto row=J::parse(text);Memory memory;for(const auto& r:row.at("regions"))memory.map(r[0],r[1]);for(const auto& f:row.at("fields"))memory.put(f[0],f[1],f[2]);
 Provider provider(row);Abi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
 abi.eflags=0x202;abi.fs_base=0x12345000;abi.x87_depth=row.value("x87_depth",4u);abi.x87={1.L,-1.L,2.L,3.L,4.L,5.L,6.L,7.L};
 abi.x87_status=row.at("fp")[2];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("fp")[0],row.at("fp")[1]};const auto initial=abi;
 J stores=J::array(),calls=J::array();std::optional<Fault> fault;
 const auto observe=[&](const SourcePhysicsContactRecordStore& s){if(s.address>=0x02000000u&&s.address<0x02010000u)return;
  const auto val=[](auto b){std::uint32_t n{};for(unsigned k=0;k<b.size();++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;};stores.push_back({s.instruction,s.address,s.written.size(),val(s.before),val(s.written)});};
 SourcePhysicsContactRuntimeServices services;services.required_owner=[&](auto&,const auto& c,auto& a,const auto& o){
  const auto call=frame(memory,a,c.target);calls.push_back(call);
  if(c.target==create_entry||c.target==pause_entry){const auto reply=provider.service(call);if(reply.contains("output_value"))memory.put(call.at("args")[2],4,reply.at("output_value"));
   a.gp[0]=reply.at("eax");a.gp[4]+=c.target==create_entry?20u:12u;
  }else kernel(c.target).run(memory,a,*a.floating_point,o,services);
 };
 try{kernel(row.at("entry")).run(memory,abi,*abi.floating_point,observe,services);}catch(const Fault& f){fault=f;}
 if(!row.value("format_abi",false))need(abi.xmm==initial.xmm&&abi.x87==initial.x87&&abi.x87_depth==initial.x87_depth&&abi.x87_status==initial.x87_status&&abi.fs_base==initial.fs_base&&!abi.active_mmx&&abi.floating_point->x87_control_word==initial.floating_point->x87_control_word&&abi.floating_point->mxcsr==initial.floating_point->mxcsr,"Integer constructor changed caller FP/SIMD/FS state");
 J pages=J::array();for(const auto& [page,bytes]:memory.pages)pages.push_back({page<<12,digest(bytes)});
 J out=provider.receipt();out.update({{"gp",abi.gp},{"xmm",abi.xmm},{"fp",{abi.floating_point->x87_control_word,abi.floating_point->mxcsr,abi.x87_status}},{"pages",pages},{"calls",calls},{"stores",stores},{"fault",fault?J::array({fault->address,fault->bytes}):J(nullptr)}});
 if(row.contains("x87_depth")){
  J live=J::array();unsigned tag=0xffffu;constexpr char digits[]="0123456789abcdef";
  for(unsigned n=0;n<abi.x87_depth;++n){const auto bytes=std::as_bytes(std::span(&abi.x87[n],1)).first(10);std::string hex;
   for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);hex+=digits[v>>4];hex+=digits[v&15];}live.push_back(hex);
   const auto kind=std::fpclassify(abi.x87[n]);const auto bits=kind==FP_ZERO?1u:kind==FP_NORMAL?0u:2u,physical=((abi.x87_status>>11u)+n)&7u;
   tag=(tag&~(3u<<(physical*2u)))|(bits<<(physical*2u));
  }
  out["x87_live"]=live;out["x87_depth"]=abi.x87_depth;out["x87_tag"]=tag;
 }
 result=out.dump();
 }catch(const std::exception& e){result=J{{"error",e.what()}}.dump();}return result.c_str();}

// Independent test oracle for the x87 primitives in the immutable1.0/2.0
// format domain. No native sound kernel/Floating helper participates. Unicorn
// preserves incoming C1 here, unlike these actual native x86 instructions.
extern "C" const char* h2_format_x87_primitives(const char* text){try{
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 const auto input=J::parse(text);const std::uint16_t cw=input.at("cw"),sw=input.at("sw"),tags=0xffffu;
 const std::uint32_t value=input.at("value");need((cw&0x3fu)==0x3fu&&!(sw&0x8080u)&&(value==0x3f800000u||value==0x40000000u),"Primitive oracle requires its declared masked1.0/2.0 domain");
 alignas(16)std::array<std::byte,108> host{};std::array<std::byte,28> environment{};
 asm volatile("fnsave %0":"=m"(host)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
 std::memcpy(environment.data(),host.data(),28);std::memcpy(environment.data(),&cw,2);std::memcpy(environment.data()+4,&sw,2);std::memcpy(environment.data()+8,&tags,2);
 asm volatile("fldenv %0"::"m"(environment):"memory");
 std::array<std::uint16_t,7> status{};std::uint32_t stored{},delta{};std::int64_t integer{};
 asm volatile("flds %[value]\n\tfnstsw %[s0]\n\tfld %%st(0)\n\tfnstsw %[s1]\n\tfsts %[stored]\n\tfnstsw %[s2]\n\tfistpq %[integer]\n\tfnstsw %[s3]\n\tfildq %[integer]\n\tfnstsw %[s4]\n\t.byte 0xde,0xe9\n\tfnstsw %[s5]\n\tfstps %[delta]\n\tfnstsw %[s6]"
  :[s0]"=m"(status[0]),[s1]"=m"(status[1]),[s2]"=m"(status[2]),[s3]"=m"(status[3]),[s4]"=m"(status[4]),[s5]"=m"(status[5]),[s6]"=m"(status[6]),[stored]"=m"(stored),[integer]"=m"(integer),[delta]"=m"(delta)
  :[value]"m"(value):"memory","st","st(1)");
 asm volatile("frstor %0"::"m"(host):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
 result=J{{"status",status},{"stored",stored},{"integer",integer},{"delta",delta},{"xbox_hardware",false}}.dump();
#else
 throw std::runtime_error("Independent original x87 primitive backend unavailable");
#endif
 }catch(const std::exception& e){result=J{{"error",e.what()}}.dump();}return result.c_str();}
