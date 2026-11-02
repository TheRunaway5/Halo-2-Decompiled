#pragma once
#include "halo2/native_tu15_sound_graph_storage.hpp"
namespace halo2::tu15_sound_combined_wet {
struct PrepareControls {
    tu15_sound_gain_fanout::Controls fanout{};
    tu15_sound_mono_distortion::Controls distortion{};
    std::array<tu15_sound_two_product_accumulator::Controls,2> accumulators{};
};
struct Controls {
    PrepareControls prepare;
    std::array<tu15_sound_ring_effect::Controls,2> reverbs;
};
struct Trace {
    std::array<std::array<tu15_sound_two_product_accumulator::ArithmeticTrace,32>,2> accumulators;
    std::array<tu15_sound_ring_effect::ArithmeticTrace,2> reverbs;
};
// Records0..3 prepare authored wet sends; records8/9 consume them. The split
// permits records4..7 to run in their original position between these stages.
// Raw input/prefix samples and entry modes are explicit caller-owned boundaries.
// This API establishes no PCM scaling, voice route, return-to-speaker mix, DMA
// receipt, supervisor/device readiness or whole original audio-engine parity.
void prepare(const PrepareControls&,tu15_sound_graph_storage::GraphStorage&,Trace&);
void reverbs(const std::array<tu15_sound_ring_effect::Controls,2>&,
             tu15_sound_graph_storage::GraphStorage&,Trace&);
// Preflight every mode before executing either stage. Data/state admission
// still belongs to each individual processor, rather than an atomic graph.
void process(const Controls&,tu15_sound_graph_storage::GraphStorage&,Trace&);
}
