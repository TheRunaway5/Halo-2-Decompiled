#pragma once
// Object looping-sound update (TU1.5 18EE50 group). Its routines call each
// other at their original addresses through object_loop_call, with the ABI
// state the original CALL leaves (return address pushed, stack arguments
// above it). Three files implement the routines:
//   native_tu15_sound_kernels_object_loop_frame.cpp   18EE50 frame and its direct helpers
//   native_tu15_sound_kernels_object_loop_update.cpp  18E970 object loop update group
//   native_tu15_sound_kernels_object_loop_sources.cpp 18D5A0/220C60 source group
#include "audio/native_tu15_sound_kernel.hpp"
#include "halo2/source_object_audio_views_tu15.hpp"

namespace halo2::native_tu15_sound {
using Views=SourceObjectAudioViewsTu15;
struct LoopContext {
    M &m;FP fp;const O &o;const Services &services;
    // Native camera views (step_native_object_audio); null reads the
    // original listener records, as an unbound original call does.
    const Views *views;
};
using LoopRoutine=void(*)(LoopContext &,Abi &);
struct LoopRoutineEntry {
    std::uint32_t address;LoopRoutine run;
};
std::span<const LoopRoutineEntry> object_loop_frame_routines();
std::span<const LoopRoutineEntry> object_loop_update_routines();
std::span<const LoopRoutineEntry> object_loop_source_routines();

// Runs the routine at an original address: a native group routine, else
// (until the group is complete) its lifted original, else the live world.
// Calls the original makes through the live world (the 220C60 loop-sound
// calls from 18D7FA and 18D93B, whose request observer feeds the native
// mixer) must use call_out() so the live world's hooks see them.
void object_loop_call(LoopContext &,Abi &,std::uint32_t address);
// CALL from native code: pushes the stack arguments (last first) and the
// original return address, runs the routine, then pops cdecl arguments.
inline void object_loop_invoke(LoopContext &c,Abi &a,std::uint32_t return_address,std::uint32_t address,
    std::span<const std::uint32_t> arguments={},bool callee_pops=false) {
    for(auto n=arguments.size();n-->0;){a.gp[4]-=4u;store(c.m,{},return_address,a.gp[4],arguments[n]);}
    a.gp[4]-=4u;store(c.m,{},return_address,a.gp[4],return_address);
    object_loop_call(c,a,address);
    if(!callee_pops)a.gp[4]+=std::uint32_t(arguments.size()*4u);
}
// The views bound by the running native object-audio frame, if any.
const Views *object_loop_views();
} // namespace halo2::native_tu15_sound
