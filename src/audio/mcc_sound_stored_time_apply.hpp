#pragma once
// Internal transaction shared by the Cache publisher and its independent
// timing-plan reconstruction. No source bytes or runtime memory are written.
#include "halo2/mcc_sound_stored_time_reference.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <set>

namespace halo2::mcc_sound_stored_time_detail {
inline void apply(MccSoundRuntimePlan& native,const MccSoundRuntimeSource& source,
                  const MccSoundStoredTimePlan& stored){
    const auto require=[](bool yes,const char*why){if(!yes)throw FormatError(why);};
    require(source.read&&source.relocate&&source.gestalt_owner,
        "Stored-time publication lacks typed source ownership");
    require(std::ranges::none_of(stored.sounds(),[](const auto&s){
        return s.status==MccSoundStoredTimeStatus::InvalidNativeProjection;
    }),"Stored-time publication rejected its complete native projection");
    struct Pending {
        std::size_t patch,definition;
        TagHandle sound,gestalt;
        std::uint32_t source_root,native_root;
        std::array<std::byte,20> before;
        std::array<std::byte,4> after;
    };
    std::vector<Pending> pending;
    std::set<std::uint32_t> sounds,roots;
    for(const auto&c:stored.corrections()){
        require(!c.sound.is_null()&&c.sound.index()<source.tags.size(),
            "Stored-time publication lost its full-salt sound index");
        // Copy the typed tag before any provider call. The source namespace
        // itself remains caller-owned and stable for this transaction.
        const auto t=source.tags[c.sound.index()];
        require(t.handle()==c.sound&&t.group=="snd!"&&t.address==c.source_root&&
            source.gestalt_owner(c.sound)==c.gestalt&&
            source.relocate(t.address,t.size)==c.native_root&&
            source.relocate(t.address,20)==c.native_root,
            "Stored-time publication lost its exact source/native root owner");
        require(sounds.insert(c.sound.value).second&&roots.insert(c.native_root).second,
            "Stored-time publication repeats a target owner");
        const auto owner=std::ranges::find(stored.owners(),c.gestalt,&MccSoundVanillaOwnerResult::gestalt);
        require(owner!=stored.owners().end()&&owner->status==MccSoundVanillaOwnerStatus::Authenticated,
            "Stored-time publication has no complete authenticated gestalt");
        // No native vector iterator/reference may cross a provider call.
        // A provider failure or allocation still precedes every write.
        const auto raw=source.read(c.source_root,20);
        require(raw.size()==20,"Stored-time publication source header is partial");
        const auto definition=std::ranges::find(native.sounds,c.sound,&MccSoundRuntimeDefinition::sound);
        require(definition!=native.sounds.end()&&definition->gestalt==c.gestalt&&
            definition->source_root==c.source_root&&definition->native_root==c.native_root&&
            std::ranges::count(native.sounds,c.sound,&MccSoundRuntimeDefinition::sound)==1,
            "Stored-time publication native definition owner differs");
        const auto patch=std::ranges::find(native.patches,c.source_root,&MccSoundRuntimePatch::source_address);
        require(patch!=native.patches.end()&&patch->owner==c.sound&&
            patch->before.size()==20&&patch->after.size()==20&&
            std::ranges::count(native.patches,c.source_root,&MccSoundRuntimePatch::source_address)==1&&
            std::ranges::equal(patch->after,c.native_header_before),
            "Stored-time publication complete20-byte native preimage differs");
        require(raw.size()==20&&std::ranges::equal(raw,patch->before)&&
            Bytes(raw).u32(16)==std::bit_cast<std::uint32_t>(c.source_ms)&&
            std::ranges::equal(std::span(patch->after).subspan(16,4),c.before)&&
            Bytes(c.after).u32(0)==std::bit_cast<std::uint32_t>(c.original_ms),
            "Stored-time publication exact source/stored word differs");
        pending.push_back({std::size_t(patch-native.patches.begin()),
            std::size_t(definition-native.sounds.begin()),c.sound,c.gestalt,
            c.source_root,c.native_root,c.native_header_before,c.after});
    }
    // All providers, allocations and guards complete before the first write.
    // Recheck the whole target set without invoking source callbacks again.
    for(const auto&p:pending){
        require(p.patch<native.patches.size()&&p.definition<native.sounds.size(),
            "Stored-time publication lost an owner while staging");
        const auto&patch=native.patches[p.patch];const auto&definition=native.sounds[p.definition];
        const auto&tag=source.tags[p.sound.index()];
        require(patch.owner==p.sound&&patch.source_address==p.source_root&&
            definition.sound==p.sound&&definition.gestalt==p.gestalt&&
            definition.source_root==p.source_root&&definition.native_root==p.native_root&&
            tag.handle()==p.sound&&tag.group=="snd!"&&tag.address==p.source_root&&
            std::ranges::equal(patch.after,p.before),
            "Stored-time publication changed while staging its atomic patch set");
    }
    for(const auto&p:pending)
        std::ranges::copy(p.after,native.patches[p.patch].after.begin()+16);
}
} // namespace halo2::mcc_sound_stored_time_detail
