#pragma once
#include "halo2/multiplayer_audio.hpp"
#include "halo2/source_loop_track_update_tu15.hpp"
#include "halo2/source_loop_combined_mix_tu15.hpp"
#include <map>
namespace halo2 {
// Real native output operations. No operation returns an Xbox voice datum,
// source availability bit or source admission result.
class SourceLoopPhasedOutputTu15 {
public:
    virtual ~SourceLoopPhasedOutputTu15()=default;
    virtual const SoundVariationDefinition& metadata(std::uint32_t,unsigned)=0;
    virtual bool single_chunk(std::uint32_t,unsigned,unsigned)=0;
    // Multi-chunk admission needs a real retained decoder map and output
    // queue. Existing single-packet ports retain their stricter capability.
    virtual bool supports_chunk_queue(std::uint32_t sound,unsigned range,unsigned permutation){
        return single_chunk(sound,range,permutation);
    }
    virtual bool active(std::uint64_t)const=0;
    virtual std::uint64_t play(const MultiplayerLoopPacketTu15&)=0;
    virtual void parameters(std::uint64_t,const MultiplayerLoopPacketParametersTu15&,Vec3)=0;
    virtual bool accepts_packet(std::uint64_t)const=0;
    virtual bool queue(std::uint64_t,std::uint32_t,unsigned,unsigned)=0;
    virtual void fade(std::uint64_t,float,bool,std::uint16_t)=0;
    virtual void retire(std::uint64_t)=0;
    virtual std::uint32_t fresh_secondary()=0;
    virtual std::uint32_t& common_secondary()=0;
    virtual unsigned common_permutation(std::uint32_t sound,unsigned range,std::uint32_t& seed)=0;
    // Preflight precedes group mutation/PCM selection. A scalar/stereo-only
    // backend cannot silently accept a four-bin combined request.
    virtual void require_combined_output()const {
        throw FormatError("Combined loop requires its native four-bin playback owner");
    }
    // Inputs from the real native12A810 listener/12D6F0 point/145200 transform
    // queries. NONE means no selected listener. No voice/admission is returned.
    virtual std::optional<SourceLoopCombinedListenerInputTu15> combined_listener(
        std::span<const std::byte,68>,std::uint32_t) {
        throw FormatError("Combined loop requires its actual listener-local inputs");
    }
};
struct SourceLoopPhasedVoiceTu15 {
    std::uint64_t id{},next{},previous{};
    std::uint32_t sound=0xffffffffu;
    unsigned range{},mode{};
    float base_pitch{};
    std::uint32_t seed{},used{};
};
struct SourceLoopPhasedTrackTu15 {
    SourceLoopTrackStateTu15 phase;
    float control{};
    std::uint32_t seed{},used{};
    std::vector<SourceLoopPhasedVoiceTu15> voices;
};
//222690 insertion and2217C0 unlink/primary replacement, over already-owned
//native identities. Allocation/admission and packet retirement stay outside.
void source_loop_phased_insert_voice_tu15(SourceLoopPhasedTrackTu15&,SourceLoopPhasedVoiceTu15);
void source_loop_phased_retire_voice_tu15(SourceLoopPhasedTrackTu15&,std::uint64_t);
struct SourceLoopPhasedOwnerTu15 {
    LoopingSound definition;
    std::uint32_t emitter=0xffffffffu,synchronization=0xffffffffu;
    std::array<std::byte,68> location{};
    std::vector<SourceLoopPhasedTrackTu15> tracks;
    std::array<std::uint32_t,4> combined_amplitudes{};
    std::uint16_t source_flags{};
    std::uint8_t last_frame{};
    bool combined{};
    bool seen{},retain{},stopping{};
};
struct SourceLoopPhasedSynchronizationTu15 {std::uint32_t seed{},references{};};
struct SourceLoopPhasedStateTu15 {
    std::map<std::pair<std::uint32_t,std::uint32_t>,SourceLoopPhasedOwnerTu15> owners;
    std::map<std::uint32_t,SourceLoopPhasedSynchronizationTu15> synchronization;
    std::uint8_t frame{};
};
//220CCD..220CF3: nonzero location mode + root bit7 shares definition-wide
// ownership. The emitter datum is never used as this combined group's key.
std::pair<std::uint32_t,std::uint32_t> source_loop_phased_key_tu15(
    const SourceLoopSoundRequestTu15&,const LoopingSound&);
// Recovered spatial root0 and synchronized root18 profiles: actual native
// voices, supported phase chunk queues, authored entry/exit/alternate transitions
// and pitch layers. Unsynchronized selection retains the common RNG/range bank.
// Other profiles remain explicit requirements, not guessed fallbacks.
void source_loop_phased_request_tu15(SourceLoopPhasedStateTu15&,SourceLoopPhasedOutputTu15&,
    const SourceLoopSoundRequestTu15&,const LoopingSound&);
void source_loop_phased_frame_tu15(SourceLoopPhasedStateTu15&,SourceLoopPhasedOutputTu15&,float elapsed);
}
