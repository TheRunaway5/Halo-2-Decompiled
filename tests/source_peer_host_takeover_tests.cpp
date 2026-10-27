#include "halo2/source_authority_receiver.hpp"
#include "halo2/source_authority_publisher.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/source_host_continuation_codec.hpp"
#include "halo2/source_authority_binding.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/source_execution_lens.hpp"
#include "halo2/effect_presentation_history.hpp"
#include "halo2/effect_sound.hpp"
#include "halo2/source_foreground_world_handoff.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/cache.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/native_lag_compensation.hpp"
#include "helpers/source_peer_held_foreground.hpp"
#ifdef HALO2_TAKEOVER_HELD_LENS_TEST
#include "helpers/source_peer_held_lens.hpp"
#endif
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <deque>
#include <exception>
#include <set>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
std::string stage="setup";
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void wait(F ready,const char *why){const auto until=std::chrono::steady_clock::now()+90s;
    while(!ready()){check(std::chrono::steady_clock::now()<until,why);std::this_thread::sleep_for(5ms);}}
template<class F>void rejects(F run){bool rejected{};try{run();}catch(const std::exception &){rejected=true;}
    check(rejected,"Invalid native takeover was accepted");}
template<class F>void rejects_native_publication(F run,std::string_view reason){
    bool rejected{};try{run();}catch(const FormatError &error){rejected=std::string_view(error.what()).find(reason)!=std::string_view::npos;}
    check(rejected,"Native publication did not refuse at its actual exclusion boundary");
}
bool same_cache_owner(const std::shared_ptr<const Cache> &a,const std::shared_ptr<const Cache> &b){
    return a&&b&&a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
}
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
SourcePlayerConfiguration profile(std::string_view name){const auto defaults=source_default_player_configuration();
    std::array<std::uint8_t,9> appearance;std::copy_n(defaults.begin()+0x40,appearance.size(),appearance.begin());
    return native_selected_player_configuration(name,appearance,-1);}
// Preserve real authenticated input datagrams until all three supplied pulses
// are present, then deliver them in one ordinary host receive/tick. Nothing is
// reconstructed or changed in the port's native control payload.
class HeldInputDatagrams final:public MultiplayerDatagrams {
    struct Packet{std::vector<std::byte> data;unsigned peer;};
    std::shared_ptr<OnlineMatchSession> session_;std::deque<Packet> held_;
public:
    explicit HeldInputDatagrams(std::shared_ptr<OnlineMatchSession> s):session_(std::move(s)){}
    void send(std::span<const std::byte> p,unsigned to)override{session_->send(p,to);}
    bool receive(std::vector<std::byte>&p,unsigned &from)override{
        if(held_.empty())return session_->receive(p,from);
        p=std::move(held_.front().data);from=held_.front().peer;held_.pop_front();return true;
    }
    unsigned retain_switches(unsigned from){
        std::vector<std::byte> p;unsigned peer;
        while(session_->receive(p,peer)){check(held_.size()<128,"Unbounded fixture input holding");held_.push_back({std::move(p),peer});}
        unsigned count{};
        for(const auto &packet:held_)if(packet.peer==from&&packet.data.size()>=46&&packet.data[6]==std::byte{2}&&
            (Bytes(packet.data).u16(44)&1024u))++count; // Native input: header7+nonce8+cookie8+sequence4+count1, then four floats/flags.
        return count;
    }
};
class NativeBindingScratch {
    std::filesystem::path path_;
public:
    NativeBindingScratch(){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
        check(scratch&&std::filesystem::is_directory(scratch),"Native binding fixture requires owned test scratch");
        auto pattern=(std::filesystem::path(scratch)/"same-cache-native-provider-XXXXXX").string();
        const auto directory=::mkdtemp(pattern.data());check(directory,"Could not create native binding fixture scratch");
        path_=directory;
    }
    ~NativeBindingScratch(){std::error_code ignored;std::filesystem::remove_all(path_,ignored);}
    const std::filesystem::path &path()const{return path_;}
};
void native_audio_publication_controls(SourcePeerHostTakeover &takeover,
    const std::shared_ptr<const Cache> &cache,const MccSoundVariationProvider &source_provider,
    const MccSoundVariationProvider &target_provider,const std::shared_ptr<const Cache> &foreign_cache,
    const std::shared_ptr<const Cache> &wrong_control_block,
    const MccSoundVariationProvider::NativeBinding &released_binding) {
    // Manual delivery still owns real queued SDL streams. The selected-MCC
    // CTest supplies the dummy driver; direct execution must never open hardware.
    const auto driver=std::getenv("SDL_AUDIODRIVER");
    check(driver&&std::string_view(driver)=="dummy","Native Audio scope fixture requires the synthetic SDL dummy driver");
    const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
    const auto sound=cache->query_tag({.handle=TagHandle{0xe268435bu},.group=TagGroup{"snd!"},.require_address=true});
    check(sound&&sound->name.ends_with("mc_warthog_p_exit"),"Actual selected MCC native Audio sound fixture differs");
    {
        auto scope=takeover.hold_native_publication();
        const auto binding=target_provider.native_binding(scope,cache);
        // The invalid alias remains backed by both real Cache owners. It is
        // an Audio negative operand, never a native factory or ownership proof.
        for(const auto &invalid:{foreign_cache,wrong_control_block}){
            MultiplayerAudio wrong(invalid,0x651d83f1u,manual);wrong.enroll_continuation();
            const auto checkpoint=wrong.checkpoint();
            rejects_native_publication([&]{(void)wrong.prepare_restore(checkpoint,binding);},"exact strong Cache owner");
            check(checkpoint.voices()==0&&wrong.voices()==0,"Rejected target Cache ownership changed Audio playback");
        }
    }
    std::vector<MultiplayerAudioSubmission> submitted;
    MultiplayerAudio audio(cache,0x651d83f1u,manual,[&](const auto &packet){submitted.push_back(packet);});
    audio.enroll_continuation();audio.bind_native_sound_metadata(source_provider);
    const auto clock=audio.make_script_sound_clock();
    audio.master_volume(.625f);
    const auto original_voice=audio.play_impulse_receipt(sound->datum);
    check(original_voice&&submitted.size()==1&&submitted.front().frames>1,
        "Actual native metadata did not create a queued SDL voice");
    (void)audio.render(std::min<std::size_t>(137,submitted.front().frames/2));audio.update();
    const auto position=audio.playback_position(*original_voice);
    const auto parameters=audio.voice_parameters(*original_voice);
    const auto clock_rows=clock->rows();const auto checkpoint=audio.checkpoint();
    check(checkpoint.voices()==1&&position.output_frames&&position.source_frames>0,
        "Native Audio checkpoint omitted actual playing PCM consumption");
    rejects_native_publication([&]{(void)audio.prepare_restore(checkpoint);},"genuine target metadata binding");
    rejects_native_publication([&]{(void)audio.prepare_restore(checkpoint,MccSoundVariationProvider::NativeBinding{});},"current target binding");
    rejects_native_publication([&]{(void)audio.prepare_restore(checkpoint,released_binding);},"current target binding");
    struct Branch {std::vector<float> pcm;std::vector<MultiplayerAudioSubmission> packets;MultiplayerAudio::VoiceReceipt created;};
    const auto branch=[&]{
        Branch value;const auto first=submitted.size();value.pcm=audio.render(512);audio.update();
        const auto next=audio.play_impulse_receipt(sound->datum,.8f);
        check(bool(next),"Native Audio branch lost its actual subsequent cue");value.created=*next;
        const auto tail=audio.render(4096);value.pcm.insert(value.pcm.end(),tail.begin(),tail.end());audio.update();
        value.packets.assign(submitted.begin()+std::ptrdiff_t(first),submitted.end());audio.stop();return value;
    };
    const auto expected=branch();double audible{};
    for(const auto sample:expected.pcm){check(std::isfinite(sample),"Native Audio branch emitted nonfinite PCM");audible=std::max(audible,std::abs(double(sample)));}
    check(audible>0&&!expected.packets.empty(),"Native Audio reference branch did not actually mix and submit MCC PCM");
    audio.master_volume(.125f);
    MultiplayerAudio::PreparedReplacement expired;
    MultiplayerAudio::PreparationIdentity expired_identity;
    {
        auto scope=takeover.hold_native_publication();
        const auto binding=target_provider.native_binding(scope,cache);
        expired=audio.prepare_restore(checkpoint,binding);expired_identity=expired.identity();
        check(expired.for_audio(audio)&&expired.matches(expired_identity)&&expired.voices()==1,
            "Genuine target metadata could not stage the saved playing Audio owner");
    }
    check(!expired.for_audio(audio)&&!expired.matches(expired_identity),
        "Transferable Audio preparation retained native exclusion eligibility after scope retirement");
    rejects_native_publication([&]{(void)audio.hold_publication(expired);},"stale, busy or foreign");
    // The expired preparation holds no native/timing gate: a new genuine scope
    // is admissible while the weak prepared value still exists.
    {auto retry=takeover.hold_native_publication();check(retry.current(),"Weak Audio preparation retained a released native gate");}
    expired=MultiplayerAudio::PreparedReplacement{};
    MultiplayerAudio::PreparedReplacement prepared;
    MultiplayerAudio::PreparationIdentity identity;
    std::optional<MultiplayerAudio::VoiceReceipt> restored;
    const auto physical=audio.mixed_frames();
    {
        auto scope=takeover.hold_native_publication();
        const auto binding=target_provider.native_binding(scope,cache);
        prepared=audio.prepare_restore(checkpoint,binding);identity=prepared.identity();restored=prepared.voice(*original_voice);
        check(restored&&prepared.matches(identity)&&prepared.for_audio(audio),"Exact genuine target preparation lost its fresh voice mapping");
        {
            // Actual native -> timing -> SDL. Audio owns the timing/SDL holds;
            // no synthetic current flag or reconstructed publication is used.
            auto held=audio.hold_publication(prepared);
            check(audio.ready(prepared,held)&&scope.current(),"Genuine native/timing/SDL Audio target was not ready");
            rejects([&]{target_provider.require_current(*cache);});
            rejects_native_publication([&]{audio.update();},"publication is held");
            check(audio.begin_commit(prepared,held)&&audio.commit(prepared,held)&&prepared.published()&&prepared.matches(identity),
                "Actual native-bound Audio did not publish its prepared playback state");
            check(!audio.commit(prepared,held),"Actual native-bound Audio committed twice");
        }
        target_provider.require_current(*cache);
    }
    check(prepared.published()&&prepared.matches(identity)&&audio.mixed_frames()==physical,
        "Committed Audio depended on a released temporary native scope or rewound physical output");
    check(!audio.voice_active(*original_voice)&&!audio.voice_active(expected.created)&&audio.voice_active(*restored),
        "Native target Audio revived old logical-ID receipts or lost the fresh playing voice");
    const auto actual_position=audio.playback_position(*restored);
    const auto actual_parameters=audio.voice_parameters(*restored);
    check(actual_position.source_frames==position.source_frames&&actual_position.output_frames==position.output_frames&&
        actual_position.first_device_frame==position.first_device_frame&&actual_position.first_presentation_frame==position.first_presentation_frame&&
        !actual_position.first_resumed_device_frame&&actual_position.publication_epoch>position.publication_epoch,
        "Native target Audio lost actual queued consumption history");
    check(actual_parameters.relative_rate_bits==parameters.relative_rate_bits&&actual_parameters.volume_hundredths_db==parameters.volume_hundredths_db&&
        actual_parameters.paused==parameters.paused&&actual_parameters.routes.size()==parameters.routes.size(),
        "Native target Audio changed saved playback parameters");
    for(unsigned i=0;i<parameters.routes.size();++i)check(actual_parameters.routes[i].pcm_channel==parameters.routes[i].pcm_channel&&
        actual_parameters.routes[i].output_channel==parameters.routes[i].output_channel&&
        std::bit_cast<std::uint32_t>(actual_parameters.routes[i].linear_gain)==std::bit_cast<std::uint32_t>(parameters.routes[i].linear_gain),
        "Native target Audio changed exact saved route gains");
    check(clock->rows()==clock_rows,"Native target Audio reset genuine script-clock metadata");
    const auto replay=branch();check(replay.pcm.size()==expected.pcm.size()&&replay.packets.size()==expected.packets.size(),
        "Native target Audio changed replay extent or actual selection count");
    for(unsigned i=0;i<expected.pcm.size();++i)check(std::bit_cast<std::uint32_t>(replay.pcm[i])==std::bit_cast<std::uint32_t>(expected.pcm[i]),
        "Native target Audio changed exact queued PCM replay");
    for(unsigned i=0;i<expected.packets.size();++i){const auto &a=expected.packets[i],&b=replay.packets[i];
        check(a.sound==b.sound&&a.voice==b.voice&&a.permutation==b.permutation&&a.pitch_range==b.pitch_range&&
            a.frames==b.frames&&a.rng_before==b.rng_before&&a.rng_after==b.rng_after&&a.source_pitch_bits==b.source_pitch_bits,
            "Native target metadata rebind reset actual Audio RNG/permutation/cue history");}
    check(expected.created.id()==replay.created.id()&&!expected.created.same(replay.created),
        "Native target Audio reused an outgoing cue's actual publication record");
    std::cout<<"PASS actual native target Audio: strong Cache/control-block/default/stale-scope refusal, weak preparation retirement, genuine native/timing/SDL publication, playing voice/history/clock and exact PCM/RNG replay\n";
}
void native_publication_controls(SourcePeerHostTakeover &takeover,const MultiplayerOptions &options,
    const SourceAuthorityRetainedWorld &retained,const PeerMigrationDecision &decision,
    const std::shared_ptr<const Cache> &resident_cache,bool shared_cache_expected) {
    // These targets come only from the actual independently retained export
    // and learned decision. No test-only capability constructor is available.
    auto disposable=SourcePeerHostTakeover::prepare(options,retained,decision,std::make_shared<Clock>());
    auto &target=takeover.world();auto *const objects=target.source_objects.get();
    auto *const native=&objects->tu15_world();const auto cache=target.cache;
    auto *const other_native=&disposable->world().source_objects->tu15_world();
    const auto foreign_cache=disposable->world().cache;
    check(cache && cache->is_mcc() && resident_cache &&
        same_cache_owner(cache,resident_cache)==shared_cache_expected &&
        foreign_cache && foreign_cache.get()!=cache.get() && !same_cache_owner(cache,foreign_cache),
        "Native scope control lost its actual shared/original or independent MCC Cache owners");
    auto provider=native->sound_variation_provider(*cache);provider.require_current(*cache);
    if(shared_cache_expected)provider.require_current(*resident_cache);
    else rejects([&]{provider.require_current(*resident_cache);});
    rejects([&]{provider.require_current(*foreign_cache);});
    rejects([&]{(void)native->sound_variation_provider(*foreign_cache);});
    auto foreign_provider=other_native->sound_variation_provider(*foreign_cache);
    // A second genuine native World uses the EXACT Cache owner A, but has
    // independently constructed native timing storage. Cache equality alone
    // must never authorize that provider for the actual takeover scope.
    NativeBindingScratch same_cache_files;
    auto same_cache_world=std::make_unique<PhysicsLiveWorldTu15>(*cache,same_cache_files.path(),options.bsp);
    same_cache_world->initialize_map();
    same_cache_world->configure_session(source_game_variant_default(0),1,30,0x12345678u);
    auto same_cache_provider=same_cache_world->sound_variation_provider(*cache);
    same_cache_provider.require_current(*cache);
    auto &bank=objects->memory();const auto source=native->source_world();
    bank.validate_mutable_view(source,4);
    std::array<std::byte,4> original_word;bank.read(source,original_word);
    const auto memory=bank.checkpoint();
    const auto region=std::ranges::find_if(memory.regions,[&](const auto &row){
        return source>=row.address && std::uint64_t(source)-row.address<row.bytes.size();
    });
    check(region!=memory.regions.end(),"Native scope write control lacks its actual allocated region");
    const auto free=bank.find_unowned_range(0x10000u,0x100000000ull,4096,4096);
    check(free.has_value(),"Native scope topology control lacks a genuine unowned range");
    SourceRuntimeMemory empty;const auto empty_before=empty.checkpoint();
    SourceExecutionInputs untracked_tape;
    SourceMatchAuthority authority(target.match,*objects);const auto before=authority.execution_boundary();
    // Deliberately INVALID negative operand: both real owners remain alive.
    // This alias is never passed to a native factory or used as proof. The
    // additive Audio control below uses it only as a live-backed negative.
    const std::shared_ptr<const Cache> wrong_control_block(foreign_cache,cache.get());
    MccSoundVariationProvider::NativeBinding released_binding;
    {
        // The target's Match/Simulation/renderer/raw-span/ownership-slot
        // writers are externally quiescent. Only rejected API probes run.
        auto held=takeover.hold_native_publication();
        check(held.current() && held.matches(*native,cache) && !held.matches(*other_native,cache) &&
            held.matches(*native,resident_cache)==shared_cache_expected &&
            !held.matches(*native,foreign_cache) && !held.matches(*native,wrong_control_block),
            "Native scope accepted a foreign nested World or Cache control block");
        auto binding=provider.native_binding(held,cache);released_binding=binding;
        auto sibling_binding=provider.native_binding(held,cache);
        check(binding.current()&&sibling_binding.current(),"Genuine native provider binding lost its live scope");
        rejects([&]{(void)provider.native_binding(held,foreign_cache);});
        rejects([&]{(void)provider.native_binding(held,wrong_control_block);});
        rejects([&]{(void)foreign_provider.native_binding(held,cache);});
        rejects_native_publication([&]{(void)same_cache_provider.native_binding(held,cache);},"different World owners");
        {
            auto timing=MccSoundVariationProvider::hold_native_binding(binding);
            check(timing.matches(binding)&&!timing.matches(sibling_binding)&&binding.current(),
                "Native timing hold accepted a sibling binding or lost its exact actual owner");
            rejects([&]{(void)MccSoundVariationProvider::hold_native_binding(binding);});
            rejects([&]{provider.require_current(*cache);});
            bool foreign_binding_current=true,foreign_binding_matches=true;unsigned binding_refusals{};
            std::exception_ptr binding_error;
            std::thread observer([&]{try{
                foreign_binding_current=binding.current();foreign_binding_matches=timing.matches(binding);
                try{(void)MccSoundVariationProvider::hold_native_binding(binding);}catch(const FormatError &){++binding_refusals;}
                try{(void)provider.native_binding(held,cache);}catch(const FormatError &){++binding_refusals;}
            }catch(...){binding_error=std::current_exception();}});observer.join();
            if(binding_error)std::rethrow_exception(binding_error);
            check(!foreign_binding_current&&!foreign_binding_matches&&binding_refusals==2,
                "Foreign executor admitted or matched a creator-held native provider binding");
        }
        provider.require_current(*cache);
        rejects([&]{(void)takeover.hold_native_publication();});
        rejects([&]{(void)takeover.world();});rejects([&]{(void)takeover.local();});
        rejects([&]{(void)takeover.camera();});
        unsigned callbacks{};const auto adoption=[](void *p)noexcept{++*static_cast<unsigned *>(p);return true;};
        rejects([&]{(void)takeover.activate(adoption,&callbacks);});
        check(!callbacks,"Held native scope entered session activation or its callback");
        rejects_native_publication([&]{(void)objects->memory();},"native publication");
        rejects_native_publication([&]{(void)objects->tu15_world();},"native publication");
        rejects_native_publication([&]{(void)native->memory();},"native publication");
        rejects_native_publication([&]{(void)native->prepare_source_call({});},"native publication");
        bool foreign_current=true,foreign_matches=true;unsigned foreign_refusals{};std::exception_ptr foreign_error;
        std::thread observer([&]{try{
            foreign_current=held.current();foreign_matches=held.matches(*native,cache);
            try{(void)objects->memory();}catch(const FormatError &){++foreign_refusals;}
            try{(void)native->memory();}catch(const FormatError &){++foreign_refusals;}
            try{(void)takeover.hold_native_publication();}catch(const FormatError &){++foreign_refusals;}
        }catch(...){foreign_error=std::current_exception();}});observer.join();
        if(foreign_error)std::rethrow_exception(foreign_error);
        check(!foreign_current && !foreign_matches && foreign_refusals==3,
            "Foreign executor observed or reentered the held native scope");
        check(!source_execution_inputs(bank),"Native scope unexpectedly started with a bound execution tape");
        rejects([&]{SourceExecutionInputBinding binding(bank,untracked_tape);});
        check(!source_execution_inputs(bank),"Rejected untracked tape left a TLS source binding behind");
        rejects_native_publication([&]{bank.write(source,original_word);},"Native publication cannot write");
        rejects_native_publication([&]{(void)bank.mutable_view(source,4);},"Native publication cannot write");
        rejects_native_publication([&]{bank.validate_mutable_view(source,4);},"Native publication cannot write");
        rejects_native_publication([&]{bank.mount(*free,original_word,SourceRuntimeMemory::Access::ReadWrite,
            "refused native publication fixture mount");},"Native publication cannot replace");
        rejects_native_publication([&]{bank.reserve(*free,4096,4096,
            "refused native publication fixture reservation");},"Native publication cannot replace");
        rejects_native_publication([&]{bank.release(region->address);},"Native publication cannot replace");
        rejects_native_publication([&]{bank.restore(memory);},"Native publication cannot replace");
        rejects_native_publication([&]{bank.swap_backing(empty);},"Native publication cannot replace");
        rejects_native_publication([&]{SourceRuntimeMemory moved(std::move(bank));},"Native publication cannot replace");
        rejects_native_publication([&]{empty=std::move(bank);},"Native publication cannot replace");
        // Native-only exclusion permits ordinary timing metadata reads before
        // the future timing/SDL composition acquires its own retained locks.
        provider.require_current(*cache);
        check(held.current() && held.matches(*native,cache),"Rejected operations consumed the actual native scope");
    }
    check(!released_binding.current(),"Released native exclusion retained provider binding eligibility");
    rejects([&]{(void)MccSoundVariationProvider::hold_native_binding(released_binding);});
    check(authority.execution_boundary()==before && bank.checkpoint()==memory && empty.checkpoint()==empty_before,
        "Native exclusion or its rejected mutations changed actual source bytes/topology/boundary");
    provider.require_current(*cache);
    {SourceExecutionInputBinding binding(bank,untracked_tape);
        check(source_execution_inputs(bank)==&untracked_tape,"Untracked tape control was invalid outside native scope");}
    check(!source_execution_inputs(bank),"Released outside-scope tape control leaked its TLS binding");
    MccSoundVariationProvider::NativeBinding closed_binding;
    {
        // This is the real nonmovable factory result, directly elided into
        // owned storage. Close only its wrapper, never the takeover/session,
        // while the one timing hold retains the actual native State.
        auto native_scope=std::unique_ptr<SourceNativePublication>(
            new SourceNativePublication(takeover.hold_native_publication()));
        closed_binding=provider.native_binding(*native_scope,cache);
        auto timing=MccSoundVariationProvider::hold_native_binding(closed_binding);
        check(timing.matches(closed_binding),"Native wrapper-close control did not start with its actual timing hold");
        native_scope.reset();
        check(!closed_binding.current()&&!timing.matches(closed_binding),
            "Closing native wrapper retained provider eligibility during its timing hold");
        rejects([&]{(void)takeover.world();});
        rejects_native_publication([&]{bank.write(source,original_word);},"Native publication cannot write");
    }
    check(!closed_binding.current(),"Closed native wrapper remained eligible after retained timing release");
    rejects([&]{(void)MccSoundVariationProvider::hold_native_binding(closed_binding);});
    provider.require_current(*cache);
    {auto retry=takeover.hold_native_publication();check(retry.current() && retry.matches(*native,cache),
        "Released native scope did not permit an unchanged creator retry");}
    std::cout<<"PASS genuine native provider binding: actual same-Cache different-World timing owner rejection, exact creator-held timing, foreign/reentry/control-block refusal, weak release and retry\n";
    native_audio_publication_controls(takeover,cache,same_cache_provider,provider,foreign_cache,wrong_control_block,released_binding);
#ifdef HALO2_TAKEOVER_HELD_LENS_TEST
    halo2::test::held_lens_controls(takeover,*disposable);
#endif
    // A valid same-byte producer write changes real mutation evidence. Do
    // this only to the disposable target, preserving the activation target.
    auto *const disposable_native=&disposable->world().source_objects->tu15_world();
    auto &disposable_bank=disposable->world().source_objects->memory();
    const auto disposable_source=disposable_native->source_world();
    disposable_bank.validate_mutable_view(disposable_source,4);
    std::array<std::byte,4> same_word;disposable_bank.read(disposable_source,same_word);
    disposable_native->write_source_call_input(disposable_source,same_word);
    rejects([&]{(void)disposable->hold_native_publication();});
    disposable.reset();
    std::cout<<"PASS genuine current-target native publication: exact Cache control block/nested World, creator/foreign/reentry refusal, unchanged actual bank and valid write/topology/move exclusions, release/retry and stale producer refusal\n";

    auto dying=SourcePeerHostTakeover::prepare(options,retained,decision,std::make_shared<Clock>());
    auto cache_before_drop=dying->world().cache;const auto *const cache_address=cache_before_drop.get();
    std::weak_ptr<const Cache> weak_cache=cache_before_drop;
    auto *const dying_native=&dying->world().source_objects->tu15_world();
    auto dying_provider=dying_native->sound_variation_provider(*cache_before_drop);
    dying_provider.require_current(*cache_before_drop);
    MccSoundVariationProvider::NativeBinding dying_binding;
    std::shared_ptr<const Cache> cache_after_drop;
    {
        // No timing/SDL/device hold is present: normal takeover destruction
        // may retire its host/camera/session wrappers. Only native target
        // lifetime, owned by the actual factory-minted scope, is asserted.
        auto held=dying->hold_native_publication();
        check(held.current() && held.matches(*dying_native,cache_before_drop),
            "Disposable genuine target failed native lifetime preflight");
        dying_binding=dying_provider.native_binding(held,cache_before_drop);
        check(dying_binding.current(),"Disposable native binding failed its actual owner admission");
        cache_before_drop.reset();dying.reset();
        check(!held.current() && !dying_binding.current() && !weak_cache.expired(),
            "Closing takeover retained eligibility or destroyed the scope's actual Cache owner");
        rejects([&]{(void)MccSoundVariationProvider::hold_native_binding(dying_binding);});
        dying_provider.require_current(*cache_address);
        // Lock the actual weak Cache before leaving, solely so the later
        // provider-staleness check has a live exact borrowed Cache operand.
        cache_after_drop=weak_cache.lock();check(bool(cache_after_drop),"Held target lost its real Cache lifetime");
    }
    rejects([&]{dying_provider.require_current(*cache_after_drop);});
    check(!dying_binding.current(),"Weak metadata binding kept a retired native scope eligible");
    rejects([&]{(void)MccSoundVariationProvider::hold_native_binding(dying_binding);});
    // The transferable binding intentionally retains Cache metadata; the
    // actual World/epoch was already destroyed despite that live weak token.
    dying_binding={};
    cache_after_drop.reset();check(weak_cache.expired(),"Retired native scope leaked the disposable Cache owner");
    std::cout<<"PASS genuine native publication target lifetime: closed takeover loses eligibility, actual native provider/Cache survive native-only hold, final release stales provider and retires exact Cache\n";
}
}
int main(int argc,char **argv)try{
    std::cout<<std::unitbuf;check(argc==5,"Supply ORIGIN DESCRIPTORS (XBE | --mcc) MAP");
    const bool selected_mcc=std::string_view(argv[3])=="--mcc";
    if(selected_mcc){
        check(std::filesystem::is_regular_file(argv[4]),"Explicit MCC native-scope map is missing");
        const auto map=std::filesystem::path(argv[4]);
        const Cache cache(map,map.parent_path(),std::shared_ptr<const OriginalData>{});
        check(cache.is_mcc() && !cache.is_vista() && cache.original_data()==OriginalData::native_tu15(),
            "MCC native-scope route requires imported MCC assets and compiled native TU1.5 gameplay");
        std::cout<<"OBSERVED selected MCC takeover input; external_xbe=0; no asset import performed\n";
    }else{
        if(!std::filesystem::is_regular_file(argv[3])||!std::filesystem::is_regular_file(argv[4]))return 77;
        std::cout<<"REFERENCE offline Xbox TU1.5 takeover route; not MCC runtime acceptance\n";
    }
    nlohmann::json descriptors;{std::ifstream in(argv[2]);in>>descriptors;}
    std::vector<OnlineAdmission> roster;
    for(const auto &row:descriptors.at(0).at("players")){
        OnlineAdmission p;p.slot=roster.size();p.name=row.at("name");p.team=row.at("team");p.peer=row.at("peer");p.guest=row.at("guest");
        const std::string identity=row.at("identity");check(identity.size()==16,"Invalid private admission fixture");
        for(unsigned i=0;i<8;++i)p.identity[i]=std::byte(std::stoul(identity.substr(i*2,2),nullptr,16));
        roster.push_back(p);
    }
    check(roster.size()==12&&descriptors.size()==3,"Expected three physical PCs with four couch seats each");
    std::array<std::shared_ptr<OnlineMatchSession>,16> sessions;
    for(const auto &d:descriptors){OnlineMatchTransportOptions transport;transport.direct_udp=true;transport.peer_host_migration=true;
        const unsigned self=d.at("slot");sessions[self]=std::make_shared<OnlineMatchSession>(argv[1],d.at("id"),d.at("token"),self,roster,true,false,false,transport);}
    const auto healthy=[&]{for(const auto &s:sessions)if(s)check(s->phase()!=OnlineMatchSession::Phase::Failed,s->error().c_str());};
    stage="direct loading";wait([&]{healthy();for(const auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Loading)return false;return true;},"Direct loading timeout");
    const UiVariant variant{"Slayer",0,{{0x59,5},{10,0},{2,0}}};constexpr std::uint64_t fingerprint=0x15c071501;
    for(const auto &s:sessions)if(s)s->loaded(fingerprint,variant);
    wait([&]{healthy();for(const auto &s:sessions)if(s&&s->phase()!=OnlineMatchSession::Phase::Running)return false;return true;},"Direct running timeout");
    const unsigned old_host=sessions[0]->host();unsigned candidate=16,remote=16;
    for(unsigned p=0;p<16;++p)if(sessions[p]){check(sessions[p]->host()==old_host,"Measured hosts disagree");
        if(p!=old_host){if(candidate==16)candidate=p;else remote=p;}}
    check(candidate<16&&remote<16,"Missing native candidate or surviving client");
    const auto before=*sessions[old_host]->authority_context();
    MultiplayerOptions producer_options;if(!selected_mcc)producer_options.gameplay_xbe=argv[3];producer_options.map=argv[4];
    producer_options.shared_maps=producer_options.map.parent_path();producer_options.source_object_world=true;
    producer_options.variant=variant;producer_options.local_players=4;producer_options.session_random_seed=0x71;
    producer_options.online_match=sessions[old_host];
    std::array<std::vector<SourcePlayerConfiguration>,16> profiles;
    for(const auto &p:roster)profiles[online_admission_peer(p)].push_back(profile(p.name));
    producer_options.selected_profiles=profiles[old_host];
    stage="native source admission";
    auto producer=std::make_unique<MultiplayerPreparedWorld>(producer_options,std::nullopt,std::make_shared<Clock>());
    for(unsigned view=0;view<4;++view){const auto slot=sessions[old_host]->local_slots()[view];
        check(producer->match.join(roster[slot].name,-1,profiles[old_host][view],slot)==slot,"Initial native host birth changed a slot");}
    producer->bind_source_local_views(sessions[old_host]->local_slots());
    auto held_datagrams=std::make_shared<HeldInputDatagrams>(sessions[old_host]);
    auto lan_host=std::make_unique<LanHost>(producer->match,held_datagrams,fingerprint,roster,variant);
    auto promoted=std::make_unique<LanClient>(sessions[candidate],old_host,fingerprint,std::span<const std::int8_t>{},profiles[candidate]);
    auto survivor_owner=std::make_unique<LanClient>(sessions[remote],old_host,fingerprint,std::span<const std::int8_t>{},profiles[remote]);
    auto &survivor=*survivor_owner;
    std::array<Controls,4> neutral{};std::array<Controls,16> controls{};
    wait([&]{promoted->send(neutral);survivor.send(neutral);lan_host->receive(controls);lan_host->broadcast();promoted->receive();survivor.receive();
        healthy();return lan_host->peers()==2&&promoted->ready()&&survivor.ready();},"Twelve actual native player/profile admissions timed out");
    const auto unbound_origin=promoted->received_origins().back();
    auto original=std::make_unique<SourceMatchAuthority>(producer->match,*producer->source_objects);
    std::vector<unsigned> slots;for(const auto &p:roster)slots.push_back(p.slot);
    const auto admitted=original->player_admissions(slots);
    check(sessions[old_host]->publish_source_players(before,admitted),"Actual native allocation export refused");
    wait([&]{healthy();for(const auto &s:sessions)if(s){const auto a=s->source_players();if(!a||a->players!=admitted)return false;}return true;},"Independent native admissions timed out");
    MultiplayerOptions remote_options=producer_options;remote_options.online_match=sessions[remote];remote_options.selected_profiles=profiles[remote];
    auto remote_resident=std::make_shared<MultiplayerPreparedWorld>(remote_options,std::nullopt,std::make_shared<Clock>());
    const auto remote_cache_owner=remote_resident->cache;
    const auto remote_match=&remote_resident->match;
    auto remote_shadow=std::make_unique<SourceAuthorityReplayWorker>(remote_options,std::make_shared<Clock>(),std::make_shared<Clock>());
    MultiplayerRuntimeAuthority remote_runtime(remote_options,remote_resident,{},std::move(survivor_owner),std::move(remote_shadow));remote_resident.reset();
    remote_runtime.receive();check(!remote_runtime.prepare_client_rebind(),"Healthy client staged an uncertified endpoint");
    // This PC has no delivered cut. A failed old-generation export must not
    // destroy its live client or prevent restarting shadow work after rebind.
    bool missing_cut{};
    wait([&]{try{(void)remote_runtime.recover();}catch(const std::exception &){missing_cut=true;}return missing_cut;},"Missing old shadow did not reject export");
    auto remote_frame=remote_runtime.begin_frame();check(bool(remote_frame),"Running remote client refused a native frame");
    for(unsigned tick=0;tick<3;++tick){for(const auto &p:roster)controls[p.slot].yaw=producer->match.unit(p.slot).yaw;
        producer->match.step(controls);lan_host->broadcast();}
    auto switches=neutral;for(unsigned view=0;view<4;++view){switches[view].switch_grenade=true;
        switches[view].yaw=producer->match.unit(sessions[candidate]->local_slots()[view]).yaw;}
    for(unsigned packet=0;packet<3;++packet)promoted->send(switches);
    wait([&]{return held_datagrams->retain_switches(candidate)==3;},"Three real couch-input pulses were not delivered over DTLS");
    controls={};for(auto slot:sessions[old_host]->local_slots())controls[slot].yaw=producer->match.unit(slot).yaw;
    lan_host->receive(controls);
    for(auto slot:sessions[candidate]->local_slots())check(controls[slot].switch_grenade,"Predecessor lost the first real couch pulse");
    producer->match.step(controls);lan_host->broadcast();
    stage="capture paired native cut";
    // Declared CPU-only pending presentation records. They cross the actual
    // authenticated cut, but their absent predecessor GPU identities cannot
    // become completed samples on this replacement PC.
    producer->source_objects->render_model_view([](auto &source){
        auto &memory=source.memory();std::array<std::byte,33*32> records{};
        for(unsigned n=0;n<records.size();++n)records[n]=std::byte((n*17u+7u)&255u);
        memory.write(0x52dfdcu,records);
        const std::array<std::byte,4> count{std::byte{33},std::byte{},std::byte{},std::byte{}};
        memory.write(0x531fbcu,count);
        std::array<std::byte,192> pending{};
        for(unsigned ring=0;ring<3;++ring){pending[ring*64]=std::byte{1};pending[ring*64+3]=std::byte{0x80};pending[ring*64+4]=std::byte{1};}
        memory.write(0x531fc0u,pending);
    });
    const auto paired=capture_source_host_continuation(*lan_host,before,1);
    const std::array<unsigned,2> fanout_recipients{candidate,remote};
    wait([&]{return std::ranges::all_of(fanout_recipients,[&](unsigned peer){
        return sessions[old_host]->source_players_confirmed(peer)&&
            sessions[old_host]->authority_replay_supported(peer)&&
            sessions[old_host]->authority_host_continuation_supported(peer);});},
        "Both actual recipients did not admit paired native/replay delivery");
    // Capture before native recipient warmup and before the post-cut sends
    // below. Recapturing the host table after warmup would import fixture idle
    // time into its peers and change the migration/input survival control.
    auto fanout_publisher=std::make_unique<SourceAuthorityPublisher>(*original,*lan_host,
        sessions[old_host],fanout_recipients);
    check(fanout_publisher->capture()&&!fanout_publisher->capture(),
        "Group publisher refused its cut or accepted overlapping capture");
    stage="encode native world";
    const auto complete=encode_source_authority(paired.world);
    // These two authenticated packets are produced after the sealed cut. The
    // predecessor never receives them, so promotion must recover them from
    // the owning client rather than only replay the saved host input bank.
    auto late=switches;
    for(auto &c:late){c.forward=.125f;c.right=-.25f;c.crouch=true;c.yaw+=.03125f;c.pitch=.0625f;
        c.zoom=-1;c.zoom_reset_serial=7;c.pickup=true;c.pickup_milliseconds=600;}
    promoted->send(late);
    for(auto &c:late){c.pickup=false;c.pickup_milliseconds=0;
        c.vehicle_interaction_target=SourcePlayerActionTarget{0,0,null_datum};}
    promoted->send(late);
    MultiplayerOptions recipient_options=producer_options;recipient_options.online_match=sessions[candidate];recipient_options.selected_profiles=profiles[candidate];
    auto presentation_borrower=std::make_shared<int>(1);std::weak_ptr<int> borrowed_lifetime=presentation_borrower;
    recipient_options.menu_audio_update=[presentation_borrower]{};
    // The actual runtime keeps its initial replica/cache alive for resident
    // rendering resources while a private full source world is promoted.
    auto resident=std::make_shared<MultiplayerPreparedWorld>(recipient_options,std::nullopt,std::make_shared<Clock>());
    const auto resident_cache=resident->cache.get();const auto resident_match=&resident->match;
    std::atomic<unsigned> recovery_observer_calls{};
    if(selected_mcc)recipient_options.source_world_observer=[&](const SourceRuntimeMemory &){
        recovery_observer_calls.fetch_add(1);throw FormatError("Private recovery invoked an external native observer");
    };
    stage="warm recipient native pair";
    const bool worker_retention=std::getenv("HALO2_TAKEOVER_WORKER_RETAIN");
    std::unique_ptr<SourceAuthorityReceiver> receiver;
    std::unique_ptr<SourceAuthorityReplayWorker> background;
    if(worker_retention)background=std::make_unique<SourceAuthorityReplayWorker>(recipient_options,std::make_shared<Clock>(),std::make_shared<Clock>());
    else receiver=std::make_unique<SourceAuthorityReceiver>(recipient_options,SourceAuthorityStreamMode::Replay,std::make_shared<Clock>(),std::make_shared<Clock>());
    if(selected_mcc){
        const auto original_cache=resident->cache;
        resident->cache=remote_cache_owner;
        rejects([&]{MultiplayerRuntimeAuthority invalid(recipient_options,resident,{},{});});
        // Explicitly negative alias: both actual Cache owners stay alive.
        // A matching pointer cannot replace the constructor's control block.
        resident->cache=std::shared_ptr<const Cache>(remote_cache_owner,original_cache.get());
        rejects([&]{MultiplayerRuntimeAuthority invalid(recipient_options,resident,{},{});});
        resident->cache=original_cache;
        bool foreign_constructor_rejected{};
        std::thread foreign_constructor([&]{try{
            MultiplayerRuntimeAuthority invalid(recipient_options,resident,{},{});
        }catch(const FormatError &){foreign_constructor_rejected=true;}});foreign_constructor.join();
        check(foreign_constructor_rejected,"Runtime admitted another thread's constructor Cache owner");
    }
    MultiplayerRuntimeAuthority runtime(recipient_options,resident,{},std::move(promoted),std::move(background));resident.reset();
    runtime.receive();
    // Resolve declared pending CPU work before the expensive native fanout.
    // Retain owning IDs/cues from this cache, never Xbox datums on MCC.
    const auto [pending_effect,pending_cues]=[&]{
        const auto &cache=*runtime.world().cache;const Tag *effect{};
        for(const auto &tag:cache.select_tags({.group=tag_groups::effect,
            .name="effects\\objects\\weapons\\rifle\\plasma_rifle\\upper_fire",.require_address=true,.minimum_size=1})){
            check(!effect,"Pending cue fixture has ambiguous authored effect identity");effect=&tag;
        }
        check(effect,"Pending cue fixture lacks its resident authored plasma firing effect");
        (void)cache.tag_payload(effect->handle());
        auto cues=fixed_effect_sound_cues(cache,effect->datum);
        check(!cues.empty(),"Pending cue fixture lost its actual authored sound schedule");
        for(const auto &cue:cues){
            const auto sound=cache.query_tag({.handle=TagHandle{cue.sound},.group=tag_groups::sound,
                .require_address=true,.minimum_size=1});
            check(sound,"Pending cue fixture references unavailable authored sound metadata");
            (void)cache.tag_payload(sound->handle());
        }
        std::cout<<"MEASURE declared pending CPU sound fixture: effect="<<effect->name
            <<" datum="<<std::hex<<effect->datum<<" first_sound="<<cues.front().sound<<std::dec
            <<" authored_pending_cues="<<cues.size()<<'\n';
        return std::pair{effect->datum,std::move(cues)};
    }();
    const auto pending_sound=pending_cues.front().sound;
    MultiplayerSnapshotOrigin candidate_origin,remote_origin;
    wait([&]{runtime.client()->receive();survivor.receive();
        for(const auto &origin:runtime.client()->received_origins())if(origin.tick()==paired.world.match.simulation.ticks)candidate_origin=origin;
        for(const auto &origin:survivor.received_origins())if(origin.tick()==paired.world.match.simulation.ticks)remote_origin=origin;
        return candidate_origin.context()==before&&remote_origin.context()==before;
    },"Exact paired snapshot origins did not arrive through admitted DTLS clients");
    recipient_options.menu_audio_update={};presentation_borrower.reset();
    check(borrowed_lifetime.expired(),"Native recovery recipe retained a presentation borrower");
    check(!runtime.prepare_takeover(),"Healthy runtime prepared an uncertified replacement");
    check(!runtime.prepare_common_native({candidate,before,{}})&&!runtime.prepared_common_native(),
        "A healthy runtime without an exact strong common pair imported an invented cut");
    if(selected_mcc){
        auto &live=runtime.world();const auto original_cache=live.cache;
        live.cache=remote_cache_owner;rejects([&]{(void)runtime.world();});
        rejects([&]{(void)runtime.prepare_takeover();});
        live.cache=std::shared_ptr<const Cache>(remote_cache_owner,original_cache.get());
        rejects([&]{(void)runtime.world();});rejects([&]{(void)runtime.prepare_takeover();});
        live.cache=original_cache;
        check(same_cache_owner(runtime.world().cache,original_cache),"Runtime Cache-slot refusal lost its actual original owner");
    }
    if(worker_retention)recipient_options.selected_profiles.front()=profile("Later Menu");
    bool foreign_rejected{};std::thread foreign([&]{try{runtime.world();}catch(const FormatError &){foreign_rejected=true;}});foreign.join();
    check(foreign_rejected,"Runtime exported a native owner to another executor");
    try{auto interrupted=runtime.begin_frame();check(bool(interrupted),"Missing interrupted frame");throw FormatError("Deliberate frame unwind");}
    catch(const FormatError &){}
    auto frame=runtime.begin_frame();check(bool(frame),"Running runtime refused its frame");
    rejects([&]{runtime.begin_frame();});rejects([&]{runtime.service();});
    rejects([&]{runtime.suspend_publication();});
    rejects([&]{runtime.prepare_takeover();});
    rejects([&]{runtime.prepare_common_native({candidate,before,{}});});
    const auto state=[&]{return receiver?receiver->state():*runtime.shadow_state();};
    const auto update=[&]{if(receiver)receiver->update();};
    const auto checkpoint=[&](auto ready){return receiver->checkpoint(ready);};
    wait([&]{if(receiver)receiver->warm();const auto s=state();check(s.error.empty(),s.error.c_str());return s.warmed&&s.workspace_warmed;},"Private recipient worlds did not warm");
    AuthorityCutSeal seal;
    std::optional<SourceAuthorityPublisherGroupCut> completed_fanout;
    // Keep this independently admitted executor through any candidate repair.
    // Its own native export, never a relabeled candidate proof, supplies the
    // surviving client's exact-cut preparation metadata after election.
    auto remote_receiver=std::make_unique<SourceAuthorityReceiver>(remote_options,SourceAuthorityStreamMode::Replay,
        std::make_shared<Clock>(),std::make_shared<Clock>());
    std::optional<SourceAuthorityRetainedWorld> remote_retained;
    {
        stage="independent paired publisher fanout admission";
        wait([&]{remote_receiver->warm();const auto s=remote_receiver->state();check(s.error.empty(),s.error.c_str());
            return s.warmed&&s.workspace_warmed;},"Independent second recipient worlds did not warm");
        const auto recipients_mask=std::uint16_t((1u<<candidate)|(1u<<remote));
        auto &publisher=*fanout_publisher;
        bool fanout_size_logged{};
        stage="candidate native ACK with delayed second recipient";
        wait([&]{publisher.update();update();const auto p=publisher.state();const auto c=state();
            if(!p.error.empty())std::cerr<<"DIAGNOSTIC takeover candidate ACK: native_receipts="<<p.native_receipts
                <<" validated_peers="<<p.pending_native_validated_peers<<" pending_group="<<bool(p.pending_group)
                <<" seal_bytes="<<seal.bytes<<" payload_bytes="<<p.payload_bytes
                <<" candidate_ready_bytes="<<(c.ready?c.ready->seal.bytes:0)
                <<" candidate_delivered="<<sessions[old_host]->authority_delivered(candidate,before,seal)
                <<" remote_delivered="<<sessions[old_host]->authority_delivered(remote,before,seal)
                <<" received="<<c.received<<" prepared="<<c.prepared<<" decoding="<<c.decoding
                <<" replaying="<<c.replaying<<" ready="<<bool(c.ready)<<" host_ready="<<c.host_ready
                <<" error_present="<<!c.error.empty()<<'\n';
            check(p.error.empty(),p.error.c_str());check(c.error.empty(),c.error.c_str());
            if(p.pending_group){
                check(p.pending_group->context==before&&p.pending_group->recipients==recipients_mask,
                    "Pending group relabeled its context or independent recipient set");
                seal=p.pending_group->seal;
                if(!fanout_size_logged){
                    const AuthorityTransferLimits limits;
                    std::cout<<"MEASURE paired native fanout: seal_bytes="<<seal.bytes
                        <<" payload_bytes="<<p.payload_bytes<<" recipients="<<fanout_recipients.size()
                        <<" bytes_per_second="<<limits.bytes_per_second<<" burst_bytes="<<limits.burst_bytes
                        <<" idle_ms="<<limits.idle_ms<<" lifetime_ms="<<limits.lifetime_ms<<'\n';
                    fanout_size_logged=true;
                }
            }
            return seal.id&&c.ready&&c.ready->seal==seal&&c.host_ready&&p.native_receipts==1&&
                sessions[old_host]->authority_delivered(remote,before,seal);
        },"First native recipient ACK or second peer byte delivery did not complete");
        const auto candidate_only=publisher.state();
        check(candidate_only.pending_group&&candidate_only.pending_group->seal==seal&&
            candidate_only.pending_native_validated_peers==std::uint16_t(1u<<candidate)&&
            !candidate_only.group_ready&&!candidate_only.ready&&candidate_only.validations==0&&
            !remote_receiver->state().ready,
            "One native ACK or a byte-complete second peer fabricated group readiness");
        auto wrong_seal=seal;++wrong_seal.tick;
        auto wrong_context=before;++wrong_context.membership_revision;
        check(!sessions[candidate]->acknowledge_authority(before,wrong_seal)&&
            !sessions[candidate]->acknowledge_authority(wrong_context,seal)&&
            !sessions[old_host]->acknowledge_authority(before,seal)&&
            !sessions[remote]->acknowledge_authority(before,seal)&&
            sessions[candidate]->acknowledge_authority(before,seal)&&
            sessions[candidate]->acknowledge_authority(before,seal),
            "Group ACK controls accepted foreign/unvalidated evidence or rejected an exact duplicate");
        publisher.update();
        const auto after_duplicate=publisher.state();
        check(after_duplicate.native_receipts==1&&after_duplicate.validations==0&&!after_duplicate.group_ready&&
            after_duplicate.pending_group&&after_duplicate.pending_group->seal==seal&&
            after_duplicate.pending_native_validated_peers==std::uint16_t(1u<<candidate),
            "Duplicate/foreign ACKs completed or changed the delayed native group");
        stage="independent second paired native validation";
        wait([&]{remote_receiver->update();publisher.update();const auto r=remote_receiver->state();const auto p=publisher.state();
            check(r.error.empty(),r.error.c_str());check(p.error.empty(),p.error.c_str());
            return r.ready&&r.ready->seal==seal&&r.host_ready&&p.group_ready;
        },"Independent second recipient did not validate the exact common paired seal");
        const auto c=state(),r=remote_receiver->state();const auto p=publisher.state();
        check(c.ready&&r.ready&&c.host_proof&&r.host_proof&&
            c.ready->peer==candidate&&r.ready->peer==remote&&
            c.ready->context==before&&r.ready->context==before&&c.ready->seal==seal&&r.ready->seal==seal&&
            c.host_proof->cut().peer==candidate&&r.host_proof->cut().peer==remote&&
            c.host_proof->prove_snapshot(*runtime.client(),candidate_origin).matches(candidate_origin)&&
            r.host_proof->prove_snapshot(survivor,remote_origin).matches(remote_origin),
            "Independent recipients did not each create native proof for their own accepted snapshot");
        rejects([&]{c.host_proof->prove_snapshot(*runtime.client(),remote_origin);});
        rejects([&]{r.host_proof->prove_snapshot(survivor,candidate_origin);});
        check(p.group_ready&&p.group_ready->context==before&&p.group_ready->seal==seal&&
            p.group_ready->recipients==recipients_mask&&!p.pending_group&&!p.ready&&
            p.native_receipts==2&&p.validations==1&&p.publications==1&&p.host_ready,
            "Group readiness omitted an actual recipient, counted duplicate receipts or fabricated a singleton peer");
        const auto group_host=publisher.host_checkpoint(*p.group_ready);
        check(runtime.client()->prove_snapshot(group_host,*c.ready,candidate_origin).matches(candidate_origin)&&
            survivor.prove_snapshot(group_host,*r.ready,remote_origin).matches(remote_origin),
            "Completed native group lost its host table's exact two accepted snapshots");
        auto wrong_group=*p.group_ready;wrong_group.recipients=std::uint16_t(1u<<candidate);
        rejects([&]{publisher.host_checkpoint(wrong_group);});
        wrong_group=*p.group_ready;++wrong_group.seal.id;
        rejects([&]{publisher.host_checkpoint(wrong_group);});
        wrong_group=*p.group_ready;++wrong_group.context.epoch;
        rejects([&]{publisher.host_checkpoint(wrong_group);});
        {remote_retained=remote_receiver->checkpoint_paired(*r.ready);const auto &remote_export=*remote_retained;
            check(remote_export.cut.peer==remote&&remote_export.cut.context==before&&remote_export.cut.seal==seal&&
                remote_export.host&&remote_export.local_views==sessions[remote]->local_slots()&&
                remote_export.producer_views==sessions[old_host]->local_slots()&&
                remote_export.admissions.players==admitted&&encode_source_authority(remote_export.checkpoint)==complete&&
                remote_receiver->state().ready&&remote_receiver->state().ready->seal==seal&&remote_receiver->world(),
                "Nonterminal second-recipient export changed the cut, admissions, own views or live receiver");}
        if(receiver){const auto candidate_export=receiver->checkpoint_paired(*c.ready);
            check(candidate_export.cut.peer==candidate&&candidate_export.cut.seal==seal&&candidate_export.host&&
                candidate_export.local_views==sessions[candidate]->local_slots()&&
                encode_source_authority(candidate_export.checkpoint)==complete&&receiver->world()&&receiver->state().ready,
                "Nonterminal first-recipient export stopped or relabeled its native owner");}
        check(sessions[candidate]->acknowledge_authority(before,seal)&&sessions[remote]->acknowledge_authority(before,seal),
            "Completed group rejected its exact duplicate recipient ACKs");
        publisher.update();const auto stable=publisher.state();
        check(stable.group_ready==p.group_ready&&stable.native_receipts==2&&stable.validations==1&&!stable.ready,
            "Completed group recounted duplicate ACKs or lost its exact common seal");
        completed_fanout=stable.group_ready;
        stage="explicit completed fanout group cancellation";
        check(stable.tracking_replay,"Completed group lost the live stream before its cancellation control");
        const auto saved_host_bytes=LanHost::encode_continuation(group_host,{before,seal});
        const auto before_cancel=original->execution_boundary();
        check(stable.host_ready&&!stable.encoding&&!stable.awaiting_validation&&!stable.pending_group&&
            !publisher.in_flight_cuts()&&
            LanHost::encode_continuation(publisher.host_checkpoint(*completed_fanout),{before,seal})==saved_host_bytes,
            "Completed group lost exact historical host bytes before explicit cancellation");
        publisher.cancel();const auto cancelled=publisher.state();
        check(!cancelled.group_ready&&!cancelled.host_ready&&!cancelled.ready&&!cancelled.pending_group&&
            !cancelled.pending_native_validated_peers&&!cancelled.awaiting_validation&&!cancelled.tracking_replay&&
            !cancelled.encoding&&!cancelled.queued_phases&&!cancelled.queued_bytes&&!publisher.in_flight_cuts()&&
            cancelled.native_receipts==stable.native_receipts&&cancelled.validations==stable.validations&&
            cancelled.publications==stable.publications&&original->execution_boundary()==before_cancel,
            "Explicit group cancellation retained historical host eligibility or changed native/validation state");
        rejects([&]{publisher.host_checkpoint(*completed_fanout);});
        std::cout<<"PASS paired publisher fanout to two independent native receivers: common seal, each own accepted-snapshot proof, delayed native ACK gate, duplicate/foreign rejection, exact recipient mask and nonterminal paired export\n";
        std::cout<<"PASS paired publisher explicit group cancellation: exact completed host bytes and counters, retired group eligibility, unchanged native boundary\n";
    }
    fanout_publisher.reset();
    stage="paired native validation";
    auto ready=*state().ready;
    const auto first_host_proof=state().host_proof;
    check(first_host_proof&&first_host_proof->cut().peer==candidate&&first_host_proof->cut().context==before&&
        first_host_proof->cut().seal==ready.seal,"Native receiver published an absent or mismatched paired proof");
    const auto check_origins=[&](const SourceAuthorityHostProof &proof){
        check(proof.prove_snapshot(*runtime.client(),candidate_origin).matches(candidate_origin)&&
            proof.prove_snapshot(survivor,remote_origin).matches(remote_origin),"Validated cut lost an exact native peer snapshot");
        rejects([&]{proof.prove_snapshot(*runtime.client(),unbound_origin);});
        rejects([&]{proof.prove_snapshot(*runtime.client(),remote_origin);});
        rejects([&]{proof.prove_snapshot(survivor,candidate_origin);});
    };
    check_origins(*first_host_proof);
    const auto adopt_replica=[](MultiplayerMatch &match,const MultiplayerSnapshotOrigin &origin){
        const auto &accepted=origin.accepted_state();
        check(accepted.source_scene_roots_authoritative,"Native receipt omitted its authoritative display scene");
        match.replicate_devices(accepted.devices);
        match.replicate_source_scene(accepted.source_scene_roots,accepted.source_body_poses);
        match.replicate_prop_removals(accepted.prop_removals);
        match.replicate_prop_regions(accepted.prop_regions);
    };
    adopt_replica(runtime.match(),remote_origin);
    const auto foreign_remote_replica=runtime.match().replica_checkpoint(remote_origin);
    adopt_replica(runtime.match(),candidate_origin);
    adopt_replica(remote_runtime.match(),remote_origin);
    const auto remote_replica=remote_runtime.match().replica_checkpoint(remote_origin);
    const auto remote_cut_collision=&remote_runtime.match().simulation().collision();
    Mesh effect_mesh;std::uint32_t effect_primary=0x713719u,effect_auxiliary=0x511599u;
    const SourceEffectRandom effect_random{effect_primary,effect_auxiliary};
    NativeWorldEffects cpu_world(*runtime.world().cache,effect_mesh,runtime.match().simulation().collision(),effect_random);
    NativeMuzzleEffects cpu_muzzle(*runtime.world().cache,effect_random);
    SourceMapEffects cpu_map(*runtime.world().cache,runtime.world().content.objects,cpu_world);
    SourceCombatAttachments cpu_combat(*runtime.world().cache,cpu_world,{});
    EffectPresentationContinuity cpu_effects(cpu_muzzle,cpu_world,cpu_map,cpu_combat,effect_random);
    NativeEffectSounds pending_sounds(*runtime.world().cache);
    pending_sounds.spawn(pending_effect,0x71370009u,{1,2,3});
    check(pending_sounds.checkpoint().pending_count()==pending_cues.size(),
        "Pending cue fixture did not retain its complete nonempty authored schedule");
    EffectPresentationHistory history(cpu_effects,pending_sounds,runtime.match().simulation(),2),
        longer(cpu_effects,pending_sounds,runtime.match().simulation(),4);
    rejects([&]{EffectPresentationHistory invalid(cpu_effects,0);});
    rejects([&]{history.remember(candidate_origin);});
    history.context(before);longer.context(before);
    history.begin_runtime(*runtime.client());longer.begin_runtime(*runtime.client());
    const auto reserved=history.reserve_runtime(before,ready.seal);
    const auto same_reservation=history.reserve_runtime(before,ready.seal);
    auto different_seal=ready.seal;++different_seal.id;
    const auto other_reservation=history.reserve_runtime(before,different_seal);
    const auto foreign_reservation=longer.reserve_runtime(before,ready.seal);
    auto excess_seal=different_seal;++excess_seal.id;
    rejects([&]{history.reserve_runtime(before,excess_seal);});
    check(reserved.valid()&&reserved.retained()==0&&!reserved.overflowed(),
        "Early common-cut reservation invented a foreground receipt");
    rejects([&]{history.remember(unbound_origin);});
    const auto effect_tick=*candidate_origin.tick();cpu_world.advance(effect_tick,tick_seconds);cpu_muzzle.advance(effect_tick,tick_seconds);
    const auto frontier_for=[&](const MultiplayerSnapshotOrigin &origin){
        adopt_replica(runtime.match(),origin);
        MultiplayerRuntimeReceiptFrontier frontier;
        frontier.replica=runtime.match().replica_checkpoint(origin);
        frontier.previous=candidate_origin;frontier.local_slots.assign(origin.local_slots().begin(),origin.local_slots().end());
        frontier.client_state_bound=frontier.joined=true;
        frontier.world_effect_tick=origin.tick();frontier.muzzle_pose_tick=origin.tick();
        frontier.world_impact_serial=71;frontier.world_voice_serial=37;frontier.client_effect_serial=19;
        frontier.world_effect_baseline=frontier.client_effect_baseline=true;
        frontier.device_events=NativeDeviceEventCursor(true);
        frontier.heard_prop_effects.insert(0x71370009u);frontier.attached_effect_objects.insert(0x51150009u);
        frontier.firing_instances.emplace(std::pair<std::uint16_t,std::uint32_t>{9,pending_effect},
            MultiplayerRuntimeFiringOwner{0x71370009u,0x51150009u,0x927a0009u});
        frontier.heard_shots[0]=origin.accepted_state().players[0].shot_tick;
        frontier.muzzle_pose_owners.push_back({0x71370009u,0x51150009u,0x927a0009u});
        frontier.sounds.push_back({pending_sound,{4,5,6}});
        frontier.weapon_sounds.emplace_back(pending_sound,0x71370009u);
        return frontier;
    };
    auto initial_frontier=frontier_for(candidate_origin);
    check(history.remember(candidate_origin,initial_frontier)&&longer.remember(candidate_origin,initial_frontier)&&
        !history.remember(candidate_origin,initial_frontier),
        "Exact presentation receipt was not retained idempotently");
    const auto bound_effect=history.bind(*first_host_proof,*runtime.client());
    check(bound_effect&&bound_effect->cut().seal==ready.seal&&!history.bind(*first_host_proof,survivor),
        "History admitted a foreign connection or lost its exact paired native cut");
    const auto bound_runtime=history.bind_runtime(*first_host_proof,*runtime.client());
    check(bound_runtime&&bound_runtime->origin().same(candidate_origin)&&bound_runtime->proof().matches(candidate_origin)&&
        bound_runtime->frontier().replica.matches(candidate_origin)&&bound_runtime->cut().seal==ready.seal&&
        !history.bind_runtime(*first_host_proof,survivor),"Runtime frontier lost exact native receipt binding");
    const auto reserved_runtime=history.bind_runtime(*first_host_proof,*runtime.client(),reserved);
    check(reserved_runtime&&reserved_runtime->origin().same(candidate_origin)&&reserved.retained()==1&&
        same_reservation.retained()==1&&!history.bind_runtime(*first_host_proof,survivor,reserved)&&
        !history.bind_runtime(*first_host_proof,*runtime.client(),other_reservation)&&
        !history.bind_runtime(*first_host_proof,*runtime.client(),foreign_reservation),
        "Common reservation lost exact receipt/connection/seal/history ownership");
    // These are declared frontend boundary controls beside actual native
    // receipts, not a rendered or playing-audio adoption fixture.
    initial_frontier.sounds.clear();initial_frontier.firing_instances.clear();initial_frontier.local_slots.clear();
    check(bound_runtime->current().tick==effect_tick&&bound_runtime->previous().tick==effect_tick&&
        bound_runtime->frontier().local_slots.size()==4&&bound_runtime->frontier().sounds.size()==1&&
        bound_runtime->frontier().weapon_sounds.size()==1&&
        bound_runtime->frontier().sounds.front().datum==pending_sound&&
        bound_runtime->frontier().weapon_sounds.front().first==pending_sound&&
        bound_runtime->frontier().firing_instances.begin()->first.second==pending_effect&&
        bound_runtime->frontier().firing_instances.begin()->second.source==0x927a0009u&&
        bound_runtime->frontier().pending_effect_sounds.pending_count()==pending_cues.size(),
        "Retained runtime frontier borrowed later caller values or lost pending work/full salts");
    const auto fresh_receipt=[&]{const auto prior=runtime.client()->received_origins().empty()?candidate_origin:runtime.client()->received_origins().back();
        lan_host->broadcast();MultiplayerSnapshotOrigin next;
        wait([&]{runtime.client()->receive();for(const auto &origin:runtime.client()->received_origins())
            if(origin.tick()==effect_tick&&!origin.same(prior))next=origin;
            return next.context()==before;},"Later same-tick native receipt did not arrive");return next;};
    const auto later=fresh_receipt();
    const auto before_replica_rejection=history.state();
    auto invalid_frontier=frontier_for(later);invalid_frontier.replica={};
    rejects([&]{history.remember(later,invalid_frontier);});
    invalid_frontier=frontier_for(later);
    adopt_replica(remote_runtime.match(),later);
    invalid_frontier.replica=remote_runtime.match().replica_checkpoint(later);
    check(invalid_frontier.replica.matches(later)&&!runtime.match().owns_replica_checkpoint(invalid_frontier.replica),
        "Foreign replica control did not isolate the same receipt on a different owner");
    rejects([&]{history.remember(later,invalid_frontier);});
    invalid_frontier=frontier_for(later);invalid_frontier.replica=bound_runtime->frontier().replica;
    check(runtime.match().owns_replica_checkpoint(invalid_frontier.replica)&&invalid_frontier.replica.tick()==later.tick()&&
        !invalid_frontier.replica.matches(later),"Replica mismatch control did not preserve owner/tick while changing receipt");
    rejects([&]{history.remember(later,invalid_frontier);});
    const auto after_replica_rejection=history.state();
    check(after_replica_rejection.captured==before_replica_rejection.captured&&
        after_replica_rejection.evicted==before_replica_rejection.evicted&&
        after_replica_rejection.retained==before_replica_rejection.retained&&
        after_replica_rejection.newest_tick==before_replica_rejection.newest_tick&&
        after_replica_rejection.newest_sequence==before_replica_rejection.newest_sequence&&
        history.bind_runtime(*first_host_proof,*runtime.client()),
        "Missing/foreign/different-receipt replica changed completed history");
    invalid_frontier=frontier_for(later);invalid_frontier.previous=remote_origin;
    rejects([&]{history.remember(later,invalid_frontier);});
    invalid_frontier=frontier_for(later);invalid_frontier.local_slots.clear();
    rejects([&]{history.remember(later,invalid_frontier);});
    NativeEffectSounds wrong_sounds(*runtime.world().cache);
    invalid_frontier=frontier_for(later);invalid_frontier.pending_effect_sounds=wrong_sounds.checkpoint();
    rejects([&]{history.remember(later,invalid_frontier);});
    invalid_frontier=frontier_for(later);invalid_frontier.pending_effect_sounds=pending_sounds.checkpoint();
    rejects([&]{history.remember(later,invalid_frontier);});
    check(history.state().captured==1&&history.bind_runtime(*first_host_proof,*runtime.client()),
        "Rejected runtime frontier consumed or evicted the completed native cut");
    check(history.remember(later,frontier_for(later))&&longer.remember(later,frontier_for(later))&&history.bind(*first_host_proof,*runtime.client()),
        "History failed to find the older exact receipt behind a same-tick refresh");
    const auto newest=fresh_receipt();
    invalid_frontier=frontier_for(newest);invalid_frontier.previous=later;
    rejects([&]{history.remember(newest,invalid_frontier);});
    check(history.remember(newest,frontier_for(newest))&&longer.remember(newest,frontier_for(newest))&&
        !history.bind(*first_host_proof,*runtime.client())&&longer.bind(*first_host_proof,*runtime.client()),
        "Evicted exact receipt was replaced by a different body/sequence at the same tick");
    check(!history.bind_runtime(*first_host_proof,*runtime.client())&&longer.bind_runtime(*first_host_proof,*runtime.client()),
        "Runtime history rebound an evicted cut to a same-tick replacement receipt");
    const auto after_eviction=history.bind_runtime(*first_host_proof,*runtime.client(),reserved);
    check(after_eviction&&after_eviction->origin().same(candidate_origin)&&reserved.retained()==2&&reserved.overflowed()&&
        after_eviction->frontier().replica.matches(candidate_origin),
        "Reserved exact receipt did not survive eviction/overflow with bounded storage");
    check(history.state().retained==2&&history.state().captured==3&&history.state().evicted==1,
        "Completed receipt history exceeded its explicit retention policy");
    rejects([&]{history.remember(candidate_origin);});
    const auto after_failure=fresh_receipt();cpu_world.advance(effect_tick+1,tick_seconds);
    rejects([&]{longer.remember(after_failure,frontier_for(after_failure));});
    check(longer.state().captured==3&&longer.bind(*first_host_proof,*runtime.client())&&
        longer.bind_runtime(*first_host_proof,*runtime.client()),
        "Failed presentation capture evicted a previously completed cut");
    auto restored_effects=cpu_effects.prepare_restore(*bound_effect,ready,{},{});
    check(cpu_effects.commit_restore(std::move(restored_effects),ready)&&longer.remember(after_failure,frontier_for(after_failure)),
        "Completed effect cut could not restore after later invalid work");
    bool foreign_history_rejected{};std::thread history_foreign([&]{try{history.state();}catch(const FormatError &){foreign_history_rejected=true;}});history_foreign.join();
    check(foreign_history_rejected,"History exported executor-local state to another thread");
    bool foreign_reservation_rejected{};std::thread reservation_foreign([&]{try{reserved.valid();}
        catch(const FormatError &){foreign_reservation_rejected=true;}});reservation_foreign.join();
    check(foreign_reservation_rejected,"Reservation exported executor-local state to another thread");
    auto retired_context=before;++retired_context.epoch;history.context(retired_context);
    rejects([&]{history.remember(candidate_origin);});
    check(history.state().retained==0&&!history.bind(*first_host_proof,*runtime.client())&&bound_effect->cut().seal==ready.seal,
        "Context retirement retained stale history or invalidated the caller's owning checkpoint");
    check(!history.bind_runtime(*first_host_proof,*runtime.client())&&bound_runtime->current().tick==effect_tick&&
        bound_runtime->proof().matches(candidate_origin)&&bound_runtime->frontier().replica.matches(candidate_origin)&&
        bound_runtime->frontier().pending_effect_sounds.pending_count()==pending_cues.size(),
        "Context retirement discarded a caller-owned runtime cut or retained stale binding eligibility");
    check(!reserved.valid()&&!history.bind_runtime(*first_host_proof,*runtime.client(),reserved)&&
        reserved_runtime->proof().matches(candidate_origin),
        "Context retirement rebound a stale reservation or discarded an owning checkpoint");
    std::cout<<"PASS bounded early common-cut reservation: exact private proof, same-tick candidates, eviction/overflow, owner/executor/context rejection\n";
    std::cout<<"PASS atomic native receipt/runtime frontier: owned states/slots, full-salt bindings, deferred sound queues, exact proof, rejection/eviction/context controls\n";
    if(receiver)check(encode_source_authority(checkpoint(ready))==complete,"Native receiver differs from complete source world");
    // Same bytes and tick still count as unrecorded native mutation. A receipt
    // cannot silently certify later private executor work. Repair from the
    // original authenticated producer before allowing any readiness/election.
    if(receiver){
        auto &bank=receiver->world()->source_objects->memory();const auto original_bytes=bank.view(0x4c65f0u,8);
        bank.write(0x4c65f0u,original_bytes);rejects([&]{checkpoint(ready);});
        // The publisher is the sole receipt-queue consumer. Its completed
        // group, checked above against both independent native ACKs, retains
        // this producer observation after those queue entries are consumed.
        check(completed_fanout&&completed_fanout->context==before&&completed_fanout->seal==seal&&
            completed_fanout->recipients==std::uint16_t((1u<<candidate)|(1u<<remote)),
            "Original exact paired cut did not reach both native recipients and its producer");
        auto repair=paired;repair.serial=2;
        seal=sessions[old_host]->publish_authority(before,original->execution_tick(),encode_source_host_continuation(repair),fanout_recipients);
        wait([&]{update();remote_receiver->update();const auto s=state(),r=remote_receiver->state();
            check(s.error.empty(),s.error.c_str());check(r.error.empty(),r.error.c_str());
            return s.ready&&s.ready->seal==seal&&s.host_ready&&r.ready&&r.ready->seal==seal&&r.host_ready;},
            "Independent native recipients did not validate the exact repaired paired cut");
        ready=*state().ready;
        check(encode_source_authority(checkpoint(ready))==complete,"Repaired native cut differs from its producer");
        check(state().host_proof&&state().host_proof->cut().seal==ready.seal&&
            first_host_proof->cut().seal!=ready.seal,"Later validation overwrote the owning historical proof");
        check_origins(*state().host_proof);check_origins(*first_host_proof);
    }
    stage="retain independent surviving recipient cut";
    const auto remote_native=remote_receiver->state();
    check(remote_native.ready&&remote_native.ready->peer==remote&&remote_native.ready->context==before&&
        remote_native.ready->seal==seal&&remote_native.host_ready&&remote_native.host_proof&&
        remote_native.host_proof->cut().peer==remote&&remote_native.host_proof->cut().context==before&&
        remote_native.host_proof->cut().seal==seal&&
        remote_native.host_proof->prove_snapshot(survivor,remote_origin).matches(remote_origin),
        "Surviving recipient lacks its own exact native paired proof after candidate repair");
    remote_retained=remote_receiver->checkpoint_paired(*remote_native.ready);
    std::optional<SourceAuthorityRetainedWorld> remote_quiesced;
    wait([&]{remote_quiesced=remote_receiver->retain_validated_world();return bool(remote_quiesced);},
        "Independent surviving recipient executor did not quiesce at its validated cut");
    check(remote_retained&&remote_quiesced->cut.peer==remote_retained->cut.peer&&
        remote_quiesced->cut.context==remote_retained->cut.context&&remote_quiesced->cut.seal==remote_retained->cut.seal&&
        remote_quiesced->cut.peer==remote&&remote_quiesced->cut.context==before&&remote_quiesced->cut.seal==seal&&
        remote_quiesced->host&&remote_retained->host&&
        LanHost::encode_continuation(*remote_quiesced->host,{before,seal})==
            LanHost::encode_continuation(*remote_retained->host,{before,seal})&&
        remote_quiesced->local_views==sessions[remote]->local_slots()&&
        remote_quiesced->producer_views==sessions[old_host]->local_slots()&&
        remote_quiesced->admissions.players==admitted&&encode_source_authority(remote_quiesced->checkpoint)==complete,
        "Quiesced surviving recipient changed its seal, native bytes, admissions or independent views");
    remote_retained=std::move(remote_quiesced);remote_receiver.reset();
    std::cout<<"PASS independent surviving recipient retention: exact final paired cut and own native proof, repaired fanout when required, quiesced executor with strong owning export\n";
    const auto migration_effect_cut=longer.bind(*state().host_proof,*runtime.client());
    const auto migration_runtime_cut=longer.bind_runtime(*state().host_proof,*runtime.client());
    const auto migration_host_proof=state().host_proof;
    const auto remote_proof=migration_host_proof->prove_snapshot(survivor,remote_origin);
    check(migration_effect_cut&&migration_effect_cut->cut().seal==ready.seal&&migration_runtime_cut&&migration_runtime_cut->cut().seal==ready.seal,
        "Migration lost its exact receipt-bound effect cut before receiver retirement");
    check(remote_proof.matches(remote_origin)&&remote_proof.cut().seal==ready.seal,
        "Surviving client lost the exact migration seal or historical snapshot proof");
    PeerMigrationDecision invented;invented.previous=before;invented.value={candidate,seal};
    if(receiver)rejects([&]{receiver->prepare_peer_host_takeover(invented);});
    check(state().ready.has_value(),"Premature takeover consumed the validated native world");
    std::optional<SourceAuthorityRetainedWorld> retained;
    if(worker_retention){
        // Foreground recovery cannot join native destruction in its frame.
        rejects([&]{runtime.recover();});check(frame->finish(),"Recovery frame did not finish");frame.reset();
        std::shared_ptr<const SourceAuthorityRetainedWorld> value;
        wait([&]{value=runtime.recover();return bool(value);},"Runtime replay worker did not retire asynchronously");
        check(!runtime.shadow_state()&&runtime.recover()==value,"Recovery retained a mutable native executor or repeated its export");
        retained=*value;check(encode_source_authority(retained->checkpoint)==complete,"Retired native receiver differs from complete source world");
    }else wait([&]{retained=receiver->retain_validated_world();return bool(retained);},"Native receiver did not quiesce");
    check(retained->host.has_value(),"Actual retained native cut omitted its host sidecar");
    check_origins(*first_host_proof);
    // The cut remains the authentic old receipt. Publish and adopt a genuinely
    // newer completed native world so the survivor must roll its display and
    // decoded-state banks back together, rather than merely change endpoints.
    stage="advance actual survivor beyond retained native cut";
    std::array<Controls,multiplayer_capacity> later_controls{};
    for(const auto &p:roster)later_controls[p.slot].yaw=producer->match.unit(p.slot).yaw;
    producer->match.step(later_controls);lan_host->broadcast();
    MultiplayerSnapshotOrigin later_remote_origin,later_candidate_origin;
    wait([&]{survivor.receive();runtime.client()->receive();
        for(const auto &origin:survivor.received_origins())if(origin.tick()==effect_tick+1)later_remote_origin=origin;
        for(const auto &origin:runtime.client()->received_origins())if(origin.tick()==effect_tick+1)later_candidate_origin=origin;
        return later_remote_origin.context()==before&&later_candidate_origin.context()==before;
    },"Later actual native receipts did not arrive before host shutdown");
    adopt_replica(remote_runtime.match(),later_remote_origin);
    check(remote_runtime.match().replica_checkpoint(later_remote_origin).matches(later_remote_origin)&&
        !later_remote_origin.same(remote_origin)&&survivor.state_origin().same(later_remote_origin)&&
        survivor.state().tick==effect_tick+1&&remote_replica.matches(remote_origin)&&remote_replica.tick()==effect_tick&&
        remote_proof.matches(remote_origin),"Later survivor adoption lost or relabeled its retained native cut");
    const auto remote_later_collision=&remote_runtime.match().simulation().collision();
    const auto remote_later_scene=remote_runtime.match().simulation().scene().objects().data();
    const auto remote_later_scene_count=remote_runtime.match().simulation().scene().objects().size();
    const auto remote_later_groups=remote_runtime.match().simulation().devices().groups().data();
    auto wrong_tick_frontier=frontier_for(later_candidate_origin);wrong_tick_frontier.replica=bound_runtime->frontier().replica;
    const auto before_wrong_tick=longer.state();
    check(wrong_tick_frontier.replica.tick()!=later_candidate_origin.tick(),"Replica wrong-tick control stayed at its cut");
    rejects([&]{longer.remember(later_candidate_origin,wrong_tick_frontier);});
    check(longer.state().captured==before_wrong_tick.captured&&longer.state().evicted==before_wrong_tick.evicted&&
        longer.bind_runtime(*migration_host_proof,*runtime.client()),"Wrong-tick replica changed completed history or exact proof");
    stage="old host shutdown and peer agreement";
    lan_host.reset();original.reset();producer.reset();
    held_datagrams.reset();producer_options.online_match.reset();sessions[old_host].reset();
    // Validation above may take longer than500ms on the shared executor. Do
    // not advertise a migration candidate while that fixture host is paused;
    // publish the real retained cut only after its socket/session has closed.
    check(sessions[candidate]->prepare_peer_migration(retained->cut),"Actual retained native cut could not become ready");
    wait([&]{for(const auto &s:sessions)if(s){const auto state=s->peer_migration();if(!state||!state->ready[candidate])return false;}return true;},"Actual native readiness did not reach peers");
    wait([&]{healthy();return sessions[candidate]->peer_migration()->suspected;},"Host silence timeout");
    wait([&]{healthy();return sessions[candidate]->peer_migration()->decision&&sessions[remote]->peer_migration()->decision;},"Surviving peers did not agree");
    const auto decision=*sessions[candidate]->peer_migration()->decision;
    const auto remote_decision=*sessions[remote]->peer_migration()->decision;
    check(decision.previous==remote_decision.previous&&decision.next==remote_decision.next&&decision.value==remote_decision.value&&
        decision.value.host==candidate&&decision.value.cut==seal,"Peers chose conflicting native authority or cuts");
    stage="all-survivor exact-cut activation exclusion";
    unsigned premature_publications{};
    const auto premature_commit=[](void *count)noexcept{++*static_cast<unsigned *>(count);return true;};
    for(const auto peer:fanout_recipients){
        const auto agreement=sessions[peer]->peer_migration();const auto phase=sessions[peer]->phase();
        check(agreement&&agreement->decision&&agreement->ready[candidate]==std::optional{seal}&&!agreement->ready[remote],
            "Missing-survivor activation control already advertised remote native readiness");
        check(!sessions[peer]->finish_peer_migration(*agreement->decision,premature_commit,&premature_publications)&&
            !premature_publications&&sessions[peer]->phase()==phase&&sessions[peer]->authority_context()==before,
            "A majority decision entered native publication before every survivor retained its exact cut");
    }
    check(remote_retained&&remote_retained->cut.peer==remote&&remote_retained->cut.context==decision.previous&&
        remote_retained->cut.seal==decision.value.cut&&!sessions[remote]->prepare_peer_migration(retained->cut)&&
        sessions[remote]->prepare_peer_migration(remote_retained->cut),
        "Surviving recipient accepted another peer's proof or rejected its genuinely retained exact cut");
    wait([&]{healthy();for(const auto peer:fanout_recipients){const auto agreement=sessions[peer]->peer_migration();
        if(!agreement||!agreement->decision||agreement->ready[candidate]!=std::optional{seal}||
            agreement->ready[remote]!=std::optional{seal})return false;}return true;},
        "Every surviving session did not observe both independently retained exact-cut preparations");
    std::cout<<"PASS all-survivor activation guard: majority decision alone invokes no native callback, genuine remote retained proof required, both exact preparations observed before activation\n";
    auto forged=decision;++forged.value.cut.tick;
    // The direct-receiver route deliberately keeps its old frame outstanding
    // for the later staging exclusion control. Quiescence is checked before
    // certificates, so that route must reject the call rather than return false.
    if(frame)rejects([&]{runtime.prepare_takeover(forged);});
    else check(!runtime.prepare_takeover(forged),"Explicit wrong runtime certificate was admitted");
    check(!runtime.pending_takeover()&&runtime.client()&&!runtime.host(),
        "Rejected explicit runtime preparation changed native ownership");
    if(receiver)rejects([&]{receiver->prepare_peer_host_takeover(forged);});
    else rejects([&]{SourcePeerHostTakeover::prepare(recipient_options,*retained,forged);});
    check((!receiver||receiver->state().ready)&&sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,"Rejected cut consumed the candidate or reopened media");
    auto foreign_admissions=*retained;foreign_admissions.admissions.players.front().player^=1u;
    rejects([&]{SourcePeerHostTakeover::prepare(recipient_options,foreign_admissions,decision,std::make_shared<Clock>());});
    stage="native takeover transaction";
    std::unique_ptr<SourcePeerHostTakeover> takeover;
    SourcePeerHostTakeover *prepared_owner{};
    if(receiver){takeover=receiver->prepare_peer_host_takeover(decision);prepared_owner=takeover.get();}
    else {
        // A later menu selection cannot replace the recipe owned by the
        // running game. The standalone factory must reject that wrong profile.
        rejects([&]{SourcePeerHostTakeover::prepare(recipient_options,*retained,decision,std::make_shared<Clock>());});
        auto mismapped=*retained;mismapped.local_views.front()=sessions[remote]->local_slots().front();
        rejects([&]{SourcePeerHostTakeover::prepare(recipient_options,mismapped,decision,std::make_shared<Clock>());});
        wait([&]{return runtime.prepare_takeover(std::make_shared<Clock>());},"Foreground runtime did not prepare its learned takeover");
        prepared_owner=runtime.pending_takeover();
        check(prepared_owner&&runtime.prepare_takeover(std::make_shared<Clock>())&&runtime.pending_takeover()==prepared_owner,
            "Repeated preparation rebuilt or replaced the pending native owner");
        if(selected_mcc){
            check(same_cache_owner(prepared_owner->world().cache,runtime.world().cache)&&
                prepared_owner->world().source_objects.get()!=runtime.world().source_objects.get()&&
                &prepared_owner->world().tags!=&runtime.world().tags&&
                &prepared_owner->world().content!=&runtime.world().content,
                "Incoming runtime reconstruction did not share its original Cache with independent native/content owners");
            std::cout<<"PASS runtime incoming shared original Cache: actual resident and incoming control block, independent content/TagSystem/native World, retired replay worker\n";
        }
    }
    if(receiver){check(!receiver->world()&&!receiver->state().ready,"Receiver retained a second mutable native owner");
        rejects([&]{receiver->prepare_peer_host_takeover(decision);});}
    check(!recovery_observer_calls.load(),"Native recovery retained or invoked an external policy writer");
    check(prepared_owner->world().local_slots==sessions[candidate]->local_slots()&&prepared_owner->local().slots==prepared_owner->world().local_slots,
        "New native host lost local/remote couch ownership");
    rejects([&]{prepared_owner->host().broadcast();});
    for(const auto &p:roster)check(prepared_owner->world().match.player(p.slot).source_datum==admitted[p.slot].player&&
        prepared_owner->world().match.player(p.slot).deaths==paired.world.match.players[p.slot].deaths,"Takeover changed a native allocation or invented a death");
    check(prepared_owner->world().match.simulation().ticks()==seal.tick&&sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,
        "Native preparation advanced simulation or opened media");
    // An outstanding frame, including one without a recording publisher,
    // prevents replacement. Rejection must leave ownership with the caller.
    if(frame){
        rejects([&]{runtime.stage_takeover(std::move(takeover));});
        check(bool(takeover)&&&runtime.match()==resident_match&&runtime.client()&&!runtime.host(),
            "Rejected staging changed the runtime's borrowed world or transport");
        unsigned frame_hooks{};const auto unexpected=[](void *count)noexcept{++*static_cast<unsigned *>(count);return false;};
        rejects([&]{runtime.activate_takeover(unexpected,&frame_hooks);});
        check(!frame_hooks,"Outstanding native frame invoked a presentation hook");
        check(frame->finish()&&frame->finish(),"Runtime frame did not close exactly once");frame.reset();
    }
    check(!runtime.begin_frame(),"Fenced runtime admitted another native frame");
    auto *const staged=prepared_owner;if(takeover)runtime.stage_takeover(std::move(takeover));
    if(selected_mcc){
        auto &incoming=staged->world();const auto original_cache=incoming.cache;
        incoming.cache=remote_cache_owner;rejects([&]{(void)staged->world();});
        incoming.cache=std::shared_ptr<const Cache>(remote_cache_owner,original_cache.get());
        rejects([&]{(void)staged->world();});incoming.cache=original_cache;
        check(same_cache_owner(staged->world().cache,original_cache),"Incoming Cache-slot refusal lost its actual original owner");
    }
    staged->world().source_objects->inspect_render_memory([](const auto &,const auto &memory){
        check(Bytes(memory.view(0x531fbcu,4)).u32(0)==33,"Native promotion lost its query high-water count");
        source_require_retired_lens_queries(memory);
    });
    check(!runtime.promoted_camera()&&!runtime.local_input_pending(),"Staging exposed promoted camera/input ownership");
    check(!takeover&&runtime.pending_takeover()==staged&&&runtime.match()==resident_match&&runtime.client(),
        "Staging exposed a private native owner before adapter rebinding");
    rejects([&]{remote_runtime.prepare_client_rebind();});
    rejects([&]{remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica);});
    rejects([&]{remote_runtime.activate_client_rebind();});
    check(remote_frame->finish(),"Remote native frame did not close");remote_frame.reset();
    remote_runtime.receive();
    if(selected_mcc){
        auto scope_options=producer_options;scope_options.online_match=sessions[candidate];
        scope_options.selected_profiles=profiles[candidate];
        const auto resident_cache_owner=runtime.world().cache;
        stage="genuine current-target native publication scope";
        native_publication_controls(*staged,scope_options,*retained,decision,resident_cache_owner,worker_retention);
        stage="native takeover transaction";
    }
    check(survivor.state_origin().same(later_remote_origin)&&survivor.state().tick==effect_tick+1,
        "Draining the fenced survivor changed its last actual later receipt");
    const auto old_receive_count=survivor.received_states().size();
    auto old_prepared=survivor.prepare_rebind_host(remote_decision.previous,remote_decision.next,remote_decision.value.cut);
    check(!runtime.client()->commit_rebind_host(std::move(old_prepared))&&old_prepared&&survivor.can_commit_rebind_host(*old_prepared),
        "Foreign client consumed another native owner's prepared endpoint");
    check(!remote_runtime.begin_frame()&&remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica),
        "Fenced survivor did not stage its exact learned receipt/replica rebind");
    check(remote_runtime.pending_client_rebind()==remote_decision&&
        remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica)&&
        remote_runtime.client()==&survivor&&&remote_runtime.match()==remote_match&&survivor.received_states().size()==old_receive_count&&
        survivor.can_commit_rebind_host(*old_prepared)&&survivor.state_origin().same(later_remote_origin)&&
        &remote_runtime.match().simulation().collision()==remote_later_collision,
        "Staging changed the old client, world or accepted receive bank");
    check(foreign_remote_replica.matches(remote_origin)&&!remote_runtime.match().owns_replica_checkpoint(foreign_remote_replica),
        "Foreign pending-retry replica control did not isolate the exact receipt on another owner");
    rejects([&]{remote_runtime.prepare_client_rebind(remote_proof,remote_origin,foreign_remote_replica);});
    check(remote_runtime.pending_client_rebind()==remote_decision&&
        remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica)&&
        survivor.can_commit_rebind_host(*old_prepared)&&survivor.state_origin().same(later_remote_origin)&&
        &remote_runtime.match().simulation().collision()==remote_later_collision,
        "Foreign exact-receipt retry consumed or replaced the valid staged client rebind");
    rejects([&]{remote_runtime.match().replicate_devices(later_remote_origin.accepted_state().devices);});
    check(remote_runtime.pending_client_rebind()==remote_decision&&
        remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica)&&
        survivor.can_commit_rebind_host(*old_prepared)&&survivor.state_origin().same(later_remote_origin)&&
        &remote_runtime.match().simulation().collision()==remote_later_collision&&
        remote_runtime.match().simulation().scene().objects().data()==remote_later_scene&&
        remote_runtime.match().simulation().devices().groups().data()==remote_later_groups,
        "Rejected staged device mutation poisoned prepared or live replica owners");
    check(remote_runtime.cancel_client_rebind()&&!remote_runtime.pending_client_rebind()&&
        !remote_runtime.cancel_client_rebind()&&survivor.can_commit_rebind_host(*old_prepared)&&
        survivor.state_origin().same(later_remote_origin)&&survivor.state().tick==effect_tick+1&&
        survivor.received_states().size()==old_receive_count&&
        &remote_runtime.match().simulation().collision()==remote_later_collision&&
        remote_runtime.match().simulation().scene().objects().data()==remote_later_scene&&
        remote_runtime.match().simulation().scene().objects().size()==remote_later_scene_count&&
        remote_runtime.match().simulation().devices().groups().data()==remote_later_groups&&
        remote_runtime.match().replica_checkpoint(later_remote_origin).matches(later_remote_origin)&&
        sessions[remote]->phase()==OnlineMatchSession::Phase::Rebinding,
        "Cancelling client rebind changed live receipt/replica storage or retained staged barriers");
    check(remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica)&&
        remote_runtime.pending_client_rebind()==remote_decision&&
        remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica)&&
        survivor.can_commit_rebind_host(*old_prepared),
        "Cancelled certified client rebind could not stage the same exact cut again");
    survivor.send(neutral);
    check(remote_runtime.pending_client_rebind()==remote_decision&&
        remote_runtime.prepare_client_rebind(remote_proof,remote_origin,remote_replica)&&
        survivor.can_commit_rebind_host(*old_prepared)&&survivor.state_origin().same(later_remote_origin)&&
        &remote_runtime.match().simulation().collision()==remote_later_collision,
        "Live neutral survivor input invalidated the staged exact client rebind");
    runtime.receive();
    staged->refresh_local_input(*runtime.client());
    const auto carried=staged->local().received_presses,undelivered=staged->local().delivered_presses;
    staged->refresh_local_input(*runtime.client());
    check(staged->local().received_presses==carried&&staged->local().delivered_presses==undelivered,
        "Repeated post-cut input refresh changed action delivery");
    unsigned refused_adoptions{};
    const auto refuse_adoption=[](void *count)noexcept{++*static_cast<unsigned *>(count);return false;};
    check(!staged->activate(refuse_adoption,&refused_adoptions)&&refused_adoptions==1&&
        runtime.pending_takeover()==staged&&runtime.client()&&&runtime.match()==resident_match&&
        sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,
        "Refused native adoption published an epoch or consumed the old runtime owner");
    rejects([&]{staged->host().broadcast();});
    SourceMatchAuthority pending_native(staged->world().match,*staged->world().source_objects);
    const auto pending_boundary=pending_native.execution_boundary();
    // Actual authored resources are prepared before the declared material
    // freeze. No renderer/GPU readiness is inferred from this headless bank.
    SourceWorldParticleDrawsTu15 source_particles(*runtime.world().cache,runtime.world().content.objects);
    const auto live_effects=source_particles.preload_live_world(staged->world().source_objects->memory());
    for(const auto effect:live_effects)(void)source_particles.preload_first_person(effect);
    std::set<std::uint32_t> live_models;
    for(const auto &object:staged->world().source_objects->live_objects()){
        const auto model=runtime.world().cache->load_initial_model_definition(object.definition).render_model;
        if(model!=null_datum)live_models.insert(model);
    }
    check(!live_models.empty(),"Foreground binding control has no actual native render model");
    const std::vector<std::uint32_t> render_models(live_models.begin(),live_models.end());
    const auto light_definitions=cpu_world.light_definitions();
    SourceBspEffectLights resident_models(*runtime.world().cache,effect_mesh,0,light_definitions,render_models);
    SourceBspEffectLights missing_models(*runtime.world().cache,effect_mesh,0,light_definitions);
    const auto frozen_materials=effect_mesh.materials.size();
    if(selected_mcc)halo2::test::held_foreground_controls(*staged,*migration_runtime_cut,
        runtime.world().cache,effect_mesh,source_particles,resident_models,frozen_materials);
    MultiplayerObjectRuntime *particle_route{},*model_route{};
    unsigned old_collision_calls{};
    const ParticleCollisionQuery old_collision=[&](auto,auto,auto)->std::optional<ParticleCollisionHit>{++old_collision_calls;return {};};
    SourceForegroundWorldHandoff source_binding(runtime.world().cache,effect_mesh,source_particles,resident_models,
        frozen_materials,{true,true},particle_route,model_route,old_collision,old_collision);
    SourceForegroundWorldHandoff foreign_binding(runtime.world().cache,effect_mesh,source_particles,resident_models,
        frozen_materials,{true,true},particle_route,model_route);
    SourceForegroundWorldHandoff missing_binding(runtime.world().cache,effect_mesh,source_particles,missing_models,
        frozen_materials,{true,true},particle_route,model_route);
    rejects([&]{missing_binding.prepare_host(*staged,*migration_runtime_cut);});
    {auto cancelled=source_binding.prepare_host(*staged,*migration_runtime_cut);
        check(source_binding.ready(cancelled)&&!foreign_binding.ready(cancelled)&&!foreign_binding.commit(cancelled)&&
            !particle_route&&!model_route&&!old_collision_calls,"Prepared source route changed owners or accepted a foreign binder");}
    check(!particle_route&&!model_route&&pending_native.execution_boundary()==pending_boundary,
        "Cancelled source resource/routing preparation changed native or foreground ownership");
    auto source_handoff=source_binding.prepare_host(*staged,*migration_runtime_cut);
    auto superseded_handoff=source_binding.prepare_host(*staged,*migration_runtime_cut);
    particle_route=staged->world().source_objects.get();
    check(!source_binding.ready(source_handoff)&&!source_binding.commit(source_handoff),"Changed source routing retained commit eligibility");
    particle_route=nullptr;
    check(source_binding.ready(source_handoff)&&bool(source_handoff.muzzle_collision())&&bool(source_handoff.world_collision())&&
        !old_collision_calls&&pending_native.execution_boundary()==pending_boundary,
        "Source foreground preflight invoked collision/native work or lost its unchanged retry");
    struct PresentationAdoption {
        MultiplayerRuntimeAuthority &runtime;EffectPresentationContinuity &effects;
        EffectPresentationContinuity::PreparedRestore prepared;ValidatedAuthorityCut cut;
        SourceForegroundWorldHandoff &binding;SourceForegroundWorldHandoff::Prepared &source;
        unsigned calls{},published{};bool allow{},reentrancy_rejected{};
    } presentation{runtime,cpu_effects,cpu_effects.prepare_restore(*migration_effect_cut,ready,
        source_handoff.muzzle_collision(),source_handoff.world_collision()),ready,source_binding,source_handoff};
    const auto adopt_presentation=[](void *opaque)noexcept{
        auto &p=*static_cast<PresentationAdoption *>(opaque);++p.calls;
        if(!p.allow){
            // A forbidden runtime call must fail before it can reenter the
            // locked session. Only the deliberately rejected path probes it.
            try{(void)p.runtime.world();}catch(const FormatError &){p.reentrancy_rejected=true;}
            return false;
        }
        if(!p.effects.restore_ready(p.prepared,p.cut)||!p.binding.ready(p.source))return false;
        if(!p.effects.commit_restore(std::move(p.prepared),p.cut)||!p.binding.commit(p.source))std::terminate();
        ++p.published;return true;
    };
    check(!runtime.activate_takeover(adopt_presentation,&presentation)&&presentation.calls==1&&
        !presentation.published&&presentation.reentrancy_rejected&&
        cpu_effects.restore_ready(presentation.prepared,ready)&&runtime.pending_takeover()==staged&&
        runtime.client()&&&runtime.match()==resident_match&&!runtime.host()&&
        sessions[candidate]->phase()==OnlineMatchSession::Phase::Migrating,
        "Rejected runtime presentation published native ownership, consumed preparation or allowed reentrancy");
    bool foreign_activation_rejected{};std::thread foreign_activation([&]{
        try{runtime.activate_takeover(adopt_presentation,&presentation);}catch(const FormatError &){foreign_activation_rejected=true;}
    });foreign_activation.join();
    check(foreign_activation_rejected&&presentation.calls==1,"Foreign executor invoked a presentation hook");
    presentation.allow=true;
    check(runtime.activate_takeover(adopt_presentation,&presentation)&&presentation.calls==2&&presentation.published==1&&
        !runtime.activate_takeover(adopt_presentation,&presentation)&&presentation.calls==2,
        "Native activation omitted/repeated prepared presentation or committed twice");
    check(!cpu_effects.restore_ready(presentation.prepared,ready)&&pending_native.execution_boundary()==pending_boundary,
        "Runtime presentation was not consumed once or changed the sealed native world");
    check(particle_route==staged->world().source_objects.get()&&model_route==particle_route&&!source_binding.ready(source_handoff)&&
        !source_binding.ready(superseded_handoff)&&!source_binding.commit(source_handoff)&&!old_collision_calls&&
        effect_mesh.materials.size()==frozen_materials,"Source foreground routes were not published once with frozen resident resources");
    std::cout<<"PASS actual source foreground world handoff: native effect/model residency, explicit collision queries, cancellation/foreign/stale-route refusal, coupled CPU effect publication and retained retired queries without native mutation\n";
    check(runtime.promoted_camera()==&staged->camera()&&runtime.local_input_pending(),"Runtime lost the restored local camera/input bank");
    check(!runtime.prepare_takeover(),"Active host prepared a second replacement");
    rejects([&]{runtime.continue_local_input(controls);});
    check(!runtime.client()&&runtime.host()&&runtime.host()->peers()==1&&!runtime.pending_takeover(),"Replacement host lost its runtime role or surviving four-seat native peer");
    check(&runtime.match()==&staged->world().match&&&runtime.match()!=resident_match&&resident_cache->path()==runtime.world().cache->path(),
        "Runtime adoption changed immutable map resources or retained the old live Match");
    // The promoted host keeps the old host's lag-compensated hitscan: the
    // source shadow adopted its netcode mode and rewind owner from the cut.
    check(runtime.match().netcode_mode()==paired.world.match.netcode_mode&&
        bool(runtime.match().lag_compensation())==paired.world.match.lag_compensation.has_value()&&
        (!runtime.match().lag_compensation()||runtime.match().lag_compensation()->mode()==paired.world.match.netcode_mode),
        "Promoted host lost the old host's netcode mode or rewind owner");
    std::cout<<"Promoted host netcode "<<netcode_mode_name(runtime.match().netcode_mode())<<", rewind owner "
        <<(runtime.match().lag_compensation()?"installed":"absent")<<'\n';
    struct ClientPresentationAdoption {
        MultiplayerRuntimeAuthority &runtime;LanClient &client;LanClient::PreparedRebind &old;
        const MultiplayerMatch &match;const MultiplayerSnapshotOrigin &later;
        const CollisionWorld *collision;const SceneObjects::Object *scene;std::size_t scene_count;
        const SourceDeviceGroup *groups;
        unsigned calls{},published{};bool allow{},reentrancy_rejected{},old_endpoint_observed{};
    } client_presentation{remote_runtime,survivor,*old_prepared,remote_runtime.match(),later_remote_origin,
        remote_later_collision,remote_later_scene,remote_later_scene_count,remote_later_groups};
    const auto adopt_client_presentation=[](void *opaque)noexcept{
        auto &p=*static_cast<ClientPresentationAdoption *>(opaque);++p.calls;
        if(!p.client.can_commit_rebind_host(p.old)||!p.client.state_origin().same(p.later)||
            p.client.state().tick!=*p.later.tick()||&p.match.simulation().collision()!=p.collision||
            p.match.simulation().scene().objects().data()!=p.scene||
            p.match.simulation().scene().objects().size()!=p.scene_count||
            p.match.simulation().devices().groups().data()!=p.groups)return false;
        p.old_endpoint_observed=true;
        if(!p.allow){
            try{(void)p.runtime.world();}catch(const FormatError &){p.reentrancy_rejected=true;}
            return false;
        }
        ++p.published;return true;
    };
    check(!remote_runtime.activate_client_rebind(adopt_client_presentation,&client_presentation)&&
        client_presentation.calls==1&&!client_presentation.published&&client_presentation.reentrancy_rejected&&
        client_presentation.old_endpoint_observed&&remote_runtime.pending_client_rebind()==remote_decision&&
        survivor.can_commit_rebind_host(*old_prepared)&&survivor.received_states().size()==old_receive_count&&
        survivor.state_origin().same(later_remote_origin)&&survivor.state().tick==effect_tick+1&&
        &remote_runtime.match().simulation().collision()==remote_later_collision&&
        remote_runtime.match().simulation().scene().objects().data()==remote_later_scene&&
        remote_runtime.match().simulation().devices().groups().data()==remote_later_groups&&
        remote_runtime.client()==&survivor&&&remote_runtime.match()==remote_match&&
        sessions[remote]->phase()==OnlineMatchSession::Phase::Rebinding,
        "Rejected client presentation changed endpoint/receive bank/world, consumed preparation or allowed reentrancy");
    client_presentation.allow=true;
    check(remote_runtime.activate_client_rebind(adopt_client_presentation,&client_presentation)&&
        client_presentation.calls==2&&client_presentation.published==1&&
        !remote_runtime.activate_client_rebind(adopt_client_presentation,&client_presentation)&&client_presentation.calls==2&&
        !remote_runtime.pending_client_rebind()&&!remote_runtime.prepare_client_rebind()&&
        !remote_runtime.cancel_client_rebind()&&survivor.received_states().empty(),
        "Existing native client did not commit exactly once and clear old receive assemblies");
    check(remote_runtime.client()==&survivor&&&remote_runtime.match()==remote_match,"Client rebind replaced its admitted native owner or resident world");
    check(survivor.state().tick==effect_tick&&!survivor.state_origin().context()&&remote_proof.matches(remote_origin)&&
        remote_runtime.match().replica_checkpoint(remote_origin).matches(remote_origin)&&
        &remote_runtime.match().simulation().collision()==remote_cut_collision,
        "Client rebind did not atomically restore its proven old receipt and complete exact replica/collision bank");
    std::cout<<"PASS exact native client replica/receipt rollback before endpoint publication: later genuine tick, authenticated cut, scene/device/prop/collision banks, foreign retry rejection, live input, mutation barrier/cancellation/retry, refusal and same owner\n";
    rejects([&]{first_host_proof->prove_snapshot(survivor,remote_origin);});
    check(!survivor.can_commit_rebind_host(*old_prepared)&&!survivor.commit_rebind_host(std::move(old_prepared))&&old_prepared,
        "A stale prepared endpoint committed twice or was consumed on rejection");
    // The three PCs share this fixture executor. Continue their real native
    // traffic while the restarted background shadow warms; otherwise this
    // fixture itself stands the successor by long enough to expire its peer.
    // Hold the wait beyond the ordinary five-second host timeout so this
    // remains a regression even when the shadow warms quickly.
    stage="post-rebind native heartbeat during shadow warmup";
    const auto warm_start=std::chrono::steady_clock::now();
    const auto warm_tick=runtime.match().simulation().ticks();
    const auto warm_received=staged->local().received_presses,warm_delivered=staged->local().delivered_presses;
    const auto heartbeat=[&]{
        std::array<Controls,16> idle_controls{};
        survivor.send(neutral);runtime.host()->receive(idle_controls);runtime.host()->broadcast();survivor.receive();
        if(runtime.host()->peers()!=1||!survivor.error().empty()){
            const auto removal=runtime.host()->last_peer_removal();
            std::cerr<<"DIAGNOSTIC post-rebind heartbeat: peers="<<runtime.host()->peers()
                <<" client_error_present="<<!survivor.error().empty()<<" removal_present="<<bool(removal);
            if(removal)std::cerr<<" idle_timeout="<<(removal->reason==LanHost::PeerRemovalReason::idle_timeout)
                <<" idle_age_ns="<<removal->idle_age_ns<<" accepted_inputs="<<removal->input_acceptances
                <<" last_input_age_ns="<<removal->last_accepted_input_age_ns.value_or(-1);
            std::cerr<<'\n';
        }
        check(runtime.host()->peers()==1&&survivor.error().empty(),"Native peer expired during serviced background warmup");
        check(runtime.match().simulation().ticks()==warm_tick&&staged->local().received_presses==warm_received&&
            staged->local().delivered_presses==warm_delivered,"Background heartbeat stepped the world or consumed carried local input");
        for(auto slot:survivor.slots()){const auto &c=idle_controls[slot];
            check(!c.jump&&!c.fire&&!c.reload&&!c.fire_secondary&&!c.pickup&&!c.dual_wield&&!c.grenade&&
                !c.switch_weapon&&!c.melee&&!c.switch_grenade,"Neutral background heartbeat injected an action");}
    };
    wait([&]{heartbeat();remote_runtime.service();heartbeat();const auto shadow=remote_runtime.shadow_state();
        return std::chrono::steady_clock::now()-warm_start>=6s&&shadow&&shadow->error.empty()&&shadow->warmed&&shadow->workspace_warmed;},
        "Rebound survivor did not restart its failed old shadow on the new generation");
    heartbeat();const auto publish_start=std::chrono::steady_clock::now();runtime.service();
    const auto publish_elapsed=std::chrono::steady_clock::now()-publish_start;heartbeat();
    std::cout<<"PASS native post-rebind background heartbeat beyond five-second timeout: unchanged native tick/carried input; warm_ms="
        <<std::chrono::duration_cast<std::chrono::milliseconds>(publish_start-warm_start).count()
        <<" publish_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(publish_elapsed).count()<<'\n';
    stage="resumed native snapshots and encoded voice";
    auto &world=runtime.world();const auto start=world.match.simulation().ticks();unsigned voice_packets{};
    for(unsigned tick=0;tick<12;++tick){auto scope=runtime.begin_frame();check(bool(scope),"Adopted runtime remained fenced");
        controls={};check(runtime.continue_local_input(controls)==(tick<4),"Retained pending input was omitted or repeated");
        if(tick<4){
            check(runtime.local_input_pending(),"Retained input released fresh hardware before its native step");
            for(unsigned view=0;view<world.local_slots.size();++view){const auto &c=controls[world.local_slots[view]];
                check(c.yaw==late[view].yaw&&c.pitch==late[view].pitch&&c.zoom==late[view].zoom&&
                    c.zoom_reset_serial==late[view].zoom_reset_serial&&c.forward==late[view].forward&&
                    c.right==late[view].right&&c.crouch==late[view].crouch,
                    "Post-cut local movement/look/zoom state changed during delivery");
                check(c.pickup==(tick==0)&&c.pickup_milliseconds==(tick==0?600u:0u),
                    "Post-cut completed interaction was lost or delivered twice");
                check(c.vehicle_interaction_target&&c.vehicle_interaction_target->action==0&&
                    c.vehicle_interaction_target->index==0&&c.vehicle_interaction_target->object==null_datum,
                    "Post-cut explicit neutral interaction target changed");}
            for(auto slot:world.local_slots)check(controls[slot].switch_grenade,"Imported couch pulse did not reach its native tick");
            const auto delivered=staged->local().delivered_presses;
            auto repeated=controls;check(runtime.continue_local_input(repeated),"Repeated retained-input poll changed ownership");
            check(staged->local().delivered_presses==delivered,"Repeated retained-input poll consumed a second queued pulse");
            for(auto slot:world.local_slots)check(repeated[slot].yaw==controls[slot].yaw&&repeated[slot].zoom==controls[slot].zoom,
                "Repeated retained-input poll changed its current native tick");
        }
        if(tick>=4)for(auto slot:world.local_slots)controls[slot].yaw=world.match.unit(slot).yaw;
        survivor.send(neutral);runtime.host()->receive(controls);runtime.match().step(controls);runtime.host()->broadcast();
        check(runtime.local_input_pending()==(tick<3),"Imported input released hardware early or failed to release after its final native step");
        sessions[candidate]->voice(remote,GameVoiceFrame{candidate,tick+1,false,{1,1},{0x60}});
        wait([&]{survivor.receive();check(survivor.error().empty(),survivor.error().c_str());return survivor.state().tick==world.match.simulation().ticks();},"Surviving native client did not follow successor snapshots");
        voice_packets+=sessions[remote]->voice_packets().size();
        check(std::ranges::equal(survivor.slots(),sessions[remote]->local_slots()),"Existing remote couch slots changed after host loss");
    }
    wait([&]{voice_packets+=sessions[remote]->voice_packets().size();return voice_packets==12;},"Encoded direct voice did not resume with native host");
    check(world.match.simulation().ticks()==start+12&&!world.match.finished(),"Native migrated match did not survive");
    const auto successor=capture_source_host_continuation(*runtime.host(),decision.next,2);
    check(!encode_source_host_continuation(successor).empty(),"Migrated native host cannot publish its next paired cut");
    std::cout<<"PASS actual paired native UDP cut, independent twelve-player allocation admission, two-survivor majority after host session shutdown, transactional four-seat view/input/camera promotion, retained departures/results, existing four-seat client rebind, twelve connected native ticks and twelve encoded direct voice packets\n";
    std::cout<<(worker_retention?"PASS foreground runtime retires the replay executor, prepares/stages its owning native value once, rejects wrong profiles/views, preserves its gameplay recipe across menu changes and retains no presentation borrower\n":"PASS same-tick unrecorded native mutation rejected and repaired before readiness\n");
    std::cout<<"PASS runtime completed-frame lease, rejection without ownership loss, private staging, atomic Match/transport adoption and resident cache lifetime\n";
    std::cout<<"PASS runtime prepared presentation hooks: host refusal/retry, exact receipt-bound four-owner cold CPU effect publication, unchanged native seal, client callback before endpoint rebind, once-only publication and rejected runtime reentrancy/foreign executor\n";
    std::cout<<"PASS restored runtime camera ownership, three pre-cut and two post-cut owning couch pulses, four retained native input ticks, latest movement/look/zoom, one completed interaction, same-tick idempotence and fresh-input ownership after stepping\n";
    std::cout<<"PASS surviving runtime staged its local decision without changing the native endpoint, committed client rebind before transport publication, retained the same client/world and restarted a failed old shadow\n";
    std::cout<<"PASS 33 declared pending CPU lens requests cross the paired cut and retire through original20E30 before native adoption; no predecessor GPU samples are fabricated\n";
    std::cout<<"PASS receiver-only owning paired proof, exact four-seat client receipts, foreign/pre-admission rejection, later-cut/worker-retirement lifetime and old-generation rejection after rebind\n";
    std::cout<<"PASS bounded completed CPU effect history: exact old receipt behind same-tick refreshes, eviction refusal, duplicate idempotence, failed-capture preservation, restored group, executor confinement and independent context retirement\n";
    if(selected_mcc)std::cout<<"PASS selected-MCC-native peer takeover scope route: genuine held controls followed by original successful activation, twelve connected native ticks and direct voice\n";
    std::cout<<"LIMIT in-process executors with three real direct sessions; internal migration gate, automatic measured candidate choice, explicit native activation, no renderer/controller/headset/WAN or default runtime acceptance\n";
    return 0;
}catch(const std::exception &e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
