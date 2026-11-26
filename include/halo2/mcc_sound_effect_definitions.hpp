#pragma once
#include "halo2/cache.hpp"
#include <array>

namespace halo2 {
// Imported parameters are definitions, not an Xbox DSP program or a working
// processing graph. Every nonempty extent below retains the exact resolving
// Cache, including shared child data outside a tag directory primary extent.
// Copies of an extent retain its backing after this bank is released. Raw
// words and curve bytes stay intact;
// no TU1.5 conversion, algorithm selection or readiness is implied here.
class MccSoundEffectDefinitions final {
public:
    struct Extent {
        std::uint32_t source_address{};
        std::span<const std::byte> bytes;
        std::shared_ptr<const Cache> cache_owner;
    };
    struct Name {std::uint32_t string_id{};std::optional<std::string> text;};
    struct DataReference {std::uint32_t size{},source_address{};Extent payload;};
    struct Component {
        Extent raw;
        std::uint32_t value_type{},default_bits{},minimum_bits{},maximum_bits{};
    };
    struct Template {
        Extent raw;
        Name input;
        std::vector<Component> components;
    };
    struct Platform {
        Extent raw;
        Name input;
        std::vector<Template> templates;
    };
    struct AdditionalInput {Extent raw;Name effect;DataReference function;std::uint32_t time_period_bits{};};
    struct EffectTemplate {
        TagHandle handle;
        std::string name;
        Extent raw;
        std::uint32_t input_effect_name_word{};
        std::vector<AdditionalInput> additional_inputs;
        std::vector<Platform> platforms;
    };
    struct Mix {Extent raw;std::uint32_t mixbin{},gain_bits{};};
    // These are literal nested override records. Input selectors, source
    // bytes, masks and float words are retained without evaluation or an
    // assumption that a particular parameter function is supported natively.
    struct Function {
        Extent raw;
        std::int16_t input{},range_input{};
        DataReference curve;
        std::uint32_t time_period_bits{};
    };
    struct Constant {Extent raw;std::uint32_t bits{};};
    struct SourceByte {Extent raw;std::uint8_t bits{};};
    struct ComponentOverride {
        Extent raw;
        std::uint32_t parameter_mask{};
        std::vector<Function> functions;
        std::vector<Constant> constants;
        std::vector<SourceByte> sources;
    };
    struct Override {
        Extent raw;
        std::uint32_t component_mask{};
        std::vector<ComponentOverride> components;
        std::vector<Function> low_frequency_inputs;
    };
    // The instance's authored looping-sound components, in source order.
    // Gain and flags retain the exact words read by TU1.5 224040.
    struct LoopComponent {
        Extent raw;
        TagHandle loop;
        std::uint32_t gain_bits{},flags{};
    };
    struct Instance {
        Extent raw;
        TagReference effect_template;
        std::vector<Extent> components,overrides;
        DataReference data;
        std::vector<Override> override_records;
        std::vector<LoopComponent> loop_components;
    };
    struct Effect {
        Extent raw;
        Name name;
        std::uint32_t flags{};
        std::vector<Mix> mix;
        std::vector<Extent> filters,pitch_lfos,filter_lfos;
        std::vector<Instance> instances;
    };
    struct Collection {TagHandle handle;std::string name;Extent raw;std::vector<Effect> effects;};
    struct Parameters {TagHandle handle;std::string group,name;Extent raw;};

    struct ClassicRadioCompanion {TagHandle loop;std::uint32_t gain_bits{},flags{};};
    struct ClassicRadioFunction {
        std::int16_t input{},range_input{};
        std::uint32_t time_period_bits{};
        std::array<std::uint32_t,29> curve_words;
    };
    struct ClassicRadioMonoOverride {
        std::uint32_t component_mask{},parameter_mask{};
        std::array<std::uint32_t,3> constants;
        std::array<std::uint8_t,4> sources;
        ClassicRadioFunction function;
    };
    struct ClassicRadioEffect {
        std::uint32_t flags{};
        std::array<std::uint32_t,18> filter;
        ClassicRadioCompanion companion;
        TagHandle mono_template;
        // A static profile selects an exact compatible instance owned by this
        // bank. A dynamic profile requires the original function's inputs.
        const Instance* mono_instance{};
        const ClassicRadioMonoOverride* mono_override{};
    };

    explicit MccSoundEffectDefinitions(std::shared_ptr<const Cache>);
    std::span<const Collection> collections() const {return collections_;}
    std::span<const EffectTemplate> templates() const {return templates_;}
    std::span<const Parameters> parameters() const {return parameters_;}
    const EffectTemplate& effect_template(TagHandle) const;
    const Collection& collection(TagHandle) const;
    // Original TU1.5 radio definitions for authenticated resident stock rows.
    // The returned Instance borrows this bank. Raw views retain MCC data.
    std::optional<ClassicRadioEffect> classic_radio_effect(TagHandle,std::uint32_t row) const;
    const std::filesystem::path& cache_path() const {return cache_->path();}
private:
    std::shared_ptr<const Cache> cache_;
    std::vector<Collection> collections_;
    std::vector<EffectTemplate> templates_;
    std::vector<Parameters> parameters_;
    // Native parser bound, including repeated/shared rows. This is not an
    // authored tag or DSP capacity; larger definition sets are unsupported.
    std::size_t remaining_rows_=65536;
    Extent extent(std::uint32_t,std::size_t) const;
    std::vector<Extent> block(const Extent&,std::size_t,std::size_t);
    DataReference data_reference(const Extent&,std::size_t) const;
    Function function(const Extent&) const;
    Override override_record(const Extent&);
    Name name(std::uint32_t) const;
};
}
