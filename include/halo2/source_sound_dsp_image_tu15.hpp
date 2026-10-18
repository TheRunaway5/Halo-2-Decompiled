#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
// Optional native owner for the original write-only GP table stores. These
// are resource transitions, not byte-backed memory or a synthetic MMIO bank.
// The owner records its actual transition; the generic before/after byte
// observer covers ordinary memory and cannot invent an unknown MMIO preimage.
class SourceSoundDspImageMmioTu15 {
public:
    virtual ~SourceSoundDspImageMmioTu15() = default;
    virtual void write_sound_dsp_mmio(std::uint32_t instruction,
        std::uint32_t address,std::uint32_t value) = 0;
};
// Complete388275/388294/38850A/388576 object construction/resource-preparation
// callers,388309/38DE81 image upload/allocation CPU callers,
//38DD0C/38DD26/38DD54/38DE07/38DE59/38E668/38E6DF scratch-table CPU bodies and
//38DF59/38DFD2/38DFFF byte-buffer decoder,3879B3 pool release wrapper and
//38F5F3 EP scratch metadata/three-page GP append bridge.
// Original relocation/request/polling
// instructions execute against caller-owned memory. Actual allocation/DMA
// resources and GP acknowledgement remain mandatory; this supplies no
// platform success, device availability or substituted request-clear value.
bool source_sound_dsp_image_tu15_owns_entry(std::uint32_t);
void source_run_sound_dsp_image_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
