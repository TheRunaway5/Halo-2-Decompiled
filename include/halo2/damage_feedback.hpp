#pragma once
#include "halo2/shader_animation.hpp"
#include <cstdint>
#include <optional>

namespace halo2 {
struct RecentDamageState {
  float visible{}, accumulated{}; // object+F4/+FC (shield), +F8/+100 (body).
  std::int8_t ticks = -1; // object+108/+109, B7B40 initializes both to NONE.
};
struct DamageFeedbackState {
  RecentDamageState shield, body;
};
struct DamageFadeDefinition {
  float visible_delay{}, visible_decay = 2;
  float accumulated_delay = 2, accumulated_decay = 2;
};
// D9640's recent-damage stores only. Report bits0/1 force body/shield amounts
// to at least1 on newly admitted depletion. Impulse, attribution and reactions
// remain their original caller services.
void report_damage_feedback(DamageFeedbackState &, float body, float shield,
                            std::uint32_t report_flags, bool shield_special=false);
// D8B50, including the signed timer, nearest-even ticks and ordered SSE stores.
// Settings are actor vitality bounds+30..3C, or defaults when no bounds exist;
// they are not the hlmt damage-info block. Return is original AL.
bool fade_recent_damage(RecentDamageState &, std::int16_t tick_rate, bool reset,
                        const DamageFadeDefinition & = {});
// D5DE0's body/shield recent-damage tail, with bit11's separate shield branch.
bool update_damage_feedback(DamageFeedbackState &, std::int16_t tick_rate,
    bool body_depleted, bool shield_depleted, bool shield_special=false,
    const DamageFadeDefinition & = {});
struct DamageFeedbackInput { float value{}; bool active{}; };
// BCC20's vitality/recent-damage inputs. Unknown names remain unresolved.
std::optional<DamageFeedbackInput> damage_feedback_input(const DamageFeedbackState &,
    float body, float shield, bool dead, std::uint32_t name);
// BE8B0/BAB40 exports use the authored object functions and BCC20's distinct
// scalar/activity outputs. An inactive lookup supplies zero to the shader.
float damage_feedback_shader_input(const DamageFeedbackState &, float body,
    float shield, bool dead, std::span<const ObjectFunction>,
    const ShaderFunctionTables &, std::uint32_t name, float seconds);
}
