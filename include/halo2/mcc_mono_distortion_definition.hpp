#pragma once
#include "halo2/native_tu15_sound_mono_parameters.hpp"
#include "halo2/mcc_sound_effect_definitions.hpp"
#include <memory>
#include <optional>

namespace halo2::tu15_mono_parameter_conversion {
class CompiledMonoDefinition;
struct ConvertedMonoParameters {
    std::array<std::uint32_t,7> bounded_float_bits;
    Parameters parameters;
    std::shared_ptr<const CompiledMonoDefinition> definition_owner;
};
// One source-proven algorithm, selected through the actual resolved MCC name.
// This owns definitions, not a graph, initial DSP state or an install receipt.
class CompiledMonoDefinition final:public std::enable_shared_from_this<CompiledMonoDefinition> {
public:
    static std::shared_ptr<const CompiledMonoDefinition> compile(
        std::shared_ptr<const MccSoundEffectDefinitions>,TagHandle);
    ConvertedMonoParameters defaults(FpMode)const;
    // The instance must be a genuine member of this retained definition bank.
    // No override selects source defaults. Only selected constant inputs are
    // evaluated; selected functions fail before any parameter/state mutation.
    ConvertedMonoParameters instance(const MccSoundEffectDefinitions::Instance&,
        std::optional<std::size_t> override_index,FpMode)const;
    TagHandle identity()const{return identity_;}
    const MccSoundEffectDefinitions& definitions()const{return *definitions_;}
    const MccSoundEffectDefinitions::Template& source_template()const{return *template_;}
private:
    CompiledMonoDefinition(std::shared_ptr<const MccSoundEffectDefinitions>,TagHandle,
        const MccSoundEffectDefinitions::Template*);
    ConvertedMonoParameters convert(const MccSoundEffectDefinitions::ComponentOverride*,FpMode)const;
    std::shared_ptr<const MccSoundEffectDefinitions> definitions_;
    TagHandle identity_;
    const MccSoundEffectDefinitions::Template* template_;
};
}
