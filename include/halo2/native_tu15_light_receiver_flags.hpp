#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct NativeTu15LightReceiverFlagCall {
    std::uint32_t light{},definition{},context{},stack{};
};
struct NativeTu15LightReceiverFlagResult {
    std::uint32_t bsp_rows{},object_rows{},changed_rows{},owner_none_passes{},root_passes{};
    bool f8_written{};
    std::uint32_t xmm0_word{};
};
// Whole31650. The four arguments remain in their original source stack cells;
// the original overwrites argument0 with its loop index and rereads argument1.
// Private callee stack and volatile GP residue are not outputs. Queue headers,
// counts and definition bytes retain the original reread/short-circuit order.
// No arithmetic changes x87/MXCSR. A reached31867 reports the MOVSS XMM0 word.
void native_tu15_light_receiver_flags_31650(SourcePhysicsContactMemory&,
    const NativeTu15LightReceiverFlagCall&,NativeTu15LightReceiverFlagResult&,
    const SourcePhysicsContactRecordObserver& = {});
}
