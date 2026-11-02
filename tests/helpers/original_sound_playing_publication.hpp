#pragma once
#include "halo2/audio.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <nlohmann/json.hpp>

namespace halo2::test {
// Inputs come from the current native World and the runtime's sealed private
// prospective factory. This helper creates no native publication capability.
inline nlohmann::json original_sound_playing_publication(const std::shared_ptr<const Cache>& cache,
    PhysicsLiveWorldTu15& source,const MccSoundVariationProvider::NativeBinding& target,std::string_view phase) {
    const auto need=[](bool v,const char* why){if(!v)throw FormatError(why);};
    need(target.current()&&cache->original_sound_catalog(),"Original Audio requires its genuine sealed factory binding and catalog");
    const auto original=source.runtime_resources().original_sound_owner();need(bool(original),"Original Audio source has no retained bank");
    need(original->bindings().size()==cache->original_sound_catalog()->size(),"Original Audio source omitted catalog bindings");
    const auto provider=source.sound_variation_provider(*cache);nlohmann::json cases=nlohmann::json::array();
    const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
    for(const auto& binding:original->bindings()) {
        std::vector<MultiplayerAudioSubmission> packets;
        MultiplayerAudio audio(cache,0x651d83f1u,manual,[&](const auto& p){packets.push_back(p);});
        audio.enroll_continuation();audio.bind_native_sound_metadata(provider);audio.master_volume(.625f);
        const auto clock=audio.make_script_sound_clock();const auto voice=audio.play_impulse_receipt(binding.sound.value);
        need(voice&&packets.size()==1&&packets.front().frames>1,"Admitted original Audio failed actual queued voice creation");
        const auto first=packets.front();const auto encoded=original->bank()->encoded(binding.sound,first.pitch_range,first.permutation);
        const auto retained=provider.encoded(*cache,binding.sound,first.pitch_range,first.permutation);
        need(retained.data==encoded.data&&retained.codec==encoded.codec&&retained.sample_rate==encoded.sample_rate&&
            retained.channels==encoded.channels&&first.frames==decode_sound(encoded).frames(),"Original Audio source used different resource data");
        (void)audio.render(std::min<std::size_t>(137,first.frames/2));audio.update();
        const auto position=audio.playback_position(*voice);const auto parameters=audio.voice_parameters(*voice);
        const auto rows=clock->rows();const auto checkpoint=audio.checkpoint();
        need(checkpoint.voices()==1&&position.output_frames&&position.source_frames>0,"Original Audio cut omitted playing PCM consumption");
        struct Branch {std::vector<float> pcm;std::vector<MultiplayerAudioSubmission> packets;MultiplayerAudio::VoiceReceipt created;};
        const auto branch=[&]{Branch b;const auto begin=packets.size();b.pcm=audio.render(512);audio.update();
            const auto next=audio.play_impulse_receipt(binding.sound.value,.8f);need(bool(next),"Original Audio suffix lost its actual subsequent cue");b.created=*next;
            const auto tail=audio.render(4096);b.pcm.insert(b.pcm.end(),tail.begin(),tail.end());audio.update();
            b.packets.assign(packets.begin()+std::ptrdiff_t(begin),packets.end());audio.stop();return b;};
        const auto expected=branch();double peak{};for(float p:expected.pcm){need(std::isfinite(p),"Original Audio emitted nonfinite PCM");peak=std::max(peak,std::abs(double(p)));}
        need(peak>0&&!expected.packets.empty(),"Original Audio suffix did not mix nonzero actual PCM");audio.master_volume(.125f);
        const auto physical=audio.mixed_frames();auto prepared=audio.prepare_restore(checkpoint,target);const auto identity=prepared.identity();
        const auto restored=prepared.voice(*voice);need(restored&&prepared.for_audio(audio)&&prepared.matches(identity),"Original factory Audio preparation lost its playing voice");
        {auto held=audio.hold_publication(prepared);need(audio.ready(prepared,held)&&target.current(),"Original factory native/timing/SDL hold is not ready");
            need(audio.begin_commit(prepared,held)&&audio.commit(prepared,held)&&prepared.published(),"Original factory Audio publication failed");
            need(!audio.commit(prepared,held),"Original factory Audio published twice");}
        need(audio.mixed_frames()==physical&&prepared.matches(identity),"Original Audio rewound physical output or lost publication identity");
        need(!audio.voice_active(*voice)&&!audio.voice_active(expected.created)&&audio.voice_active(*restored),"Original Audio revived stale voice receipts");
        const auto after=audio.playback_position(*restored);const auto settings=audio.voice_parameters(*restored);
        need(after.source_frames==position.source_frames&&after.output_frames==position.output_frames&&
            after.first_device_frame==position.first_device_frame&&after.first_presentation_frame==position.first_presentation_frame&&
            !after.first_resumed_device_frame&&after.publication_epoch>position.publication_epoch,"Original Audio changed saved playing position");
        need(settings.relative_rate_bits==parameters.relative_rate_bits&&settings.volume_hundredths_db==parameters.volume_hundredths_db&&
            settings.paused==parameters.paused&&settings.routes.size()==parameters.routes.size(),"Original Audio changed saved voice parameters");
        for(unsigned n=0;n<settings.routes.size();++n)need(settings.routes[n].pcm_channel==parameters.routes[n].pcm_channel&&
            settings.routes[n].output_channel==parameters.routes[n].output_channel&&
            std::bit_cast<std::uint32_t>(settings.routes[n].linear_gain)==std::bit_cast<std::uint32_t>(parameters.routes[n].linear_gain),"Original Audio changed route gain bits");
        need(clock->rows()==rows,"Original Audio changed script clock metadata");const auto replay=branch();
        need(replay.pcm.size()==expected.pcm.size()&&replay.packets.size()==expected.packets.size(),"Original Audio changed replay extent");
        std::uint64_t hash=0xcbf29ce484222325ull;
        for(unsigned n=0;n<replay.pcm.size();++n){const auto bits=std::bit_cast<std::uint32_t>(replay.pcm[n]);
            need(bits==std::bit_cast<std::uint32_t>(expected.pcm[n]),"Original Audio changed exact PCM replay");hash=(hash^bits)*0x100000001b3ull;}
        for(unsigned n=0;n<replay.packets.size();++n){const auto& a=replay.packets[n];const auto& b=expected.packets[n];
            need(a.sound==b.sound&&a.voice==b.voice&&a.pitch_range==b.pitch_range&&a.permutation==b.permutation&&a.frames==b.frames&&
                a.rng_before==b.rng_before&&a.rng_after==b.rng_after&&a.source_pitch_bits==b.source_pitch_bits,"Original Audio changed PCM selection/RNG history");}
        need(replay.created.id()==expected.created.id()&&!replay.created.same(expected.created),"Original Audio reused an outgoing publication record");
        cases.push_back({{"sound",binding.sound.value},{"name",binding.bank->metadata().name},{"initial_pcm_frames",first.frames},
            {"saved_output_frames",position.output_frames},{"branch_stereo_samples",replay.pcm.size()},{"branch_pcm_fnv64",hash},{"branch_peak",peak},
            {"original_bank",true},{"genuine_native_timing_sdl_publication",true},{"saved_playing_voice_restored",true},{"exact_pcm_rng_replay",true},
            {"fresh_voice_receipts",true},{"physical_output_clock_not_rewound",true},{"passed",true}});
    }
    return {{"phase",phase},{"original_context",cache->original_sound_catalog()->original_context()},{"cases",cases},{"passed",true}};
}
}
