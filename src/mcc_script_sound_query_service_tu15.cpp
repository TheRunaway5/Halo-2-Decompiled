#include "halo2/mcc_script_sound_query_service_tu15.hpp"
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_script_sound_clock_native64_tu15.hpp"
#include <thread>

namespace halo2 {
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
}
struct MccScriptSoundQueryServiceTu15::Impl {
    struct Generation {
        MccSoundVariationProvider provider;
        MccScriptSoundWorldAccess access;
        std::shared_ptr<SourceScriptSoundClockNative64Tu15> clock;
    };
    const Cache* cache;
    std::shared_ptr<PhysicsLiveWorldTu15> world;
    std::shared_ptr<const Generation> current;
    const std::thread::id thread=std::this_thread::get_id();
    bool active=false,closed=false;
    Impl(const Cache& c,std::shared_ptr<PhysicsLiveWorldTu15> w):cache(&c),world(std::move(w)){}
    ~Impl(){if(world&&thread!=std::this_thread::get_id())std::terminate();}
    struct Operation {
        Impl& owner;
        explicit Operation(Impl& o):owner(o){
            // Check immutable creating-thread identity before mutable state.
            require(owner.thread==std::this_thread::get_id(),"Script sound query service requires its creating thread");
            require(!owner.active,"Script sound query service cannot reenter its operation");
            require(!owner.closed,"Script sound query service is closed");owner.active=true;
        }
        ~Operation(){owner.active=false;}
    };
    void expected(const Cache& c)const{require(&c==cache,"Script sound query service belongs to another Cache");}
    std::shared_ptr<const Generation> admitted(const Cache& c)const{
        expected(c);const auto generation=current;
        require(bool(generation),"Script sound query service has no current generation");
        generation->access.require_metadata_owner(c,generation->provider);
        require(generation->access.clock_rows(c)==generation->clock->rows(),
            "Script sound query metadata changed; explicit refresh required");
        return generation;
    }
    static std::int32_t stored(const std::shared_ptr<Impl>& retained,std::uint32_t sound){
        const auto owner=retained;Operation operation(*owner);const auto generation=owner->admitted(*owner->cache);
        const auto& access=generation->access;const auto& clock=generation->clock;const auto& c=*owner->cache;
        const auto fp=access.floating_point_context(c);const auto maximum=access.stored_milliseconds(c,{sound});
        const auto matched=source_script_sound_clock_find_first_tu15(clock->rows(),sound);
        const auto now=matched?access.time(c):SourceScriptSoundClockTimeTu15{};
        const auto returned=access.time(c);
        return clock->stored_remaining(sound,maximum,now,returned,fp).value;
    }
    static std::int32_t explicit_time(const std::shared_ptr<Impl>& retained,std::uint32_t sound,std::int32_t ticks){
        const auto owner=retained;Operation operation(*owner);const auto generation=owner->admitted(*owner->cache);
        const auto& access=generation->access;const auto& clock=generation->clock;const auto& c=*owner->cache;
        const auto fp=access.floating_point_context(c);const auto matched=source_script_sound_clock_find_first_tu15(clock->rows(),sound);
        const auto now=matched?access.time(c):SourceScriptSoundClockTimeTu15{};
        const auto game=clock->explicit_remaining(sound,ticks,now,fp);
        const auto returned=access.time(c);
        return source_script_sound_clock_hs_explicit_tail_tu15(game.value,returned.delta_time_bits,fp);
    }
    static std::int32_t language(const std::shared_ptr<Impl>& retained,std::uint32_t sound){
        const auto owner=retained;Operation operation(*owner);const auto generation=owner->admitted(*owner->cache);
        const auto& access=generation->access;const auto& c=*owner->cache;
        return generation->clock->language_remaining(sound,access.policy(c),access.floating_point_context(c)).value;
    }
};
MccScriptSoundQueryServiceTu15::MccScriptSoundQueryServiceTu15(std::shared_ptr<Impl> state):impl_(std::move(state)){}
MccScriptSoundQueryServiceTu15 MccScriptSoundQueryServiceTu15::create(const Cache& c,std::shared_ptr<PhysicsLiveWorldTu15> world){
    require(bool(world),"Script sound query service requires its actual World owner");
    MccScriptSoundQueryServiceTu15 service(std::make_shared<Impl>(c,std::move(world)));service.refresh(c);return service;
}
void MccScriptSoundQueryServiceTu15::require_current(const Cache& c)const{
    const auto owner=impl_;Impl::Operation operation(*owner);(void)owner->admitted(c);
}
SourceScriptSoundClockRowsTu15 MccScriptSoundQueryServiceTu15::rows(const Cache& c)const{
    const auto owner=impl_;Impl::Operation operation(*owner);return owner->admitted(c)->clock->rows();
}
SourceScriptSoundClockQueriesTu15 MccScriptSoundQueryServiceTu15::queries(const Cache& c)const{
    const auto owner=impl_;Impl::Operation operation(*owner);(void)owner->admitted(c);
    return {[owner](auto sound){return Impl::stored(owner,sound);},
        [owner](auto sound,auto ticks){return Impl::explicit_time(owner,sound,ticks);},
        [owner](auto sound){return Impl::language(owner,sound);}};
}
void MccScriptSoundQueryServiceTu15::refresh(const Cache& c)const{
    const auto owner=impl_;Impl::Operation operation(*owner);owner->expected(c);
    // The strong actual World owner remains valid when its previous epoch is
    // stale. It is never recovered by dereferencing a borrowed retired Access.
    auto provider=owner->world->sound_variation_provider(c);
    auto access=owner->world->script_sound_access(c);access.require_metadata_owner(c,provider);
    const auto rows=access.clock_rows(c);
    auto clock=std::make_shared<SourceScriptSoundClockNative64Tu15>();clock->initialize_metadata(rows);
    access.require_metadata_owner(c,provider);
    require(access.clock_rows(c)==rows,"Script sound query rows changed during fresh import");
    auto next=std::make_shared<const Impl::Generation>(Impl::Generation{std::move(provider),std::move(access),std::move(clock)});
    owner->current=std::move(next);
}
void MccScriptSoundQueryServiceTu15::close()const{
    const auto owner=impl_;Impl::Operation operation(*owner);owner->closed=true;
    owner->current.reset();owner->world.reset();
}
} // namespace halo2
