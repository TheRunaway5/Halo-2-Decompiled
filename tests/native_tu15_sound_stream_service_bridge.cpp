// Test-only bridge: real SDL services at an explicitly native ABI boundary.
// No Xbox device object, source cache admission or hardware result is made.
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/sdl_sound_platform.hpp"
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
constexpr std::uint32_t status_entry=0x07000000u,submit_entry=0x07000010u,boundary_entry=0x07000020u;
struct Provider{
 std::unique_ptr<SdlSoundPlatform> platform;SdlSoundPlatform::Voice voice{};
 SdlSoundPlatform& output(){if(!platform){
  platform=std::make_unique<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}});
  voice=platform->create_voice(0,{std::bit_cast<std::uint32_t>(1.f),0,false,{{0,0,1.f},{1,1,1.f}}});
 }return *platform;}
 struct Chunk{std::uint32_t descriptor,data,bytes;std::shared_ptr<const std::vector<float>> pcm;};
 std::vector<Chunk> chunks;J trace=J::array(),event=nullptr;bool remove_on_submit{};unsigned boundaries{},submitted{},acknowledged_events{};std::optional<std::uint64_t> pending_ack;
 explicit Provider(const J& row){
  for(const auto& c:row.at("chunks")){std::vector<float> pcm;for(unsigned n=0;n<c.at("frames").get<unsigned>();++n){
    pcm.push_back(float(int((n*17u+c.at("descriptor").get<unsigned>())%41u)-20)/64.f);pcm.push_back(float(int(n%13u)-6)/32.f);}
   chunks.push_back({c.at("descriptor"),c.at("data"),c.at("bytes"),std::make_shared<const std::vector<float>>(std::move(pcm))});}
  const auto initial=row.value("prefill",0u);need(initial<=chunks.size(),"Invalid native prefill");
  for(unsigned n=0;n<initial;++n)need(output().submit(voice,{chunks[n].pcm,0,chunks[n].pcm->size()/2,chunks[n].descriptor,{}})==SdlSoundPlatform::Submission::Accepted,"Native prefill was not accepted");
  remove_on_submit=row.value("remove_on_submit",false);
  if(row.value("removed_before_status",false))need(output().device_removed(output().device_id()),"Native removal was not observed");
  const auto kind=row.value("event",std::string{});
  if(!kind.empty()){
   if(kind=="success")(void)output().render(row.at("render_frames"));
   else if(kind=="flush")output().flush(voice);
   else if(kind=="failure")need(output().device_removed(output().device_id()),"Native completion failure was not observed");
   else throw std::runtime_error("Unknown native event");
   const auto c=output().completion(voice);need(bool(c),"Actual native completion missing");
   event={{"channel",c->channel_context},{"context",c->packet_context},{"status",std::uint32_t(c->status)},{"frames",c->pcm_frames}};
   pending_ack=c->submission;
  }
 }
 ~Provider(){if(!platform)return;try{output().flush(voice);while(const auto c=output().completion(voice))output().acknowledge(voice,c->submission);output().destroy_voice(voice);}catch(...) {}}
 J service(std::uint32_t target,const J& packet=J::array()){
  J result{{"eax",0u}};const auto before=output().status(voice);
  if(target==status_entry){result["status_word"]=before.accepts_input?1u:0u;if(before.device_removed)result["eax"]=0x80004005u;}
  else if(target==submit_entry){
   if(remove_on_submit){need(output().device_removed(output().device_id()),"Native submit removal missing");remove_on_submit=false;}
   const auto context=packet.at(4).get<std::uint32_t>();
   const auto p=std::ranges::find(chunks,context,&Chunk::descriptor);
   if(p==chunks.end() || packet.at(0)!=p->data || packet.at(1)!=p->bytes ||
       packet.at(2)!=0 || packet.at(3)!=0 || packet.at(5)!=0){
    result["eax"]=0x80070057u;result["native_provider_mapping_refusal"]=true;
   }else try{const auto accepted=output().submit(voice,{p->pcm,0,p->pcm->size()/2,context,{}});
     if(accepted==SdlSoundPlatform::Submission::Accepted)++submitted;else result["eax"]=0x80004005u;}
    catch(const std::runtime_error&){result["eax"]=0x80004005u;}
  }else if(target==boundary_entry){
   // The native continuous-PCM queue needs no hardware discontinuity command.
   // Retain this input-boundary annotation over its actual queue state. This
   // is an explicit native adapter policy, not an Xbox SDK/DSP equivalence.
   ++boundaries;if(before.device_removed)result["eax"]=0x80004005u;
  }else throw std::runtime_error("Unknown native service");
  const auto after=output().status(voice);result["ecx"]=after.pending_packets;result["edx"]=after.completed_packets;
  result["pending"]=after.pending_packets;result["completed"]=after.completed_packets;
  trace.push_back({{"target",target},{"packet",packet},{"result",result}});return result;
 }
 void account_event(){if(pending_ack){
  const auto completion=output().completion(voice);
  need(completion&&completion->submission==*pending_ack,"Native callback lost its retained packet before source accounting");
  output().acknowledge(voice,*pending_ack);pending_ack.reset();++acknowledged_events;
 }}
 J receipt()const{return {{"actual_event_acknowledged_after_source_callback",acknowledged_events},{"event",event},{"sdk_trace",trace},{"actual_accepted",submitted},{"native_boundary_annotations",boundaries}};}
};
std::unique_ptr<Provider> external;std::string result;
J get_packet(const Memory& m,std::uint32_t at){J out=J::array();for(unsigned k=0;k<6;++k)out.push_back(m.get(at+k*4u));return out;}
J frame(const Memory& m,const Abi& a,std::uint32_t target){
 unsigned count=target==submit_entry?3:target==boundary_entry?1:target==0x2b99c0u?3:target==0x2b8530u?0:2;
 J args=J::array();for(unsigned n=0;n<count;++n)args.push_back(m.get(a.gp[4]+4u+n*4u));
 J out{{"target",target},{"gp",a.gp},{"return",m.get(a.gp[4])},{"args",args}};
 if(target==submit_entry)out["packet"]=get_packet(m,args[1]);return out;
}
}
extern "C" const char* h2_stream_provider(const char* text){try{
 const auto request=J::parse(text);const auto operation=request.at("op");
 if(operation=="reset"){external=std::make_unique<Provider>(request.at("row"));result=external->receipt().dump();}
 else if(operation=="service"){need(bool(external),"Native service owner absent");result=external->service(request.at("target"),request.value("packet",J::array())).dump();}
 else if(operation=="account_event"){need(bool(external),"Native service owner absent");external->account_event();result=external->receipt().dump();}
 else if(operation=="receipt"){need(bool(external),"Native service owner absent");result=external->receipt().dump();}
 else if(operation=="close"){external.reset();result="{}";}else throw std::runtime_error("Unknown native provider operation");
 }catch(const std::exception& e){result=J{{"error",e.what()}}.dump();}return result.c_str();}
extern "C" const char* h2_stream_native(const char* text){try{
 const auto row=J::parse(text);Memory memory;
 for(const auto& r:row.at("regions"))memory.map(r[0],r[1]);
 for(const auto& f:row.at("fields"))memory.put(f[0],f[1],f[2]);
 Provider provider(row);Abi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
 abi.eflags=0x202;abi.fs_base=0x12345000;abi.x87_status=row.at("fp")[2];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("fp")[0],row.at("fp")[1]};
 if(!provider.event.is_null())for(unsigned n=0;n<3;++n)memory.put(abi.gp[4]+4u+n*4u,4,provider.event[std::array{"channel","context","status"}[n]]);
 const auto stack=abi.gp[4];J stores=J::array(),calls=J::array();std::optional<Fault> fault;
 const auto observe=[&](const SourcePhysicsContactRecordStore& s){
  if(s.address>=0x02000000u&&s.address<stack)return;
  const auto val=[](auto b){std::uint32_t n{};for(unsigned k=0;k<b.size();++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;};
  stores.push_back({s.instruction,s.address,s.written.size(),val(s.before),val(s.written)});
 };
 SourcePhysicsContactRuntimeServices services;
 services.required_owner=[&](auto&,const auto& c,auto& a,const auto& o){
  calls.push_back(frame(memory,a,c.target));
  if(c.target==status_entry||c.target==submit_entry||c.target==boundary_entry){
   const auto args=c.target==submit_entry?get_packet(memory,memory.get(a.gp[4]+8u)):J::array();
   const auto reply=provider.service(c.target,args);
   if(c.target==status_entry)memory.put(memory.get(a.gp[4]+8u),4,reply.at("status_word"));
   a.gp[0]=reply.at("eax");a.gp[1]=reply.at("ecx");a.gp[2]=reply.at("edx");
   a.gp[4]+=c.target==submit_entry?16u:c.target==boundary_entry?8u:12u;
  }else{
   const auto table=native_tu15_sound::resource_kernels();const auto k=std::ranges::find(table,c.target,&native_tu15_sound::Kernel::entry);
   need(k!=table.end(),"Unowned native CPU callee");k->run(memory,a,*a.floating_point,o,services);
  }
 };
 const auto table=native_tu15_sound::resource_kernels();const auto kernel=std::ranges::find(table,row.at("entry").get<std::uint32_t>(),&native_tu15_sound::Kernel::entry);
 need(kernel!=table.end(),"Native stream entry absent");
 try{kernel->run(memory,abi,*abi.floating_point,observe,services);provider.account_event();}catch(const Fault& f){fault=f;}
 J pages=J::array();for(const auto& [page,bytes]:memory.pages)if((page<<12)<0x02000000u || (page<<12)>=0x02010000u)
  pages.push_back({page<<12,digest(bytes)});
 J out=provider.receipt();out.update({{"gp",abi.gp},{"xmm",abi.xmm},{"fp",{abi.floating_point->x87_control_word,abi.floating_point->mxcsr,abi.x87_status}},
  {"pages",pages},
  {"calls",calls},{"stores",stores},{"fault",fault?J::array({fault->address,fault->bytes}):J(nullptr)}});result=out.dump();
 }catch(const std::exception& e){result=J{{"error",e.what()}}.dump();}return result.c_str();}
