#pragma once
#include "halo2/mcc_sound_effect_definitions.hpp"
#include <array>
#include <memory>
namespace halo2::tu15_effect_function {
// The actual owner is four input words followed by three direction words.
// One C++ array owns all seven, including admitted mod-selected words4..6.
struct Inputs {std::array<std::uint32_t,7> words;};
struct SpeakerGeometry {std::array<std::array<std::uint32_t,3>,5> directions;};
struct ExplicitFp {std::uint16_t loaded_x87_control;std::uint32_t mxcsr;};
struct CubicPolynomial {std::uint32_t cubic,quadratic,linear,constant;};
struct CubicParameterValues {
    std::int16_t input,range_input;
    std::uint32_t period_bits,lower_bits,upper_bits;
    std::uint8_t flags;
    std::array<CubicPolynomial,2> polynomials;
};
struct Values {std::uint32_t fraction_bits,value_bits;};
// Source228CD0 selected-function math. Inputs, FP and speaker geometry are
// supplied values, never an inferred clock, caller environment or install.
Values evaluate_cubic_values(const CubicParameterValues&,const Inputs&,std::int32_t speaker,
    const SpeakerGeometry*,ExplicitFp);
// Original IEEE results before later builder bounds; overflow outputs are retained.
Values evaluate_cubic_unbounded_values(const CubicParameterValues&,const Inputs&,std::int32_t speaker,
    const SpeakerGeometry*,ExplicitFp);
class CompiledParameterFunction;
struct EvaluatedParameter {
    Values values;
    std::shared_ptr<const CompiledParameterFunction> definition_owner;
};
class CompiledParameterFunction final:public std::enable_shared_from_this<CompiledParameterFunction> {
public:
    static std::shared_ptr<const CompiledParameterFunction> compile(
        std::shared_ptr<const MccSoundEffectDefinitions>,const MccSoundEffectDefinitions::Function&);
    EvaluatedParameter evaluate(const Inputs&,std::int32_t speaker,const SpeakerGeometry*,ExplicitFp)const;
    EvaluatedParameter evaluate_unbounded(const Inputs&,std::int32_t speaker,const SpeakerGeometry*,ExplicitFp)const;
    const CubicParameterValues& values()const{return values_;}
    const MccSoundEffectDefinitions::Function& source()const{return *source_;}
private:
    CompiledParameterFunction(std::shared_ptr<const MccSoundEffectDefinitions>,
        const MccSoundEffectDefinitions::Function*,CubicParameterValues);
    std::shared_ptr<const MccSoundEffectDefinitions> definitions_;
    const MccSoundEffectDefinitions::Function* source_;
    CubicParameterValues values_;
};
}
