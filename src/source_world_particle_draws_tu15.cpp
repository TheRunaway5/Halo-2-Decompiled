#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_particle_render_state_tu15.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include "halo2/particle_movement.hpp"
#include "halo2/particle_vertex.hpp"
#include "halo2/particle_view.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <map>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {return Bytes(m.view(p,4)).u32(0);}
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t value) {m.write(p,std::as_bytes(std::span(&value,1)));}
Bytes block(const Cache &cache,const Bytes &root,unsigned offset,unsigned stride) {
    const auto d=read_tag_block_descriptor(root,offset);const auto bytes=d.extent(stride,65536);
    if(!d.count)return Bytes(std::span<const std::byte>{});
    for(const auto &owner:cache.select_tags({.contains=TagAddressRange{d.address,bytes},.require_address=true,.minimum_size=1})) {
        (void)cache.tag_payload(owner.handle());return Bytes(cache.read_tag_bytes(d.address,bytes));
    }
    throw FormatError("Source swarm draw block has no retained metadata owner");
}
std::uint32_t identity(const SourceRuntimeMemory &m,std::uint32_t global,std::uint32_t pointer,unsigned stride) {
    const auto pool=word(m,global),first=word(m,pool+0x44),high=word(m,pool+0x38);
    if(word(m,pool+0x24)!=stride || high>word(m,pool+0x20) || pointer<first ||
        (pointer-first)%stride || (pointer-first)/stride>=high)
        throw FormatError("Source particle command lost its actual pool pointer");
    const auto index=(pointer-first)/stride;
    return std::uint32_t(Bytes(m.view(pointer,2)).u16(0))*65536u+index;
}
}
struct SourceWorldParticleDrawsTu15::Impl {
    struct Resource {
        ParticleSpriteDefinition sprite;
        SourceParticleSpriteContext context;
        std::shared_ptr<const ParticleMaterial> material;
    };
    const Cache &cache;
    std::set<std::uint32_t> effects;
    std::set<std::tuple<std::uint32_t,std::uint32_t,std::uint16_t>> systems;
    std::map<std::uint32_t,Resource> resources;
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
                        if(!resources.contains(render.particle_datum)) {
                            Resource resource;resource.sprite=c.load_particle_sprite(render.particle_datum);
                            const auto context=source_particle_sprite_context(c,render.particle_datum,resource.sprite);
                            if(!context)throw FormatError("Authored swarm sprite has no admitted bitmap data");
                            resource.context=*context;
                            if(!context->has_bitmap)resource.sprite.bitmap_type=-1;
                            resource.material=std::make_shared<const ParticleMaterial>(c.load_particle_material(render.particle_datum,15,0,0));
                            if(resource.material->vertex.classification!=7 && resource.material->vertex.classification!=8 && resource.material->vertex.classification!=9)
                                throw FormatError("Source swarm requires its completed particle vertex owner");
                            resources.emplace(render.particle_datum,std::move(resource));
                        }
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
bool SourceWorldParticleDrawsTu15::active()const{return !impl_->effects.empty();}
void SourceWorldParticleDrawsTu15::advance(PhysicsLiveWorldTu15 &world,float seconds) {
    if(!active())return;
    auto &m=world.memory();
    if(!word(m,0x4c2aa8u) && !(world.call_actual(0x1e0f0u)&255u))
        throw FormatError("Original transparent particle storage could not construct");
    world.advance_effect_particles(seconds);
}
std::vector<ParticleDraw> SourceWorldParticleDrawsTu15::draw(PhysicsLiveWorldTu15 &world,
    const SourcePortalVisibilitySet &visibility,const MuzzleEffectView &view,
    unsigned local_view,float angle,float lighting_scale) {
    std::vector<ParticleDraw> out;if(!active())return out;
    auto &m=world.memory();
    if(local_view>=4 || !word(m,0x4c2aa8u))
        throw FormatError("Original particle queue requires its constructed native world-pass owner");
    // This native world pass supplies layer0 below. Literal40EC2/40EC8
    // reset the queue's two counts in that pass; model command banks have their own
    // existing native graphics owner. No source particle state is cleared.
    put(m,0x4c2aa0u,0);put(m,0x4c2aa4u,0);
    for(const auto p:{0x4c65c0u,0x491e38u}) {
        for(const auto &[offset,v]:std::array<std::pair<unsigned,Vec3>,3>{{{0,view.position},{12,view.forward},{24,view.up}}}) {
            put(m,p+offset,std::bit_cast<std::uint32_t>(v.x));put(m,p+offset+4,std::bit_cast<std::uint32_t>(v.y));put(m,p+offset+8,std::bit_cast<std::uint32_t>(v.z));
        }
        put(m,p+0x28,std::bit_cast<std::uint32_t>(angle));put(m,p+0x2c,std::bit_cast<std::uint32_t>(lighting_scale));
    }
    put(m,0x4c66f8u,local_view);world.publish_render_visibility(visibility);
    SourcePhysicsWorldRuntimeAbi reset{};reset.gp[4]=0x600f0000u;reset.gp[2]=0x556e18u;
    reset.fs_base=0x60200000u;reset.eflags=2;reset.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    put(m,reset.gp[4],0x60300000u);world.run_actual(0x1373f0u,reset);
    world.call_actual(0x136ea0u,std::array{0x556e18u});world.call_actual(0x1770c0u,std::array{0u});
    const auto count=word(m,0x4c2aa0u),commands=word(m,0x4c2aa8u);
    if(count>1024)throw FormatError("Original particle queue exceeds its constructed capacity");
    const auto hide=std::getenv("HALO2_REFERENCE_SOURCE_ATTACHMENTS_HIDE");
    for(unsigned n=0;n<count;++n) {
        const auto p=commands+n*112;
        if(word(m,p)!=3 || word(m,p+0x20)!=0x176b30u)continue;
        const auto s=word(m,p+0x24),l=word(m,p+0x28),effect=word(m,s+0x10);
        if(!impl_->effects.contains(effect))continue;
        if(!impl_->systems.contains({effect,word(m,s+0x14),Bytes(m.view(s+0xe,2)).u16(0)}))continue;
        const auto state=source_particle_render_state_tu15(impl_->cache,m,
            identity(m,0x51df74u,s,84),identity(m,0x52d71cu,l,52),{view.position,view.forward,view.up,{}});
        const auto &resource=impl_->resources.at(state.definition.particle_datum);
        ParticleRenderValueCache values;values.system_identity=state.state.system_identity;values.location_identity=state.state.location_identity;
        complete_particle_values(values.values,resource.sprite.required_values&0x107f0u,state.state.sources);
        const auto effect_id=word(m,s+0x18);std::uint32_t owner=none;
        if(effect_id!=none) {
            const auto pool=word(m,0x4f720cu);owner=word(m,word(m,pool+0x44)+(effect_id&65535u)*400u+0x48);
        }
        for_each_particle_render_input(state.definition,state.state,values,*impl_->cache.shader_function_tables(),
            [&](auto,const ParticleSpriteInput &input,ParticleValueCache &cache_values,const ParticleValueSources &sources) {
                auto completed=input;completed.width=resource.context.width;completed.selected_sequence=resource.context.selected_sequence;
                const auto sprite=particle_sprite_submission(resource.sprite,completed,cache_values,sources,*impl_->cache.shader_function_tables());
                ParticleDraw d;d.effect_owner=owner;d.effect_definition=effect;d.material=resource.material;
                std::copy(resource.material->pass.constants[1].begin(),resource.material->pass.constants[1].end(),d.vertex.constants.begin()+18);
                std::copy(view.camera.begin(),view.camera.end(),d.vertex.constants.begin());
                std::copy(view.fog.begin(),view.fog.end(),d.vertex.constants.begin()+12);
                const auto orientation=particle_view_constants(view.orientation);std::copy(orientation.begin(),orientation.end(),d.vertex.constants.begin()+148);
                d.vertex.constants[174]={1,0,0,0};d.vertex.constants[175]={0,1,0,0};d.vertex.constants[176]={0,0,1,0};
                d.vertex.attributes=particle_sprite_vertex_attributes(sprite);d.vertex.clip_range=view.clip_range;
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
