#include "halo2/source_combat_attachments.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/source_effect_world_locations.hpp"
#include <map>
#include <algorithm>

namespace halo2 {
struct SourceCombatAttachments::Impl {
    struct Attachment {std::uint32_t effect{},marker{};};
    const Cache &cache;
    NativeWorldEffects &effects;
    std::map<std::uint32_t,std::map<unsigned,Attachment>> definitions;
    std::map<std::pair<std::uint32_t,unsigned>,std::uint32_t> instances;
    std::set<std::string> gaps;
    std::optional<std::uint64_t> tick;
    bool complete=true;
    Impl(const Cache &c,NativeWorldEffects &e,std::span<const std::uint32_t> objects):cache(c),effects(e) {
        for(const auto datum:objects) {
            if(definitions.contains(datum))continue;
            auto &attachments=definitions[datum];
            const auto tag=c.load_source_tag_metadata(datum);
            const Bytes root(tag.bytes_at(tag.source_address(),0x9c));
            if(tag.group()!="weap" && tag.group()!="proj")
                throw FormatError("Combat effect attachment requires its authored weapon/projectile definition");
            const auto descriptor=read_tag_block_descriptor(root,0x94);
            const auto extent=descriptor.extent(0x18,4095);
            if(!descriptor.count)continue;
            const auto owner=c.query_tag({.contains=TagAddressRange{descriptor.address,extent},.require_address=true});
            if(!owner)throw FormatError("Combat attachment bank has no retained metadata owner");
            (void)c.tag_payload(owner->handle());
            const Bytes rows(c.read_tag_bytes(descriptor.address,extent));
            for(unsigned n=0;n<unsigned(descriptor.count);++n) {
                const auto at=n*0x18;
                if(rows.u32(at)!=0x65666665u || rows.u32(at+4)==0xffffffffu)continue;
                try {
                    effects.preload_attached(rows.u32(at+4));
                    attachments.emplace(n,Attachment{rows.u32(at+4),rows.u32(at+8)});
                }catch(const FormatError &error) {
                    const auto effect=c.query_tag({.handle=TagHandle{rows.u32(at+4)}});
                    gaps.insert((effect?effect->name:std::to_string(rows.u32(at+4)))+": "+error.what());
                }
            }
        }
    }
};
SourceCombatAttachments::SourceCombatAttachments(const Cache &c,NativeWorldEffects &e,
    std::span<const std::uint32_t> objects):impl_(std::make_unique<Impl>(c,e,objects)){}
SourceCombatAttachments::~SourceCombatAttachments()=default;
struct SourceCombatAttachments::Checkpoint::Data {
    std::shared_ptr<const int> owner;
    Impl state;
    Data(std::shared_ptr<const int> token,const Impl &from):owner(std::move(token)),state(from){}
};
std::optional<std::uint64_t> SourceCombatAttachments::Checkpoint::tick()const noexcept {
    return data_?data_->state.tick:std::nullopt;
}
std::size_t SourceCombatAttachments::Checkpoint::active_instances()const noexcept {
    if(!data_)return 0;
    return std::count_if(data_->state.instances.begin(),data_->state.instances.end(),[](const auto &row){return row.second!=0xffffffffu;});
}
SourceCombatAttachments::PreparedRestore::PreparedRestore()=default;
SourceCombatAttachments::PreparedRestore::~PreparedRestore()=default;
SourceCombatAttachments::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
SourceCombatAttachments::PreparedRestore &SourceCombatAttachments::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
SourceCombatAttachments::Checkpoint SourceCombatAttachments::checkpoint()const {
    if(!impl_->complete)throw FormatError("Combat attachment checkpoint requires a completed synchronization");
    Checkpoint result;result.data_=std::make_shared<Checkpoint::Data>(owner_,*impl_);return result;
}
SourceCombatAttachments::PreparedRestore SourceCombatAttachments::prepare_restore(const Checkpoint &cut)const {
    if(!cut.data_ || cut.data_->owner!=owner_)throw FormatError("Combat attachment checkpoint belongs to another owner");
    PreparedRestore result;result.replacement_=std::make_unique<Impl>(cut.data_->state);result.owner_=owner_;return result;
}
bool SourceCombatAttachments::restore_ready(const PreparedRestore &prepared)const noexcept {
    return prepared.replacement_ && prepared.owner_==owner_;
}
bool SourceCombatAttachments::commit_restore(PreparedRestore &&prepared)noexcept {
    if(!restore_ready(prepared))return false;
    impl_.swap(prepared.replacement_);prepared.owner_.reset();return true;
}
std::set<std::uint32_t> SourceCombatAttachments::synchronize(MultiplayerObjectRuntime &world,std::uint64_t tick) {
    auto &i=*impl_;
    if(!world.owns_physics())throw FormatError("Combat attachments require the actual source effect owner");
    i.complete=false;
    std::set<std::uint32_t> present;
    std::set<std::pair<std::uint32_t,unsigned>> slots;
    for(const auto &object:world.live_objects()) {
        if(object.type!=2 && object.type!=5)continue;
        const auto definition=i.definitions.find(object.definition);
        if(definition==i.definitions.end() || definition->second.empty())continue;
        const auto attachments=world.attached_effects(object.object);
        for(const auto &source:attachments) {
            const auto authored=definition->second.find(source.slot);
            if(authored==definition->second.end())continue;
            const auto key=std::pair{object.object,unsigned(source.slot)};
            slots.insert(key);present.insert(object.object);
            auto [prior,inserted]=i.instances.try_emplace(key,0xffffffffu);
            if(source.instance!=0xffffffffu && source.definition!=authored->second.effect)
                throw FormatError("Combat attachment source instance differs from its authored slot");
            NativeWorldEffects::ObjectMarkers locations;
            if(source.instance!=0xffffffffu)for(const auto &location:source_effect_world_locations_tu15(
                world.memory(),source.instance,source.definition)) {
                const auto &frame=location.frame;
                locations[i.cache.string_id(location.name)].push_back({frame.position,frame.basis[2],
                    frame.basis[0],frame.basis[0],frame.basis[2],false,source.scale_a,source.scale_b});
            }
            if(prior->second!=source.instance) {
                if(prior->second!=0xffffffffu)
                    i.effects.update_looping_attached(object.object,prior->second,SourceAttachedEffect{});
                prior->second=source.instance;
                if(source.instance!=0xffffffffu)
                    i.effects.spawn_looping_attached(source.definition,object.object,tick,
                        authored->second.marker?i.cache.string_id(authored->second.marker):std::string{},
                        source.scale_a,source.scale_b,&source,&locations);
            }else if(source.instance!=0xffffffffu &&
                !i.effects.update_looping_attached(object.object,source.instance,source,locations) && !(source.flags&0x40u))
                throw FormatError("Live source combat attachment lost its presentation lifetime");
        }
    }
    for(auto at=i.instances.begin();at!=i.instances.end();) {
        if(!slots.contains(at->first))at=i.instances.erase(at);else ++at;
    }
    i.tick=tick;i.complete=true;return present;
}
std::vector<std::string> SourceCombatAttachments::unavailable()const {
    return {impl_->gaps.begin(),impl_->gaps.end()};
}
}
