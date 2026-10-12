#include "halo2/source_portal_mask.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace halo2 {
namespace {
void validate(const SourcePortalMaskDefinition &d) {
    if(d.portal_flags.size()>512 || d.portal_indices.size()>128 || d.associations.size()>32767)
        throw FormatError("source portal mask exceeds original bank");
    for(const auto p:d.portal_indices)if(p<0 || std::size_t(p)>=d.portal_flags.size())
        throw FormatError("source portal mask index outside BSP");
    for(const auto &r:d.associations)if(r.count<0 || (r.count && (r.first<0 ||
        std::size_t(r.first)+std::size_t(r.count)>d.portal_indices.size())))
        throw FormatError("source portal association range outside authored list");
}
void validate_owners(std::span<const SourcePortalOwnerState> owners) {
    std::vector<unsigned> slots;
    for(const auto &o:owners) {
        if(o.handle==0xffffffffu || !(o.handle>>16) || (o.handle&65535)>=32768)
            throw FormatError("source portal owner lacks valid full generation/slot");
        slots.push_back(o.handle&65535);
    }
    std::sort(slots.begin(),slots.end());
    if(std::adjacent_find(slots.begin(),slots.end())!=slots.end())
        throw FormatError("source portal owner duplicates a live pool slot");
}
void validate_range(const SourcePortalMaskDefinition &d,const SourcePortalOwnerState &o) {
    if(o.count<0 || (o.count && (o.first<0 || std::size_t(o.first)+std::size_t(o.count)>d.portal_indices.size())))
        throw FormatError("source portal owner range outside authored list");
}
std::uint16_t acoustic_value(const SourcePortalOwnerState &o) {
    const float difference=o.definition.open_acoustic-o.definition.closed_acoustic;
    const float scaled=difference*o.current_position;
    const float value=scaled+o.definition.closed_acoustic;
    if(!std::isfinite(value))throw FormatError("nonfinite source portal acoustic input");
    const float product=std::clamp(value,0.f,1.f)*65535.f;
    // Original spills the SSE float product before x87 FISTP with CW027F.
    // Its nearest-even integer result is independent of the host FP mode.
    const auto lower=std::uint32_t(std::floor(double(product)));
    const double fraction=double(product)-double(lower);
    return std::uint16_t(lower+(fraction>.5 || (fraction==.5 && (lower&1))));
}
bool matches(const SourcePortalOwnerIdentity &a,const SourcePortalOwnerIdentity &b) {
    return a.unique_id==b.unique_id && a.type==b.type && a.source==b.source &&
        (a.source!=0 || a.origin_bsp==b.origin_bsp);
}
}
std::uint32_t source_machine_initial_device_flags(std::uint32_t flags) {
    return 0x80u|((flags&8u)>>3)|((flags&16u)>>3);
}
void SourcePortalMaskState::reset_owners(std::span<SourcePortalOwnerState> owners) {
    validate_owners(owners);
    for(auto &o:owners)if(o.header_type==7){o.first=-1;o.count=0;}
}
void SourcePortalMaskState::bind(const SourcePortalMaskDefinition &d,std::span<SourcePortalOwnerState> owners) {
    validate(d);reset_owners(owners);
    std::vector<unsigned> order(owners.size());std::iota(order.begin(),order.end(),0);
    std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return (owners[a].handle&65535)<(owners[b].handle&65535);});
    for(const auto &r:d.associations)for(const auto i:order) {
        auto &o=owners[i];if(o.header_type!=7 || !matches(r.owner,o.identity))continue;
        o.first=r.first;o.count=r.count;break;
    }
}
void SourcePortalMaskState::clear_flag8(const SourcePortalMaskDefinition &d) {
    validate(d);for(unsigned i=0;i<d.portal_flags.size();++i)
        if(d.portal_flags[i]&8)words[i/32]&=~(1u<<(i%32));
}
void SourcePortalMaskState::initialize(const SourcePortalMaskDefinition &d,std::span<SourcePortalOwnerState> owners) {
    validate(d);validate_owners(owners);words.fill(0xffffffffu);bind(d,owners);clear_flag8(d);
}
void SourcePortalMaskState::step(const SourcePortalMaskDefinition &d,const SourcePortalOwnerState &o) {
    step(d,o,{o.position_group!=0xffffffffu,o.current_position,o.group_position,o.device_flags});
}
void SourcePortalMaskState::step(const SourcePortalMaskDefinition &d,const SourcePortalOwnerState &owner,const SourcePortalDoorInput &input) {
    auto o=owner;o.current_position=input.current_position;o.group_position=input.group_position;o.device_flags=input.device_flags;
    validate(d);validate_range(d,o);
    if(o.header_type!=7)throw FormatError("source portal step requires actual machine owner");
    if(o.definition.type!=0 || !o.count)return;
    if(!std::isfinite(o.current_position) || !std::isfinite(o.group_position) ||
        !std::isfinite(o.definition.closed_acoustic) || !std::isfinite(o.definition.open_acoustic))
        throw FormatError("nonfinite source portal owner state");
    const bool open=input.group_present &&
        (((o.device_flags&0x100) && o.group_position!=0.f) || o.current_position!=0.f);
    const auto acoustic_word=acoustic_value(o);
    for(unsigned i=unsigned(o.first);i<unsigned(o.first)+unsigned(o.count);++i) {
        const unsigned portal=unsigned(d.portal_indices[i]);const auto bit=1u<<(portal%32);
        if(open)words[portal/32]|=bit;else words[portal/32]&=~bit;
        acoustic[i]=acoustic_word;
    }
}
void SourcePortalMaskState::destroy(const SourcePortalMaskDefinition &d,const SourcePortalOwnerState &o) {
    validate(d);validate_range(d,o);
    if(o.header_type!=7)throw FormatError("source portal destroy requires actual machine owner");
    for(unsigned i=unsigned(o.first);o.count && i<unsigned(o.first)+unsigned(o.count);++i) {
        const unsigned portal=unsigned(d.portal_indices[i]);words[portal/32]|=1u<<(portal%32);acoustic[i]=0;
    }
}
void SourcePortalMaskState::reset_acoustic(){acoustic.fill(0);}
SourcePortalMaskDefinition Cache::load_source_portal_mask_definition(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("source portal mask BSP index outside scenario");
    const auto &bsp=bsps_[index];const Bytes data(storage_);const Bytes m(data.slice(bsp.file_offset,bsp.size));
    
    
    SourcePortalMaskDefinition out;
    const auto [portals,portal]=TagAddressSpace(m, bsp.memory_address).block(16+0x5c, 36, 512);
    for(unsigned i=0;i<portals;++i)out.portal_flags.push_back(m.u32(portal+i*36+0x18));
    const auto header=TagAddressSpace(m, bsp.memory_address).offset(m.u32(16+0x220), 16);
    const auto [records,record]=TagAddressSpace(m, bsp.memory_address).block(header, 12, 32767);const auto [indices,list]=TagAddressSpace(m, bsp.memory_address).block(header+8, 2, 128);
    for(unsigned i=0;i<records;++i){const auto p=record+i*12;
        out.associations.push_back({{m.u32(p),std::int16_t(m.u16(p+4)),std::int8_t(std::to_integer<std::uint8_t>(m.slice(p+6,1)[0])),std::int8_t(std::to_integer<std::uint8_t>(m.slice(p+7,1)[0]))},
            std::int16_t(m.u16(p+8)),std::int16_t(m.u16(p+10))});}
    for(unsigned i=0;i<indices;++i)out.portal_indices.push_back(std::int16_t(m.u16(list+i*2)));
    validate(out);return out;
}
SourceMachinePortalDefinition Cache::load_source_machine_portal_definition(std::uint32_t datum) const {
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::device_machine, .require_address=true});
    if(!tag)throw FormatError("source portal machine definition missing");
    const Bytes data(storage_);const auto p=metadata_offset(tag->address,0x12c);
    if(data.u16(p)!=7)throw FormatError("source portal definition object type mismatch");
    return {data.u16(p+0x11c),data.f32(p+0x124),data.f32(p+0x128)};
}
}
