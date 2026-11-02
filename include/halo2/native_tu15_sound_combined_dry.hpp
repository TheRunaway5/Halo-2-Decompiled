#pragma once
#include "halo2/native_tu15_sound_graph_storage.hpp"
namespace halo2::tu15_sound_combined_dry {
struct Controls {
    std::array<tu15_sound_chorus::Controls,4> chorus;
    tu15_sound_crosstalk::Controls crosstalk;
};
struct Trace {
    std::array<std::array<tu15_sound_chorus::SampleTrace,32>,4> chorus;
    std::array<tu15_sound_crosstalk::ArithmeticTrace,64> crosstalk;
};
// Original records4..7 followed by record10, over the real shared native
// histories and graph-owned buffers. Input bins6..9/common14 and existing
// output bins0/1/4/5 belong to the caller. Modes are preflighted before any
// module runs; malformed data/state can still fault at its owning module.
// No record8/9 wet processing, voice-to-bin producer, GP supervisor, output
// receipt, driver readiness or hardware/device completion is supplied here.
void process(const Controls&,tu15_sound_graph_storage::GraphStorage&,Trace&);
}
