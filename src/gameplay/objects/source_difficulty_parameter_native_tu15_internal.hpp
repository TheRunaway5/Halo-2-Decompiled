#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "halo2/source_difficulty_parameter_native_tu15.hpp"
#include <bit>

namespace halo2::physics_world_runtime_detail {

constexpr std::uint32_t difficulty_parameter_index_tu15(std::uint16_t parameter,
    std::uint16_t difficulty) {
    const auto signed_parameter=std::bit_cast<std::int16_t>(parameter);
    const auto signed_difficulty=std::bit_cast<std::int16_t>(difficulty);
    const auto selected=signed_difficulty<0?0:signed_difficulty>3?3:signed_difficulty;
    return std::uint32_t(std::int32_t(signed_parameter))*4u+std::uint32_t(selected);
}

// Complete1F1950: preserve the default-word read before the null gates, the
// nonzero count test, signed low-word difficulty, signed parameter indexing,
// raw SSE payload and original fault/ABI boundaries. Admission requires whole
// unchanged-original and retained-literal comparisons, including read faults.
template<class State,class Site>
void difficulty_parameter_native_tu15(State& s,Site site) {
    difficulty_parameter_native_detail::parameter_calls.fetch_add(1,std::memory_order_relaxed);
    site(0x1ed030u,0x1f1950u);s.gp[0]=r(s.memory,0x4ecc1cu,4);
    site(0x1ed035u,0x1f1955u);s.flags.logic(s.gp[0],4);
    site(0x1ed037u,0x1f1957u);
    s.xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(r(s.memory,0x46a0d0u,4))));
    site(0x1ed03fu,0x1f195fu);
    if(s.gp[0]) {
        site(0x1ed041u,0x1f1961u);s.gp[2]=r(s.memory,s.gp[0]+0xf8u,4);
        site(0x1ed047u,0x1f1967u);s.flags.logic(s.gp[2],4);
        site(0x1ed049u,0x1f1969u);
        if(s.gp[2]) {
            site(0x1ed04bu,0x1f196bu);s.gp[0]=r(s.memory,s.gp[0]+0xfcu,4);
            site(0x1ed051u,0x1f1971u);s.flags.logic(s.gp[0],4);
            site(0x1ed053u,0x1f1973u);
            if(s.gp[0]) {
                const auto difficulty=std::uint16_t(s.gp[1]);
                site(0x1ed055u,0x1f1975u);s.flags.logic(difficulty,2);
                site(0x1ed058u,0x1f1978u);
                if(std::bit_cast<std::int16_t>(difficulty)<0) {
                    site(0x1ed05au,0x1f197au);
                    const auto parameter=std::uint16_t(r(s.memory,s.gp[4]+4u,2));
                    s.gp[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(parameter)));
                    site(0x1ed05fu,0x1f197fu);s.gp[1]=s.flags.logic(0u,4);
                    site(0x1ed061u,0x1f1981u);s.gp[1]=0;
                    site(0x1ed064u,0x1f1984u);s.gp[2]=difficulty_parameter_index_tu15(parameter,difficulty);
                    site(0x1ed067u,0x1f1987u);
                    s.xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(r(s.memory,s.gp[0]+s.gp[2]*4u,4))));
                    site(0x1ed06cu,0x1f198cu);s.pop();s.gp[4]+=4u;return;
                }
                site(0x1ed06fu,0x1f198fu);s.flags.minus(difficulty,3u,2);
                site(0x1ed073u,0x1f1993u);
                if(difficulty>3u) {site(0x1ed075u,0x1f1995u);s.gp[1]=3u;}
                site(0x1ed07au,0x1f199au);
                const auto parameter=std::uint16_t(r(s.memory,s.gp[4]+4u,2));
                s.gp[2]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(parameter)));
                site(0x1ed07fu,0x1f199fu);s.gp[1]=std::uint16_t(s.gp[1]);
                site(0x1ed082u,0x1f19a2u);s.gp[2]=difficulty_parameter_index_tu15(parameter,difficulty);
                site(0x1ed085u,0x1f19a5u);
                s.xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(r(s.memory,s.gp[0]+s.gp[2]*4u,4))));
            }
        }
    }
    site(0x1ed08au,0x1f19aau);s.pop();s.gp[4]+=4u;
}

// Complete1F19B0: campaign mode reads the signed authored difficulty; other
// modes select the original literal1 before the same parameter owner.
template<class State,class Site>
void current_difficulty_parameter_native_tu15(State& s,Site site) {
    difficulty_parameter_native_detail::current_difficulty_calls.fetch_add(1,std::memory_order_relaxed);
    site(0x1ed090u,0x1f19b0u);s.gp[0]=r(s.memory,0x4f3218u,4);
    site(0x1ed095u,0x1f19b5u);s.gp[2]=r(s.memory,s.gp[0]+8u,4);
    site(0x1ed098u,0x1f19b8u);s.gp[1]=1;
    site(0x1ed09du,0x1f19bdu);s.flags.minus(s.gp[2],1u,4);
    site(0x1ed09fu,0x1f19bfu);
    if(s.gp[2]==1u) {
        site(0x1ed0a1u,0x1f19c1u);
        s.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,s.gp[0]+0x132u,2)))));
    }
    site(0x1ed0a8u,0x1f19c8u);difficulty_parameter_native_tu15(s,site);
}
}
