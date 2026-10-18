#include "sound_gp_cpu_tu15_ffi.h"
#include "dsp56300.h"
#include <string.h>
static Dsp56300MemoryRegion buffer(uint32_t start,uint32_t end,uint32_t *base) {
  Dsp56300MemoryRegion out={0};out.start=start;out.end=end;
  out.kind=DSP56300_REGION_BUFFER;out.data.buffer.base=base;return out;
}
static Dsp56300MemoryRegion callback(uint32_t start,uint32_t end,struct Halo2GpCoreCallbacks owner,int unmapped) {
  Dsp56300MemoryRegion out={0};out.start=start;out.end=end;out.kind=DSP56300_REGION_CALLBACK;
  out.data.callback.opaque=owner.owner;out.data.callback.read=unmapped?owner.unmapped_read:owner.read;
  out.data.callback.write=unmapped?owner.unmapped_write:owner.write;return out;
}
void *halo2_gp_core_create(uint32_t *x,uint32_t *mix,uint32_t *y,uint32_t *program,struct Halo2GpCoreCallbacks owner) {
  Dsp56300MemoryRegion xr[]={buffer(0,0xc00,x),buffer(0xc00,0x1000,mix),buffer(0x1400,0x1800,mix),
    callback(0xffff80,0x1000000,owner,0),callback(0x1000,0x1400,owner,1),callback(0x1800,0xffff80,owner,1)};
  Dsp56300MemoryRegion yr[]={buffer(0,0x800,y),callback(0x800,0x1000000,owner,1)};
  // Limit the translated P cache to physical PRAM plus an extension-word
  // sentinel. The C++ owner also rejects a PC outside physical PRAM.
  Dsp56300MemoryRegion pr[]={buffer(0,0x1000,program),callback(0x1000,0x1001,owner,1)};
  const Dsp56300CreateInfo info={{xr,6,yr,2,pr,2}};return dsp56300_create(&info);
}
void halo2_gp_core_destroy(void *core){dsp56300_destroy(core);}
void halo2_gp_core_initial_registers(void *core,const uint32_t *registers) {
  Dsp56300State state;dsp56300_get_state(core,&state);state.pc=0;
  memcpy(state.registers,registers,sizeof(state.registers));dsp56300_set_state(core,&state);
}
void halo2_gp_core_state(const void *core,uint32_t *pc,uint32_t *registers) {
  Dsp56300State state;dsp56300_get_state(core,&state);*pc=state.pc;
  if(registers)memcpy(registers,state.registers,sizeof(state.registers));
}
void halo2_gp_core_step(void *core){dsp56300_step(core);}
void halo2_gp_core_invalidate(void *core){dsp56300_invalidate_cache(core);}

uint32_t halo2_gp_core_run(void *core,uint32_t cycles){
  Dsp56300State state;dsp56300_get_state(core,&state);const uint32_t before=state.cycle_count;
  /* Fresh host scheduling slice, retaining actual architectural state. */
  state.cycle_budget=0;state.halt_requested=false;dsp56300_set_state(core,&state);
  dsp56300_run(core,(int32_t)cycles);return dsp56300_cycle_count(core)-before;
}
void halo2_gp_core_request_stop(void *core){dsp56300_set_halt_requested(core,true);}

uint32_t halo2_gp_core_cycles(const void *core){return dsp56300_cycle_count(core);}
void halo2_gp_core_execution_state(const void *core,uint32_t *pc,uint32_t *cycles,
    uint32_t *registers,uint32_t *stack,uint32_t *repeating,uint32_t *pc_on_repeat){
  Dsp56300State state;dsp56300_get_state(core,&state);
  *pc=state.pc;*cycles=state.cycle_count;
  memcpy(registers,state.registers,sizeof(state.registers));
  memcpy(stack,state.stack,sizeof(state.stack));
  *repeating=state.loop_rep;*pc_on_repeat=state.pc_on_rep;
}
