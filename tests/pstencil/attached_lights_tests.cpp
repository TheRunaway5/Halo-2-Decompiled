#include "halo2/pstencil/point_lights.hpp"
#include "halo2/pstencil/native_lights.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/campaign_lights.hpp"
#include "halo2/path_tracing/source_lights.hpp"
#include "halo2/object_sky_lighting.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_portal_visibility.hpp"
#include <cstring>
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <iostream>
#include <unistd.h>
using namespace halo2;
namespace {
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
bool same(Vec3 a,Vec3 b){return a.x==b.x && a.y==b.y && a.z==b.z;}
bool same(const EffectLightState& a,const EffectLightState& b){return a.size==b.size && a.control==b.control &&
    same(a.diffuse,b.diffuse) && same(a.specular,b.specular) && a.brightness==b.brightness;}
}
int main(int argc,char** argv){try {
    require(argc==3,"Supply TU1.5 gameplay XBE and current Coagulation cache");
    Cache cache(argv[2]);const auto save=std::filesystem::temp_directory_path()/("pstencil-attachments-"+std::to_string(getpid()));
    PhysicsLiveWorldTu15 world(cache,argv[1],save);
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);world.initialize_map();
    const auto object=world.create_netgame_object(7);require(object!=0xffffffffu,"native Plasma Rifle row missing");
    auto& memory=world.memory();const auto body=world.source_object_body(object);
    const auto clusters=cache.load_bsp_atmosphere(0).clusters.size();
    const auto snapshot=[&] {
        const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
        const auto pool=word(0x4ecbdcu),records=word(pool+0x44u);
        const auto before=std::vector<std::byte>(memory.view(records,350u*272u).begin(),memory.view(records,350u*272u).end());
        auto result=pstencil::native_lights_tu15(memory,clusters);
        require(std::ranges::equal(before,memory.view(records,350u*272u)),"native light snapshot changed simulation records");
        for(const auto identity:result.lights.identities()) {
            const auto row=records+(identity&65535u)*272u;
            require(std::ranges::equal(std::as_bytes(std::span(result.lights.record(identity))),memory.view(row,272u)),
                "native light snapshot changed a completed state word or full salt");
        }
        return result;
    };
    (void)snapshot();
    const auto definition=Bytes(memory.view(body,4)).u32(0);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& t){return t.datum==definition;});
    require(tag!=cache.tags().end() && tag->name=="objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle",
        "native row no longer creates its authored Plasma Rifle");
    pstencil::PointLights lights(cache,0);require(lights.has(definition),"named direct-light attachment was rejected");
    const auto mortar=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& t) {
        return t.group=="weap" && t.name=="objects\\vehicles\\wraith\\turrets\\mortar\\mortar_turret";
    });require(mortar!=cache.tags().end() && lights.has(mortar->datum),
        "native parent-marker light was rejected because its child has no render model");
    auto native=pstencil::attached_lights_tu15(memory,object,definition);
    require(native.size()==1,"native weapon light did not retain one authored slot");
    pstencil::PointLightOwner owner{object,definition,{}, {}, {},native};
    // No source snapshot must not turn a function-driven light permanently on.
    auto unowned=owner;unowned.native_lights.reset();lights.synchronize(std::span(&unowned,1),0);
    require(lights.lights().empty(),"missing native input invented an always-on weapon light");
    std::uint32_t identity=0xffffffffu;unsigned samples=0;
    for(float heat:{0.f,.25f,.5f,1.f,.5f,0.f}) {
        // Explicit fixture input at the literal native weapon heat field.
        // Complete C2CB0/BCB70/1046C0 and authored color interpolation run.
        world.write_source_call_input(body+0x180,std::as_bytes(std::span(&heat,1)));
        const std::array arguments{native.front().identity};auto call=world.prepare_source_call(arguments);
        world.run_actual(0xc2cb0,call);
        native=pstencil::attached_lights_tu15(memory,object,definition);owner.native_lights=native;
        const auto before=memory.checkpoint();lights.synchronize(std::span(&owner,1),++samples);
        const auto after=memory.checkpoint();require(before==after,"pStencil attachment readback changed native state");
        const auto adapted=lights.lights();require(adapted.size()==1,"native weapon light lost its presentation lifetime");
        require(same(adapted[0].state,native[0].state),"pStencil changed completed native light function/color/brightness/size");
        require(same(adapted[0].position,native[0].frame.position) &&
            same(adapted[0].forward,native[0].frame.basis[0]) && same(adapted[0].up,native[0].frame.basis[2]),
            "pStencil rebuilt a native attachment transform");
        if(identity==0xffffffffu)identity=adapted[0].identity;
        require(adapted[0].identity==identity,"function changes reallocated a stable light generation");
        require(adapted[0].state.control==heat,"actual Plasma Rifle heat/illumination consumer changed");
        require((adapted[0].registration.reference_head!=0xffffffffu)==(heat>0),
            "off/on native attachment did not unregister/register spatially");
    }
    // The normal native object scheduler must advance the attachment too;
    // a successful explicit C2CB0 fixture alone would not prove live wiring.
    const float heated=1.f;world.write_source_call_input(body+0x180,std::as_bytes(std::span(&heated,1)));
    world.step_objects();native=pstencil::attached_lights_tu15(memory,object,definition);
    require(native.size()==1 && native.front().state.control>0 && (native.front().flags&2u),
        "normal native object tick did not activate its function-driven attachment");
    owner.native_lights=native;const auto tick_before=memory.checkpoint();
    lights.synchronize(std::span(&owner,1),++samples);
    require(tick_before==memory.checkpoint() && same(lights.lights().at(0).state,native.front().state),
        "normal native attachment tick did not reach read-only pStencil presentation");
    // A real camera/PVS query over the native spatial snapshot, including
    // the actual attached-owner priority and root-owner ABI consumers.
    pstencil::CampaignLights campaign(cache,0);
    const auto eye=native.front().frame.position+Vec3{-2,0,1};
    const auto forward=normalized(native.front().frame.position-eye);
    SourceFrustumProjectionInput projection;projection.projection_angle=1.57079632679f;
    projection.outer=projection.safe={0,0,720,1280};projection.scenario_present=true;
    const auto projected=source_frustum_projection(projection);
    const auto bank=source_view_projection({eye,forward,{0,0,1},.015625f,1024.f,{},projected.projection_program});
    const auto camera=source_frustum_camera({eye,forward,{0,0,1},.015625f,1024.f,projected.clipping_rectangle()});
    const auto atmosphere=cache.load_bsp_atmosphere(0);const auto fog=atmosphere.sample(eye,forward);
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    std::array<std::uint32_t,16> doors;std::memcpy(doors.data(),memory.view(word(0x505e7cu),sizeof(doors)).data(),sizeof(doors));
    const auto visible=source_portal_visibility(cache.load_source_portal_bsp(0),std::span(&camera,1),fog.cluster,doors);
    const auto frustums=visible.sphere_bank();SourceEffectLightCollectionContext collection{};
    collection.visibility.frustums=&frustums;collection.containing_cluster=std::int16_t(fog.cluster);
    collection.owners={1,0,0,2,{}};
    SourceEffectLightCandidateContext candidates{};candidates.admission=collection.owners;
    candidates.depth_plane={bank.inverse_view.basis[0].z,bank.inverse_view.basis[1].z,bank.inverse_view.basis[2].z,bank.inverse_view.position.z};
    candidates.pixel_scale=bank.viewport_half_metrics.y;
    const auto before_view=snapshot();
    const auto headers=word(word(0x4ecbd0u)+0x44u);const auto body_size=Bytes(memory.view(headers+(object&65535u)*12u+6u,2)).u16(0);
    const auto owner_before=std::vector<std::byte>(memory.view(body,body_size).begin(),memory.view(body,body_size).end());
    const auto view=campaign.view(world,eye,collection,candidates,1,true);
    const auto after_view=snapshot();
    // Actual native getter calls use their caller stack. Compare simulation
    // light/spatial banks and the complete owner body, not scratch-stack bytes.
    require(before_view.lights.records==after_view.lights.records && before_view.lights.occupied==after_view.lights.occupied &&
        before_view.spatial.heads==after_view.spatial.heads && before_view.spatial.entries.records==after_view.spatial.entries.records &&
        before_view.spatial.references.records==after_view.spatial.references.records &&
        std::ranges::equal(owner_before,memory.view(body,body_size)),"native campaign light view mutated simulation or owner state");
    require(std::ranges::find(view.visible,native.front().identity)!=view.visible.end(),"real camera omitted the native active Plasma Rifle light");
    require(std::ranges::any_of(view.candidates,[&](const auto& row){return row.identity==native.front().identity;}),
        "campaign light candidate changed or omitted the full native light generation");
    const auto actual_shape=pstencil::native_light_shape_tu15(world,native.front().identity);
    const auto expected_shape=source_effect_light_sphere(load_effect_light(cache,native.front().definition),
        native.front().frame.position,native.front().frame.basis[0],native.front().frame.basis[2],native.front().state.size);
    require(actual_shape.type()==0,"Native sphere producer changed its shape family");
    for(const unsigned at:{1,2,6,7,8,9,11,12,13,15,16,17}) {
        for(unsigned byte=0;byte<4;++byte)require(actual_shape.written[at*4+byte],"Native sphere field was not actually written");
        require(actual_shape.words[at]==expected_shape.shape[at],"Complete native light shape differs from the checked sphere arithmetic");
    }
    auto rt_records=view.records;
    for(auto& row:rt_records)row.runtime.overall_fade=0;
    const auto rt_lights=path_tracing::source_lights(world,rt_records);
    const auto rt=std::find_if(rt_lights.begin(),rt_lights.end(),[&](const auto& light) {
        return same(light.position,native.front().frame.position);
    });
    require(rt!=rt_lights.end() && rt->radius==expected_shape.diffuse_radius &&
        same(rt->color,native.front().state.diffuse) && rt->intensity==native.front().state.brightness,
        "Path Tracing dropped an off-camera emitter or changed its native sphere/energy");
    require(snapshot().lights.records==before_view.lights.records,
        "Path Tracing source emitter extraction changed actual light state");
    ObjectSkyLightingDefinition source_sky;source_sky.outdoor_ambient={.1f,.2f,.3f};
    require(!path_tracing::source_sky_light(source_sky),"Ambient-only source sky acquired an invented directional light");
    ObjectSkyLightingDefinition::Light source_sun;
    source_sun.radiosity=ObjectSkyLightingDefinition::Light::Radiosity{{.2f,.4f,.6f},8};
    source_sky.lights.push_back(source_sun);
    const auto sky_light=path_tracing::source_sky_light(source_sky);
    require(sky_light && sky_light->direction && same(*sky_light->direction,{1,0,0}) &&
        same(sky_light->color,{.2f,.4f,.6f}) && sky_light->intensity==1,
        "Path Tracing changed source sky incident orientation or radiosity energy");
    std::array<ShaderVector,9> previous;
    for(unsigned row=0;row<previous.size();++row)for(unsigned component=0;component<4;++component)
        previous[row][component]=float(int(row*4+component)-12)*.03125f;
    const EffectLightViewFades supplied_fades{.75f,.5f,.371f,.625f,.125f};
    const auto native_constants=pstencil::native_light_constants_tu15(world,native.front().identity,supplied_fades,.5f,previous);
    const auto checked_constants=source_effect_light_view_uniforms(load_effect_light(cache,native.front().definition),
        native.front().state,native.front().frame.position,native.front().frame.basis[0],native.front().frame.basis[2],supplied_fades,.5f,previous);
    require(native_constants.constants==checked_constants.constants && native_constants.color_shift==checked_constants.color_shift &&
        native_constants.gel_basis_u==checked_constants.gel_basis_u && native_constants.gel_basis_v==checked_constants.gel_basis_v,
        "Whole native constant builder differs from checked sphere/fade arithmetic");
    require(snapshot().lights.records==before_view.lights.records,"Native light constants changed actual light state");
    const auto burst=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& t) {
        return t.group=="ligh" && t.name=="effects\\objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade_explosion";
    });require(burst!=cache.tags().end(),"native timed-light regression lost its authored explosion");
    // C2380's literal ABI: definition/owner/slot/up on the stack,
    // EBX=position, EDI=forward, XMM0=control. Borrow actual body vectors.
    const std::array burst_args{burst->datum,0xffffffffu,0xffffu,body+0x7cu};
    auto burst_call=world.prepare_source_call(burst_args);burst_call.gp[3]=body+0x64u;burst_call.gp[7]=body+0x70u;
    burst_call.xmm[0][0]=std::bit_cast<std::uint32_t>(1.f);world.run_actual(0xc2380u,burst_call);
    const auto burst_id=burst_call.gp[0];require(burst_id!=0xffffffffu,"native timed-light allocation was declined");
    const auto occupied=[&] {
        const auto pool=word(0x4ecbdcu);
        return (word(word(pool+0x48u)+(burst_id&65535u)/32u*4u)&(1u<<(burst_id&31u)))!=0;
    };
    require(occupied(),"native timed-light allocation did not publish occupancy");
    require(snapshot().lights.contains(burst_id),"native light snapshot omitted the real timed effect generation");
    for(unsigned tick=0;tick<90 && occupied();++tick)world.step_objects();
    require(!occupied(),"normal native light phase did not expire its authored timed explosion");
    require(!snapshot().lights.contains(burst_id),"native light snapshot retained a retired effect generation");
    bool stale=false;try{(void)pstencil::attached_lights_tu15(memory,object^0x10000u,definition);}catch(const FormatError&){stale=true;}
    require(stale,"stale object salt was accepted by the native attachment reader");
    world.destroy_object(object);lights.synchronize({},samples+1);require(lights.lights().empty(),"destroyed weapon retained a phantom light");
    const auto replacement=world.create_netgame_object(7);require(replacement!=0xffffffffu && replacement!=object,"native replacement reused its full old generation");
    native=pstencil::attached_lights_tu15(memory,replacement,definition);owner.object=replacement;owner.native_lights=native;
    lights.synchronize(std::span(&owner,1),samples+2);
    require(lights.lights().size()==1 && lights.lights()[0].identity!=identity,"native replacement retained the old presentation light generation");
    std::filesystem::remove_all(save);
    std::cout<<"PASS: six original native attachment function states, normal native object-tick activation and timed-light expiry, exact color/size/brightness/transform, off/on spatial registration, read-only full native light/spatial snapshots, lifetime and salt reuse\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
