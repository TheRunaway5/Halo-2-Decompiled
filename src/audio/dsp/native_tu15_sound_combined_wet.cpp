#include "halo2/native_tu15_sound_combined_wet.hpp"
namespace halo2::tu15_sound_combined_wet {
namespace {
void validate_prepare(const PrepareControls& controls){
    tu15_sound_gain_fanout::validate_controls(controls.fanout);
    tu15_sound_mono_distortion::validate_controls(controls.distortion);
    for(const auto& control:controls.accumulators)tu15_sound_two_product_accumulator::validate_controls(control);
}
}
void prepare(const PrepareControls& controls,tu15_sound_graph_storage::GraphStorage& graph,Trace& trace){
    validate_prepare(controls);auto& m=graph.effects();
    tu15_sound_gain_fanout::process(controls.fanout,m.fanout_parameters,m.fanout_state,graph.fanout_input(),graph.fanout_routes());
    tu15_sound_mono_distortion::process(controls.distortion,m.distortion_parameters,m.distortion_state,graph.mixbin(13),graph.mixbin(13));
    for(unsigned i=0;i<2;++i)(void)tu15_sound_two_product_accumulator::process(controls.accumulators[i],
        m.accumulator_parameters[i],m.accumulator_states[i],graph.mixbin(10),graph.fanout_destination(4),graph.mixbin(11+i),trace.accumulators[i]);
}
void reverbs(const std::array<tu15_sound_ring_effect::Controls,2>& controls,tu15_sound_graph_storage::GraphStorage& graph,Trace& trace){
    for(const auto& control:controls)tu15_sound_ring_effect::validate_controls(control);
    graph.copy_common_to_reverb_buffers();
    // Both original records share one workspace, with no reload between calls.
    for(unsigned i=0;i<2;++i){auto& r=graph.reverb(i);
        const std::array<std::span<std::uint32_t,32>,4> outputs{r.outputs[0],r.outputs[1],r.outputs[2],r.outputs[3]};
        tu15_sound_ring_effect::process(r.state,graph.raw_reverb_workspace(),r.input,outputs,controls[i],&trace.reverbs[i]);
    }
    graph.copy_reverb_buffers_to_common();
}
void process(const Controls& controls,tu15_sound_graph_storage::GraphStorage& graph,Trace& trace){
    validate_prepare(controls.prepare);
    for(const auto& control:controls.reverbs)tu15_sound_ring_effect::validate_controls(control);
    prepare(controls.prepare,graph,trace);reverbs(controls.reverbs,graph,trace);
}
}
