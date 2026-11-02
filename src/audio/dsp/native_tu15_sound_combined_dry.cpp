#include "halo2/native_tu15_sound_combined_dry.hpp"
namespace halo2::tu15_sound_combined_dry {
void process(const Controls& controls,tu15_sound_graph_storage::GraphStorage& graph,Trace& trace){
    for(const auto& control:controls.chorus)tu15_sound_chorus::validate_controls(control);
    tu15_sound_crosstalk::validate_controls(controls.crosstalk);
    auto& modules=graph.effects();
    for(unsigned i=0;i<4;++i)tu15_sound_chorus::process(controls.chorus[i],modules.chorus_parameters[i],
        modules.chorus_states[i],graph.chorus_bindings(i),graph.chorus_history(i),trace.chorus[i]);
    const std::array<tu15_sound_crosstalk::Input,4> inputs{
        graph.fanout_destination(0),graph.fanout_destination(1),graph.fanout_destination(2),graph.fanout_destination(3)};
    const std::array<tu15_sound_crosstalk::Output,4> outputs{
        graph.mixbin(0),graph.mixbin(1),graph.mixbin(4),graph.mixbin(5)};
    (void)tu15_sound_crosstalk::process(controls.crosstalk,modules.crosstalk_state,inputs,outputs,trace.crosstalk);
}
}
