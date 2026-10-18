#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Private C bridge for upstream's C-only generated API. No source CPU/DSP
// state, math, device results or peripheral behavior is supplied here.
struct Halo2GpCoreCallbacks {
  void *owner;
  uint32_t (*read)(void*,uint32_t);
  void (*write)(void*,uint32_t,uint32_t);
  uint32_t (*unmapped_read)(void*,uint32_t);
  void (*unmapped_write)(void*,uint32_t,uint32_t);
};
void *halo2_gp_core_create(uint32_t *x,uint32_t *mix,uint32_t *y,uint32_t *program,
    struct Halo2GpCoreCallbacks);
void halo2_gp_core_destroy(void*);
void halo2_gp_core_initial_registers(void*,const uint32_t*);
void halo2_gp_core_state(const void*,uint32_t *pc,uint32_t *registers);
void halo2_gp_core_step(void*);
void halo2_gp_core_invalidate(void*);
uint32_t halo2_gp_core_run(void*,uint32_t);
void halo2_gp_core_request_stop(void*);
uint32_t halo2_gp_core_cycles(const void*);
// Copy declared scalar/stack/repeat fields individually. Never compare raw
// generated C structs: their padding and host scheduling state are not DSP
// architectural bytes.
void halo2_gp_core_execution_state(const void*,uint32_t *pc,uint32_t *cycles,
    uint32_t *registers,uint32_t *stack,uint32_t *repeating,uint32_t *pc_on_repeat);
#ifdef __cplusplus
}
#endif
