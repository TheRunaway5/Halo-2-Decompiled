// Composed native stock kernels over authenticated original chunks and real
// manual SDL consumption. Source cache/driver rows are declared ABI inputs;
// this does not admit a World, source voice, Xbox SDK or DSP/device result.
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/audio.hpp"
#include "halo2/original_sound_runtime_bank_tu15.hpp"
#include "halo2/sdl_sound_platform.hpp"
#ifdef HALO2_SOUND_COLD_CACHE
#include "halo2/source_sound_cache_io_tu15.hpp"
#endif
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using J=nlohmann::json;using Abi=SourcePhysicsContactRuntimeAbi;
void need(bool v,const char* why){if(!v)throw FormatError(why);}
std::string sha(std::span<const std::byte> bytes){std::array<unsigned char,32> result{};unsigned n{};
 need(EVP_Digest(bytes.data(),bytes.size(),result.data(),&n,EVP_sha256(),nullptr)==1&&n==32,"Payload digest failed");
 constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:result){out+=digits[b>>4];out+=digits[b&15];}return out;}
void same(std::span<const float>a,std::span<const float>b){need(a.size()==b.size(),"Native pipeline changed output extent");
 for(std::size_t n=0;n<a.size();++n)need(std::bit_cast<std::uint32_t>(a[n])==std::bit_cast<std::uint32_t>(b[n]),"Native kernel pipeline changed uninterrupted PCM bits");}
constexpr std::uint32_t table=0x04000000u,stream=table+12u,cache=0x04200000u,entries=0x04300000u,
 allocator=0x04400000u,block_owner=0x04500000u,blocks=0x04600000u,sdk=0x04800000u,pool=0x09000000u,
 stack=0x02008000u,codec=0x0048b908u,codec_table=0x004633b4u,
 sdk_status=0x07000000u,sdk_process=0x07000010u,sdk_boundary=0x07000020u;
#ifdef HALO2_SOUND_COLD_CACHE
constexpr std::uint32_t cache_bits=0x04900000u,page_bits=0x04a00000u;
#endif
const native_tu15_sound::Kernel& kernel(std::uint32_t entry){
 for(const auto table:{native_tu15_sound::resource_kernels(),native_tu15_sound::startup_kernels()}){
  const auto k=std::ranges::find(table,entry,&native_tu15_sound::Kernel::entry);if(k!=table.end())return *k;}
 throw FormatError("Composed stream has no native CPU owner");
}
struct Memory final:SourcePhysicsContactMemory {
 std::shared_ptr<OriginalSoundRuntimeBankTu15> bank;
 std::map<std::uint32_t,std::vector<std::byte>> regions;
 explicit Memory(std::shared_ptr<OriginalSoundRuntimeBankTu15> value):bank(std::move(value)){}
 void map(std::uint32_t at,std::size_t bytes){need(regions.emplace(at,std::vector<std::byte>(bytes)).second,"Duplicate native fixture backing");}
 std::span<const std::byte> view(std::uint32_t at,std::size_t bytes)const{
  if(at>=bank->address()&&std::uint64_t(at)+bytes<=std::uint64_t(bank->address())+bank->bytes().size())return bank->bytes().subspan(at-bank->address(),bytes);
  auto i=regions.upper_bound(at);need(i!=regions.begin(),"Native fixture read lacks backing");--i;
  need(at>=i->first&&std::uint64_t(at)+bytes<=std::uint64_t(i->first)+i->second.size(),"Native fixture read escapes backing");return std::span(i->second).subspan(at-i->first,bytes);
 }
 void read(std::uint32_t at,std::span<std::byte> out)const override{const auto data=view(at,out.size());std::copy(data.begin(),data.end(),out.begin());}
 void write(std::uint32_t at,std::span<const std::byte> in)override{
  if(at>=bank->address()&&std::uint64_t(at)+in.size()<=std::uint64_t(bank->address())+bank->bytes().size()){
   const auto target=bank->memory(at,in.size());std::copy(in.begin(),in.end(),target.begin());return;}
  auto i=regions.upper_bound(at);need(i!=regions.begin(),"Native fixture write lacks backing");--i;
  need(at>=i->first&&std::uint64_t(at)+in.size()<=std::uint64_t(i->first)+i->second.size(),"Native fixture write escapes backing");
  std::copy(in.begin(),in.end(),i->second.begin()+std::ptrdiff_t(at-i->first));
 }
 std::uint32_t get(std::uint32_t at,unsigned n=4)const{std::uint32_t v{};const auto data=view(at,n);for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(data[k])<<(8*k);return v;}
 void put(std::uint32_t at,std::uint32_t v,unsigned n=4){std::array<std::byte,4> data{};for(unsigned k=0;k<n;++k)data[k]=std::byte(v>>(8*k));write(at,std::span(data).first(n));}
};
const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
SdlSoundVoiceParameters unity(){return {std::bit_cast<std::uint32_t>(1.f),0,false,{{0,0,1.f},{1,1,1.f}}};}
struct Plan {
 struct Chunk{std::uint32_t descriptor,data,bytes;std::size_t first,frames;};
 std::shared_ptr<const std::vector<float>> pcm;std::vector<Chunk> chunks;unsigned channels{};
 Plan(Memory& memory,TagHandle sound,unsigned pitch,unsigned permutation,const EncodedSound& expected){
  const auto encoded=memory.bank->encoded(sound,pitch,permutation);need(encoded.data==expected.data&&encoded.codec==expected.codec&&encoded.channels==expected.channels&&encoded.sample_rate==expected.sample_rate,"Native graph differs from admitted original bytes");
  need(encoded.codec==1&&encoded.sample_rate==44100&&(encoded.channels==1||encoded.channels==2),"Require authenticated original stock ADPCM domain");
  channels=encoded.channels;
  const auto decoded=decode_sound(encoded);need(decoded.rate==48000&&decoded.frames()&&decoded.chunk_boundaries.size()==encoded.packet_chunks.size(),"Native decoder omitted resource boundaries");
  const auto root=memory.bank->load(sound,pitch);const Bytes p(root.permutations.at(permutation));const auto chunk_table=memory.bank->block(8);
  need(p.u16(14)==encoded.packet_chunks.size(),"Native graph chunk count differs");std::size_t offset{},first{};
#ifdef HALO2_SOUND_COLD_CACHE
  std::size_t physical{};for(const auto& chunk:encoded.packet_chunks)physical+=(chunk.bytes+511u)&~511u;
  memory.map(pool,physical);
#else
  memory.map(pool,encoded.data.size());
#endif
  for(unsigned n=0;n<encoded.packet_chunks.size();++n){
   const auto descriptor=chunk_table.address+(p.u16(12)+n)*12u;const auto count=encoded.packet_chunks[n].bytes;
   std::vector<std::byte> resource(count);memory.bank->read_resource(descriptor,0,resource);
   need(std::ranges::equal(resource,std::span(encoded.data).subspan(offset,count)),"Actual descriptor read differs from original encoded interval");
#ifndef HALO2_SOUND_COLD_CACHE
   memory.write(pool+std::uint32_t(offset),resource);
#endif
   const auto end=decoded.chunk_boundaries[n].end_frame;
   need(decoded.chunk_boundaries[n].encoded_end==offset+count&&end>first,"Zero-output resource requires its separate completion owner");
   chunks.push_back({descriptor,pool+std::uint32_t(offset),count,first,end-first});offset+=count;first=end;
  }
  need(offset==encoded.data.size()&&first==decoded.frames(),"Native map omitted encoded bytes or decoder flush");
  pcm=std::make_shared<const std::vector<float>>(decoded.stereo);
 }
};
struct Pipeline {
 Memory memory;Plan plan;SdlSoundPlatform& output;SdlSoundPlatform::Voice voice{};Abi abi{};
 SourcePhysicsContactRuntimeServices services;std::map<std::uint32_t,unsigned> calls;
 unsigned callback_submissions{},accepted{},callbacks{},successes{},flushed{},failed{},acknowledged{},boundaries{},submitted_bytes{},retained_witnesses{};
 unsigned constructions{},constructor_pauses{};
#ifdef HALO2_SOUND_COLD_CACHE
 struct ReadReceipt {std::uint32_t descriptor,destination,bytes;std::string sha256;};
 std::vector<ReadReceipt> read_receipts;SourceSoundCacheIoTu15Services io;
#endif
 std::size_t next{};bool remove_on_submit{},in_callback{};
 Pipeline(std::shared_ptr<OriginalSoundRuntimeBankTu15> bank,TagHandle sound,unsigned pitch,unsigned permutation,const EncodedSound& expected,SdlSoundPlatform& port)
  :memory(std::move(bank)),plan(memory,sound,pitch,permutation,expected),output(port){
  for(const auto at:{0x00463000u,0x0046a000u,0x0048b000u,0x0050e000u,0x0052d000u,cache,entries,allocator,block_owner,blocks,sdk})memory.map(at,4096);
  memory.map(table,0x4000);memory.map(0x02000000u,65536);
  for(const auto [at,value]:std::array<std::pair<std::uint32_t,std::uint32_t>,11>{{{0x50ebdc,cache},{0x50ebe0,pool},{0x50ebe4,allocator},{0x52d674,table},{cache+0x44,entries},{allocator+0x64,block_owner},{allocator+0x38,0x1357},{block_owner+0x44,blocks},{table+4,1},{table+0x2ab0,output.device_id()},{codec,codec_table}}})memory.put(at,value);
  const std::array<std::uint32_t,9> stock{0x2e4d60,0x2e4d60,0x2b9840,0x2b98d0,0x2b9950,0x2b9980,0x2b99c0,0x31d400,0x2b9a00};
  for(unsigned n=0;n<stock.size();++n)memory.put(codec_table+n*4u,stock[n]);
  memory.put(0x46a0d0u,0x3f800000u);memory.put(0x46a11cu,0x40000000u);
  memory.put(sdk,sdk+0x100u);memory.put(sdk+0x10cu,sdk_status);memory.put(sdk+0x110u,sdk_process);memory.put(sdk+0x114u,sdk_boundary);
#ifdef HALO2_SOUND_COLD_CACHE
  memory.map(cache_bits,4096);memory.map(page_bits,4096);
  const auto capacity=std::uint32_t(plan.chunks.size()+1u);
  for(const auto [owner,rows,bits,stride]:std::array<std::array<std::uint32_t,4>,2>{{{cache,entries,cache_bits,16},{block_owner,blocks,page_bits,24}}}){
   for(const auto [offset,value]:std::array<std::pair<unsigned,std::uint32_t>,8>{{{0x20,capacity},{0x24,stride},{0x34,0},{0x38,0},{0x3c,0},{0x40,0x8000},{0x44,rows},{0x48,bits}}})memory.put(owner+offset,value);
  }
  for(const auto [offset,value]:std::array<std::pair<unsigned,std::uint32_t>,9>{{{0x20,0x21f3f0},{0x24,0x21f3b0},{0x2c,0},{0x30,std::uint32_t(memory.regions.at(pool).size()/512u)},{0x34,9},{0x38,1},{0x3c,0xffffffff},{0x40,0xffffffff},{0x64,block_owner}}})memory.put(allocator+offset,value);
  for(unsigned n=0;n<8;++n)memory.put(allocator+0x44u+n*4u,0x7fffffffu);
  for(const auto& chunk:plan.chunks)memory.put(chunk.descriptor+8u,0xffffffffu);
  io.read=[&](const auto& request){return memory.bank->read_cache_resource(request);};
  io.retain_completed_read=[&](auto descriptor,auto destination,auto bytes){
   need(memory.get(descriptor+8u)!=0xffffffffu,"Resource receipt lacks its actually allocated cache handle");
   const auto handle=memory.get(descriptor+8u);const auto entry=entries+(handle&65535u)*16u;
   need(!memory.get(entry+2u,1),"Cache became ready before its actual read receipt");
   const auto original=memory.bank->read_cache_resource({descriptor,memory.get(descriptor),memory.get(descriptor+4u),bytes});
   need(std::ranges::equal(memory.view(destination,bytes),original),"Resource receipt preceded its actual complete physical bytes");
   read_receipts.push_back({descriptor,destination,bytes,sha(memory.view(destination,bytes))});
   return std::uint32_t(read_receipts.size());
  };
#else
  for(unsigned n=0;n<plan.chunks.size();++n){
   // Declared already-loaded cache ABI state over actual resource backing.
   // No source I/O completion, cache admission or voice is claimed here.
   memory.put(plan.chunks[n].descriptor+8u,0xa1230000u|n);memory.put(entries+n*16u+2u,1,1);
   memory.put(blocks+n*24u+8u,plan.chunks[n].data-pool);
  }
#endif
  abi.gp={0x1234,0x5678,0x9abc,0x12345678,stack,0x87654321,0x13572468,0x24681357};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  services.required_owner=[&](auto&,const auto& call,auto& a,const auto& observer){
   ++calls[call.target];
#ifdef HALO2_SOUND_COLD_CACHE
   if(source_try_sound_cache_io_tu15(memory,call,a,io))return;
#endif
   if(call.target==sdk_status||call.target==sdk_process||call.target==sdk_boundary||call.target==0x3875c2u||call.target==0x3858f8u){service(call.target,a);return;}
   kernel(call.target).run(memory,a,*a.floating_point,observer,services);
  };
#ifdef HALO2_SOUND_COLD_CACHE
  need(memory.get(cache+0x3cu)==0&&memory.get(block_owner+0x3cu)==0&&read_receipts.empty(),"Cold fixture did not start from empty pools");
  for(auto& chunk:plan.chunks){
   need(memory.get(chunk.descriptor+8u)==0xffffffffu,"Cold fixture supplied a loaded cache datum");
   abi.gp[3]=chunk.descriptor;run(0x21f420u,std::array{sound.value});
   const auto handle=memory.get(chunk.descriptor+8u),index=handle&65535u;
   need(handle!=0xffffffffu&&memory.get(entries+index*16u,2)==(handle>>16u)&&memory.get(entries+index*16u+2u,1)==1,
    "Native cache allocation/read did not publish its actual salted ready entry");
   chunk.data=pool+(memory.get(blocks+index*24u+8u)<<9u);
   need(read_receipts.size()==std::size_t(&chunk-plan.chunks.data()+1)&&read_receipts.back().destination==chunk.data&&
    read_receipts.back().bytes==((chunk.bytes+511u)&~511u),"Actual resource receipt differs from native page allocation");
   const auto resource=memory.bank->read_cache_resource({chunk.descriptor,memory.get(chunk.descriptor),memory.get(chunk.descriptor+4u),(chunk.bytes+511u)&~511u});
   need(std::ranges::equal(memory.view(chunk.data,resource.size()),resource),"Native allocation/read changed exact authenticated sector bytes");
  }
  need(memory.get(cache+0x3cu)==plan.chunks.size()&&memory.get(block_owner+0x3cu)==plan.chunks.size(),"Native cold read lost its actual allocated datums");
  auto cached=expected;cached.data.clear();
  for(const auto& chunk:plan.chunks){const auto bytes=memory.view(chunk.data,chunk.bytes);cached.data.insert(cached.data.end(),bytes.begin(),bytes.end());}
  const auto decoded_cache=decode_sound(cached);
  need(decoded_cache.chunk_boundaries.size()==plan.chunks.size(),"Actual cold-cache decoder lost its resource boundaries");
  for(unsigned n=0;n<plan.chunks.size();++n)need(decoded_cache.chunk_boundaries[n].end_frame==plan.chunks[n].first+plan.chunks[n].frames,
   "Actual cold-cache decoding changed its packet PCM intervals");
  plan.pcm=std::make_shared<const std::vector<float>>(decoded_cache.stereo);
#endif
  // The real native caller now owns descriptor/format construction and its
  // reused index/output slot. Driver allocation/admission remains a declared
  // component input, as do already-loaded cache rows and producer timing.
  abi.gp[3]=expected.channels-1u;run(0x225ef0u,std::array{0u});
  need(abi.gp[0]==1&&constructions==1&&constructor_pauses==1&&memory.get(stream+0x24u)==sdk&&memory.get(stream+0x30u)==codec,
   "Native stream constructor did not publish its actual created/paused voice");
  need(!output.status(voice).paused&&!queued()&&!submitted(),"Native stream construction left paused or queued state");
 }
 ~Pipeline(){try{output.flush(voice);while(const auto c=output.completion(voice))output.acknowledge(voice,c->submission);output.destroy_voice(voice);}catch(...) {}}
 unsigned queued()const{return memory.get(stream+2u,1)&7u;}
 unsigned submitted()const{return(memory.get(stream+2u,1)>>3u)&7u;}
 unsigned cache_row(unsigned n)const{return entries+(memory.get(plan.chunks.at(n).descriptor+8u)&65535u)*16u;}
 unsigned pin(unsigned n)const{return memory.get(cache_row(n)+4u,1);}
 unsigned references(unsigned n)const{return memory.get(cache_row(n)+5u,1);}
 void run(std::uint32_t entry,std::span<const std::uint32_t> args={}){
  abi.gp[4]=stack;memory.put(stack,0x03000000u);for(unsigned n=0;n<args.size();++n)memory.put(stack+4u+n*4u,args[n]);
  ++calls[entry];kernel(entry).run(memory,abi,*abi.floating_point,{},services);need(abi.gp[4]==stack+4u+args.size()*4u,"Composed native root changed caller stack balance");
 }
 void enqueue(unsigned n){abi.gp[6]=stream;abi.gp[7]=plan.chunks.at(n).descriptor;run(0x2b8210);}
 void pump(){abi.gp[0]=stream;run(0x2b8530);}
 void feed(){while(next<plan.chunks.size()&&queued()<2){const auto before=queued();enqueue(unsigned(next));need(queued()==before+1,"Producer lost its source queue acceptance");++next;}pump();}
 void service(std::uint32_t target,Abi& a){
  const auto sp=a.gp[4];std::uint32_t result{};
  if(target==0x3875c2u){
   const auto d=memory.get(sp+8u),format=memory.get(d+8u);
   need(!voice&&memory.get(sp+4u)==output.device_id()&&memory.get(sp+12u)==stream+0x24u&&!memory.get(sp+16u),"Constructor lost its actual native device/output binding");
   need(memory.get(d)==0x20000000u&&memory.get(d+4u)==2&&format==stack-0x2cu&&memory.get(d+12u)==0x227140u&&!memory.get(d+16u)&&!memory.get(d+20u),"Constructor changed its actual source descriptor");
   need(memory.get(format,2)==0x69u&&memory.get(format+2u,2)==plan.channels&&memory.get(format+4u)==44100u&&memory.get(format+8u)==36u*plan.channels*689u&&memory.get(format+12u,2)==36u*plan.channels&&memory.get(format+14u,2)==4u&&memory.get(format+16u,2)==2u&&memory.get(format+18u,2)==64u,"Original stream format differs");
   auto parameters=unity();parameters.paused=true;voice=output.create_voice(memory.get(d+16u),parameters);
   memory.put(memory.get(sp+12u),sdk);++constructions;a.gp[0]=0;a.gp[4]+=20u;return;
  }
  if(target==0x3858f8u){
   need(voice&&memory.get(sp+4u)==sdk&&!memory.get(sp+8u),"Constructor pause lost its actual created voice");
   output.parameters(voice,unity());++constructor_pauses;a.gp[0]=0;a.gp[4]+=12u;return;
  }
  if(in_callback){need(output.completion(voice).has_value(),"Completed PCM was released before its source callback returned");++retained_witnesses;}
  if(target==sdk_status){const auto status=output.status(voice);memory.put(memory.get(sp+8u),status.accepts_input?1u:0u);if(status.device_removed)result=0x80004005u;}
  else if(target==sdk_process){
   const auto packet=memory.get(sp+8u);const auto context=memory.get(packet+16u);const auto c=std::ranges::find(plan.chunks,context,&Plan::Chunk::descriptor);need(c!=plan.chunks.end(),"Native packet lost its original descriptor context");
   need(memory.get(packet)==c->data&&memory.get(packet+4u)==c->bytes&&!memory.get(packet+8u)&&!memory.get(packet+12u)&&!memory.get(packet+20u),"Native CPU packet does not describe its exact original resource");
   std::vector<std::byte> original(c->bytes);memory.bank->read_resource(c->descriptor,0,original);
   need(std::ranges::equal(memory.view(c->data,c->bytes),original),"Submitted compressed backing changed its authenticated resource bytes");
   if(remove_on_submit){need(output.device_removed(output.device_id()),"Actual SDL removal-on-submit missing");remove_on_submit=false;}
   try{if(output.submit(voice,{plan.pcm,c->first,c->frames,context,{}})==SdlSoundPlatform::Submission::Accepted){++accepted;callback_submissions+=in_callback;submitted_bytes+=c->bytes;}else result=0x80004005u;}
   catch(const std::runtime_error&){result=0x80004005u;}
  }else if(target==sdk_boundary){++boundaries;if(output.status(voice).device_removed)result=0x80004005u;}
  else throw FormatError("Unknown native SDL service");
  const auto status=output.status(voice);a.gp[0]=result;a.gp[1]=status.pending_packets;a.gp[2]=status.completed_packets;a.gp[4]+=target==sdk_process?16u:target==sdk_boundary?8u:12u;
 }
 void account(){while(const auto c=output.completion(voice)){
  const auto at=std::ranges::find(plan.chunks,std::uint32_t(c->packet_context),&Plan::Chunk::descriptor);need(at!=plan.chunks.end(),"Actual SDL completion lost its encoded source owner");
  const auto index=unsigned(at-plan.chunks.begin());const auto before_pin=pin(index),before_reference=references(index);need(before_pin&&before_reference&&queued()&&submitted(),"Actual completion lacks source cache ownership");
  const auto before_q=queued(),before_s=submitted(),before_accepted=accepted;
  in_callback=true;const std::array args{std::uint32_t(c->channel_context),std::uint32_t(c->packet_context),std::uint32_t(c->status)};
  run(0x227140,args);in_callback=false;
  need(output.completion(voice)&&output.completion(voice)->submission==c->submission,"Source completion acknowledged SDL before returning");
  need(queued()==before_q-1&&submitted()==before_s-1+accepted-before_accepted&&pin(index)==before_pin-1&&references(index)==before_reference-1,"Source completion lost exact queue/pin/reference accounting");
  output.acknowledge(voice,c->submission);++acknowledged;++callbacks;
  if(c->status==SdlSoundCompletionStatus::Success)++successes;else if(c->status==SdlSoundCompletionStatus::Flushed)++flushed;else ++failed;
 }}
 void release(){abi.gp[1]=stream;run(0x2b85f0);}
 void clear()const{need(!queued()&&!submitted()&&callbacks==acknowledged,"Source queue or native completion remains unaccounted");
  for(unsigned n=0;n<plan.chunks.size();++n)need(!pin(n)&&!references(n),"Native pipeline leaked or invented a cache pin/data reference");}
};
}
namespace {
J exercise(const std::shared_ptr<OriginalSoundRuntimeBankTu15>& bank,TagHandle sound,unsigned pitch,unsigned permutation,
 const EncodedSound& expected,SdlSoundPlatform& output,SdlSoundPlatform& reference){
 const auto prepared=decode_sound(expected);const auto data=std::make_shared<const std::vector<float>>(prepared.stereo);
 unsigned success_packets{},callback_refill_packets{},flush_packets{},failure_packets{},flush_remainders{},failure_remainders{},boundary_witness{},rejected_pins{},retained{},constructions{},pauses{};std::map<std::uint32_t,unsigned> roots;
#ifdef HALO2_SOUND_COLD_CACHE
 unsigned cold_reads{};std::uint64_t cold_bytes{};
#endif
 const auto record=[&](const Pipeline& p){for(const auto& [pc,n]:p.calls)roots[pc]+=n;retained+=p.retained_witnesses;constructions+=p.constructions;pauses+=p.constructor_pauses;
#ifdef HALO2_SOUND_COLD_CACHE
  cold_reads+=unsigned(p.read_receipts.size());for(const auto& r:p.read_receipts)cold_bytes+=r.bytes;
#endif
 };
 const auto play=[&](bool deferred_second){
  Pipeline p(bank,sound,pitch,permutation,expected,output);same(*p.plan.pcm,*data);
  const auto compare_voice=reference.create_voice(0x888,unity());
  need(reference.submit(compare_voice,{data,0,data->size()/2,0x777,{}})==SdlSoundPlatform::Submission::Accepted,"Uninterrupted reference was not accepted");
  try{
   if(deferred_second){p.enqueue(0);p.next=1;p.pump();p.enqueue(1);p.next=2;}
   else p.feed();
   need(p.accepted==(deferred_second?1u:std::min<std::size_t>(2,p.plan.chunks.size())),"Native source pump did not fill its real initial slots");
   if(p.queued()==2){const auto n=unsigned(p.next<p.plan.chunks.size()?p.next:0);const auto before=p.pin(n);const auto count=p.accepted;
    p.enqueue(n);need(p.queued()==2&&p.pin(n)==before&&p.accepted==count,"Full source queue consumed another chunk/pin");}
   std::size_t rendered{};std::vector<float> actual;actual.reserve(data->size()+1024);
   const auto render=[&](std::size_t n){const auto left=output.render(n),right=reference.render(n);same(left,right);actual.insert(actual.end(),left.begin(),left.end());rendered+=n;};
   if(p.plan.chunks.size()>1){need(p.plan.chunks[1].frames>17,"Authored stream lacks its first-boundary-before-refill witness");
    render(p.plan.chunks.front().frames+(deferred_second?0u:17u));need(output.completion(p.voice).has_value()&&p.callbacks==0&&p.pin(0)==1&&p.references(0)==1,"Mixer completion retired source cache ownership before its callback");
    if(!deferred_second){const auto accepted=p.accepted;p.pump();need(p.accepted==accepted,"Completed unaccounted slot was reused before callback ownership");++boundary_witness;}
    p.account();if(deferred_second)need(p.callback_submissions==1,"Source callback did not refill its real free slot while retaining completed PCM");p.feed();
   }
   unsigned blocks{};
   while(rendered<prepared.frames()){
    const auto consumed=output.playback_position(p.voice).source_frames;need(consumed==double(rendered),"Native pipeline underrun changed actual consumed frames");
    const auto boundary=std::ranges::find_if(p.plan.chunks,[&](const auto& c){return double(c.first+c.frames)>consumed;});need(boundary!=p.plan.chunks.end(),"Native pipeline lost its next actual PCM interval");
    const auto remaining=boundary->first+boundary->frames-std::size_t(consumed);
    const auto n=std::min(remaining,std::array<std::size_t,5>{137,511,1600,311,733}[blocks++%5]);need(n!=0,"Native stream made no consumption progress");
    render(n);p.account();p.feed();need(blocks<100000,"Native stream did not finish");
   }
   p.account();need(p.next==p.plan.chunks.size()&&p.accepted==p.plan.chunks.size()&&p.successes==p.accepted&&!p.flushed&&!p.failed,"Native source did not retire each authenticated chunk exactly once");p.clear();
   need((p.memory.get(stream+1u,1)&7u)==1u&&p.submitted_bytes==expected.data.size(),"Terminal native stream mode or encoded extent differs");
   need(output.playback_position(p.voice).source_frames==double(prepared.frames()),"Native source frame extent differs from original decoding");
   same(output.render(31),reference.render(31));if(deferred_second)callback_refill_packets=p.accepted;else success_packets=p.accepted;record(p);
   while(const auto c=reference.completion(compare_voice))reference.acknowledge(compare_voice,c->submission);
   reference.destroy_voice(compare_voice);
  }catch(...){reference.flush(compare_voice);while(const auto c=reference.completion(compare_voice))reference.acknowledge(compare_voice,c->submission);reference.destroy_voice(compare_voice);throw;}
 };
 play(false);if(expected.packet_chunks.size()>1)play(true);
 {
  Pipeline p(bank,sound,pitch,permutation,expected,output);p.enqueue(0);p.next=1;p.pump();if(p.plan.chunks.size()>1){p.enqueue(1);p.next=2;}
  (void)output.render(std::min<std::size_t>(17,p.plan.chunks.front().frames/2));output.flush(p.voice);p.account();flush_remainders=p.queued();p.release();p.clear();
  need(p.flushed==p.accepted&&p.callbacks==p.accepted&&!p.successes&&!p.failed,"Real flush invented a successful source completion");
  need(std::ranges::all_of(output.render(31),[](float v){return v==0.f;}),"Flushed source stream continued PCM");flush_packets=p.flushed;record(p);
 }
 {
  SdlSoundPlatform removed(manual);Pipeline p(bank,sound,pitch,permutation,expected,removed);p.enqueue(0);p.next=1;p.pump();if(p.plan.chunks.size()>1){p.enqueue(1);p.next=2;}(void)removed.render(std::min<std::size_t>(17,p.plan.chunks.front().frames/2));
  need(removed.device_removed(removed.device_id()),"Real device-removal control was not observed");p.account();failure_remainders=p.queued();p.release();p.clear();
  need(p.failed==p.accepted&&p.callbacks==p.accepted&&!p.successes&&!p.flushed,"Removed native device invented successful completions");failure_packets=p.failed;record(p);
 }
 {
  SdlSoundPlatform removed(manual);Pipeline p(bank,sound,pitch,permutation,expected,removed);p.remove_on_submit=true;p.feed();
  need(p.accepted==0&&p.callbacks==0&&p.boundaries==0&&p.submitted()==0&&p.queued()==std::min<std::size_t>(2,p.plan.chunks.size()),"Rejected actual submission changed source submitted/queued state");
  for(unsigned n=0;n<p.next;++n)need(p.pin(n)==1&&!p.references(n),"Rejected packet released its pin or leaked its retained data reference");
  rejected_pins=unsigned(p.next);p.release();p.clear();record(p);
 }
 J calls=J::object();for(const auto& [pc,n]:roots)calls[std::to_string(pc)]=n;
 J result{{"success_packets",success_packets},{"callback_refill_packets",callback_refill_packets},{"flush_packets",flush_packets},{"failed_packets",failure_packets},{"flush_unsubmitted_pins",flush_remainders},{"failure_unsubmitted_pins",failure_remainders},{"rejected_queued_pins",rejected_pins},
  {"first_boundary_witness",boundary_witness},{"retained_callback_service_witnesses",retained},{"native_stream_constructions",constructions},{"actual_constructor_pauses",pauses},{"pcm_frames",prepared.frames()},
  {"pcm_sha256",sha(std::as_bytes(std::span(prepared.stereo)))},{"root_and_callee_calls",calls}};
#ifdef HALO2_SOUND_COLD_CACHE
 result["cold_cache_reads"]=cold_reads;result["authenticated_sector_bytes"]=cold_bytes;
#endif
 return result;
}
}
int main(int argc,char**argv){J report{{"passed",false},{"goal_complete",false},{"cases",J::array()},
 {"scope","Authenticated prepared/native original resources through native common stream construction, stock enqueue, packet submission, actual SDL consumption, source completion and pin release; declared producer schedule, unity gain/rate and already-loaded cache ABI inputs"},
 {"caller_format_preparation","Actual native224E20/225EF0 caller with private32CC0C conversion; declared driver/cache ABI inputs, unity gain and manual timing"},
 {"full_format_caller_cpu_chain",true},
 {"source_world_cache_voice_or_device_admission",false},{"retail_sdk_dsp_or_xbox_waveform_equivalence",false},{"source_event_schedule_or_normal_gameplay_delivery",false},
 {"whole_encoded_chunks_only",true},{"seeked_or_zero_frame_packet_mapping_verified",false},{"native_boundary_policy","Input-boundary annotation over actual PCM queue; no Xbox discontinuity command equivalence"}};
#ifdef HALO2_SOUND_COLD_CACHE
 report["scope"]="Authenticated original resources through complete native21F420/13FE50 allocation/binding, actual native1A3B70 resource reads and receipts, native stock stream lifecycle and manual SDL consumption; declared empty pool/physical backing, driver, producer timing and unity gain/rate";
 report["cache_loaded_rows_supplied"]=false;report["native_cache_allocator_and_actual_resource_read"]=true;report["pcm_decoded_from_actual_cold_cache_bytes"]=true;
 report["caller_format_preparation"]="Actual native224E20/225EF0 caller; cold cache readiness follows actual native resource receipts; driver ABI, unity gain and manual timing remain declared";
#endif
 try{
  need(argc==4||argc==5,"Supply authenticated prepared catalog, external catalog SHA256, report and optional unique case index");if(!*argv[1])return 77;
  need(std::getenv("HALO2_TEST_SCRATCH"),"Run through the owned test lifecycle");std::ifstream file(argv[1],std::ios::binary);need(bool(file),"Prepared catalog unavailable");
  const std::string raw((std::istreambuf_iterator<char>(file)),{});need(sha(std::as_bytes(std::span(raw)))==argv[2],"Catalog differs from its external producer admission pin");
  const auto catalog=J::parse(raw);const auto selected=argc==5?std::optional<unsigned>(unsigned(std::stoul(argv[4]))):std::nullopt;
  report["complete_corpus"]=!selected;report["selected_case"]=selected?J(*selected):J(nullptr);std::set<std::string> unique;
  unsigned contexts{},routes{},multi{},chunks{},exercised{};std::set<std::string> names;SdlSoundPlatform output(manual),reference(manual);
  for(const auto& row:catalog.at("cases")){
   const auto prepared=PreparedOriginalSoundBank::load(row.at("directory").get<std::string>(),row.at("sha256").get<std::string>());const auto& m=prepared->metadata();need(m.origin.has_value(),"Prepared source bank lacks its original provenance");++contexts;names.insert(m.name);
   auto strings=std::make_shared<std::map<std::uint32_t,std::string>>();for(const auto& range:m.pitch_ranges){strings->emplace(*range.import_name_id,range.name);for(const auto& p:range.permutations)strings->emplace(*p.import_name_id,p.name);}
   const OriginalSoundRuntimeStringsTu15 labels{strings,[strings](auto id,auto text){need(strings->at(id)==text,"Declared original label differs");return id;},[strings](auto id){return strings->at(id);}};
   Tag tag;tag.group="snd!";tag.name=m.name;tag.datum=m.origin->sound.value;tag.address=0x20000000;tag.size=20;
   const auto tags=std::make_shared<const std::vector<Tag>>(std::vector{tag});const std::array bindings{OriginalSoundRuntimeBindingTu15{m.origin->sound,prepared}};
   const auto bank=OriginalSoundRuntimeBankTu15::create(0x31000000u,8*1024*1024,tags,bindings,labels);
   for(unsigned pitch=0;pitch<m.pitch_ranges.size();++pitch)for(unsigned p=0;p<m.pitch_ranges[pitch].permutations.size();++p){
    const auto& encoded=prepared->encoded(pitch,p);const auto actual=bank->encoded(m.origin->sound,pitch,p);need(actual.data==encoded.data,"Native original route changed admitted encoded bytes");++routes;
    const auto key=m.name+"|"+std::to_string(pitch)+"|"+std::to_string(p)+"|"+sha(encoded.data);if(!unique.insert(key).second)continue;
    const auto ordinal=unsigned(unique.size()-1);if(selected&&ordinal!=*selected)continue;
    report["case_in_progress"]={{"index",ordinal},{"name",m.name},{"pitch",pitch},{"permutation",p}};
    auto result=exercise(bank,m.origin->sound,pitch,p,encoded,output,reference);result.update({{"index",ordinal},{"name",m.name},{"pitch",pitch},{"permutation",p},{"encoded_sha256",sha(encoded.data)},{"chunks",encoded.packet_chunks.size()}});
    report["cases"].push_back(std::move(result));++exercised;chunks+=unsigned(encoded.packet_chunks.size());multi+=encoded.packet_chunks.size()>1;
   }
  }
  need(contexts==908&&routes==9450&&names.size()==31&&unique.size()==273,"Native stream payload corpus is incomplete");
  need(selected?exercised==1:(exercised==273&&multi==55&&chunks==375),"Native stream lifecycle omitted an authenticated case");
  report.erase("case_in_progress");report.update({{"passed",true},{"contexts",contexts},{"routes",routes},{"names",names.size()},{"unique_permutations",unique.size()},
   {"exercised_permutations",exercised},{"multi_chunk_permutations",multi},{"source_chunks",chunks},{"lifecycle_scenarios",4*exercised+multi},{"callback_acknowledged_only_after_source_return",true}});
 }catch(const std::exception& e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=4){const auto data=report.dump(2);need(data.size()<=1024*1024,"Native stream payload report exceeds its bounded budget");std::ofstream(argv[3])<<data<<'\n';}
 std::cout<<report.at("passed")<<" native stock payload cases="<<report.at("cases").size()<<'\n';return report.at("passed").get<bool>()?0:1;
}
