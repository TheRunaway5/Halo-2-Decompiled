#include "halo2/mcc_sound_effect_definitions.hpp"
#include "halo2/content.hpp"
#include "halo2/tags/binary.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <set>

namespace halo2 {
namespace {
using Definitions=MccSoundEffectDefinitions;

std::uint32_t word(const Definitions::Extent& e,std::size_t offset) {
    return Bytes(e.bytes).u32(offset);
}

// Authored global_fx records from retail Cairo Station and Gas Giant. The
// companion corpus and DSP recovery receipts verify the stock collections,
// templates, and resident child identities across the original and MCC maps.
constexpr std::array<std::uint32_t,18> original_centered_filter{
    2,1,0x44960000,0x44960000,0,0,0x40400000,0x40400000,0,0,
    0x43160000,0x43160000,0,0,0xc2800000,0xc2800000,0,0};
constexpr std::array<std::uint32_t,18> original_spatial_filter{
    2,0,0x45fa0000,0x44af0000,0,0,0,0,0,0,0x43160000,0x43160000,0,0,0,0xc2800000,0,0};
constexpr std::array<std::uint32_t,18> mcc_centered_filter{
    2,1,0x44960000,0x44480000,0,0,0x40400000,0x40400000,0,0,
    0x447a0000,0x447a0000,0,0,0x41400000,0x41400000,0,0};
constexpr std::array<std::uint32_t,18> mcc_default_filter{
    2,1,0x447a0000,0x44c80000,0,0,0x41500000,0x41500000,0,0,
    0x447a0000,0x44c80000,0,0,0x41500000,0x41500000,0,0};
constexpr std::array<std::uint32_t,18> mcc_spatial_filter{
    2,0,0x44960000,0x44480000,0,0,0,0,0,0,0x447a0000,0x447a0000,0,0,0,0x41400000,0,0};

constexpr std::array original_constants{
    0x3e800000u,0x42f00000u,0xc1f00000u,0x3f000000u,0x42a00000u,0xc1f00000u,0x3f000000u};
constexpr std::array low_constants{
    0x3e4ccccdu,0x42f00000u,0xc1f00000u,0x3f000000u,0x42a00000u,0xc1f00000u,0x3f000000u};
constexpr std::array mcc_release_constants{
    0x3dcccccdu,0x42f00000u,0xc0a00000u,0x3f000000u,0x42a00000u,0xc0a00000u,0x3f000000u};
constexpr std::array<std::uint8_t,7> constant_sources{0,2,4,6,8,10,12};
constexpr Definitions::ClassicRadioMonoOverride original_spatial_mono{
    1,0x71,{0x42a00000,0xc1f00000,0x3f000000},{1,0,2,4},
    {3,0,0x3f800000,{
        0x04040007,0,0x3e800000,0,0,0,0x3f800000,0x3ed9999a,0x3f800000,0x3f65c28f,
        0x3ecfa4fa,0x3f800000,0,0xbffa7323,0x3f74e645,0,0x3f800000,0,0x3f800000,
        0x3eaaaaab,0,0x3f2aaaab,0,0x3f800000,0x3f800000,0x34900000,0x403ffffe,0xc0400000,0x3f800000}}};
constexpr std::array<std::uint32_t,29> mcc_spatial_curve{
    0x04040007,0x3eb74bc7,0,0,0,0,0x3f800000,0x3edc28f6,0x3f800000,0x3f63d70a,
    0x3ecccccd,0x3f800000,0,0xbfd1745a,0x3f22e8b4,0,0x3f800000,0,0x3f800000,
    0x3eaaaaab,0,0x3f2aaaab,0,0x3f800000,0x3f800000,0x34800000,0x403fffff,0xc0400000,0x3f800000};

struct RadioRow {
    unsigned row;
    std::string_view name,suffix;
    bool covenant;
};
constexpr std::array radio_rows{
    RadioRow{0,"radio_default","",false}, RadioRow{2,"radio_3d","",false},
    RadioRow{8,"radio_covy","",true}, RadioRow{10,"radio_covy_3d","",true},
    RadioRow{11,"radio_default_in","_in",false}, RadioRow{12,"radio_default_out","_out",false},
    RadioRow{13,"radio_default_loop","_loop",false}, RadioRow{14,"radio_covy_in","_in",true},
    RadioRow{15,"radio_covy_loop","_loop",true}, RadioRow{16,"radio_covy_out","_out",true},
    RadioRow{17,"radio_default_low","",false}, RadioRow{18,"radio_covy_low","",true}};

bool same_words(const Definitions::Extent& bytes,std::span<const std::uint32_t> expected) {
    if(bytes.bytes.size()!=expected.size_bytes())return false;
    for(std::size_t i=0;i<expected.size();++i)
        if(word(bytes,i*4)!=expected[i])return false;
    return true;
}

const Definitions::ComponentOverride* mono_inputs(const Definitions::Instance& instance) {
    if(instance.data.size || word(instance.raw,0x10) || word(instance.raw,0x14) ||
        instance.override_records.size()!=1)return nullptr;
    const auto& root=instance.override_records.front();
    if(root.component_mask!=1 || !root.low_frequency_inputs.empty() || root.components.size()!=1)return nullptr;
    return &root.components.front();
}

bool same_constants(const Definitions::ComponentOverride& inputs,std::span<const std::uint32_t> constants,
    std::span<const std::uint8_t> sources) {
    return std::ranges::equal(inputs.constants,constants,{},&Definitions::Constant::bits) &&
        std::ranges::equal(inputs.sources,sources,{},&Definitions::SourceByte::bits);
}

bool stock_mono_template(const Definitions::EffectTemplate& effect) {
    if(effect.name!="sound\\dsp_effects\\sound_effect_templates\\mono_distortion" ||
        effect.input_effect_name_word!=null_datum || !effect.additional_inputs.empty() ||
        effect.platforms.size()!=1)return false;
    const auto& platform=effect.platforms.front();
    if(platform.input.text!="sfx_mono_distortion" || platform.templates.size()!=1)return false;
    const auto& parameters=platform.templates.front();
    if(parameters.input.text!="sfx_mono_distortion" || word(parameters.raw,4)!=2 ||
        word(parameters.raw,8)!=1 || word(parameters.raw,12)!=0x00340000)return false;
    constexpr std::array<std::array<std::uint32_t,4>,7> original_parameters{{
        {1,0x3eb74bc7,0,0x3f800000}, {1,0x458ca000,0,0x469c4000},
        {1,0,0xc1f00000,0x41f00000}, {1,0x3f800000,0x3a83126f,0x41f00000},
        {1,0x458ca000,0,0x469c4000}, {1,0,0xc1f00000,0x41f00000},
        {1,0x3f800000,0x3a83126f,0x41f00000}}};
    return std::ranges::equal(parameters.components,original_parameters,
        [](const auto& component,const auto& words){return same_words(component.raw,words);});
}

bool stock_mono_instance(const Definitions::Instance& instance,unsigned row) {
    const auto* inputs=mono_inputs(instance);
    if(!inputs)return false;
    if(row==2) {
        if(inputs->parameter_mask!=0x71 || inputs->functions.size()!=1 ||
            !same_constants(*inputs,original_spatial_mono.constants,original_spatial_mono.sources))return false;
        const auto& f=inputs->functions.front();
        return f.input==3 && f.range_input==0 && f.time_period_bits==0x3f800000 &&
            same_words(f.curve.payload,mcc_spatial_curve);
    }
    const auto& expected=row>=17?low_constants:row==12 || row==13?mcc_release_constants:original_constants;
    return inputs->parameter_mask==0x7f && inputs->functions.empty() && same_constants(*inputs,expected,constant_sources);
}

TagHandle stock_radio_loop(const Cache& cache,const RadioRow& row) {
    const std::string_view base=row.covenant?"sound\\dsp_effects\\covy_radio\\covy_radio":
        "sound\\dsp_effects\\radio_static\\radio_static";
    const std::string path=std::string(base)+std::string(row.suffix);
    const auto* tag=cache.query_tag({.group=tag_groups::sound_looping,.name=path,.require_address=true});
    if(!tag)return {};
    const auto loop=cache.load_looping_sound(tag->datum);
    if(loop.flags || loop.continuous_damage!=null_datum || !loop.details.empty() || loop.tracks.size()!=1 ||
        loop.music_time!=std::array{0.f,0.f} || loop.distance!=std::array{3.f,20.f})return {};
    const auto& track=loop.tracks.front();
    const auto fade_in_bits=row.covenant?0x3f800000u:0x3dcccccdu;
    if(track.flags!=3 || track.gain_db!=(row.covenant?-24.f:-9.f) ||
        std::bit_cast<std::uint32_t>(track.fade_in)!=fade_in_bits ||
        std::bit_cast<std::uint32_t>(track.fade_out)!=0x3dcccccdu || track.output_effect || track.unknown3e ||
        track.alternate!=null_datum || track.alternate_end!=null_datum ||
        track.alternate_transition_in!=null_datum || track.alternate_transition_out!=null_datum ||
        track.alternate_crossfade!=0.f || track.alternate_fade_out!=0.f)return {};
    const std::string_view clips=row.covenant?"sound\\dsp_effects\\covy_radio\\covy_radio\\":
        "sound\\dsp_effects\\radio_static\\radio_loop\\";
    const auto clip=[&](std::uint32_t datum,std::string_view name,bool expected){
        if(!expected)return datum==null_datum;
        const auto* sound=cache.find_tag(TagHandle{datum},tag_groups::sound);
        return sound && sound->address && sound->name==std::string(clips)+std::string(name);
    };
    if(!clip(track.start,"in",row.suffix.empty() || row.suffix=="_in") ||
        !clip(track.loop,"loop",true) || !clip(track.end,"out",row.suffix.empty() || row.suffix=="_out"))return {};
    return tag->handle();
}
}
MccSoundEffectDefinitions::Extent MccSoundEffectDefinitions::extent(std::uint32_t address,std::size_t bytes)const {
    if(!address||!bytes||bytes>std::numeric_limits<std::uint32_t>::max()||
        std::uint64_t(address)+bytes>(std::uint64_t{1}<<32))
        throw FormatError("Invalid MCC sound-effect metadata extent");
    return {address,cache_->read_tag_bytes(address,bytes),cache_};
}
std::vector<MccSoundEffectDefinitions::Extent> MccSoundEffectDefinitions::block(
    const Extent& parent,std::size_t offset,std::size_t stride) {
    const TagBlock descriptor{std::bit_cast<std::int32_t>(word(parent,offset)),word(parent,offset+4)};
    const auto size=descriptor.extent(stride,4096);
    if(!size)return {};
    if(std::size_t(descriptor.count)>remaining_rows_)
        throw FormatError("MCC sound-effect definitions exceed native parser row budget");
    remaining_rows_-=std::size_t(descriptor.count);
    const auto payload=extent(descriptor.address,size);
    std::vector<Extent> rows;rows.reserve(descriptor.count);
    for(std::size_t i=0;i<std::size_t(descriptor.count);++i)
        rows.push_back({descriptor.address+std::uint32_t(i*stride),payload.bytes.subspan(i*stride,stride),payload.cache_owner});
    return rows;
}
MccSoundEffectDefinitions::DataReference MccSoundEffectDefinitions::data_reference(
    const Extent& parent,std::size_t offset)const {
    DataReference result{word(parent,offset),word(parent,offset+4),{}};
    // Zero-size references retain the original pointer but have no borrowed
    // payload. Nonempty references must resolve to actual resident metadata.
    if(result.size) {
        if(result.size>16u*1024u*1024u)throw FormatError("MCC sound-effect data reference exceeds supported metadata domain");
        result.payload=extent(result.source_address,result.size);
    }
    return result;
}
MccSoundEffectDefinitions::Function MccSoundEffectDefinitions::function(const Extent& row)const {
    const Bytes raw(row.bytes);
    return {row,std::bit_cast<std::int16_t>(raw.u16(0)),std::bit_cast<std::int16_t>(raw.u16(2)),
        data_reference(row,4),word(row,0xc)};
}
MccSoundEffectDefinitions::Override MccSoundEffectDefinitions::override_record(const Extent& row) {
    Override result{row,word(row,0x10),{}, {}};
    for(const auto& item:block(row,0,0x1c)) {
        ComponentOverride component{item,word(item,0x18),{}, {}, {}};
        for(const auto& f:block(item,0,0x10))component.functions.push_back(function(f));
        for(const auto& c:block(item,8,4))component.constants.push_back({c,word(c,0)});
        for(const auto& s:block(item,0x10,1))component.sources.push_back({s,std::to_integer<std::uint8_t>(s.bytes[0])});
        result.components.push_back(std::move(component));
    }
    for(const auto& f:block(row,8,0x10))result.low_frequency_inputs.push_back(function(f));
    return result;
}
MccSoundEffectDefinitions::Name MccSoundEffectDefinitions::name(std::uint32_t value)const {
    return {value,value==0xffffffffu?std::optional<std::string>{}:cache_->string_id(value)};
}
MccSoundEffectDefinitions::MccSoundEffectDefinitions(std::shared_ptr<const Cache> cache):cache_(std::move(cache)) {
    if(!cache_||!cache_->is_mcc())throw FormatError("MCC sound-effect definitions require an actual retained MCC cache");
    std::set<std::uint32_t> identities;
    for(const auto& tag:cache_->select_tags({.require_address=true})) {
        if(tag.group!="sfx+"&&tag.group!="<fx>"&&tag.group!="snmx"&&tag.group!="snde")continue;
        if(!identities.insert(tag.datum).second)throw FormatError("Duplicate MCC sound-effect datum");
        const Extent raw{tag.address,cache_->tag_payload(tag.handle()),cache_};
        if(tag.group=="sfx+") {
            Collection collection{tag.handle(),tag.name,raw,{}};
            for(const auto& row:block(raw,0,0x38)) {
                Effect effect{row,name(word(row,0)),word(row,0xc),{},
                    block(row,0x18,0x48),block(row,0x20,0x30),block(row,0x28,0x40),{}};
                for(const auto& mix:block(row,4,8))effect.mix.push_back({mix,word(mix,0),word(mix,4)});
                for(const auto& instance:block(row,0x30,0x28)) {
                    const TagReference reference{TagGroup{word(instance,0)},TagHandle{word(instance,4)}};
                    if(!reference.handle.is_null()&&(reference.group!=TagGroup{"<fx>"}||
                        !cache_->find_tag(reference.handle,TagGroup{"<fx>"})))
                        throw FormatError("MCC sound-effect instance has an invalid salted template reference");
                    Instance result{instance,reference,block(instance,8,0x10),
                        block(instance,0x20,0x14),data_reference(instance,0x18),{}, {}};
                    for(const auto& component:result.components) {
                        const Bytes fields(component.bytes);
                        const TagHandle loop{fields.u32(4)};
                        if(fields.u32(0)!=tag_groups::sound_looping.value || loop.is_null())continue;
                        const auto* target=cache_->find_tag(loop,tag_groups::sound_looping);
                        if(!target || !target->address)
                            throw FormatError("MCC sound-effect component has an invalid salted looping sound reference");
                        result.loop_components.push_back({component,loop,fields.u32(8),fields.u32(0xc)});
                    }
                    for(const auto& row:result.overrides)result.override_records.push_back(override_record(row));
                    effect.instances.push_back(std::move(result));
                }
                collection.effects.push_back(std::move(effect));
            }
            collections_.push_back(std::move(collection));
        }else if(tag.group=="<fx>") {
            EffectTemplate effect{tag.handle(),tag.name,raw,word(raw,8),{}, {}};
            for(const auto& row:block(raw,0xc,0x10))effect.additional_inputs.push_back(
                {row,name(word(row,0)),data_reference(row,4),word(row,0xc)});
            for(const auto& row:block(raw,0x14,0xc)) {
                Platform platform{row,name(word(row,8)),{}};
                for(const auto& item:block(row,0,0x18)) {
                    Template t{item,name(word(item,0)),{}};
                    for(const auto& c:block(item,0x10,0x10))t.components.push_back(
                        {c,word(c,0),word(c,4),word(c,8),word(c,12)});
                    platform.templates.push_back(std::move(t));
                }
                effect.platforms.push_back(std::move(platform));
            }
            templates_.push_back(std::move(effect));
        }else {
            if(raw.bytes.size()!=(tag.group=="snde"?0x48u:0x58u))
                throw FormatError("MCC sound environment/mix definition has an unsupported layout");
            parameters_.push_back({tag.handle(),tag.group,tag.name,raw});
        }
    }
    for(const auto& collection:collections_)for(const auto& effect:collection.effects)
        for(const auto& instance:effect.instances)if(!instance.effect_template.handle.is_null())
            (void)effect_template(instance.effect_template.handle);
}
const MccSoundEffectDefinitions::EffectTemplate& MccSoundEffectDefinitions::effect_template(TagHandle handle)const {
    const auto found=std::ranges::find(templates_,handle,&EffectTemplate::handle);
    if(found==templates_.end())throw FormatError("MCC sound-effect template is outside retained resident definitions");
    return *found;
}
const MccSoundEffectDefinitions::Collection& MccSoundEffectDefinitions::collection(TagHandle handle)const {
    const auto found=std::ranges::find(collections_,handle,&Collection::handle);
    if(found==collections_.end())throw FormatError("MCC sound-effect collection is outside retained resident definitions");
    return *found;
}

std::optional<MccSoundEffectDefinitions::ClassicRadioEffect> MccSoundEffectDefinitions::classic_radio_effect(
    TagHandle handle,std::uint32_t row)const {
    if(cache_->is_custom_mcc() || cache_->preserves_mcc_authoring())return std::nullopt;
    const auto profile=std::ranges::find(radio_rows,row,&RadioRow::row);
    if(profile==radio_rows.end())return std::nullopt;
    const auto found=std::ranges::find(collections_,handle,&Collection::handle);
    if(found==collections_.end() || found->name!="sound\\global_fx" || found->effects.size()!=19)
        return std::nullopt;
    const auto global_sounds=read_tag_block_descriptor(Bytes(cache_->tag_payload(cache_->globals_tag())),0xc0);
    if(global_sounds.count!=1 || !global_sounds.address ||
        Bytes(cache_->read_tag_bytes(global_sounds.address,0x18)).u32(0x14)!=handle.value)return std::nullopt;

    const auto& authored=found->effects[row];
    const bool spatial=row==2 || row==10;
    const bool missing_instance=row==0 || row==8 || row==10;
    const auto& mcc_filter=spatial?mcc_spatial_filter:row==0?mcc_default_filter:mcc_centered_filter;
    if(authored.name.text!=profile->name || authored.flags!=unsigned(spatial) ||
        !authored.mix.empty() || !authored.pitch_lfos.empty() || !authored.filter_lfos.empty() ||
        word(authored.raw,0x10) || word(authored.raw,0x14) || authored.filters.size()!=1 ||
        !same_words(authored.filters.front(),mcc_filter) || authored.instances.size()!=unsigned(!missing_instance))
        return std::nullopt;

    const auto& prototypes=found->effects[11];
    if(prototypes.name.text!="radio_default_in" || prototypes.instances.size()!=1 ||
        !stock_mono_instance(prototypes.instances.front(),11))return std::nullopt;
    const auto& prototype=prototypes.instances.front();
    const auto mono=prototype.effect_template.handle;
    if(mono.is_null() || !stock_mono_template(effect_template(mono)))return std::nullopt;

    const auto child=stock_radio_loop(*cache_,*profile);
    if(child.is_null())return std::nullopt;
    if(!missing_instance) {
        const auto& instance=authored.instances.front();
        const auto mcc_gain=row==2?0x41400000u:0xc0400000u;
        if(instance.effect_template.handle!=mono || instance.components.size()!=1 ||
            instance.loop_components.size()!=1 || !stock_mono_instance(instance,row))return std::nullopt;
        const auto& component=instance.loop_components.front();
        if(component.loop!=child || component.gain_bits!=mcc_gain || component.flags)return std::nullopt;
    }

    const Instance* original_instance=nullptr;
    if(!spatial)original_instance=row>=17?&authored.instances.front():&prototype;
    return ClassicRadioEffect{authored.flags,spatial?original_spatial_filter:original_centered_filter,
        {child,spatial?0u:0xc0400000u,0},mono,original_instance,spatial?&original_spatial_mono:nullptr};
}
}
