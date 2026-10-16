#include "halo2/mcc_mono_distortion_definition.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2::tu15_mono_parameter_conversion {
namespace {
using Definitions=MccSoundEffectDefinitions;
constexpr std::array<float,7> minimum{0,0,-30,0.001f,0,-30,0.001f};
constexpr std::array<float,7> maximum{1,20000,30,30,20000,30,30};
bool finite(std::uint32_t bits){return (bits&0x7fffffff)<0x7f800000;}
std::uint32_t ordered(std::uint32_t bits){
    // Finite IEEE floats, with both zero signs comparing equal. This keeps
    // metadata admission independent of the host caller's DAZ/FTZ mode.
    if(!(bits&0x7fffffff))bits=0;
    return bits&0x80000000?~bits:bits|0x80000000;
}
bool mono(const Definitions::Name& name){return name.text&&*name.text=="sfx_mono_distortion";}
void validate(const Definitions::Template& item){
    if(!mono(item.input)||item.components.size()!=7||!(Bytes(item.raw.bytes).u32(8)&1))
        throw FormatError("Unproved MCC mono-distortion template layout");
    for(std::size_t i=0;i<7;++i){
        const auto& p=item.components[i];
        const auto low=ordered(p.minimum_bits),high=ordered(p.maximum_bits);
        if(p.value_type!=1||!finite(p.default_bits)||!finite(p.minimum_bits)||!finite(p.maximum_bits)||
            low>high||low<ordered(std::bit_cast<std::uint32_t>(minimum[i]))||
            high>ordered(std::bit_cast<std::uint32_t>(maximum[i])))
            throw FormatError("Unproved MCC mono-distortion parameter type/bounds");
    }
}
}
CompiledMonoDefinition::CompiledMonoDefinition(std::shared_ptr<const Definitions> bank,TagHandle id,
    const Definitions::Template* item):definitions_(std::move(bank)),identity_(id),template_(item){}
std::shared_ptr<const CompiledMonoDefinition> CompiledMonoDefinition::compile(
    std::shared_ptr<const Definitions> bank,TagHandle identity){
    if(!bank)throw FormatError("Mono-distortion conversion needs retained MCC definitions");
    const auto& effect=bank->effect_template(identity);
    if(!effect.additional_inputs.empty())throw FormatError("Additional MCC mono inputs are unresolved");
    const Definitions::Template* selected=nullptr;
    for(const auto& platform:effect.platforms)if(mono(platform.input))for(const auto& item:platform.templates)if(mono(item.input)){
        if(selected)throw FormatError("Ambiguous resolved MCC mono-distortion definition");
        validate(item);selected=&item;
    }
    if(!selected)throw FormatError("MCC definition is not the proved mono-distortion algorithm");
    return std::shared_ptr<const CompiledMonoDefinition>(new CompiledMonoDefinition(std::move(bank),identity,selected));
}
ConvertedMonoParameters CompiledMonoDefinition::defaults(FpMode mode)const{return convert(nullptr,mode);}
ConvertedMonoParameters CompiledMonoDefinition::instance(const Definitions::Instance& value,
    std::optional<std::size_t> override_index,FpMode mode)const{
    bool owned=false;
    for(const auto& collection:definitions_->collections())for(const auto& effect:collection.effects)
        for(const auto& candidate:effect.instances)if(&candidate==&value)owned=true;
    if(!owned||value.effect_template.group!=TagGroup{"<fx>"}||value.effect_template.handle!=identity_)
        throw FormatError("MCC mono instance is not owned by this exact retained definition bank");
    const Definitions::ComponentOverride* selected=nullptr;
    if(override_index){
        if(*override_index>=value.override_records.size())throw FormatError("MCC mono override index is absent");
        const auto& root=value.override_records[*override_index];
        if(root.component_mask&1){
            if(root.components.empty())throw FormatError("Selected MCC mono override component is absent");
            selected=&root.components[0];
        }
    }
    return convert(selected,mode);
}
ConvertedMonoParameters CompiledMonoDefinition::convert(const Definitions::ComponentOverride* override,FpMode mode)const{
    std::array<std::uint32_t,7> values{};std::size_t source_index=0;
    for(std::size_t i=0;i<7;++i){
        const auto& p=template_->components[i];auto bits=p.default_bits;
        if(override&&(override->parameter_mask&(1u<<i))){
            if(source_index>=override->sources.size())throw FormatError("Selected MCC mono parameter source is absent");
            const auto source=override->sources[source_index++].bits;
            // Original MOVSX/SAR consumes a signed byte index. Negative
            // source indices have no proved native metadata owner.
            if(source&0x80)throw FormatError("Negative MCC mono parameter source is unresolved");
            if(source&1)throw FormatError("Selected MCC mono parameter function is unresolved");
            const auto index=std::size_t(source>>1);
            if(index>=override->constants.size())throw FormatError("Selected MCC mono constant is absent");
            bits=override->constants[index].bits;
        }
        if(!finite(bits))throw FormatError("Nonfinite MCC mono input is unresolved");
        //228F01..228F1B compares lower first, then upper; all admitted inputs
        // are finite. Preserve the chosen float word, including signed zero.
        const auto value=ordered(bits),low=ordered(p.minimum_bits),high=ordered(p.maximum_bits);
        if(value<low)bits=p.minimum_bits;else if(value>high)bits=p.maximum_bits;
        values[i]=bits;
    }
    const auto parameters=convert_bounded_values(values,mode);
    return {values,parameters,shared_from_this()};
}
}
