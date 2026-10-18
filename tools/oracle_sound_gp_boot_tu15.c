/* Analysis only. Unchanged TU1.5 GP boot/control/effects from the owned XBE.
 * No capture is a runtime input. Scratch is an explicit zero-backed offline ABI.
 * Original xemu DSP/DMA sources are a reference tool, not game dependencies.
 * Two instruction cores share the same declared emulator peripheral model.
 * This does not establish hardware DMA timing or physical Xbox PCM parity.
 */
#include "qemu/osdep.h"
#include "dsp.h"
#include "dsp_state.h"
#ifdef USE_JIT
#include "dsp56300.h"
static Dsp56300Jit *jit;
#endif
static uint8_t scratch[0x100000];
static size_t scratch_size;
static unsigned scratch_reads,scratch_writes,peripheral_reads,peripheral_writes;
static unsigned steps,blocks;
static DSPState *d;
static FILE *state_trace;
static void trace_word(uint32_t word){
 uint8_t bytes[4];for(unsigned i=0;i<4;++i)bytes[i]=(uint8_t)(word>>(8*i));assert(fwrite(bytes,1,4,state_trace)==4);
}
static void trace_state(void){
 if(!state_trace)return;
#ifdef USE_JIT
 Dsp56300State s;dsp56300_get_state(jit,&s);
 trace_word(s.pc);for(unsigned i=0;i<64;++i)trace_word(s.registers[i]);
 for(unsigned half=0;half<2;++half)for(unsigned i=0;i<16;++i)trace_word(s.stack[half][i]);
 trace_word(s.loop_rep);trace_word(s.pc_on_rep);
#else
 trace_word(d->core.pc);for(unsigned i=0;i<64;++i)trace_word(d->core.registers[i]);
 for(unsigned half=0;half<2;++half)for(unsigned i=0;i<16;++i)trace_word(d->core.stack[half][i]);
 trace_word(d->core.loop_rep);trace_word(d->core.pc_on_rep);
#endif
}
static size_t load(const char *dir,const char *name,void *dst,size_t maximum) {
 char path[2048];assert(snprintf(path,sizeof(path),"%s/%s",dir,name)<sizeof(path));
 FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0&&(size_t)n<=maximum);
 rewind(f);assert(fread(dst,1,n,f)==n);fclose(f);return n;
}
static void scratch_rw(void *opaque,uint8_t *ptr,uint32_t at,size_t len,bool write) {
 assert(at<=scratch_size&&len<=scratch_size-at);
 if(write){memcpy(scratch+at,ptr,len);++scratch_writes;}else{memcpy(ptr,scratch+at,len);++scratch_reads;}
}
static void fifo_rw(void *opaque,uint8_t *ptr,unsigned index,size_t len,bool write) {
 fprintf(stderr,"Unexpected FIFO request index=%u bytes=%zu write=%u\n",index,len,write);abort();
}
#ifdef USE_JIT
static uint32_t periph_read(void *opaque,uint32_t a) {
 assert((a>=0xffffb0&&a<=0xffffb3)||a==0xffffc4||a==0xffffc5||(a>=0xffffd4&&a<=0xffffd7));++peripheral_reads;
 return d->core.read_peripheral(&d->core,a);
}
static void periph_write(void *opaque,uint32_t a,uint32_t v) {
 assert((a>=0xffffb0&&a<=0xffffb3)||a==0xffffc4||a==0xffffc5||(a>=0xffffd4&&a<=0xffffd7));++peripheral_writes;
 d->core.write_peripheral(&d->core,a,v);
}
static Dsp56300MemoryRegion buffer(uint32_t lo,uint32_t hi,uint32_t *base) {
 Dsp56300MemoryRegion r={0};r.start=lo;r.end=hi;r.kind=DSP56300_REGION_BUFFER;r.data.buffer.base=base;return r;
}
static void init_jit(void) {
 Dsp56300MemoryRegion xr[]={buffer(0,0xc00,d->core.xram),buffer(0xc00,0x1000,d->core.mixbuffer),buffer(0x1400,0x1800,d->core.mixbuffer),{0}};
 xr[3].start=0xffff80;xr[3].end=0x1000000;xr[3].kind=DSP56300_REGION_CALLBACK;
 xr[3].data.callback.opaque=d;xr[3].data.callback.read=periph_read;xr[3].data.callback.write=periph_write;
 Dsp56300MemoryRegion yr[]={buffer(0,0x800,d->core.yram)},pr[]={buffer(0,0x1000,d->core.pram)};
 Dsp56300CreateInfo info={.memory_map={.x_regions=xr,.x_count=4,.y_regions=yr,.y_count=1,.p_regions=pr,.p_count=1}};
 jit=dsp56300_create(&info);assert(jit);Dsp56300State s;dsp56300_get_state(jit,&s);
 for(unsigned i=0;i<64;++i)s.registers[i]=d->core.registers[i];s.pc=0;dsp56300_set_state(jit,&s);
}
#endif
static void frame(void) {
 d->core.is_idle=false;dsp_start_frame(d);
#ifdef USE_JIT
 Dsp56300State s;
#endif
 for(unsigned i=0;i<200000;++i) {
#ifdef USE_JIT
  dsp56300_get_state(jit,&s);uint32_t pc=s.pc;
  assert(!(s.registers[DSP56300_REG_SR]&((1u<<17)|(1u<<13)|(1u<<14))));
  assert(!(s.registers[DSP56300_REG_OMR]&((1u<<20)|(1u<<7))));
#else
  uint32_t pc=d->core.pc;
#endif
  if(d->core.is_idle){assert(pc==0x2b);trace_state();++blocks;return;}
  if(pc>0xf50){fprintf(stderr,"Out-of-interval PC %x\n",pc);abort();}
#ifdef USE_JIT
  dsp56300_step(jit);
#else
  dsp_step(d);
#endif
  ++steps;
 }
 fprintf(stderr,"Original GP call did not complete; DMA control=%x eol=%u\n",d->dma.control,d->dma.eol);abort();
}
int main(int argc,char **argv) {
 assert(argc==5||argc==7);if(argc==7){assert(!strcmp(argv[5],"--state-trace"));state_trace=fopen(argv[6],"wb");assert(state_trace);}
 unsigned frames=strtoul(argv[3],NULL,0),mask=strtoul(argv[4],NULL,0);assert(frames&&frames<=4096);
 d=dsp_init(NULL,scratch_rw,fifo_rw);assert(d);d->is_gp=d->core.is_gp=true;
 scratch_size=load(argv[1],"initial-scratch.bin",scratch,sizeof(scratch));
 assert(*(uint32_t*)(scratch+0x810)==3);
 dsp_bootstrap(d);
#ifdef USE_JIT
 init_jit();
#endif
 // Execute the original boot and first actual request, including its clear
 // of header word4. No CPU return, DSP state or ready word is supplied.
 frame();frame();assert(*(uint32_t*)(scratch+0x810)==0);
 uint32_t expected[0xde0],states[15];
 assert(load(argv[1],"expected-effects.bin",expected,sizeof(expected))==sizeof(expected));
 assert(!memcmp(d->core.pram+0x171,expected,sizeof(expected)));
 assert(load(argv[1],"effect-states.bin",states,sizeof(states))==sizeof(states));
 for(unsigned i=0;i<15;++i)assert(states[i]+4<0xc00&&(d->core.xram[states[i]+4]&1));
 FILE *f=fopen(argv[2],"wb");assert(f);
 for(unsigned block=0;block<frames;++block) {
  memset(d->core.mixbuffer,0,sizeof(d->core.mixbuffer));
  for(unsigned bus=0;bus<32;++bus)if(mask&(1u<<bus))for(unsigned i=0;i<32;++i)
   d->core.mixbuffer[bus*32+i]=(((block*32+i)*0x12843u+bus*0x45678u)&0x3ffff)-0x20000u&0xffffff;
  frame();assert(fwrite(d->core.mixbuffer,4,1024,f)==1024);
 }
 assert(fwrite(d->core.pram,4,0x1000,f)==0x1000);assert(fwrite(d->core.xram,4,0xc00,f)==0xc00);assert(fwrite(d->core.yram,4,0x800,f)==0x800);
 assert(fwrite(scratch,1,scratch_size,f)==scratch_size);fclose(f);
 printf("{\"blocks\":%u,\"instructions\":%u,\"scratch_reads\":%u,\"scratch_writes\":%u,\"peripheral_reads_instrumented_jit_only\":%u,\"peripheral_writes_instrumented_jit_only\":%u,\"native_pcm_verified\":false}\n",blocks,steps,scratch_reads,scratch_writes,peripheral_reads,peripheral_writes);
#ifdef USE_JIT
 dsp56300_destroy(jit);
#endif
 if(state_trace){assert(!fclose(state_trace));state_trace=NULL;}
 dsp_destroy(d);return 0;
}
