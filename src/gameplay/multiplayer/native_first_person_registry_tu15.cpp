#include "halo2/native_first_person_registry_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <set>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    std::array<std::byte,4> bytes;memory.read(at,bytes);return Bytes(bytes).u32(0);
}
std::uint32_t row(const SourcePhysicsContactMemory &memory,std::uint32_t pool,
    std::uint32_t datum,std::uint32_t stride) {
    if(word(memory,pool+0x24u)!=stride)throw FormatError("Native FP registry pool stride differs");
    std::array<std::byte,4> bytes;SourceRawDatumPoolServices services;
    services.read=[&](auto at,std::size_t count)->std::span<const std::byte> {
        if(count>bytes.size())throw FormatError("Native FP registry datum operand exceeds its source width");
        memory.read(at,std::span(bytes).first(count));return std::span(bytes).first(count);
    };
    const auto result=source_raw_datum_lookup(pool,datum,services);
    if(!result)throw FormatError("Native FP registry lost a live full-salt owner");
    return result;
}
std::uint32_t body(const SourcePhysicsContactMemory &memory,std::uint32_t datum) {
    const auto result=word(memory,row(memory,word(memory,0x4ecbd0u),datum,12u)+8u);
    if(!result)throw FormatError("Native FP registry object lost its body owner");
    return result;
}
std::optional<unsigned> local_view(unsigned slot,std::span<const unsigned> local_players) {
    const auto at=std::ranges::find(local_players,slot);
    if(at==local_players.end())return {};
    const auto view=unsigned(at-local_players.begin());
    if(view>=4u)throw FormatError("Native FP registry local view exceeds source capacity");
    return view;
}
std::optional<SourceFirstPersonContextInputTu15> input(const SourcePhysicsContactMemory &memory,
    const NativeFirstPersonRegistryTu15::Binding &binding) {
    if(!binding.enabled)return {};
    const auto player=row(memory,word(memory,0x4f54f4u),binding.player,0x21cu);
    const auto unit=word(memory,player+0x2cu);if(unit==none)return {};
    SourceFirstPersonContextInputTu15 result;auto &identity=result.identity;
    identity.player=binding.player;identity.unit=unit;identity.representation=binding.representation;
    const auto native=body(memory,unit);identity.unit_definition=word(memory,native);
    if(identity.unit_definition!=binding.unit_definition || word(memory,native+0x13cu)!=binding.player ||
        (identity.representation!=2 && identity.representation!=3))
        throw FormatError("Native FP registry player/unit/representation identity differs");
    for(unsigned hand=0;hand<2;++hand) {
        std::array<std::byte,1> value;memory.read(native+0x212u+hand,value);
        const auto slot=std::bit_cast<std::int8_t>(value[0]);
        if(slot < -1 || slot>=4)throw FormatError("Native FP registry inventory slot exceeds source capacity");
        if(slot>=0) {
            identity.weapons[hand]=word(memory,native+0x218u+std::uint32_t(slot)*4u);
            if(identity.weapons[hand]!=none)identity.weapon_definitions[hand]=word(memory,body(memory,identity.weapons[hand]));
        }
    }
    memory.read(0x60070000u+(binding.player&65535u)*0xa0u,result.look);
    memory.read(word(memory,0x4f3cd8u),std::as_writable_bytes(std::span(result.initial_rng)));
    return result;
}
bool same_lifetime(const SourceFirstPersonContextIdentityTu15 &a,const SourceFirstPersonContextIdentityTu15 &b) {
    return a.player==b.player && a.unit==b.unit && a.unit_definition==b.unit_definition && a.representation==b.representation;
}
}
struct NativeFirstPersonRegistryTu15::Impl {
    struct Entry {
        std::uint64_t generation{},action_sequence{};
        std::optional<unsigned> local_view;
        std::unique_ptr<SourceFirstPersonContextTu15> context;
        std::optional<SourceFirstPersonPosePacketTu15> packet;
        std::array<std::optional<SourceFirstPersonTemporalStateTu15>,2> composition;
    };
    std::shared_ptr<const Cache> cache;
    std::shared_ptr<const SourceRuntimeResources> resources;
    std::array<Entry,16> players;
    explicit Impl(std::shared_ptr<const Cache> value):cache(std::move(value)),
        resources(std::make_shared<SourceRuntimeResources>(*cache)){}
    Entry &bind(unsigned slot,const SourceFirstPersonContextInputTu15 &in,
        const SourcePhysicsWorldRuntimeServices &cpu) {
        auto &entry=players.at(slot);
        const bool fresh=!entry.context || !same_lifetime(entry.context->state().identity,in.identity);
        if(fresh) {
            if(entry.generation==UINT64_MAX)throw FormatError("Native FP registry generation exhausted");
            auto next=std::make_unique<SourceFirstPersonContextTu15>(*cache,resources,cpu);
            entry.context=std::move(next);++entry.generation;entry.packet.reset();
        }
        entry.context->bind_cpu_services(cpu);return entry;
    }
    void retire(unsigned slot) {
        auto &entry=players.at(slot);entry.context.reset();entry.packet.reset();entry.local_view.reset();
        entry.composition={};
    }
};
NativeFirstPersonRegistryTu15::NativeFirstPersonRegistryTu15(std::shared_ptr<const Cache> cache) {
    if(!cache)throw FormatError("Native FP registry requires its real immutable Cache owner");
    impl_=std::make_unique<Impl>(std::move(cache));
}
NativeFirstPersonRegistryTu15::~NativeFirstPersonRegistryTu15()=default;
void NativeFirstPersonRegistryTu15::action(const SourcePhysicsContactMemory &memory,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourcePhysicsWorldRuntimeServices &cpu,
    const SourceFirstPersonNativeActionTu15 &action,const Bindings &bindings,
    std::span<const unsigned> local_players) {
    const auto native=body(memory,action.unit),player=word(memory,native+0x13cu);
    if(player==none)return; // Nonplayer native units retain their original action owner.
    const auto slot=player&65535u;
    if(slot>=bindings.size() || !bindings[slot].enabled || bindings[slot].player!=player)return;
    const auto in=input(memory,bindings[slot]);
    const auto view=local_view(slot,local_players);
    auto hand=action.hand;
    const bool weapon_request=action.instruction==0xf0c5au || action.instruction==0xf0c7fu ||
        action.instruction==0xf0cd8u || action.instruction==0xf0d19u;
    if(!view && weapon_request && action.view==none && in) {
        // Original10B060 performs this comparison after169112 identifies a
        // physical view's player unit. A remote player's private projection
        // supplies that same validated unit, while the literal NONE-view
        // request and its original hand remain unchanged.
        if(action.hand!=0u)throw FormatError("Native remote weapon request differs from its original unbound hand");
        hand=std::uint32_t(in->identity.weapons[0]!=action.weapon);
    }
    if(!in || in->identity.unit!=action.unit || hand>=2u || in->identity.weapons[hand]!=action.weapon)
        throw FormatError("Native FP registry action differs from the actual current player/hand");
    auto &entry=view?impl_->players.at(slot):impl_->bind(slot,*in,cpu);
    if(entry.action_sequence==UINT64_MAX)throw FormatError("Native FP registry action sequence exhausted");
    const auto sequence=entry.action_sequence+1u;
    if(view) {
        if(action.view!=*view)throw FormatError("Native FP registry local action differs from its physical view");
        // The real native dispatch immediately following this callback owns
        // this request. Its completed state is copied at the genuine phase.
    } else {
        if(action.view!=none)throw FormatError("Native FP registry remote action unexpectedly owns a physical view");
        entry.context->prepare_identity(memory,caller,*in);
        entry.context->apply_action(memory,caller,{sequence,action.tick,action.unit,action.weapon,
            std::bit_cast<std::int16_t>(std::uint16_t(action.action)),std::uint8_t(hand)});
    }
    entry.action_sequence=sequence;entry.local_view=view;
}
void NativeFirstPersonRegistryTu15::composition(const SourcePhysicsContactMemory &memory,
    unsigned view,unsigned hand,const Bindings &bindings,std::span<const unsigned> local_players) {
    if(view>=local_players.size() || view>=4u || hand>=2u)
        throw FormatError("Native FP composition escaped its actual local hand/view");
    const auto slot=local_players[view];
    if(slot>=bindings.size())throw FormatError("Native FP composition escaped its actual player roster");
    const auto in=input(memory,bindings[slot]);
    if(!in)throw FormatError("Native FP composition lacks its actual live player input");
    auto &entry=impl_->players[slot];
    const bool fresh=!entry.context || !same_lifetime(entry.context->state().identity,in->identity);
    if(fresh && entry.generation==UINT64_MAX)throw FormatError("Native FP registry generation exhausted");
    const auto generation=entry.generation+std::uint64_t(fresh);
    auto captured=SourceFirstPersonContextTu15::capture_local(*impl_->cache,memory,view,
        in->identity,generation,entry.action_sequence);
    for(const auto &previous:entry.composition)if(previous &&
        (previous->source_tick!=captured.source_tick || previous->identity!=captured.identity)) {
        entry.composition={};break;
    }
    for(const auto &previous:entry.composition)if(previous &&
        (previous->generation!=captured.generation ||
            previous->accepted_action_sequence!=captured.accepted_action_sequence))
        throw FormatError("Native FP composition changed lifetime or action metadata within its phase");
    if(entry.composition[hand])throw FormatError("Native FP composition repeated a completed phase hand");
    entry.composition[hand]=std::move(captured);
}
void NativeFirstPersonRegistryTu15::phase(const SourcePhysicsContactMemory &memory,
    const SourcePhysicsWorldRuntimeAbi &caller,const SourcePhysicsWorldRuntimeServices &cpu,
    const Bindings &bindings,std::span<const unsigned> local_players) {
    for(unsigned slot=0;slot<bindings.size();++slot) {
        const auto in=input(memory,bindings[slot]);if(!in){impl_->retire(slot);continue;}
        auto &entry=impl_->bind(slot,*in,cpu);const auto view=local_view(slot,local_players);
        if(view)entry.context->adopt_local(memory,caller,*view,in->identity,entry.generation,entry.action_sequence);
        else entry.context->advance_tick(memory,caller,*in);
        auto completed=entry.context->state();completed.generation=entry.generation;
        completed.accepted_action_sequence=entry.action_sequence;
        auto temporal=view?SourceFirstPersonContextTu15::composition_inputs(completed,entry.composition):
            entry.context->pose_inputs();
        temporal.generation=entry.generation;
        temporal.accepted_action_sequence=entry.action_sequence;
        // The packet carries the exact inputs before169C46 composes each
        // hand. The private continuation retains its later feedback clocks.
        // Both belong to this same genuine N -> N+1 source boundary.
        auto packet=source_capture_first_person_packet_tu15(impl_->resources->identity(),temporal,memory);
        entry.packet=std::move(packet);entry.local_view=view;entry.composition={};
    }
}
std::optional<SourceFirstPersonPosePacketTu15> NativeFirstPersonRegistryTu15::packet(unsigned slot,
    std::uint32_t completed_tick,std::uint32_t player,std::uint32_t unit)const {
    const auto &packet=impl_->players.at(slot).packet;
    if(!packet || packet->temporal.source_tick+1u!=completed_tick ||
        packet->temporal.identity.player!=player || packet->temporal.identity.unit!=unit)return {};
    return packet;
}
void NativeFirstPersonRegistryTu15::retire(unsigned slot)noexcept {
    if(slot<impl_->players.size())impl_->retire(slot);
}
void NativeFirstPersonRegistryTu15::install_local_views(PhysicsLiveWorldTu15 &world,
    std::span<const unsigned> local_players) {
    if(local_players.size()>4u)throw FormatError("Native FP role binding exceeds four physical views");
    std::set<unsigned> slots;
    for(unsigned view=0;view<local_players.size();++view) {
        const auto slot=local_players[view];
        if(slot>=impl_->players.size() || !slots.insert(slot).second)
            throw FormatError("Native FP role binding repeats or escapes the player roster");
        const auto &entry=impl_->players[slot];
        if(entry.packet) {
            if(!entry.context || entry.packet->temporal.source_tick+1u!=
                word(world.memory(),word(world.memory(),0x51df54u)+8u))
                throw FormatError("Native FP local role binding has no exact completed temporal owner");
            auto continuation=entry.context->state();
            continuation.generation=entry.generation;
            continuation.accepted_action_sequence=entry.action_sequence;
            SourceFirstPersonContextTu15::install_local(*impl_->cache,world,view,
                continuation,entry.packet->temporal);
        } else {
            const auto &memory=world.memory();const auto native_view=word(memory,0x4f6498u)+view*0x20ccu;
            if(word(memory,native_view+4u)!=none)
                throw FormatError("Native FP local role binding lost an active player's completed temporal inputs");
        }
    }
    // Retain both the exact last genuine phase N and its private RNG/cache owner.
    // The next local phase adopts the actual advanced view; rebind itself
    // neither invents N+1 inputs nor advances a private clock.
    for(unsigned slot=0;slot<impl_->players.size();++slot)
        if(impl_->players[slot].context)impl_->players[slot].local_view=local_view(slot,local_players);
}
NativeFirstPersonRegistryCheckpointTu15 NativeFirstPersonRegistryTu15::checkpoint()const {
    NativeFirstPersonRegistryCheckpointTu15 saved;saved.resources=impl_->resources->identity();
    for(unsigned slot=0;slot<saved.players.size();++slot) {
        const auto &entry=impl_->players[slot];auto &out=saved.players[slot];
        out.generation=entry.generation;out.action_sequence=entry.action_sequence;out.local_view=entry.local_view;
        if(entry.context) {
            out.context=entry.context->checkpoint();
            // Registry lifetime metadata survives a fresh private Context at
            // respawn. Native clock/control bytes remain the completed owner.
            out.context->temporal.generation=entry.generation;
            out.context->temporal.accepted_action_sequence=entry.action_sequence;
        }
        out.packet=entry.packet;
    }
    return saved;
}
std::unique_ptr<NativeFirstPersonRegistryTu15> NativeFirstPersonRegistryTu15::prepare_restore(
    const SourcePhysicsContactMemory &memory,const SourcePhysicsWorldRuntimeAbi &caller,
    const NativeFirstPersonRegistryCheckpointTu15 &saved,
    const Bindings &bindings,std::uint32_t completed_tick)const {
    if(saved.resources!=impl_->resources->identity())throw FormatError("Native FP registry continuation Cache identity differs");
    auto result=std::make_unique<NativeFirstPersonRegistryTu15>(impl_->cache);
    std::set<unsigned> views;std::uint64_t total{};
    for(unsigned slot=0;slot<saved.players.size();++slot) {
        const auto &source=saved.players[slot];auto &target=result->impl_->players[slot];
        if(source.local_view && (*source.local_view>=4 || !views.insert(*source.local_view).second))
            throw FormatError("Native FP registry continuation repeats a physical view");
        if(source.context) {
            const auto in=input(memory,bindings[slot]);
            if(!source.generation || !source.packet || !source.context->initialized ||
                source.context->temporal.generation!=source.generation || !source.context->last_advanced_tick ||
                *source.context->last_advanced_tick!=source.context->temporal.source_tick ||
                source.context->temporal.source_tick+1u!=completed_tick ||
                !in || source.context->temporal.identity!=in->identity ||
                source.context->temporal.accepted_action_sequence!=source.action_sequence)
                throw FormatError("Native FP registry continuation changed its full player lifetime");
            for(const auto &range:source.context->private_ranges)total+=range.bytes.size();
            if(total>128u*1024u*1024u)throw FormatError("Native FP registry continuation exceeds sixteen private cache owners");
            target.context=std::make_unique<SourceFirstPersonContextTu15>(*impl_->cache,result->impl_->resources);
            // prepare_restore validates owning bytes and enters no kernel.
            target.context->prepare_restore(memory,caller,*source.context);
        } else if(source.packet || source.local_view)throw FormatError("Inactive native FP registry continuation carries live presentation owners");
        if(source.packet) {
            const auto &p=*source.packet;
            if(!source.context || p.resources!=saved.resources || p.temporal.generation!=source.generation ||
                p.temporal.accepted_action_sequence!=source.action_sequence || p.temporal.source_tick+1u!=completed_tick ||
                p.temporal.identity!=source.context->temporal.identity ||
                p.temporal.clock_controls!=source.context->temporal.clock_controls)
                throw FormatError("Native FP registry packet differs from its exact completed continuation");
            source_validate_first_person_packet_tu15(p,p.temporal.identity);
        }
        target.generation=source.generation;target.action_sequence=source.action_sequence;
        target.local_view=source.local_view;target.packet=source.packet;
    }
    return result;
}
}
