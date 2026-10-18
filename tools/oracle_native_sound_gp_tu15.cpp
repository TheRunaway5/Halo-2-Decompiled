// Analysis only: native GP execution owner with the unchanged pinned xemu
// peripheral model and selectable unchanged reference/native scratch DMA.
// No GPL reference source links into the game.
// Inputs are exported from the owned TU1.5 XBE, never captured DSP RAM.
#include "halo2/sound_gp_cpu_tu15.hpp"
#include "halo2/sound_gp_dma_tu15.hpp"
#include <algorithm>
#include <chrono>
#include <time.h>
#include <openssl/evp.h>
extern "C" {
#include "qemu/osdep.h"
#include "dsp.h"
#include "dsp_state.h"
}
#include <array>
#include <iostream>
#include <memory>
#include <vector>

namespace {
using Space=halo2::SoundGpSpaceTu15;
std::unique_ptr<halo2::SoundGpCpuTu15> cpu;
std::unique_ptr<halo2::SoundGpDmaTu15> native_dma;
std::array<std::uint8_t,0x100000> scratch;
std::size_t scratch_size;
unsigned reads,writes,steps,blocks;
FILE *state_trace;
FILE *peripheral_trace;
std::uint32_t trace_pc{};
bool tracing_instruction{};
std::uint64_t block_cycles{};bool use_blocks{},collect_timing{};
std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> frame_states{nullptr,EVP_MD_CTX_free};
struct Timing {std::uint64_t wall,cpu;};std::vector<Timing> timings;
std::uint64_t thread_ns(){timespec time{};if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&time))throw std::runtime_error("Cannot read GP thread clock");return std::uint64_t(time.tv_sec)*1000000000+time.tv_nsec;}

DSPState *peripheral;
halo2::SoundGpDmaStateTu15 dma_state(){
 if(native_dma)return native_dma->state();
 const auto& d=peripheral->dma;return {d.configuration,d.control,d.start_block,d.next_block,d.eol,d.error};
}
std::uint32_t peripheral_read(std::uint32_t address){
 std::uint32_t value;
 if(native_dma&&address==0xffffd6)value=native_dma->read(address);
 else {
  value=peripheral->core.read_peripheral(&peripheral->core,address);
  // Frame/timer/idle remain explicitly reference-owned. The native DMA owns
  // only EOL; this bit comes from completed actual native list execution.
  if(native_dma&&address==0xffffc5)value=(value&~0x80u)|(native_dma->state().eol?0x80u:0u);
 }
 if(peripheral_trace)fprintf(peripheral_trace,"{\"event\":\"read\",\"pc\":%u,\"address\":%u,\"value\":%u}\n",trace_pc,address,value);
 return value;
}
void peripheral_write(std::uint32_t address,std::uint32_t value){
 if(peripheral_trace){
  const auto d=dma_state();
  fprintf(peripheral_trace,"{\"event\":\"write\",\"pc\":%u,\"address\":%u,\"value\":%u,\"dma_next\":%u,\"dma_start\":%u,\"dma_control\":%u",trace_pc,address,value,d.next,d.start,d.control);
  if(address==0xffffd6){const auto at=d.next&0x3fff;fprintf(peripheral_trace,",\"node_address\":%u,\"node\":[",at);for(unsigned i=0;i<8;++i){if(i)fputc(',',peripheral_trace);fprintf(peripheral_trace,"%u",cpu->read(Space::x,at+i));}fputc(']',peripheral_trace);}
  fputs("}\n",peripheral_trace);
 }
 if(native_dma&&address>=0xffffd4&&address<=0xffffd7)native_dma->write(address,value);
 else {
  if(native_dma&&address==0xffffc5&&(value&0x80))native_dma->acknowledge_eol();
  peripheral->core.write_peripheral(&peripheral->core,address,value);
 }
 if(peripheral_trace&&address==0xffffd6){const auto d=dma_state();fprintf(peripheral_trace,"{\"event\":\"dma_after\",\"pc\":%u,\"next\":%u,\"start\":%u,\"control\":%u,\"eol\":%s,\"error\":%s}\n",trace_pc,d.next,d.start,d.control,d.eol?"true":"false",d.error?"true":"false");}
 if(peripheral->core.is_idle)cpu->request_stop();
}
std::size_t load(const char *dir,const char *name,void *dst,std::size_t maximum) {
 char path[2048];assert(snprintf(path,sizeof(path),"%s/%s",dir,name)<int(sizeof(path)));
 FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0&&std::size_t(n)<=maximum);
 rewind(f);assert(fread(dst,1,n,f)==std::size_t(n));fclose(f);return n;
}
void scratch_rw(void*,std::uint8_t *ptr,std::uint32_t at,std::size_t length,bool write) {
 assert(at<=scratch_size&&length<=scratch_size-at);
 if(write){memcpy(scratch.data()+at,ptr,length);++writes;}else{memcpy(ptr,scratch.data()+at,length);++reads;}
 if(peripheral_trace){
  std::array<unsigned char,32> digest{};unsigned bytes{};
  assert(EVP_Digest(ptr,length,digest.data(),&bytes,EVP_sha256(),nullptr)==1&&bytes==digest.size());
  fprintf(peripheral_trace,"{\"event\":\"scratch\",\"instruction_active\":%s,\"pc\":%u,\"offset\":%u,\"bytes\":%zu,\"write\":%s,\"sha256\":\"",tracing_instruction?"true":"false",trace_pc,at,length,write?"true":"false");
  for(auto byte:digest)fprintf(peripheral_trace,"%02x",byte);
  fputs("\",\"first64_hex\":\"",peripheral_trace);for(std::size_t i=0;i<std::min<std::size_t>(length,64);++i)fprintf(peripheral_trace,"%02x",ptr[i]);fputs("\"}\n",peripheral_trace);
 }
}
void fifo_rw(void*,std::uint8_t*,unsigned,std::size_t,bool) {throw std::runtime_error("Unexpected GP FIFO in declared control-loop interval");}
Space space(int value) {
 switch(value){case DSP_SPACE_X:return Space::x;case DSP_SPACE_Y:return Space::y;case DSP_SPACE_P:return Space::program;}
 throw std::runtime_error("Unknown reference DMA memory space");
}
void execute_frame() {
 peripheral->core.is_idle=false;dsp_start_frame(peripheral);
 if(use_blocks){std::uint64_t total{};
  while(!peripheral->core.is_idle){const auto result=cpu->run_original_gp(1000000);total+=result.cycles;assert(total<=1000000);if(!result.stopped)throw std::runtime_error("GP block budget exhausted before actual idle");}
  assert(cpu->pc()==0x2b);++blocks;block_cycles+=total;return;
 }
 for(unsigned i=0;i<200000;++i) {
  const auto pc=cpu->pc();
  if(peripheral->core.is_idle){assert(pc==0x2b);++blocks;return;}
  assert(pc<=0xf50);trace_pc=pc;tracing_instruction=true;cpu->step();tracing_instruction=false;++steps;
 }
 throw std::runtime_error("Native GP execution did not reach actual idle");
}
void trace_state(){
 if(state_trace){
  const auto state=cpu->execution_state();
  const auto put=[](std::uint32_t word){std::array<std::uint8_t,4> bytes{};for(unsigned j=0;j<4;++j)bytes[j]=std::uint8_t(word>>(8*j));assert(fwrite(bytes.data(),1,4,state_trace)==4);};
  put(state.pc);for(auto word:state.registers)put(word);
  for(const auto &half:state.stack)for(auto word:half)put(word);
  put(state.repeating);put(state.pc_on_repeat);
 }
}
void frame() {
 if(!collect_timing)execute_frame();
 else {
  const auto wall=std::chrono::steady_clock::now();const auto cpu_start=thread_ns();execute_frame();
  const auto cpu_end=thread_ns();const auto elapsed=std::chrono::steady_clock::now()-wall;
  timings.push_back({std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()),cpu_end-cpu_start});
 }
 trace_state();
 std::array<std::uint32_t,66> state{};state[0]=cpu->pc();state[1]=cpu->cycles();
 const auto registers=cpu->registers();std::copy(registers.begin(),registers.end(),state.begin()+2);
 std::array<std::uint8_t,264> bytes{};
 for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::uint8_t(state[i/4]>>(8*(i%4)));
 assert(EVP_DigestUpdate(frame_states.get(),bytes.data(),bytes.size())==1);
}
void timing_json() {
 if(timings.empty())return;
 std::cout<<R"(,"cold_frame_cpu_ns":)"<<timings[0].cpu;
 if(timings.size()<=16)return;
 std::vector<std::uint64_t> cpu;std::uint64_t total{};
 for(std::size_t i=16;i<timings.size();++i){cpu.push_back(timings[i].cpu);total+=timings[i].cpu;}
 std::sort(cpu.begin(),cpu.end());
 const auto p99=cpu[(cpu.size()-1)*99/100];
 std::cout<<R"(,"warm_frames":)"<<cpu.size()<<R"(,"warm_total_cpu_ns":)"<<total
  <<R"(,"warm_mean_cpu_ns":)"<<total/cpu.size()<<R"(,"warm_p99_cpu_ns":)"<<p99
  <<R"(,"warm_max_cpu_ns":)"<<cpu.back()<<R"(,"warm_compute_within_audio_duration":)"
  <<(total*48000<=std::uint64_t(cpu.size())*32*1000000000?"true":"false");
}
void save(FILE *file,Space area,unsigned first,unsigned count) {
 std::vector<std::uint32_t> words(count);
 for(unsigned i=0;i<count;++i)words[i]=cpu->read(area,first+i);
 assert(fwrite(words.data(),4,words.size(),file)==words.size());
}
}
// Compile only dsp_dma.c with these two symbol substitutions. Its original
// descriptor/transfer logic stays unchanged and accesses the native GP RAM.
extern "C" std::uint32_t native_gp_read(dsp_core_t*,int area,std::uint32_t address) {return cpu->read(space(area),address);}
extern "C" void native_gp_write(dsp_core_t*,int area,std::uint32_t address,std::uint32_t value) {cpu->write(space(area),address,value);}

int main(int argc,char **argv)try {
 if(argc==3&&!strcmp(argv[1],"--export-reference-context")){
  peripheral=dsp_init(nullptr,scratch_rw,fifo_rw);assert(peripheral);peripheral->is_gp=peripheral->core.is_gp=true;
  FILE *out=fopen(argv[2],"w");assert(out);const auto d=dma_state();
  fputs("{\"scope\":\"Declared reference-model initial context and frame/timer inputs; not authenticated MCPX reset or hardware clock\",\"registers\":[",out);
  for(unsigned i=0;i<64;++i){if(i)fputc(',',out);fprintf(out,"%u",peripheral->core.registers[i]);}
  dsp_start_frame(peripheral);
  fprintf(out,"],\"dma\":{\"configuration\":%u,\"control\":%u,\"start\":%u,\"next\":%u,\"eol\":%s,\"error\":%s},\"frame_status\":%u,\"timer_read\":%u}\n",d.configuration,d.control,d.start,d.next,d.eol?"true":"false",d.error?"true":"false",peripheral->core.read_peripheral(&peripheral->core,0xffffc5),peripheral->core.read_peripheral(&peripheral->core,0xffffb3));
  assert(!fclose(out));dsp_destroy(peripheral);return 0;
 }
 assert(argc>=5&&argc<=14);bool blocks_requested=false,timing_requested=false,dma_requested=false,upload_only=false;
 auto format2=halo2::SoundGpDmaFormat2Tu15::unverified;
 for(int i=5;i<argc;++i){if(!strcmp(argv[i],"--native-dma"))dma_requested=true;else if(!strcmp(argv[i],"--upload-only"))upload_only=true;else if(!strcmp(argv[i],"--declared-format2-lsb"))format2=halo2::SoundGpDmaFormat2Tu15::declared_low24;else if(!strcmp(argv[i],"--declared-format2-msb"))format2=halo2::SoundGpDmaFormat2Tu15::declared_high24;else if(!strcmp(argv[i],"--blocks"))blocks_requested=true;else if(!strcmp(argv[i],"--timing"))timing_requested=true;else if(!strcmp(argv[i],"--state-trace")&&i+1<argc&&!state_trace){state_trace=fopen(argv[++i],"wb");assert(state_trace);}else if(!strcmp(argv[i],"--peripheral-trace")&&i+1<argc&&!peripheral_trace){peripheral_trace=fopen(argv[++i],"wb");assert(peripheral_trace);}else throw std::runtime_error("Unknown GP probe option");}
 if(!dma_requested&&format2!=halo2::SoundGpDmaFormat2Tu15::unverified)throw std::runtime_error("Declared native layout requires native DMA");
 if(upload_only&&(blocks_requested||format2!=halo2::SoundGpDmaFormat2Tu15::unverified))throw std::runtime_error("Upload-only keeps format2 unresolved and uses single-step execution");
 if(peripheral_trace&&blocks_requested)throw std::runtime_error("Peripheral instruction tracing requires actual single-step PCs");
 unsigned frames=strtoul(argv[3],nullptr,0),mask=strtoul(argv[4],nullptr,0);assert(frames&&frames<=4096);
 scratch_size=load(argv[1],"initial-scratch.bin",scratch.data(),scratch.size());
 assert(scratch[0x810]==3&&scratch[0x811]==0&&scratch[0x812]==0&&scratch[0x813]==0);
 peripheral=dsp_init(nullptr,scratch_rw,fifo_rw);assert(peripheral);peripheral->is_gp=peripheral->core.is_gp=true;
 halo2::SoundGpCpuInputTu15 input{};
 // Explicit offline reset context from the reference model, not asserted to
 // be an authenticated MCPX hardware reset or captured live register state.
 std::copy_n(peripheral->core.registers,input.registers.size(),input.registers.begin());
 input.read_peripheral=peripheral_read;input.write_peripheral=peripheral_write;
 cpu=std::make_unique<halo2::SoundGpCpuTu15>(std::move(input));
 if(dma_requested){
  halo2::SoundGpDmaResourcesTu15 resources;
  resources.read_word=[](auto area,auto at){return cpu->read(area,at);};
  resources.write_word=[](auto area,auto at,auto value){cpu->write(area,at,value);};
  resources.read_scratch=[](auto at,auto bytes){scratch_rw(nullptr,reinterpret_cast<std::uint8_t*>(bytes.data()),at,bytes.size(),false);};
  resources.write_scratch=[](auto at,auto bytes){scratch_rw(nullptr,const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(bytes.data())),at,bytes.size(),true);};
  resources.format2=format2;
  const auto initial=dma_state();native_dma=std::make_unique<halo2::SoundGpDmaTu15>(std::move(resources),initial);
 }
 frame_states.reset(EVP_MD_CTX_new());assert(frame_states&&EVP_DigestInit_ex(frame_states.get(),EVP_sha256(),nullptr)==1);
 std::array<std::uint8_t,0x2000> boot_bytes{};
 scratch_rw(nullptr,boot_bytes.data(),0,boot_bytes.size(),false);
 std::array<std::uint32_t,0x800> boot{};
 for(unsigned i=0;i<boot.size();++i)for(unsigned j=0;j<4;++j)boot[i]|=std::uint32_t(boot_bytes[i*4+j])<<(j*8);
 cpu->bootstrap(boot);
 if(upload_only){
  // Observe the actual original header DMA store. Frame events still come
  // from the declared reference harness after an actual idle, not hardware.
  // Stop the host audit at the store and retain the original PC.
  unsigned frame_events{};
  while(scratch[0x810]){
   if(steps>=200000)throw std::runtime_error("Original upload did not acknowledge within instruction bound");
   if(peripheral->core.is_idle){peripheral->core.is_idle=false;dsp_start_frame(peripheral);++frame_events;}
   trace_pc=cpu->pc();tracing_instruction=true;cpu->step();tracing_instruction=false;++steps;
  }
  trace_state();
  FILE *out=fopen(argv[2],"wb");assert(out);
  save(out,Space::program,0,0x1000);save(out,Space::x,0,0xc00);save(out,Space::y,0,0x800);
  assert(fwrite(scratch.data(),1,scratch_size,out)==scratch_size);assert(!fclose(out));
  std::cout<<R"({"scope":"actual original request3 clear; declared reference initial context/frame/timer, no hardware/output clock","dma":")"<<(native_dma?"native":"reference")<<R"(","format2":"unverified","declared_frame_events":)"<<frame_events<<R"(,"instructions":)"<<steps<<R"(,"pc":)"<<cpu->pc()<<R"(,"cycles":)"<<cpu->cycles()<<R"(,"scratch_reads":)"<<reads<<R"(,"scratch_writes":)"<<writes<<R"(,"native_pcm_verified":false})"<<'\n';
  if(peripheral_trace){assert(!fclose(peripheral_trace));peripheral_trace=nullptr;}
  if(state_trace){assert(!fclose(state_trace));state_trace=nullptr;}
  native_dma.reset();cpu.reset();dsp_destroy(peripheral);return 0;
 }
 frame();frame();
 assert(scratch[0x810]==0&&scratch[0x811]==0&&scratch[0x812]==0&&scratch[0x813]==0);
 std::array<std::uint32_t,0xde0> expected{};std::array<std::uint32_t,15> states{};
 assert(load(argv[1],"expected-effects.bin",expected.data(),sizeof(expected))==sizeof(expected));
 for(unsigned i=0;i<expected.size();++i)assert(cpu->read(Space::program,0x171+i)==expected[i]);
 assert(load(argv[1],"effect-states.bin",states.data(),sizeof(states))==sizeof(states));
 for(auto state:states)assert(state+4<0xc00&&(cpu->read(Space::x,state+4)&1));
 use_blocks=blocks_requested;collect_timing=timing_requested;
 FILE *file=fopen(argv[2],"wb");assert(file);
 for(unsigned block=0;block<frames;++block) {
  for(unsigned bus=0;bus<32;++bus)for(unsigned i=0;i<32;++i) {
   const auto sample=(mask&(1u<<bus))?((((block*32+i)*0x12843u+bus*0x45678u)&0x3ffff)-0x20000u)&0xffffff:0;
   cpu->write(Space::x,0xc00+bus*32+i,sample);
  }
  frame();save(file,Space::x,0xc00,0x400);
 }
 save(file,Space::program,0,0x1000);save(file,Space::x,0,0xc00);save(file,Space::y,0,0x800);
 assert(fwrite(scratch.data(),1,scratch_size,file)==scratch_size);fclose(file);
 std::cout<<R"({"blocks":)"<<blocks<<R"(,"dma":")"<<(native_dma?"native":"reference")<<R"(","execution":")"<<(use_blocks?"blocks":"step")<<'"';
 if(native_dma)std::cout<<R"(,"declared_format2":")"<<(format2==halo2::SoundGpDmaFormat2Tu15::declared_high24?"high24":format2==halo2::SoundGpDmaFormat2Tu15::declared_low24?"low24":"unverified")<<'"';
 if(use_blocks)std::cout<<R"(,"single_step_instructions":)"<<steps<<R"(,"compiled_cycles":)"<<block_cycles;
 else std::cout<<R"(,"instructions":)"<<steps;
 std::cout<<R"(,"cpu_cycles":)"<<cpu->cycles()<<R"(,"scratch_reads":)"<<reads<<R"(,"scratch_writes":)"<<writes;
 std::array<unsigned char,32> state_hash{};unsigned hash_bytes{};
 assert(EVP_DigestFinal_ex(frame_states.get(),state_hash.data(),&hash_bytes)==1&&hash_bytes==state_hash.size());
 std::array<char,65> state_hex{};constexpr char hex[]="0123456789abcdef";
 for(unsigned i=0;i<state_hash.size();++i){state_hex[i*2]=hex[state_hash[i]>>4];state_hex[i*2+1]=hex[state_hash[i]&15];}
 std::cout<<R"(,"frame_state_sha256":")"<<state_hex.data()<<'"';
 timing_json();std::cout<<R"(,"native_pcm_verified":false})"<<'\n';
 std::cerr<<"REGISTERS ";for(auto word:cpu->registers())std::cerr<<std::hex<<word<<' ';std::cerr<<std::dec<<'\n';
 if(state_trace){assert(!fclose(state_trace));state_trace=nullptr;}
 if(peripheral_trace){assert(!fclose(peripheral_trace));peripheral_trace=nullptr;}
 native_dma.reset();cpu.reset();dsp_destroy(peripheral);return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
