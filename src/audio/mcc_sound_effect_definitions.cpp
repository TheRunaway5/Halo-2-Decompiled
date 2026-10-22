#include "halo2/mcc_sound_effect_definitions.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <set>

namespace halo2 {
namespace {
std::uint32_t word(const MccSoundEffectDefinitions::Extent& e,std::size_t offset) {
    return Bytes(e.bytes).u32(offset);
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
                        block(instance,0x20,0x14),data_reference(instance,0x18),{}};
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
}
