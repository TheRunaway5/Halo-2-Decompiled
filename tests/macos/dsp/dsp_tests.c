#include "audio/dsp/sound_gp_cpu_tu15_ffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(int value,const char *message){if(!value){fprintf(stderr,"%s\n",message);exit(1);}}
struct Owner {void *core;unsigned writes,invalid;};
static uint32_t read_device(void *owner,uint32_t address){(void)address;((struct Owner*)owner)->invalid++;return 0;}
static void write_device(void *owner,uint32_t address,uint32_t value){struct Owner *o=owner;
    check(address==0xffffc5 && value==0xfff,"DSP peripheral write differs");++o->writes;halo2_gp_core_request_stop(o->core);}
static void unmapped_write(void *owner,uint32_t address,uint32_t value){(void)address;(void)value;((struct Owner*)owner)->invalid++;}
int main(void){
    uint32_t x[0xc00]={0},mix[0x400]={0},y[0x800]={0},program[0x1000]={0},registers[64]={0};
    struct Owner owner={0};
    struct Halo2GpCoreCallbacks callbacks={&owner,read_device,write_device,read_device,unmapped_write};
    owner.core=halo2_gp_core_create(x,mix,y,program,callbacks);check(owner.core!=NULL,"DSP core allocation failed");
    for(unsigned i=32;i<40;++i)registers[i]=0xffffff;
    halo2_gp_core_initial_registers(owner.core,registers);
    /* Actual MOVE-immediate X0 and absolute JMP encodings already exercised
       by the shared GP owner tests. Verify real translation on each host ISA. */
    program[0]=0x44f400;program[1]=0x123456;program[2]=0x0af080;program[3]=0;
    halo2_gp_core_step(owner.core);uint32_t pc=0;halo2_gp_core_state(owner.core,&pc,registers);
    check(pc==2 && registers[4]==0x123456 && halo2_gp_core_cycles(owner.core)>0,"DSP instruction/PC/cycles differ");
    halo2_gp_core_step(owner.core);halo2_gp_core_state(owner.core,&pc,NULL);check(pc==0,"DSP jump differs");
    program[1]=0x654321;halo2_gp_core_invalidate(owner.core);halo2_gp_core_step(owner.core);
    halo2_gp_core_state(owner.core,&pc,registers);check(registers[4]==0x654321,"DSP program invalidation retained translated bytes");
    /* The production bridge maps both X windows to the same mix allocation. */
    halo2_gp_core_initial_registers(owner.core,registers);program[0]=0x44f000;program[1]=0x1400;
    program[2]=0x447000;program[3]=0x17ff;mix[0]=0xabcdef;halo2_gp_core_invalidate(owner.core);
    halo2_gp_core_step(owner.core);halo2_gp_core_state(owner.core,&pc,registers);
    check(registers[4]==0xabcdef,"DSP mix-window alias read differs");
    halo2_gp_core_step(owner.core);check(mix[0x3ff]==0xabcdef,"DSP mix-window alias write differs");
    program[0]=0x08f485;program[1]=0xfff;program[2]=0x44f400;program[3]=1;
    halo2_gp_core_initial_registers(owner.core,registers);halo2_gp_core_invalidate(owner.core);
    check(halo2_gp_core_run(owner.core,1000)>0,"DSP scheduling slice did not execute");
    halo2_gp_core_state(owner.core,&pc,registers);
    check(owner.writes==1 && !owner.invalid && pc==2 && registers[4]==0xabcdef,"DSP peripheral stop overran its callback boundary");
    halo2_gp_core_destroy(owner.core);
    puts("PASS pinned DSP core through production C bridge: real translation, invalidation, memory aliases, peripheral callback and stop boundary");
    return 0;
}
