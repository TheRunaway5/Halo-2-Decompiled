#pragma once
#include "halo2/damage_feedback.hpp"
#include <array>

namespace halo2 {
class Cache;
// Resolve only verified builtin symbols in the owning cache. Authored outputs,
// enable gates and scale aliases keep their original cache-local identities.
class DamageFeedbackNames {
public:
    DamageFeedbackNames()=default;
    static DamageFeedbackNames from_cache(const Cache &);
    std::optional<std::uint32_t> builtin(std::uint32_t authored) const;
private:
    static constexpr std::array<std::uint32_t,8> original_{
        0x030005a6u,0x040005a7u,0x0500055bu,0x0d000558u,
        0x0f000559u,0x13000556u,0x15000557u,0x1800055au};
    std::array<std::uint32_t,8> authored_=original_;
};
float damage_feedback_shader_input(const DamageFeedbackState &,float body,
    float shield,bool dead,std::span<const ObjectFunction>,const ShaderFunctionTables &,
    std::uint32_t name,float seconds,const DamageFeedbackNames &);
}
