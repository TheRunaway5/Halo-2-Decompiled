#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/native_weapon_zoom_reset_tu15.hpp"

namespace halo2 {
// TU1.5 106F90/108A70 selection/dispatch,17DFF0 live-effect refresh,
// 178E30 creation,17A1A0 effect restart,17B130 event reset,17DC60 component reset,17DCE0 location iteration,
// 17DD50/17DDC0 location-record source selection and ordered matrix copies,
// 17C020 next-event selection,17C140 event advancement,17B0D0 placement/scalars,17E5E0 component-handle initialization,
// 17AFB0 allocation/retry,17B960 location-pair initialization,16E6A0 raw datum allocation,
// BBE00 object origin,1791C0 direction records,179610 attached request,
// CABC0 scope reset,18BE00 local-sound decision,18BE50 direct sound,18C4B0 forced zoom-out request.
// Lower allocation and sound owners retain their complete ABI and actual results.
bool native_weapon_effect_dispatch_tu15_owns_entry(std::uint32_t);
// Complete integer register/flag boundary of17DCE0. Compiled callers keep
// their live XMM/x87/MMX file in place; this owner never consumes or changes it.
void native_weapon_effect_iterator_tu15_run(SourcePhysicsContactMemory &,
    std::array<std::uint32_t,8> &,std::uint32_t &eflags,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
// Integer-only raw pool allocation preserves the complete live FP/MMX file.
void native_weapon_effect_datum_allocate_tu15_run(SourcePhysicsContactMemory &,
    std::array<std::uint32_t,8> &,std::uint32_t &eflags,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
// Complete integer owners (16E6A0,17DCE0,18BE00) share the live caller FP file.
void native_weapon_effect_integer_tu15_run(SourcePhysicsContactMemory &,std::uint32_t,
    std::array<std::uint32_t,8> &,std::uint32_t &eflags,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});
void native_weapon_effect_dispatch_tu15_run(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {},
    const NativeWeaponZoomResetObserverTu15 & = {});
}
