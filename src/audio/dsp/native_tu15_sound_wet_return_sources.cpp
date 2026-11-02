#include "halo2/native_tu15_sound_wet_return_sources.hpp"
namespace halo2::tu15_sound_wet_return {
View Sources::view(unsigned index)const{
    const auto route=source_sound_wet_return_route_tu15(index);
    const auto offset=6*tu15_gp_output::ring_words+(route.input_bin-11u)*32u;
    return {route,receipt_.retained_values().subspan(offset,32).first<32>()};
}
View Sources::logical_view(const SourceSoundWetVoiceLayoutTu15& layout)const{
    return view(layout.input_bin()-15u);
}
}
