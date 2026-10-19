#include "halo2/source_foreground_world_handoff.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <limits>
#include <utility>

namespace halo2 {
namespace {
void need(bool value){if(!value)throw FormatError("Invalid foreground world handoff owner, cut or resident resources");}
bool same_resources(const SourceWorldParticleDrawsTu15::ResourceState &a,const SourceWorldParticleDrawsTu15::ResourceState &b){
    return a.materials==b.materials&&a.world_systems==b.world_systems&&a.first_person_systems==b.first_person_systems;
}
ParticleCollisionQuery target_collision(MultiplayerPreparedWorld &world){
    return [&world](std::uint32_t flags,Vec3 origin,Vec3 delta)->std::optional<ParticleCollisionHit>{
        if(world.source_objects&&world.source_objects->owns_physics())return world.source_objects->particle_collision(flags,origin,delta);
        const auto distance=length(delta);if(!(distance>0))return {};
        const auto hit=world.match.simulation().collision().raycast(origin,delta,distance);if(!hit)return {};
        return ParticleCollisionHit{hit->distance/distance,hit->position,hit->normal,true,std::int16_t(hit->material)};
    };
}
}
struct SourceForegroundWorldHandoff::Data {
    std::shared_ptr<const int> owner;
    std::uint64_t revision{};
    MultiplayerPreparedWorld *target{};
    const Cache *target_cache{};
    MultiplayerObjectRuntime *native{},*previous_particle{},*previous_model{},*next_particle{},*next_model{};
    std::array<std::uint64_t,2> mutations{};
    std::uint64_t tick{};
    ParticleCollisionQuery muzzle,world;
};
SourceForegroundWorldHandoff::SourceForegroundWorldHandoff(std::shared_ptr<const Cache> cache,const Mesh &mesh,
    const SourceWorldParticleDrawsTu15 &particles,const SourceBspEffectLights &models,std::size_t materials,Policy policy,
    MultiplayerObjectRuntime *&particle,MultiplayerObjectRuntime *&model,ParticleCollisionQuery muzzle,ParticleCollisionQuery world)
    :cache_(std::move(cache)),mesh_(mesh),particles_(particles),models_(models),materials_(materials),
     model_sections_(models.resident_model_sections()),particle_resources_(particles.resource_state()),policy_(policy),
     particle_slot_(particle),model_slot_(model),muzzle_collision_(std::move(muzzle)),world_collision_(std::move(world)){
    need(cache_&&cache_->original_data()&&cache_->original_data()->image==OriginalData::Image::TitleUpdate15&&
        &particles_.resident_cache()==cache_.get()&& &models_.resident_cache()==cache_.get()&& &models_.resident_mesh()==&mesh_&&
        mesh_.materials.size()==materials_&&(!particle_slot_||policy_.particles)&&(!model_slot_||policy_.models)&&
        (!particle_slot_||!model_slot_||particle_slot_==model_slot_));
}
SourceForegroundWorldHandoff::~SourceForegroundWorldHandoff()=default;
SourceForegroundWorldHandoff::Prepared::Prepared()=default;
SourceForegroundWorldHandoff::Prepared::~Prepared()=default;
SourceForegroundWorldHandoff::Prepared::Prepared(Prepared &&)noexcept=default;
SourceForegroundWorldHandoff::Prepared &SourceForegroundWorldHandoff::Prepared::operator=(Prepared &&)noexcept=default;
const ParticleCollisionQuery &SourceForegroundWorldHandoff::Prepared::muzzle_collision()const{need(data_&&data_->owner);return data_->muzzle;}
const ParticleCollisionQuery &SourceForegroundWorldHandoff::Prepared::world_collision()const{need(data_&&data_->owner);return data_->world;}
void SourceForegroundWorldHandoff::own()const{need(std::this_thread::get_id()==executor_);}
bool SourceForegroundWorldHandoff::resources_unchanged()const noexcept {
    return mesh_.materials.size()==materials_&&models_.resident_model_sections()==model_sections_&&
        same_resources(particles_.resource_state(),particle_resources_);
}
SourceForegroundWorldHandoff::Prepared SourceForegroundWorldHandoff::prepare_host(SourcePeerHostTakeover &candidate,
    const EffectPresentationHistory::RuntimeCheckpoint &checkpoint)const {
    own();const auto &decision=candidate.decision();const auto &cut=checkpoint.cut();
    need(cut.peer==decision.value.host&&cut.context==decision.previous&&cut.seal==decision.value.cut);
    return prepare(candidate.world(),checkpoint,true);
}
SourceForegroundWorldHandoff::Prepared SourceForegroundWorldHandoff::prepare_client(MultiplayerPreparedWorld &world,
    const EffectPresentationHistory::RuntimeCheckpoint &checkpoint)const {
    own();need(!world.source_objects&&world.match.owns_replica_checkpoint(checkpoint.frontier().replica));
    return prepare(world,checkpoint,false);
}
SourceForegroundWorldHandoff::Prepared SourceForegroundWorldHandoff::prepare(MultiplayerPreparedWorld &target,
    const EffectPresentationHistory::RuntimeCheckpoint &checkpoint,bool native)const {
    own();const auto &cut=checkpoint.cut();
    need(resources_unchanged()&&revision_!=std::numeric_limits<std::uint64_t>::max()&&target.cache&&
        target.cache->path()==cache_->path()&&target.cache->shared_directory()==cache_->shared_directory()&&
        target.cache->original_data()&&target.cache->original_data()->image==OriginalData::Image::TitleUpdate15&&
        target.cache->original_data()->source==cache_->original_data()->source&&
        checkpoint.proof().matches(checkpoint.origin())&&checkpoint.frontier().replica.matches(checkpoint.origin())&&
        checkpoint.origin().tick()==cut.seal.tick);
    Prepared out;out.data_=std::make_unique<Data>();auto &p=*out.data_;
    p.owner=identity_;p.revision=revision_;p.target=&target;p.target_cache=target.cache.get();
    p.previous_particle=particle_slot_;p.previous_model=model_slot_;p.tick=target.match.simulation().ticks();
    if(native){
        need(target.source_objects&&target.source_objects->owns_physics()&&p.tick==cut.seal.tick);
        p.native=target.source_objects.get();p.mutations=p.native->execution_mutations();
        // Read only actual occupied generations. No native resource loading,
        // world/model-view entry, constructors or RNG consumption is allowed.
        if(policy_.particles)(void)particles_.require_live_world_resident(p.native->memory());
        if(policy_.models)for(const auto &object:p.native->live_objects()){
            const auto model=cache_->load_initial_model_definition(object.definition).render_model;
            if(model==null_datum)continue;
            const auto metadata=cache_->load_source_tag_metadata(model);
            const Bytes root(metadata.bytes_at(metadata.source_address(),0x2c));
            for(unsigned section=0;section<root.u32(0x24);++section){
                const Bytes row(metadata.bytes_at(root.u32(0x28)+section*92u,92));
                if(row.u16(4)&&!models_.resident_model_section(model,section))
                    throw FormatError("Target native model section was not resident before renderer upload");
            }
        }
        need(p.native->execution_mutations()==p.mutations&&resources_unchanged());
        p.next_particle=policy_.particles?p.native:nullptr;p.next_model=policy_.models?p.native:nullptr;
    }
    p.muzzle=target_collision(target);p.world=target_collision(target);return out;
}
bool SourceForegroundWorldHandoff::ready(const Prepared &prepared)const noexcept {
    try{
        const auto *p=prepared.data_.get();
        if(std::this_thread::get_id()!=executor_||!p||p->owner!=identity_||p->revision!=revision_||
            !resources_unchanged()||particle_slot_!=p->previous_particle||model_slot_!=p->previous_model||
            !p->target||p->target->cache.get()!=p->target_cache||p->target->source_objects.get()!=p->native||
            p->target->match.simulation().ticks()!=p->tick)return false;
        return !p->native||p->native->execution_mutations()==p->mutations;
    }catch(...){return false;}
}
bool SourceForegroundWorldHandoff::commit(Prepared &prepared)noexcept {
    if(!ready(prepared))return false;auto &p=*prepared.data_;
    particle_slot_=p.next_particle;model_slot_=p.next_model;
    muzzle_collision_.swap(p.muzzle);world_collision_.swap(p.world);++revision_;p.owner.reset();return true;
}
}
