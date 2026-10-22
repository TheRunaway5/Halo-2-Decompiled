#pragma once
#include "halo2/multiplayer_scene_continuity.hpp"
#include "halo2/multiplayer_scene_runtime_values.hpp"
#include "halo2/material_presentation_continuity.hpp"
#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/source_prop_scene.hpp"
#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/native_weapon_projectile_scene.hpp"
#include "halo2/source_grenade_scene.hpp"
#include "halo2/pstencil/point_lights.hpp"
#include "halo2/renderer.hpp"
#include <cstring>

// Existing native receipt fixture supplies real admitted origins, independently
// validated source cuts, original resources and two genuine simulation ticks.
// This adds CPU scene adapters. It does not change or rewind native memory,
// provide a camera/PVS, or claim a complete runtime/GPU/audio migration.
struct NativeSceneContinuityFixture {
    using Scene=halo2::MultiplayerSceneContinuity;
    halo2::MultiplayerPreparedWorld &world;
    halo2::Mesh mesh;
    halo2::MultiplayerSceneRuntimeValues values;
    halo2::SourceSceneryScene scenery;
    halo2::SourceMachineScene machines;
    halo2::SourcePropScene props;
    halo2::SourceDynamicObjectScene dynamic;
    std::vector<std::uint32_t> projectile_definitions;
    halo2::NativeWeaponProjectileScene projectiles;
    halo2::SourceGrenadeScene grenades;
    halo2::pstencil::PointLights points;
    std::unique_ptr<halo2::MaterialPresentationContinuity> materials;
    std::unique_ptr<Scene> continuity;
    Scene::Snapshot early;
    std::vector<halo2::Vertex> early_vertices;
    std::vector<halo2::ObjectLightingBinding> early_light;
    halo2::SourcePortalMaskDefinition portal_definition;
    std::size_t material_count{};
    static void require(bool ok,const char *why){if(!ok)throw halo2::FormatError(why);}
    static std::vector<std::uint32_t> definitions(const halo2::LevelContent &content){
        std::vector<std::uint32_t> result;
        for(const auto &[id,weapon]:content.weapons)if(weapon.projectile!=halo2::null_datum)result.push_back(weapon.projectile);
        return result;
    }
    explicit NativeSceneContinuityFixture(halo2::MultiplayerPreparedWorld &w)
        :world(w),scenery(*w.cache,w.content.objects,0,mesh,w.match.simulation().collision(),{},{},true),
        machines(*w.cache,w.content.objects,0,mesh,w.match.simulation().collision(),true),
        props(*w.cache,w.content.objects,0,mesh,w.match.simulation().collision(),true),
        dynamic(*w.cache,0,mesh,w.match.simulation().collision()),projectile_definitions(definitions(w.content)),
        projectiles(*w.cache,0,mesh,w.match.simulation().collision(),projectile_definitions,w.match.native_object_capacity()),
        grenades(*w.cache,0,mesh,w.match.simulation().collision(),w.match.native_object_capacity(),
            w.cache->original_data()->object_construction_operands->one_bits),points(*w.cache,0),
        portal_definition(w.cache->load_source_portal_mask_definition(0)) {
        props.region_state([&](std::uint32_t handle)->const halo2::SourceInitialModelSelection *{
            values.actual_prop_regions=world.source_objects->object_model_regions(handle).selection;
            return &*values.actual_prop_regions;
        });
        materials=std::make_unique<halo2::MaterialPresentationContinuity>(mesh);
        continuity=std::make_unique<Scene>(targets());
        material_count=mesh.materials.size();
    }
    Scene::Targets targets(){return {values,scenery,machines,props,dynamic,projectiles,&grenades,points,*materials};}
    void populate(const halo2::MultiplayerSnapshotOrigin &origin,bool first){
        continuity->mutating();materials->mutating();
        const auto &state=origin.accepted_state();const auto &scene=world.match.simulation().scene();
        scenery.synchronize(scene,[&](std::size_t,std::uint32_t handle,std::uint64_t){
            return world.source_objects->object_model_regions(handle).selection;
        });
        machines.synchronize(scene,&world.match.simulation().devices());props.synchronize(scene);
        scenery.update_pose(state.tick,std::int32_t(state.tick),mesh);
        machines.update_pose(state.devices.tick,std::int32_t(state.tick),mesh);props.update_pose(std::int32_t(state.tick),mesh);
        dynamic.synchronize(state.source_dynamic_objects,state.source_body_poses);
        projectiles.synchronize(state.weapon_projectiles);
        std::vector<halo2::NativeGrenadeProjectile> released;
        for(const auto &projectile:state.grenade_projectiles)if(projectile.parent==halo2::null_datum)released.push_back(projectile);
        grenades.synchronize(released);points.synchronize({},state.tick);
        const auto portals=machines.portal_owners();
        if(first){std::vector<halo2::SourcePortalOwnerState> initial;
            for(const auto &p:portals)initial.push_back(p.owner);
            values.source_portal_mask.reset_acoustic();values.source_portal_mask.initialize(portal_definition,initial);
            for(const auto &owner:initial)values.source_portal_owners.emplace(owner.handle,owner);
        }else for(const auto &p:portals){auto [at,created]=values.source_portal_owners.try_emplace(p.owner.handle,p.owner);
            if(!created){const auto first=at->second.first,count=at->second.count;
                at->second=p.owner;at->second.first=first;at->second.count=count;}
            values.source_portal_mask.step(portal_definition,at->second,p.door);
        }
        values.source_portal_tick=state.tick;
    }
    void append(std::vector<halo2::Vertex> &vertices,std::vector<halo2::ObjectLightingBinding> &bindings){
        machines.append(vertices,bindings);props.append(vertices,bindings);
    }
    void capture(const halo2::MultiplayerSnapshotOrigin &origin){
        populate(origin,true);append(early_vertices,early_light);
        require(!early_vertices.empty()&&!early_light.empty(),"Native scene fixture lacks actual source presentation owners");
        early=continuity->capture(origin);
        require(early.matches(origin)&&continuity->owns_snapshot(early),"Native scene capture lost its authentic owning receipt");
    }
    void restore(const halo2::MultiplayerSnapshotOrigin &origin,const halo2::MultiplayerSnapshotProof &proof,
        const halo2::MultiplayerSnapshotProof &alternate,const halo2::MultiplayerSnapshotOrigin &later,
        const halo2::MultiplayerSnapshotProof &later_proof){
        populate(later,false);
        require(later.tick()>origin.tick()&&values.source_portal_tick==later.tick(),"Native scene fixture lacks a genuinely later source tick");
        const auto unchanged=[&]{return values.source_portal_tick==later.tick()&&world.match.state().tick==later.tick()&&mesh.materials.size()==material_count;};
        auto lighting=early_light.end();unsigned material{};
        for(auto at=early_light.begin();at!=early_light.end()&&lighting==early_light.end();++at)
            for(unsigned n=0;n<at->material_count;++n){const auto slot=at->first_material+n;
                if(mesh.materials.at(slot).environment_pass){lighting=at;material=slot;break;}}
        require(lighting!=early_light.end(),"Native scene material COW control lacks an authored environment pass");
        const auto old_pass=mesh.materials[material].environment_pass;
        materials->mutating();halo2::apply_object_lighting(mesh.materials[material],lighting->state);
        require(mesh.materials[material].environment_pass!=old_pass,"Native scene actual lighting did not detach its pinned historical material graph");
        {auto cancelled=continuity->prepare_restore(early,proof);
            require(continuity->ready(cancelled,proof),"Native scene grouped exact historical cut did not prepare");}
        require(unchanged(),"Cancelled native scene preparation changed live values or native world");
        bool rejected{};try{(void)continuity->prepare_restore(early,later_proof);}catch(const halo2::FormatError &){rejected=true;}
        require(rejected&&unchanged(),"Native scene history accepted a different actual source receipt");
        Scene foreign(targets());rejected=false;
        try{(void)foreign.prepare_restore(early,proof);}catch(const halo2::FormatError &){rejected=true;}
        require(rejected&&unchanged(),"Native scene history crossed aggregate ownership");
        auto stale=continuity->prepare_restore(early,proof);continuity->mutating();
        require(!continuity->ready(stale,proof)&&!continuity->commit(stale)&&unchanged(),"Native scene mutation kept stale grouped publication eligible");
        auto prepared=continuity->prepare_restore(early,proof),competing=continuity->prepare_restore(early,proof);
        require(!continuity->ready(prepared,alternate)&&!continuity->ready(prepared,later_proof),"Native scene readiness accepted a wrong full seal/receipt proof");
        bool foreign_ready=true;std::thread thread([&]{foreign_ready=continuity->ready(prepared,proof);});thread.join();
        require(!foreign_ready&&continuity->ready(prepared,proof)&&unchanged(),"Native scene preparation crossed its executor or changed the world");
        require(continuity->commit(prepared)&&!continuity->commit(prepared)&&!continuity->commit(competing),"Native scene transaction did not publish exactly once");
        std::vector<halo2::Vertex> restored;std::vector<halo2::ObjectLightingBinding> lights;append(restored,lights);
        require(values.source_portal_tick==origin.tick()&&world.match.state().tick==later.tick()&&
            mesh.materials.size()==material_count&&mesh.materials[material].environment_pass==old_pass&&
            restored.size()==early_vertices.size()&&!std::memcmp(restored.data(),early_vertices.data(),restored.size()*sizeof(halo2::Vertex)),
            "Native scene transaction lost exact historical values/pose/material graph or changed native simulation");
        require(lights.size()==early_light.size(),"Native scene grouped restore changed completed lighting coverage");
        for(std::size_t n=0;n<lights.size();++n)require(lights[n].first_material==early_light[n].first_material&&
            lights[n].material_count==early_light[n].material_count&&lights[n].state.source_values()==early_light[n].state.source_values(),
            "Native scene grouped restore reconstructed different lighting state");
        std::cout<<"PASS proof-bound composed native scene/material restore: actual two source ticks and four-seat receipt; exact poses/portals/material COW graph, cancellation/stale/foreign/executor/full-seal/once controls; native simulation untouched\n";
        std::cout<<"LIMIT scene composition fixture is CPU-local; scene query caches/point/projectile active branches have separate component controls, and HUD/camera/playing audio/native activation remain open\n";
    }
};
