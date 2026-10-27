#include "halo2/source_effect_frame_tu15.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_particle_render_state_tu15.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include "halo2/particle_movement.hpp"
#include "halo2/particle_vertex.hpp"
#include "halo2/particle_view.hpp"
#include "halo2/particle_model.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {return Bytes(m.view(p,4)).u32(0);}
void put(PhysicsLiveWorldTu15 &world,std::uint32_t p,std::uint32_t value) {world.write_source_call_input(p,std::as_bytes(std::span(&value,1)));}
Bytes block(const Cache &cache,const Bytes &root,unsigned offset,unsigned stride) {
    const auto d=read_tag_block_descriptor(root,offset);const auto bytes=d.extent(stride,65536);
    if(!d.count)return Bytes(std::span<const std::byte>{});
    for(const auto &owner:cache.select_tags({.contains=TagAddressRange{d.address,bytes},.require_address=true,.minimum_size=1})) {
        (void)cache.tag_payload(owner.handle());return Bytes(cache.read_tag_bytes(d.address,bytes));
    }
    throw FormatError("Source swarm draw block has no retained metadata owner");
}
Bytes particle_system_rows(const Cache &cache,std::uint32_t owner,std::uint32_t event) {
    const auto tag=cache.query_tag({.handle=TagHandle{owner},.require_address=true});
    if(!tag)throw FormatError("Live particle parent has no actual Cache identity");
    const auto metadata=cache.load_source_tag_metadata(owner);
    if(tag->group=="prt3" && event==none) {
        // Original13A6CA selects48603C; slots18/1C (22AC90/22ACA0)
        // read this parent's +84/+88 child-system descriptor, stride38.
        const Bytes root(metadata.bytes_at(metadata.source_address(),0x8c));
        return block(cache,root,0x84,0x38);
    }
    if(tag->group=="effe") {
        const Bytes root(metadata.bytes_at(metadata.source_address(),0x1c));
        if(event>=root.u32(0x14))throw FormatError("Live particle system has no authored event");
        const auto events=block(cache,root,0x14,0x38);
        return block(cache,events,event*0x38+0x30,0x38);
    }
    throw FormatError("Live particle parent group/event requires its original resource dispatch");
}
std::uint32_t identity(const SourceRuntimeMemory &m,std::uint32_t global,std::uint32_t pointer,unsigned stride) {
    const auto pool=word(m,global),first=word(m,pool+0x44),high=word(m,pool+0x38);
    if(word(m,pool+0x24)!=stride || high>word(m,pool+0x20) || pointer<first ||
        (pointer-first)%stride || (pointer-first)/stride>=high)
        throw FormatError("Source particle command lost its actual pool pointer");
    const auto index=(pointer-first)/stride;
    return std::uint32_t(Bytes(m.view(pointer,2)).u16(0))*65536u+index;
}
struct LiveParticleCensus {
    std::set<std::uint32_t> definitions;
    std::vector<std::tuple<std::uint32_t,std::uint32_t,std::uint16_t>> systems;
};
LiveParticleCensus live_particle_census(const SourceRuntimeMemory &m){
    LiveParticleCensus out;
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
    const auto inspect=[&](std::uint32_t global,unsigned stride,unsigned definition_offset) {
        const auto pool=word(m,global);
        if(!pool)throw FormatError("Live particle resource census lacks its original pool owner");
        const auto capacity=word(m,pool+0x20u),high=word(m,pool+0x38u);
        const auto rows=word(m,pool+0x44u),bits=word(m,pool+0x48u);
        if(!capacity || capacity>65536u || high>capacity || word(m,pool+0x24u)!=stride)
            throw FormatError("Live particle resource census lost its actual pool layout");
        (void)m.view(rows,std::size_t(capacity)*stride);
        (void)m.view(bits,((capacity+31u)/32u)*4u);
        for(unsigned index=0;index<high;++index) {
            if(!(word(m,bits+(index/32u)*4u)&(1u<<(index%32u))))continue;
            const auto body=rows+index*stride;
            const auto salt=Bytes(m.view(body,2)).u16(0);
            const auto id=(std::uint32_t(salt)<<16)|index;
            if(!salt || source_raw_datum_lookup(pool,id,storage)!=body)
                throw FormatError("Live particle resource census lost a full allocation identity");
            const auto definition=word(m,body+definition_offset);
            const auto tag=word(m,0x4f0414u)+(definition&65535u)*16u;
            const auto group=definition==none?0u:word(m,tag);
            const bool typed=group==0x65666665u ||
                (global==0x51df74u && group==0x70727433u && word(m,body+0x14u)==none);
            if(definition==none || !typed || word(m,tag+4u)!=definition) {
                std::ostringstream message;
                message<<"Live particle resource census lost its full authored effect identity"
                    <<std::hex<<" [pool_global="<<global<<" allocation="<<id
                    <<" body="<<body<<" definition_offset="<<definition_offset
                    <<" definition="<<definition;
                if(definition!=none)try {
                    message<<" tag_group="<<word(m,tag)<<" tag_handle="<<word(m,tag+4u);
                }catch(const std::exception &error){message<<" tag_read_error="<<error.what();}
                message<<']';
                throw FormatError(message.str());
            }
            out.definitions.insert(definition);
            if(global==0x51df74u)out.systems.emplace_back(definition,word(m,body+0x14u),Bytes(m.view(body+0xeu,2)).u16(0));
        }
    };
    // A retained system can outlive its effect; both original pools remain
    // authoritative. The same scan serves preload and read-only admission.
    inspect(0x4f720cu,400u,4u);inspect(0x51df74u,84u,0x10u);return out;
}
}
struct SourceWorldParticleDrawsTu15::Impl {
    struct Resource {
        ParticleSpriteDefinition sprite;
        SourceParticleSpriteContext context;
        std::shared_ptr<const ParticleModelDefinition> model;
        std::shared_ptr<const ParticleMaterial> material;
    };
    bool lens_requests{};
    const Cache &cache;
    std::set<std::uint32_t> effects;
    std::set<std::uint32_t> preloaded_world_effects;
    std::set<std::uint32_t> inspected_world_effects;
    std::set<std::tuple<std::uint32_t,std::uint32_t,std::uint16_t>> systems;
    std::set<std::uint32_t> first_person_effects;
    std::set<std::tuple<std::uint32_t,std::uint32_t,std::uint16_t>> first_person_systems;
    std::map<std::uint32_t,Resource> resources;
    Resource load_resource(std::uint32_t datum) {
        const auto tag=cache.query_tag({.handle=TagHandle{datum},.require_address=true});
        if(!tag)throw FormatError("Source particle resource lost its actual Cache identity");
        Resource resource;
        if(tag->group=="PRTM") {
            resource.model=std::make_shared<const ParticleModelDefinition>(load_particle_model(cache,datum));
        }else {
            resource.sprite=cache.load_particle_sprite(datum);
            const auto context=source_particle_sprite_context(cache,datum,resource.sprite);
            if(!context)throw FormatError("Source particle has no admitted bitmap data");
            resource.context=*context;
            if(!context->has_bitmap)resource.sprite.bitmap_type=-1;
        }
        resource.material=std::make_shared<const ParticleMaterial>(
            cache.load_particle_material(datum,15,0,resource.model?1:0));
        const auto type=resource.material->vertex.classification;
        if(resource.model ? type!=1 : type!=7 && type!=8 && type!=9)
            throw FormatError("Source particle requires its original vertex owner");
        return resource;
    }
    bool preload(std::uint32_t effect,bool first_person) {
        auto &admitted_effects=first_person?first_person_effects:effects;
        auto &admitted_systems=first_person?first_person_systems:systems;
        if(first_person && first_person_effects.contains(effect))return true;
        if(!first_person && inspected_world_effects.contains(effect))
            return preloaded_world_effects.contains(effect);
        const auto tag=cache.query_tag({.handle=TagHandle{effect},.require_address=true});
        if(!tag || (tag->group!="effe" && tag->group!="prt3"))return false;
        const auto packet=cache.load_source_tag_metadata(effect);
        bool admitted=false;
        const auto admit_rows=[&](std::uint32_t event,const Bytes &rows) {
            for(unsigned system=0;system<rows.size()/0x38;++system) {
                // World-only camera2 cannot submit in the FP pass, nor
                // FP-only camera1 in WORLD. Original
                // creation/render owners still decide actual environment and
                // view eligibility; resource admission supplies no particles.
                const auto camera=rows.u16(system*0x38+0x12);
                if(camera==(first_person?2:1))continue;
                const auto render=cache.load_particle_system_render_definition(effect,event,system);
                if(!resources.contains(render.particle_datum))
                    resources.emplace(render.particle_datum,load_resource(render.particle_datum));
                admitted_systems.emplace(effect,event,std::uint16_t(system));admitted=true;
            }
        };
        if(tag->group=="prt3") {
            admit_rows(none,particle_system_rows(cache,effect,none));
        }else {
            const Bytes definition(packet.bytes_at(packet.source_address(),0x1c));
            const auto events=block(cache,definition,0x14,0x38);
            for(unsigned event=0;event<definition.u32(0x14);++event) {
                if(!first_person) {
                    const auto parts=block(cache,events,event*0x38+0x18,0x38);
                    for(unsigned part=0;part<events.u32(event*0x38+0x18);++part)
                        if(parts.u32(part*0x38+8)==0x6c656e73u && parts.u32(part*0x38+0x10)!=none)
                            lens_requests=true;
                }
                admit_rows(event,block(cache,events,event*0x38+0x30,0x38));
            }
        }
        if(admitted) {
            admitted_effects.insert(effect);
            if(!first_person)preloaded_world_effects.insert(effect);
        }
        if(!first_person)inspected_world_effects.insert(effect);
        return admitted;
    }
    explicit Impl(const Cache &c,std::span<const ScenarioObject> objects):cache(c) {
        std::set<std::uint32_t> inspected;
        for(const auto &object:objects) {
            if(object.definition==none || (object.type!=6 && object.type!=7 && object.type!=11))continue;
            const auto metadata=c.load_source_tag_metadata(object.definition);
            const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
            const auto attachments=block(c,root,0x94,24);
            for(unsigned a=0;a<root.u32(0x94);++a) {
                const auto effect=attachments.u32(a*24+4);
                if(effect==none || !inspected.insert(effect).second)continue;
                const auto tag=c.query_tag({.handle=TagHandle{effect},.require_address=true});
                if(!tag || tag->group!="effe")continue;
                const auto packet=c.load_source_tag_metadata(effect);
                const Bytes definition(packet.bytes_at(packet.source_address(),0x1c));
                const auto events=block(c,definition,0x14,0x38);
                for(unsigned event=0;event<definition.u32(0x14);++event) {
                    const auto parts=block(c,events,event*0x38+0x18,0x38);
                    for(unsigned part=0;part<events.u32(event*0x38+0x18);++part)
                        if(parts.u32(part*0x38+8)==0x6c656e73u && parts.u32(part*0x38+0x10)!=none)
                            lens_requests=true;
                    const auto systems=block(c,events,event*0x38+0x30,0x38);
                    for(unsigned system=0;system<events.u32(event*0x38+0x30);++system) {
                        const auto at=system*0x38;const auto emitters=block(c,systems,at+0x30,0xb8);bool swarm{};
                        for(unsigned e=0;e<systems.u32(at+0x30);++e) {
                            const auto movement=emitters.u32(e*0xb8+4);if(movement==none)continue;
                            const auto controllers=load_particle_movement_definition(c,movement);
                            swarm|=std::any_of(controllers.controllers.begin(),controllers.controllers.end(),
                                [](const auto &v){return v.type==2;});
                        }
                        if(!swarm)continue;
                        const auto render=c.load_particle_system_render_definition(effect,event,system);
                        if(!resources.contains(render.particle_datum))
                            resources.emplace(render.particle_datum,load_resource(render.particle_datum));
                        effects.insert(effect);
                        this->systems.emplace(effect,event,std::uint16_t(system));
                    }
                }
            }
        }
    }
};
SourceWorldParticleDrawsTu15::SourceWorldParticleDrawsTu15(const Cache &c,std::span<const ScenarioObject> objects)
    :impl_(std::make_unique<Impl>(c,objects)){}
SourceWorldParticleDrawsTu15::~SourceWorldParticleDrawsTu15()=default;
bool SourceWorldParticleDrawsTu15::active()const{return impl_->lens_requests || !impl_->effects.empty() || !impl_->first_person_effects.empty();}
const Cache &SourceWorldParticleDrawsTu15::resident_cache()const noexcept{return impl_->cache;}
SourceWorldParticleDrawsTu15::ResourceState SourceWorldParticleDrawsTu15::resource_state()const noexcept {
    return {impl_->resources.size(),impl_->systems.size(),impl_->first_person_systems.size()};
}
bool SourceWorldParticleDrawsTu15::preload_first_person(std::uint32_t effect){return impl_->preload(effect,true);}
bool SourceWorldParticleDrawsTu15::first_person_effect(std::uint32_t effect)const{return impl_->first_person_effects.contains(effect);}
bool SourceWorldParticleDrawsTu15::preload_world(std::uint32_t effect){return impl_->preload(effect,false);}
bool SourceWorldParticleDrawsTu15::world_effect(std::uint32_t effect)const{return impl_->preloaded_world_effects.contains(effect);}
std::vector<std::uint32_t> SourceWorldParticleDrawsTu15::preload_live_world(const SourceRuntimeMemory &m) {
    const auto live=live_particle_census(m);
    for(const auto definition:live.definitions) {
        (void)impl_->preload(definition,false);
        (void)impl_->preload(definition,true);
    }
    return {live.definitions.begin(),live.definitions.end()};
}
std::vector<std::uint32_t> SourceWorldParticleDrawsTu15::require_live_world_resident(const SourceRuntimeMemory &m)const {
    const auto live=live_particle_census(m);
    for(const auto &[effect,event,system]:live.systems){
        const auto rows=particle_system_rows(impl_->cache,effect,event);
        if(system>=rows.size()/0x38)throw FormatError("Live particle system has no authored system");
        const auto camera=rows.u16(system*0x38+0x12);
        const auto &admitted=camera==1?impl_->first_person_systems:impl_->systems;
        if(!admitted.contains({effect,event,system}))throw FormatError("Live particle system was not resident before renderer upload");
        const auto render=impl_->cache.load_particle_system_render_definition(effect,event,system);
        if(!impl_->resources.contains(render.particle_datum))throw FormatError("Live particle material was not resident before renderer upload");
    }
    return {live.definitions.begin(),live.definitions.end()};
}
void SourceWorldParticleDrawsTu15::publish_observer(PhysicsLiveWorldTu15 &world,
    unsigned local_view,Vec3 position,Vec3 forward,Vec3 up,float horizontal_angle) {
    if(!(std::isfinite(horizontal_angle) && horizontal_angle>0 && horizontal_angle<3.141592654f))
        throw FormatError("Source particle observer requires the actual horizontal lens");
    world.publish_native_observer_position(local_view,position);
    auto owner=world.prepare_source_call();owner.gp[1]=local_view;
    world.run_actual(0x172200u,owner);
    if(!owner.gp[0])throw FormatError("Source particle observer has no retained camera record");
    const auto record=owner.gp[0];
    // Native supplies its completed aiming frame in retained caller-stack
    // scratch. Only the completed lens scale belongs to this boundary;
    // preserve the real observer's controller/aim/displacement history.
    const auto scratch=owner.gp[4]+0x100u;
    const std::array<std::byte,0x54> packet{};
    world.write_source_call_input(scratch,packet);
    const std::array<float,10> input{forward.x,forward.y,forward.z,up.x,up.y,up.z,
        horizontal_angle,1.3333333730697632f,0.f,0.f};
    world.write_source_call_input(scratch+0x20,std::as_bytes(std::span(input)));
    owner=world.prepare_source_call();owner.gp[6]=scratch;
    world.run_actual(0x174bf0u,owner);
    const auto scale=word(world.memory(),scratch+0x50);
    world.write_source_call_input(record+0x50,std::as_bytes(std::span(&scale,1)));
}
void SourceWorldParticleDrawsTu15::prepare_storage(PhysicsLiveWorldTu15 &world) {
    if(!active())return;
    auto &m=world.memory();
    if(!word(m,0x4c2aa8u) && !(world.call_actual(0x1e0f0u)&255u))
        throw FormatError("Original transparent particle storage could not construct");
}
void SourceWorldParticleDrawsTu15::advance(PhysicsLiveWorldTu15 &world,float seconds) {
    // Graphics census gates storage construction, not the global source frame.
    prepare_storage(world);source_effect_frame_tu15(world,seconds,SourceFirstPersonPhaseObserverTu15{});
}
std::vector<ParticleDraw> SourceWorldParticleDrawsTu15::draw(PhysicsLiveWorldTu15 &world,
    const SourcePortalVisibilitySet &visibility,const MuzzleEffectView &view,
    unsigned local_view,float angle,float lighting_scale,bool lens_gates_prepared,bool main_commands_prepared) {
    return draw_pass(world,visibility,view,local_view,angle,lighting_scale,false,
        lens_gates_prepared,main_commands_prepared);
}
std::vector<ParticleDraw> SourceWorldParticleDrawsTu15::draw_first_person(PhysicsLiveWorldTu15 &world,
    const SourcePortalVisibilitySet &visibility,const MuzzleEffectView &view,
    unsigned local_view,float angle,float lighting_scale) {
    return draw_pass(world,visibility,view,local_view,angle,lighting_scale,true,false,false);
}
void SourceWorldParticleDrawsTu15::prepare_view(PhysicsLiveWorldTu15 &world,
    const SourcePortalVisibilitySet &visibility,const MuzzleEffectView &view,
    unsigned local_view,float angle,float lighting_scale,std::optional<std::int32_t> actual_user) {
    if(local_view>=4 || (actual_user && (*actual_user< -1 || *actual_user>=4)))
        throw FormatError("Original particle view exceeds its actual ordinal/user bank");
    for(const auto p:{0x4c65c0u,0x491e38u}) {
        for(const auto &[offset,v]:std::array<std::pair<unsigned,Vec3>,3>{{{0,view.position},{12,view.forward},{24,view.up}}}) {
            put(world,p+offset,std::bit_cast<std::uint32_t>(v.x));put(world,p+offset+4,std::bit_cast<std::uint32_t>(v.y));put(world,p+offset+8,std::bit_cast<std::uint32_t>(v.z));
        }
        put(world,p+0x28,std::bit_cast<std::uint32_t>(angle));put(world,p+0x2c,std::bit_cast<std::uint32_t>(lighting_scale));
    }
    put(world,0x4c66f8u,std::bit_cast<std::uint32_t>(actual_user.value_or(std::int32_t(local_view))));
    world.publish_render_visibility(visibility);
    SourcePhysicsWorldRuntimeAbi reset{};reset.gp[4]=0x600f0000u;reset.gp[2]=0x556e18u;
    reset.fs_base=0x60200000u;reset.eflags=2;reset.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    put(world,reset.gp[4],0x60300000u);world.run_actual(0x1373f0u,reset);
    world.call_actual(0x136ea0u,std::array{0x556e18u});
}
std::vector<ParticleDraw> SourceWorldParticleDrawsTu15::draw_pass(PhysicsLiveWorldTu15 &world,
    const SourcePortalVisibilitySet &visibility,const MuzzleEffectView &view,
    unsigned local_view,float angle,float lighting_scale,bool first_person,
    bool lens_gates_prepared,bool main_commands_prepared) {
    std::vector<ParticleDraw> out;if(!active())return out;
    auto &m=world.memory();
    if(local_view>=4 || !word(m,0x4c2aa8u))
        throw FormatError("Original particle queue requires its constructed native world-pass owner");
    if(main_commands_prepared && (first_person || !lens_gates_prepared))
        throw FormatError("Collected world particles lack their actual main-view admission");
    if(!main_commands_prepared) {
      // Standalone/FP callers retain their original queue producer. Ordinary
      // main40E80 already reset and filled this bank; never repeat its phase.
      put(world,0x4c2aa0u,0);put(world,0x4c2aa4u,0);
      prepare_view(world,visibility,view,local_view,angle,lighting_scale);
      if(first_person)world.call_actual(0x1770c0u,std::array{1u});
      else {
        // Original12B4E and2B68C: run the actual pool gates with retained source
        // state. The native query owner supplies this once before real views;
        // standalone particle consumers retain the existing frame-zero gate.
        if(!lens_gates_prepared) {
            if(local_view==0)world.call_actual(0x20680u);
            auto request_view=world.prepare_source_call();request_view.gp[0]=local_view;
            world.run_actual(0x20700u,request_view);
        }
        put(world,0x4c66f4u,local_view); // Literal2BA24, actual submitting view.
        // Original main layer40F14/40F31 requests completed effect parts before
        // the ordinary40F1A particle queue producer. Use the retained mode owner.
        const auto mode_owner=word(m,0x4f3218u);
        if(!mode_owner)throw FormatError("Original lens request phase lacks its retained40F08 mode owner");
        if(word(m,mode_owner+8)==3)world.call_actual(0x179b40u);
        else {world.call_actual(0x179a40u);world.call_actual(0x1770c0u,std::array{0u});}
      }
    }
    const auto count=word(m,0x4c2aa0u),commands=word(m,0x4c2aa8u);
    if(count>1024)throw FormatError("Original particle queue exceeds its constructed capacity");
    const auto hide=std::getenv("HALO2_REFERENCE_SOURCE_ATTACHMENTS_HIDE");
    for(unsigned n=0;n<count;++n) {
        const auto p=commands+n*112;
        if(word(m,p)!=3 || word(m,p+0x20)!=0x176b30u)continue;
        const auto s=word(m,p+0x24),l=word(m,p+0x28),effect=word(m,s+0x10);
        const auto &effects=first_person?impl_->first_person_effects:impl_->effects;
        const auto &systems=first_person?impl_->first_person_systems:impl_->systems;
        if(!effects.contains(effect))continue;
        if(!systems.contains({effect,word(m,s+0x14),Bytes(m.view(s+0xe,2)).u16(0)}))continue;
        const auto state=source_particle_render_state_tu15(impl_->cache,m,
            identity(m,0x51df74u,s,84),identity(m,0x52d71cu,l,52),{view.position,view.forward,view.up,{}});
        const auto &resource=impl_->resources.at(state.definition.particle_datum);
        ParticleRenderValueCache values;
        if(!resource.model) {
            values.system_identity=state.state.system_identity;values.location_identity=state.state.location_identity;
            complete_particle_values(values.values,resource.sprite.required_values&0x107f0u,state.state.sources);
        }
        const auto effect_id=word(m,s+0x18);std::uint32_t owner=none;
        if(effect_id!=none) {
            const auto pool=word(m,0x4f720cu);owner=word(m,word(m,pool+0x44)+(effect_id&65535u)*400u+0x48);
        }
        for_each_particle_render_input(state.definition,state.state,values,*impl_->cache.shader_function_tables(),
            [&](auto,const ParticleSpriteInput &input,ParticleValueCache &cache_values,const ParticleValueSources &sources) {
                ParticleDraw d;d.effect_owner=owner;d.effect_definition=effect;d.material=resource.material;
                std::copy(resource.material->pass.constants[1].begin(),resource.material->pass.constants[1].end(),d.vertex.constants.begin()+18);
                std::copy(view.camera.begin(),view.camera.end(),d.vertex.constants.begin());
                std::copy(view.fog.begin(),view.fog.end(),d.vertex.constants.begin()+12);
                const auto orientation=particle_view_constants(view.orientation);std::copy(orientation.begin(),orientation.end(),d.vertex.constants.begin()+148);
                d.vertex.constants[174]={1,0,0,0};d.vertex.constants[175]={0,1,0,0};d.vertex.constants[176]={0,0,1,0};
                if(resource.model) {
                    if(!sources.particle)throw FormatError("Source particle model lost its actual simulation seed owner");
                    const auto model=particle_model_submission(*resource.model,input,sources.particle->random[3],
                        cache_values,sources,*impl_->cache.shader_function_tables());
                    d.model=resource.model;d.model_first=model.first;d.model_count=model.count;
                    std::copy(model.transform.begin(),model.transform.end(),d.vertex.attributes.begin()+12);
                    d.vertex.attributes[2]=input.color;
                }else {
                    auto completed=input;completed.width=resource.context.width;
                    completed.selected_sequence=resource.context.selected_sequence;
                    const auto sprite=particle_sprite_submission(resource.sprite,completed,cache_values,sources,*impl_->cache.shader_function_tables());
                    d.vertex.attributes=particle_sprite_vertex_attributes(sprite);
                }
                d.vertex.clip_range=view.clip_range;
                d.vertex.surface_fog={float(view.width),float(view.height),1,1};d.depth={view.clip_range,{0,0,1,1}};
                d.scene_sort_depth=std::bit_cast<float>(word(m,p+4));
                d.scene_sort_owner=ParticleDraw::SortOwner{state.state.system_identity,state.state.location_identity,
                    {std::bit_cast<float>(word(m,p+0x64)),std::bit_cast<float>(word(m,p+0x68)),std::bit_cast<float>(word(m,p+0x6c))}};
                if(!(hide && *hide))out.push_back(std::move(d));
            });
    }
    return out;
}
}
