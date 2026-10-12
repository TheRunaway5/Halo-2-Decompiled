#pragma once
#include <cstdint>
namespace halo2 {
//17A7E4..17A818:part+30/+34 bit5 selects the effect's retained A/B
// controls (+6C/+70). The caller owns their actual production and admission.
float source_effect_part_control(std::uint32_t flags_a,std::uint32_t flags_b,
    float scale_a,float scale_b);
}
