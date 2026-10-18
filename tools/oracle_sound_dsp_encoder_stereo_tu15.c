/* Analysis-only original EP program oracle. The pinned emulator core/DMA is
 * linked only to this reference tool. Fixture inputs are explicitly declared.
 */
#include "qemu/osdep.h"
#include "dsp.h"
#include "dsp_state.h"
static DSPState *d;
static uint8_t scratch[0x13000],pcm[1024];
static uint32_t input[2+6*256],left[256],right[256];
static unsigned emitted;
static void load(const char *path,void *dst,size_t bytes) {
 FILE *f=fopen(path,"rb");assert(f);assert(fread(dst,1,bytes,f)==bytes&&fgetc(f)==EOF);fclose(f);
}
static void scratch_rw(void *o,uint8_t *ptr,uint32_t at,size_t len,bool write) {
 assert(at<=sizeof(scratch)&&len<=sizeof(scratch)-at);
 if(write)memcpy(scratch+at,ptr,len);else memcpy(ptr,scratch+at,len);
}
static void fifo_rw(void *o,uint8_t *ptr,unsigned index,size_t len,bool write) {
 assert(write&&index==0&&len==1024&&!emitted);assert(!(d->core.registers[DSP_REG_SR]&0x10c00));
 memcpy(pcm,ptr,1024);memcpy(left,d->core.xram+0x500,1024);memcpy(right,d->core.xram+0x600,1024);++emitted;
}
int main(int argc,char **argv) {
 assert(argc==4);load(argv[1],scratch,sizeof(scratch));load(argv[2],input,sizeof(input));
 uint32_t config;memcpy(&config,scratch+0x450,4);config*=4;assert(config+24*4<=sizeof(scratch));
 memcpy(scratch+config+16*4,input,8);
 for(unsigned ch=0;ch<6;++ch)for(unsigned half=0;half<2;++half)
  memcpy(scratch+0x10000+ch*0x800+half*0x400,input+2+ch*256,0x400);
 d=dsp_init(NULL,scratch_rw,fifo_rw);assert(d);
 for(unsigned i=0;i<4096;++i)d->core.pram[i]=d->core.xram[i]=0xcacacaca;
 for(unsigned i=0;i<2048;++i)d->core.yram[i]=0xcacacaca;
 dsp_bootstrap(d);
 for(unsigned frame=0;frame<64&&!emitted;++frame){
  dsp_start_frame(d);d->core.is_idle=false;unsigned slices=0;
  do{dsp_run(d,1000);assert(++slices<10000);}while(!d->core.is_idle);
 }
 assert(emitted==1);FILE *f=fopen(argv[3],"wb");assert(f);
 assert(fwrite(left,4,256,f)==256&&fwrite(right,4,256,f)==256&&fwrite(pcm,1,1024,f)==1024);fclose(f);
 dsp_destroy(d);return 0;
}
