#include "halo2/native_weapon_zoom_audio_tu15.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_audio.hpp"
#include <bit>
#include <cmath>
#include <exception>
#include <utility>
namespace halo2 {
void validate_native_weapon_zoom_audio_tu15(std::span<const NativeWeaponZoomAudioEventTu15> events,std::uint32_t tick) {
    if(events.size()>64)throw FormatError("Native zoom audio journal exceeds retention");
    std::uint64_t previous{};
    for(const auto &e:events) {
        const auto control=std::bit_cast<float>(e.control_bits);
        if(!e.serial || e.serial<=previous || e.tick>tick || e.player==null_datum ||
            !(e.player>>16u) || (e.player&65535u)>=multiplayer_capacity ||
            e.unit==null_datum || !(e.unit>>16u) || (e.unit&65535u)>=2048 ||
            !std::isfinite(control) || control<0 || (!e.forced_reset && e.sound==null_datum))
            throw FormatError("Invalid native zoom audio event");
        if(!e.forced_reset) {
            const Bytes raw(e.request);const auto flags=raw.u32(0),location=std::to_integer<unsigned>(e.request[15])&15u;
            if((flags&(0x10u|0x400u)) || location>1 || raw.u32(0x10)!=e.control_bits ||
                !std::isfinite(raw.f32(0x14)) || ((flags&8u) && raw.u32(0x50)!=e.unit))
                throw FormatError("Native zoom audio lost its source one-shot request");
        }
        previous=e.serial;
    }
}
void collect_native_weapon_zoom_audio_tu15(const MultiplayerState &state,std::span<const unsigned> locals,
    NativeWeaponZoomAudioCursorTu15 &cursor,std::vector<NativeWeaponZoomAudioPendingTu15> &pending,bool enabled) {
    validate_native_weapon_zoom_audio_tu15(state.weapon_zoom_audio,state.tick);
    if(!cursor.baselined) {
        if(!state.weapon_zoom_audio.empty())cursor.serial=state.weapon_zoom_audio.back().serial;
        cursor.baselined=true;return;
    }
    for(const auto &event:state.weapon_zoom_audio)if(event.serial>cursor.serial) {
        // Retention loss is observable; no missing request is reconstructed
        // from zoom state, FOV, a shot counter or an animation activity.
        if(event.serial>cursor.serial+1)cursor.missing_prefix+=event.serial-cursor.serial-1;
        cursor.serial=event.serial;if(!enabled)continue;
        for(unsigned view=0;view<locals.size();++view) {
            const auto id=locals[view];if(id>=state.players.size())throw FormatError("Native zoom audio has an invalid local slot");
            const auto &player=state.players[id];
            if(player.connected && player.source_player==event.player) {
                if(pending.size()>=2048)throw FormatError("Native zoom audio pending boundary exceeds its budget");
                pending.push_back({event,view});break;
            }
        }
    }
}
std::size_t deliver_native_weapon_zoom_audio_tu15(MultiplayerAudio &audio,std::vector<NativeWeaponZoomAudioPendingTu15> &pending) {
    auto events=std::exchange(pending,{});std::size_t delivered{};std::exception_ptr error;
    for(const auto &p:events)try {
        const auto &e=p.event;if(e.sound==null_datum)continue;
        if(e.forced_reset) {
            // CABC0 ->18C4B0: unowned, control1, no world location or gain
            // offset. Native impulse selection derives the same class/direct
            // route from the recipient's authenticated sound publication.
            if(audio.play_impulse(e.sound,std::bit_cast<float>(e.control_bits),MultiplayerAudioKind::FirstPerson,null_datum,p.view))++delivered;
        }else {
            const auto kind=(std::to_integer<unsigned>(e.request[15])&15u)==1u?MultiplayerAudioKind::Impulse:MultiplayerAudioKind::FirstPerson;
            if(audio.source_impulse(e.sound,e.request,kind,p.view))++delivered;
        }
    }catch(...){if(!error)error=std::current_exception();}
    if(error)std::rethrow_exception(error);return delivered;
}
}
