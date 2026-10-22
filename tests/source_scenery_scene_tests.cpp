#include "halo2/source_scenery_scene.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/scenery.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/renderer.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_scenery_idle_phase.hpp"
#include "halo2/multiplayer_scene_runtime_values.hpp"
#include "halo2/source_machine_scene.hpp"
#include "halo2/render_interpolation.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void require(bool condition,const char *message){if(!condition)throw FormatError(message);}
bool same(Vec3 a,Vec3 b){return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
    std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
    std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);}
bool same_idle(const SourceSceneryIdlePhaseState &a,const SourceSceneryIdlePhaseState &b){
    return std::bit_cast<std::uint32_t>(a.frame)==std::bit_cast<std::uint32_t>(b.frame)&&
        std::bit_cast<std::uint32_t>(a.playback_rate)==std::bit_cast<std::uint32_t>(b.playback_rate)&&
        a.delay==b.delay&&a.status==b.status&&a.playback_flags==b.playback_flags&&
        a.crossed_events==b.crossed_events&&a.next_events==b.next_events;
}
bool same_idle_definition(const std::optional<SourceSceneryIdlePhaseDefinition> &a,
    const std::optional<SourceSceneryIdlePhaseDefinition> &b){
    return a.has_value()==b.has_value()&&(!a||(a->graph==b->graph&&a->animation==b->animation&&
        a->frames==b->frames&&a->loop_frame==b->loop_frame&&a->previous_sibling==b->previous_sibling&&
        a->next_sibling==b->next_sibling&&a->event_counts==b->event_counts));
}
bool same_lifetime(const MultiplayerSceneRuntimeValues::SceneryQueryLifetime &a,
    const MultiplayerSceneRuntimeValues::SceneryQueryLifetime &b){
    return a.placement==b.placement&&a.created_tick==b.created_tick&&a.advanced_tick==b.advanced_tick&&
        a.native_animated_bounds_admitted==b.native_animated_bounds_admitted&&a.idle_paired==b.idle_paired&&
        same_idle_definition(a.idle_definition,b.idle_definition)&&same_idle(a.idle_state,b.idle_state)&&
        same_idle(a.previous_idle_state,b.previous_idle_state);
}
struct Counts {unsigned models{},baked{},dynamic{},unresolved{},streams{},baked_vertices{},empty{},idle{};};
Counts verify(const std::filesystem::path &path) {
    Cache cache(path);auto content=cache.load_content(0,true);
    // Retain the original automatic initial MP2/current-BSP fixture corpus.
    for(auto &o:content.objects)if(o.type==6 && (!scenery_automatic_creation_allowed(o,2) || o.origin_bsp>0))o.definition=null_datum;
    const auto lighting=cache.load_bsp_lighting(0);CollisionWorld collision(content.collision);Counts counts;
    std::map<std::size_t,SourceInitialModelSelection> selections;
    for(std::size_t index=0;index<content.objects.size();++index) {
        const auto &object=content.objects[index];
        if(object.type!=6 || !scenery_automatic_creation_allowed(object,2) || object.origin_bsp>0)continue;
        const auto selected=cache.load_initial_model_selection(object.definition,object.variant);
        if(!selected.resolved()) {++counts.unresolved;continue;}
        selections.emplace(index,selected);
        if(const auto idle=cache.load_initialized_object_idle(object.definition)) {
            ++counts.idle;std::cout<<cache.name()<<" initialized SCEN clip: "<<idle->name<<'\n';
        }
    }
    Mesh mesh;SourceSceneryScene scene(cache,content.objects,0,mesh,collision,{},
        [&](std::size_t index,const ScenarioObject &object) {
            require(content.objects.at(index).definition==object.definition && !selections.contains(index),
                "resolved SCEN escaped through unresolved-only omission");return true;
        });SceneObjects objects(content,0,SourceSceneGameContext{2,2});
    require(scene.stats().unresolved_selections==counts.unresolved &&
        scene.unresolved_selection_owners().size()==counts.unresolved,"SCEN unresolved owner boundary lost content identity");
    scene.synchronize(objects);
    if(cache.name()=="coagulation") {
        const auto vehicle=std::find_if(content.objects.begin(),content.objects.end(),[](const auto &o){
            return o.type==1 && o.name<0 && o.definition!=null_datum;
        });
        require(vehicle!=content.objects.end(),"SCEN unrelated-lifetime fixture has no vehicle placement");
        auto repeated=objects;const auto placement=std::size_t(vehicle-content.objects.begin());
        const auto first=repeated.create_placement(placement,19),second=repeated.create_placement(placement,20);
        require(first!=second,"SCEN unrelated vehicle fixture lost distinct full generations");
        scene.synchronize(repeated);scene.synchronize(objects);
        std::cout<<"SCEN lifetime: repeated vehicle placements stay outside the scenery bank\n";
    }
    std::map<std::size_t,unsigned> reads;
    scene.animation_time([&](std::size_t index,std::uint32_t handle,std::uint64_t born,std::uint64_t tick) {
        require(objects.get(handle).placement==index && objects.get(handle).created_tick==born && tick>=born,
            "SCEN animation producer lost actual owner/birth clock");
        return float(tick-born)*(1.f/30.f); // Explicit native test controller input.
    });
    // This test supplies an explicit constructor-pose sphere service. Live
    // cached/physics history remains the production caller's responsibility.
    scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform &node,float scale) {
        require(objects.get(handle).placement==index,"SCEN sphere lost full lifetime identity");++reads[index];
        const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(content.objects[index].definition),node,scale);
        return SourceSceneryQueryState{sphere.center,sphere.radius,{},0,true};
    });
    scene.update_pose(0,0,mesh);require(reads.empty(),"SCEN pose update queried an unsubmitted owner");
    std::vector<Vertex> close,far;std::vector<ObjectLightingBinding> close_bindings,far_bindings;
    std::vector<SourceObjectViewSelection> close_selected,far_selected;
    const auto view=[&](unsigned id,float distance,unsigned flags) {
        return SourceObjectViewLod{id,1,false,[=](std::size_t,std::uint32_t,Vec3,float) {
            return SourceObjectLodInput{distance,flags,0};}};
    };
    scene.append_view(close,close_bindings,view(0,0,0x80000),&close_selected);
    scene.append_view(far,far_bindings,view(1,100000,0),&far_selected);
    require(close_selected.size()==scene.stats().render_models && far_selected.size()==close_selected.size(),
        "SCEN exact resident collector inventory incomplete");
    const auto verify_submission=[&](const std::vector<Vertex>&vertices,const auto&selected) {
        std::size_t at=0;
        for(const auto &draw:selected) {
            const auto &object=content.objects.at(draw.source_object);
            auto model=cache.load_model(object.definition,false,object.variant,selections.at(draw.source_object),draw.lod);
            require(model.mesh.vertices.size()==draw.vertices,"SCEN selected geometry differs from exact source LOD");
            ++counts.streams;if(!draw.vertices)++counts.empty;
            const auto definition=*cache.load_scenery_definition(object.definition);
            const auto implementation=scenery_lighting_implementation(lighting,model,object,definition);
            if(implementation==SceneryLightingImplementation::per_vertex) {
                const auto ref=std::find_if(lighting.scenery.begin(),lighting.scenery.end(),[&](const auto &r) {
                    return r.unique_id==object.unique_id && r.type==object.type && r.source==object.source &&
                        r.checksum==model.checksum && (r.source!=0 || r.origin_bsp==object.origin_bsp);
                });
                require(ref!=lighting.scenery.end(),"SCEN source baked identity missing");
                const auto &bucket=lighting.buckets.at(unsigned(ref->reference.bucket));
                for(unsigned i=0;i<draw.vertices;++i) {
                    const auto origin=model.vertex_origins[i];
                    const auto offset=ref->reference.section_offsets.at(origin.section);
                    require(offset!=0xffff,"SCEN selected source section has no authored bucket offset");
                    const auto source=std::size_t(offset)+origin.vertex;
                    require(same(vertices.at(at+i).incident,decode_bsp_incident(bucket.incident.at(source))),
                        "SCEN lowerLOD incident substituted another section or rotated world data");
                    require(same(vertices.at(at+i).color,decode_bsp_radiance(bucket.radiance.at(source))),
                        "SCEN lowerLOD RGB substituted another source section");
                    ++counts.baked_vertices;
                }
            }
            for(unsigned i=0;i<draw.vertices;++i)require(vertices.at(at+i).material<mesh.materials.size(),
                "SCEN selected face escaped its resident material range");
            at+=draw.vertices;
        }
        require(at==vertices.size(),"SCEN per-view geometry emitted a different collector");
    };
    verify_submission(close,close_selected);verify_submission(far,far_selected);
    for(const auto &draw:close_selected)require(draw.lod==5,"SCEN source flag19 forceLOD5 ignored");
    for(const auto &draw:far_selected)require(draw.lod==0,"SCEN distant source collector ignored");
    for(const auto &[index,count]:reads)require(count==1,"SCEN repeated cached sphere producer in one render frame");
    require(close_bindings.size()==far_bindings.size(),"SCEN lowerLOD lost dynamic owner binding");
    for(unsigned i=0;i<close_bindings.size();++i)require(close_bindings[i].state.source_values()==far_bindings[i].state.source_values(),
        "SCEN later view replaced the first owner lighting result");
    // Hidden owners must neither retain a new sphere nor advance LOD/query.
    const auto before=reads;for(const auto &object:objects.objects())objects.visible(object.handle,false);
    scene.synchronize(objects);scene.update_pose(1,1,mesh);
    std::vector<Vertex> hidden;std::vector<ObjectLightingBinding> banks;std::vector<SourceObjectViewSelection> draws;
    scene.append_view(hidden,banks,view(0,0,0),&draws);
    require(hidden.empty() && banks.empty() && draws.empty() && reads==before,"hidden SCEN submitted or queried");
    counts.models=scene.stats().render_models;counts.baked=scene.stats().baked_models;counts.dynamic=scene.stats().dynamic_models;
    std::cout<<cache.name()<<": "<<counts.models<<" resolved models, "<<counts.baked<<" baked + "<<counts.dynamic
        <<" dynamic, "<<counts.unresolved<<" unresolved selections excluded explicitly, "<<counts.baked_vertices
        <<" exact source incident/RGB vertices, "<<counts.idle<<" admitted initial idle controllers\n";
    return counts;
}
void prepared_scenery_birth() {
    Cache cache("assets/disc/maps/ascension.map");auto content=cache.load_content(0,true);
    const auto at=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&o){
        return o.type==6 && scenery_automatic_creation_allowed(o,2) && cache.load_initial_model_selection(o.definition,o.variant).resolved();});
    require(at!=content.objects.end(),"Deferred SCEN fixture has no authored resource");
    const auto placement=std::size_t(at-content.objects.begin());
    for(std::size_t i=0;i<content.objects.size();++i)if(i!=placement)content.objects[i].definition=null_datum;
    content.objects[placement].flags|=1u;
    CollisionWorld collision(content.collision);Mesh mesh;SourceSceneryScene scene(cache,content.objects,0,mesh,collision);
    SceneObjects objects(content,0,SourceSceneGameContext{2,2});
    unsigned queries{};scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform&node,float scale){
        require(index==placement && objects.get(handle).placement==placement,"Deferred SCEN query lost actual lifetime");++queries;
        const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(content.objects[index].definition),node,scale);
        return SourceSceneryQueryState{sphere.center,sphere.radius,{},0,true};});
    scene.animation_frame([](auto,auto,auto,auto,const auto&){return 0.f;});
    const auto submit=[&]{std::vector<Vertex> v;std::vector<ObjectLightingBinding>b;std::vector<SourceObjectViewSelection>d;
        scene.append_view(v,b,SourceObjectViewLod{0,1,false,[](auto,auto,auto,auto){return SourceObjectLodInput{0,0x80000,0};}},&d);return d;};
    require(scene.stats().render_models==1 && submit().empty() && queries==0,"Prepared SCEN submitted before actual birth");
    scene.synchronize(objects);require(submit().empty() && queries==0,"Absent SCEN acquired a draw owner");
    const auto born=objects.create_placement(placement,7);scene.synchronize(objects);scene.update_pose(7,1,mesh);
    require(born!=null_datum && submit().size()==1 && queries==1,"Actual deferred SCEN birth did not activate prepared geometry");
    objects.destroy(born);scene.synchronize(objects);require(submit().empty() && queries==1,"Retired SCEN kept submitting");
    std::cout<<"Prepared SCEN: no prebirth draw/query, actual deferred birth and retirement\n";
}
void lifetime_inputs() {
    Cache cache("assets/map-pack/maps/elongation.map");const auto content=cache.load_content(0,true);
    const auto found=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &o) {
        return o.type==6 && scenery_automatic_creation_allowed(o,2) &&
            !cache.load_source_object_color_definition(o.definition).channels.empty();
    });
    require(found!=content.objects.end(),"source changing-color SCEN lifetime fixture missing");
    LevelContent one;one.objects={*found};one.objects.front().name=0;
    one.object_names={{"explicit_test_lifetime",6,found->placement}};
    CollisionWorld collision(content.collision);Mesh mesh;
    const auto prepared=cache.load_initial_model_selection(found->definition,found->variant);
    require(prepared.resolved(),"SCEN lifetime fixture requires fixed source model output");
    SourceSceneryScene scene(cache,one.objects,0,mesh,collision);SceneObjects objects(one,0);
    unsigned births=0;std::map<std::uint32_t,unsigned> sphere_reads;
    std::map<std::uint32_t,Transform> initial_nodes;
    scene.birth_query_capture([&](std::size_t index,std::uint32_t handle,std::uint64_t birth,
        std::uint32_t definition,const Transform &node,float scale) {
        require(index==0 && objects.get(handle).created_tick==birth && definition==found->definition && scale==found->scale,
            "SCEN constructor capture lost actual source birth inputs");
        require(initial_nodes.emplace(handle,node).second,"SCEN constructor bound captured twice for one generation");
    });
    const auto selected=[&](std::size_t index,std::uint32_t handle,std::uint64_t birth) {
        require(index==0 && objects.get(handle).created_tick==birth,"SCEN lifetime provider lost actual birth");
        ++births;return prepared;
    };
    scene.synchronize(objects,selected);
    scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform &node,float scale) {
        require(index==0,"SCEN cached sphere escaped owner");++sphere_reads[handle];
        const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(found->definition),node,scale);
        return SourceSceneryQueryState{sphere.center,sphere.radius,{},0,true};
    });
    const auto submit=[&] {
        std::vector<Vertex> v;std::vector<ObjectLightingBinding> b;
        scene.append_view(v,b,SourceObjectViewLod{0,1,false,[](std::size_t,std::uint32_t,Vec3,float) {
            return SourceObjectLodInput{0,0x80000,0};}});return v;
    };
    const auto first=objects.resolve(0);scene.update_pose(0,0,mesh);const auto initial=submit();
    require(!initial.empty(),"SCEN lifetime initial source stream missing");
    objects.destroy(first);scene.synchronize(objects);scene.update_pose(130,1,mesh);
    require(submit().empty(),"destroyed SCEN retained submitted geometry");
    const auto replacement=objects.create(0,false,130);require(replacement!=first,"SCEN full generation did not change");
    bool rejected=false;
    try {scene.synchronize(objects,[&](std::size_t,std::uint32_t,std::uint64_t) {
        auto wrong=prepared;wrong.unresolved_rng_regions={0};return wrong;
    });}catch(const FormatError &){rejected=true;}
    require(rejected,"SCEN recreated lifetime admitted unresolved historical bytes");
    scene.synchronize(objects,selected);scene.update_pose(130,2,mesh);const auto born=submit();
    require(born.size()==initial.size() && births==2,"SCEN source birth reused previous resident owner");
    for(unsigned i=0;i<born.size();++i)require(same(born[i].position,initial[i].position) &&
        same(born[i].incident,initial[i].incident) && same(born[i].color,initial[i].color),
        "SCEN birth retained another generation's pose or baked stream");
    require(sphere_reads[first]==1 && sphere_reads[replacement]==1,"SCEN birth reused previous cached sphere state");
    require(initial_nodes.size()==2 && same(initial_nodes.at(first).position,initial_nodes.at(replacement).position),
        "SCEN constructor bound did not reset before later animation sampling");
    objects.get(replacement).created_tick=131;rejected=false;
    try{scene.synchronize(objects,selected);}catch(const FormatError &){rejected=true;}
    require(rejected,"SCEN unchanged generation admitted different source birth tick");
    std::cout<<"SCEN lifetime: full generation/birth, supplied historical selections, destroyed geometry and cached-state reset\n";
}
void animation_input() {
    Cache cache("assets/disc/maps/ascension.map");const auto content=cache.load_content(0,true);
    CollisionWorld collision(content.collision);Mesh mesh;SourceSceneryScene scene(cache,content.objects,0,mesh,collision);
    SceneObjects objects(content,0);scene.synchronize(objects);bool rejected=false;
    try{scene.update_pose(0,0,mesh);}catch(const FormatError &){rejected=true;}
    require(rejected,"moving SCEN silently derived an unproved animation phase");
    unsigned calls=0;scene.animation_time([&](std::size_t index,std::uint32_t handle,std::uint64_t birth,std::uint64_t tick) {
        require(objects.get(handle).placement==index && objects.get(handle).created_tick==birth && tick==0,
            "SCEN explicit controller time lost full source clock identity");++calls;return 0.f;
    });scene.update_pose(0,0,mesh);require(calls==2,"source initialized teleporter clips bypassed phase service");
    scene.animation_time([](std::size_t,std::uint32_t,std::uint64_t,std::uint64_t){return -1.f;});rejected=false;
    try{scene.update_pose(1,1,mesh);}catch(const FormatError &){rejected=true;}
    require(rejected,"SCEN malformed controller time admitted");
    unsigned frame_calls=0;
    scene.animation_frame([&](std::size_t index,std::uint32_t handle,std::uint64_t birth,
        std::uint64_t tick,const AnimationClip &clip) {
        require(objects.get(handle).placement==index && objects.get(handle).created_tick==birth && tick==2,
            "SCEN retained controller frame lost its full lifetime identity");
        const auto authored=cache.load_initialized_object_idle(content.objects.at(index).definition);
        require(authored && authored->source_graph==clip.source_graph && authored->source_index==clip.source_index &&
            clip.frame_count==100,"SCEN frame producer lost its actual initialized clip");
        ++frame_calls;return 9.50000095367431640625f;
    });
    scene.update_pose(2,2,mesh);
    require(frame_calls==2,"SCEN frame producer did not supersede elapsed seconds");
    scene.animation_frame([](std::size_t,std::uint32_t,std::uint64_t,std::uint64_t,const AnimationClip &clip) {
        return float(clip.frame_count);
    });rejected=false;
    try{scene.update_pose(3,3,mesh);}catch(const FormatError &){rejected=true;}
    require(rejected,"SCEN retained frame silently wrapped outside the source clip");
    float fraction=0;unsigned paired_calls=0;
    scene.animation_frames([&](std::size_t index,std::uint32_t handle,std::uint64_t birth,
        std::uint64_t tick,const AnimationClip &clip) {
        require(objects.get(handle).placement==index && objects.get(handle).created_tick==birth && tick==100,
            "SCEN fractional pose lost its actual owner and clock");
        const auto definition=cache.load_source_scenery_idle_phase_definition(clip);
        auto current=source_scenery_idle_phase_initial(definition,1.f/30.f);
        for(unsigned t=0;t<99;++t)source_scenery_idle_phase_step(current,definition,1.f/30.f);
        const auto before=current;source_scenery_idle_phase_step(current,definition,1.f/30.f);
        require((current.status&4u)!=0,"SCEN source loop fixture did not wrap");++paired_calls;
        return AnimationFrameWindow{before.frame,current.frame,fraction};
    });
    scene.lighting_query_state([&](std::size_t index,std::uint32_t,const Transform &node,float scale) {
        const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(content.objects[index].definition),node,scale);
        return SourceSceneryQueryState{sphere.center,sphere.radius,{},0,true};
    });
    std::vector<Vertex> previous;
    for(unsigned sample=0;sample<=4;++sample) {
        fraction=float(sample)/4;scene.update_pose(100,4+int(sample),mesh);
        std::vector<Vertex> vertices;std::vector<ObjectLightingBinding> bindings;
        scene.append_view(vertices,bindings,SourceObjectViewLod{0,1,false,[](std::size_t,std::uint32_t,Vec3,float) {
            return SourceObjectLodInput{0,0x80000,0};}});
        require(!vertices.empty(),"SCEN fractional teleporter pose emitted no geometry");
        if(sample) {
            require(previous.size()==vertices.size(),"SCEN fractional pose changed selected geometry");
            bool moved=false;for(unsigned v=0;v<vertices.size();++v)moved|=!same(vertices[v].position,previous[v].position);
            require(moved,"SCEN geometry held its last key across a loop boundary");
        }
        previous=std::move(vertices);
    }
    require(paired_calls==10,"SCEN loop pose did not sample both real owners at each fraction");
    const auto resident_materials=mesh.materials.size();
    scene.animation_frames({});scene.animation_frame({});
    scene.animation_time([](std::size_t,std::uint32_t,std::uint64_t,std::uint64_t){return 0.f;});
    rejected=false;try{scene.update_pose(0,0,mesh);}catch(const FormatError &){rejected=true;}
    require(rejected,"Unannounced SCEN clock rollback was accepted");
    scene.reset_lifetimes();std::vector<Vertex> retired;std::vector<ObjectLightingBinding> retired_bindings;
    scene.append_view(retired,retired_bindings,SourceObjectViewLod{0,1,false,[](std::size_t,std::uint32_t,Vec3,float){
        return SourceObjectLodInput{0,0x80000,0};}});
    require(retired.empty()&&retired_bindings.empty(),"Retired SCEN still drew stale geometry");
    scene.synchronize(objects);scene.update_pose(0,0,mesh);
    scene.append_view(retired,retired_bindings,SourceObjectViewLod{0,1,false,[](std::size_t,std::uint32_t,Vec3,float){
        return SourceObjectLodInput{0,0x80000,0};}});
    require(!retired.empty()&&mesh.materials.size()==resident_materials,"SCEN adoption lost geometry or changed uploaded material slots");
    // Compare the adopted frame against an independently prepared scene at
    // that cut, rather than deriving expected vertices from the reset code.
    Mesh fresh_mesh;SourceSceneryScene fresh(cache,content.objects,0,fresh_mesh,collision);
    fresh.animation_time([](std::size_t,std::uint32_t,std::uint64_t,std::uint64_t){return 0.f;});
    fresh.lighting_query_state([&](std::size_t index,std::uint32_t,const Transform &node,float scale){
        const auto sphere=source_shadow_sphere(cache.load_object_shadow_definition(content.objects[index].definition),node,scale);
        return SourceSceneryQueryState{sphere.center,sphere.radius,{},0,true};});
    fresh.synchronize(objects);fresh.update_pose(0,0,fresh_mesh);
    std::vector<Vertex> expected;std::vector<ObjectLightingBinding> expected_bindings;
    fresh.append_view(expected,expected_bindings,SourceObjectViewLod{0,1,false,[](std::size_t,std::uint32_t,Vec3,float){
        return SourceObjectLodInput{0,0x80000,0};}});
    require(retired.size()==expected.size()&&retired_bindings.size()==expected_bindings.size(),"SCEN adoption changed collector coverage");
    for(unsigned v=0;v<expected.size();++v)require(same(retired[v].position,expected[v].position)&&
        same(retired[v].normal,expected[v].normal)&&retired[v].material==expected[v].material,"SCEN adoption retained an old pose/LOD/material");
    std::cout<<"SCEN world adoption: same salted handles, tick100 to tick0, resident resources and fresh-scene geometry equality\n";
    std::cout<<"SCEN presentation: actual Ascension teleporter geometry moves at four fractional samples across source loop wrap\n";
    std::cout<<"SCEN animation: two actual moving initial clips require explicit retained owner/time service\n";
}
void checkpoint_roundtrip() {
    Cache cache("assets/disc/maps/ascension.map");const auto content=cache.load_content(0,true);
    CollisionWorld collision(content.collision);Mesh mesh;SourceSceneryScene scene(cache,content.objects,0,mesh,collision);
    SceneObjects objects(content,0);unsigned animation_calls{},query_calls{},birth_calls{},reentrant_calls{};
    MultiplayerSceneRuntimeValues callback_values;
    SourceSceneryScene::Checkpoint cut;const SourceSceneryScene::PreparedRestore *observed{};
    const auto reentrant=[&]{
        if(!observed)return;++reentrant_calls;bool capture_rejected{},prepare_rejected{};
        try{(void)scene.checkpoint();}catch(const FormatError &){capture_rejected=true;}
        try{(void)scene.prepare_restore(cut);}catch(const FormatError &){prepare_rejected=true;}
        require(capture_rejected&&prepare_rejected&&!scene.restore_ready(*observed),
            "SCEN callback admitted a checkpoint/preparation during an unfinished mutation");
        // The existing noexcept lifetime reset must also decline reentry.
        scene.reset_lifetimes();
    };
    scene.birth_query_capture([&](std::size_t index,std::uint32_t handle,std::uint64_t born,
        std::uint32_t definition,const Transform &node,float scale){
        ++birth_calls;callback_values.scenery_initial_state.capture(handle,definition,
            cache.load_object_shadow_definition(definition),node,scale);
        MultiplayerSceneRuntimeValues::SceneryQueryLifetime lifetime;
        lifetime.placement=index;lifetime.created_tick=born;lifetime.advanced_tick=born;
        require(callback_values.scenery_query_lifetimes.emplace(handle,std::move(lifetime)).second,
            "SCEN callback values repeated an actual full lifetime");
    });
    scene.animation_frames([&](std::size_t index,std::uint32_t handle,std::uint64_t born,std::uint64_t tick,const AnimationClip &clip){
        require(objects.get(handle).placement==index&&objects.get(handle).created_tick==born&&tick>=born,
            "SCEN checkpoint controller lost its actual full lifetime");++animation_calls;
        auto &lifetime=callback_values.scenery_query_lifetimes.at(handle);
        if(!lifetime.idle_definition){lifetime.idle_definition=cache.load_source_scenery_idle_phase_definition(clip);
            lifetime.idle_state=source_scenery_idle_phase_initial(*lifetime.idle_definition,1.f/30.f);}
        require(tick>=lifetime.advanced_tick,"SCEN external idle history moved backwards");
        while(lifetime.advanced_tick<tick){lifetime.previous_idle_state=lifetime.idle_state;lifetime.idle_paired=true;
            source_scenery_idle_phase_step(lifetime.idle_state,*lifetime.idle_definition,1.f/30.f);++lifetime.advanced_tick;}
        lifetime.native_animated_bounds_admitted=true;
        return AnimationFrameWindow{lifetime.previous_idle_state.frame,lifetime.idle_state.frame,callback_values.scenery_render_fraction};
    });
    scene.lighting_query_state([&](std::size_t index,std::uint32_t handle,const Transform &node,float scale){
        require(objects.get(handle).placement==index,"SCEN checkpoint query lost its actual full handle");
        ++query_calls;reentrant();const auto &state=callback_values.scenery_initial_state.refresh_cached_bounds(handle,
            content.objects[index].definition,cache.load_object_shadow_definition(content.objects[index].definition),node,scale,true);
        return SourceSceneryQueryState{state.sphere.center,state.sphere.radius,state.velocity,state.query_runtime_flags,state.frozen};
    });
    callback_values.scenery_render_fraction=.375f;
    scene.synchronize(objects);scene.update_pose(10,10,mesh);
    struct Output {std::vector<Vertex> vertices;std::vector<ObjectLightingBinding> bindings;std::vector<SourceObjectViewSelection> draws;};
    const auto draw=[&](unsigned mode,bool inspect=false){Output out;
        for(unsigned view=0;view<4;++view){SourceObjectViewLod input{view,1,false,[&](std::size_t index,std::uint32_t,Vec3,float){
                if(inspect)reentrant();float metric=0;
                if(mode==1){const auto definition=cache.load_source_geometry_lod_definition(content.objects[index].definition);
                    const auto &bank=definition.model_bank;
                    const bool authored=definition.model_present&&bank[3]>0&&bank[3]>=bank[4]&&bank[4]>=bank[5]&&
                        bank[5]>=bank[6]&&bank[6]>=bank[7]&&bank[8]==0;
                    metric=authored?(bank[6]+bank[7])*.5f:3.f;
                }else if(mode==2)metric=100000.f;
                return SourceObjectLodInput{metric,mode==0?0x80000u:0u,0};}};
            input.detail_policy=SourceGeometryLodPolicy{5,1,1,1,{}};
            scene.append_view(out.vertices,out.bindings,input,&out.draws);
        }return out;};
    const auto equal_output=[&](const Output &a,const Output &b){
        if(a.vertices.size()!=b.vertices.size()||a.bindings.size()!=b.bindings.size()||a.draws.size()!=b.draws.size())return false;
        if(!a.vertices.empty()&&std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(Vertex)))return false;
        for(std::size_t i=0;i<a.bindings.size();++i){const auto &x=a.bindings[i],&y=b.bindings[i];
            if(x.first_material!=y.first_material||x.material_count!=y.material_count)return false;
            const auto p=x.state.source_values(),q=y.state.source_values();if(std::memcmp(p.data(),q.data(),sizeof(p)))return false;}
        for(std::size_t i=0;i<a.draws.size();++i){const auto &x=a.draws[i],&y=b.draws[i];
            if(x.source_object!=y.source_object||x.lod!=y.lod||x.vertices!=y.vertices||
                std::bit_cast<std::uint32_t>(x.transition)!=std::bit_cast<std::uint32_t>(y.transition)||
                std::bit_cast<std::uint32_t>(x.source_distance)!=std::bit_cast<std::uint32_t>(y.source_distance))return false;}
        return true;};
    (void)draw(0);const auto expected=draw(1);cut=scene.checkpoint();
    const auto callback_cut=callback_values;
    auto &lifetime_alias=callback_values.scenery_query_lifetimes;
    auto &bounds_alias=callback_values.scenery_initial_state;
    require(cut.valid()&&scene.owns_checkpoint(cut)&&!expected.vertices.empty()&&birth_calls&&query_calls,
        "SCEN exact checkpoint lacks actual posed/submitted source lifetimes");
    struct Owner {std::size_t placement;std::uint32_t handle;std::uint64_t birth;std::vector<Transform> nodes;std::vector<std::uint8_t> permutations;};
    std::vector<Owner> expected_owners;
    for(const auto &o:scene.effect_owners())expected_owners.push_back({o.placement,o.object,o.created_tick,
        {o.nodes.begin(),o.nodes.end()},{o.permutations.begin(),o.permutations.end()}});
    const auto material_count=mesh.materials.size();const auto *material_bank=mesh.materials.data();
    callback_values.scenery_render_fraction=.625f;
    scene.update_pose(100,31,mesh);(void)draw(2);const auto later=draw(1);
    require(scene.owns_checkpoint(cut)&&!equal_output(later,expected),"SCEN historical checkpoint expired or later authored frame did not change");
    bool hysteresis_control{};for(std::size_t i=0;i<expected.draws.size()&&i<later.draws.size();++i)
        hysteresis_control|=expected.draws[i].source_object==later.draws[i].source_object&&expected.draws[i].lod!=later.draws[i].lod;
    require(hysteresis_control,"SCEN authored four-view corpus did not exercise different retained LOD choices");
    {auto cancelled=scene.prepare_restore(cut);require(scene.restore_ready(cancelled),"SCEN exact checkpoint did not prepare");}
    require(equal_output(draw(1),later),"Cancelled SCEN preparation changed the live posed/query/LOD bank");
    Mesh foreign_mesh;SourceSceneryScene foreign(cache,content.objects,0,foreign_mesh,collision);
    bool rejected{};try{(void)foreign.prepare_restore(cut);}catch(const FormatError &){rejected=true;}
    require(rejected&&!foreign.owns_checkpoint(cut),"SCEN checkpoint crossed a resident scene owner");
    rejected=false;try{(void)scene.prepare_restore({});}catch(const FormatError &){rejected=true;}
    require(rejected,"SCEN accepted an empty checkpoint");
    auto stale=scene.prepare_restore(cut);rejected=false;
    try{scene.update_pose(99,32,mesh);}catch(const FormatError &){rejected=true;}
    require(rejected&&!scene.restore_ready(stale)&&!scene.commit_restore(stale)&&scene.owns_checkpoint(cut),
        "Rejected SCEN clock mutation preserved preparation eligibility or retired historical ownership");
    auto query_token=scene.prepare_restore(cut);observed=&query_token;scene.update_pose(100,32,mesh);(void)draw(1,true);observed=nullptr;
    require(reentrant_calls&&!scene.restore_ready(query_token),"SCEN query/view callbacks did not exercise the active mutation barrier");
    auto prepared=scene.prepare_restore(cut),competing=scene.prepare_restore(cut);
    const auto retired_owners=scene.effect_owners();require(!retired_owners.empty()&&!retired_owners.front().nodes.empty(),"SCEN retired WORLD bank missing");
    const auto retired_nodes=std::vector<Transform>(retired_owners.front().nodes.begin(),retired_owners.front().nodes.end());
    std::exception_ptr thread_error;std::thread thread([&]{try{
        require(!scene.owns_checkpoint(cut)&&!scene.restore_ready(prepared)&&!scene.commit_restore(prepared),"SCEN restore crossed its executor");
        bool denied{};try{(void)scene.checkpoint();}catch(const FormatError &){denied=true;}require(denied,"SCEN foreign executor captured state");
        denied=false;try{(void)scene.prepare_restore(cut);}catch(const FormatError &){denied=true;}require(denied,"SCEN foreign executor prepared state");
        scene.reset_lifetimes();
    }catch(...){thread_error=std::current_exception();}});thread.join();if(thread_error)std::rethrow_exception(thread_error);
    const auto callbacks=std::array{animation_calls,query_calls,birth_calls};
    const auto later_callback_values=callback_values;
    require(scene.restore_ready(prepared)&&scene.commit_restore(prepared)&&!scene.restore_ready(prepared)&&
        !scene.commit_restore(prepared)&&!scene.commit_restore(competing),"SCEN exact restore was not owner/revision-bound and single use");
    auto retired_callback_values=callback_cut;callback_values.swap(retired_callback_values);
    require(&lifetime_alias==&callback_values.scenery_query_lifetimes&&&bounds_alias==&callback_values.scenery_initial_state,
        "SCEN external value publication invalidated stable callback aliases");
    require(callback_values.scenery_render_fraction==.375f&&retired_callback_values.scenery_render_fraction==.625f,
        "SCEN value swap lost the actual captured and retired render fractions");
    require(callback_values.scenery_query_lifetimes.size()==callback_cut.scenery_query_lifetimes.size()&&
        retired_callback_values.scenery_query_lifetimes.size()==later_callback_values.scenery_query_lifetimes.size(),
        "SCEN callback value swap changed occupied lifetime coverage");
    unsigned animated_carry{},static_carry{};
    for(const auto &[handle,lifetime]:callback_values.scenery_query_lifetimes){
        const auto &saved=callback_cut.scenery_query_lifetimes.at(handle);
        const auto &future=retired_callback_values.scenery_query_lifetimes.at(handle);
        require(same_lifetime(lifetime,saved),
            "SCEN external copy/swap reconstructed or lost its actual idle controller carry");
        require(same_lifetime(future,later_callback_values.scenery_query_lifetimes.at(handle)),
            "SCEN callback value swap discarded the retired later idle history");
        // Every admitted source owner has constructor bounds, but only the
        // authored multi-frame idle path calls the controller phase producer.
        const auto authored=cache.load_initialized_object_idle(content.objects.at(lifetime.placement).definition);
        if(authored&&authored->frame_count>1){
            ++animated_carry;
            require(lifetime.idle_definition&&lifetime.idle_definition->graph==authored->source_graph&&
                lifetime.idle_definition->animation==authored->source_index&&lifetime.advanced_tick==10&&
                future.advanced_tick==100&&lifetime.idle_paired&&future.idle_paired&&
                lifetime.native_animated_bounds_admitted&&future.native_animated_bounds_admitted&&
                !same_idle(lifetime.idle_state,future.idle_state),
                "SCEN animated copy/swap failed actual tick10/tick100 retained controller coverage");
        }else{
            ++static_carry;
            require(!lifetime.idle_definition&&!future.idle_definition&&lifetime.advanced_tick==lifetime.created_tick&&
                future.advanced_tick==future.created_tick&&!lifetime.idle_paired&&!future.idle_paired&&
                !lifetime.native_animated_bounds_admitted&&!future.native_animated_bounds_admitted,
                "SCEN static copy/swap fabricated an idle controller or advanced its constructor clock");
        }
    }
    require(animated_carry==2&&static_carry>0,"SCEN callback copy corpus lost its actual animated/static owner controls");
    require(callbacks==std::array{animation_calls,query_calls,birth_calls}&&mesh.materials.size()==material_count&&mesh.materials.data()==material_bank,
        "SCEN commit invoked a source callback or replaced resident material resources");
    require(!std::memcmp(retired_owners.front().nodes.data(),retired_nodes.data(),retired_nodes.size()*sizeof(Transform)),
        "SCEN commit destroyed the caller token's retired WORLD containers");
    const auto restored_owners=scene.effect_owners();require(restored_owners.size()==expected_owners.size(),"SCEN restored lifetime count changed");
    for(std::size_t i=0;i<restored_owners.size();++i){const auto &o=restored_owners[i];const auto &e=expected_owners[i];
        require(o.placement==e.placement&&o.object==e.handle&&o.created_tick==e.birth&&o.nodes.size()==e.nodes.size()&&
            !std::memcmp(o.nodes.data(),e.nodes.data(),e.nodes.size()*sizeof(Transform))&&std::ranges::equal(o.permutations,e.permutations),
            "SCEN restore changed full salted lifetime, regions or exact posed WORLD nodes");}
    for(const auto &o:restored_owners){const auto &authored=content.objects.at(o.placement);
        const auto &bounds=callback_values.scenery_initial_state.retained(o.object,authored.definition,o.nodes.front(),authored.scale,false);
        const auto actual=source_shadow_sphere(cache.load_object_shadow_definition(authored.definition),o.nodes.front(),authored.scale);
        require(same(bounds.sphere.center,actual.center)&&std::bit_cast<std::uint32_t>(bounds.sphere.radius)==std::bit_cast<std::uint32_t>(actual.radius),
            "SCEN external historical bounds did not match the restored actual WORLD pose");}
    const auto reads=query_calls;require(equal_output(draw(1),expected)&&query_calls==reads,
        "SCEN restore lost its six posed streams, four-view LOD history or retained query/lighting cache");
    auto before_reset=scene.prepare_restore(cut);scene.reset_lifetimes();require(!scene.restore_ready(before_reset)&&draw(1).vertices.empty(),
        "SCEN reset preserved stale preparation or active draws");
    auto after_reset=scene.prepare_restore(cut);require(scene.commit_restore(after_reset)&&equal_output(draw(1),expected),
        "Historical SCEN restore required cold reconstruction after lifetime retirement");
    auto before_rebind=scene.prepare_restore(cut);scene.animation_time({});
    require(!scene.owns_checkpoint(cut)&&!scene.restore_ready(before_rebind),"SCEN callback rebinding retained an incompatible checkpoint");
    std::cout<<"PASS exact owning SCEN checkpoint: authored Ascension poses, full lifetimes/regions, four-view LOD/query/lighting, cancellation/staleness/reentry/executor and deferred retirement; resident resources unchanged\n";
    std::cout<<"PASS owning scene callback values: actual Ascension constructor bounds and retained idle carry survive historical copy/swap; field aliases remain stable\n";
    std::cout<<"SCEN callback copy census: animated="<<animated_carry<<" static="<<static_carry<<'\n';
}
void callback_auxiliary_values() {
    MultiplayerSceneRuntimeValues values;
    require(std::ranges::all_of(values.object_light_units,[](auto unit){return unit==null_datum;})&&
        std::ranges::none_of(values.object_light_alive,[](auto alive){return alive;})&&
        std::ranges::none_of(values.object_light_reported,[](auto reported){return reported;}),
        "Scene callback values changed the actual empty object-light defaults");
    Cache prop_cache("assets/disc/maps/lockout.map");const auto prop_content=prop_cache.load_content(0,true);
    SceneObjects prop_objects(prop_content,0);const auto prop=std::find_if(prop_objects.objects().begin(),prop_objects.objects().end(),[&](const auto &o){
        return o.handle!=null_datum&&prop_content.objects.at(o.placement).type==11&&
            prop_cache.load_initial_model_selection(prop_content.objects.at(o.placement).definition,
                prop_content.objects.at(o.placement).variant).resolved();});
    require(prop!=prop_objects.objects().end(),"Scene callback copy fixture lacks an actual Lockout prop");
    const auto &authored=prop_content.objects.at(prop->placement);
    const auto selected=prop_cache.load_initial_model_selection(authored.definition,authored.variant);
    const auto model=prop_cache.load_model(authored.definition,false,authored.variant,selected);
    const auto nodes=pose_model_nodes_world(model,scene_object_transform(*prop));
    require(!nodes.empty(),"Actual prop constructor has no WORLD node0");
    const auto initial=values.prop_initial_state.capture(prop->handle,authored.definition,
        prop_cache.load_object_shadow_definition(authored.definition),nodes.front(),prop->scale);
    values.prop_query_handles.insert(prop->handle);values.actual_prop_regions=selected;
    Cache portal_cache("assets/map-pack/maps/gemini.map");const auto portal_content=portal_cache.load_content(0,true);
    CollisionWorld collision(portal_content.collision);Mesh mesh;
    SourceMachineScene machines(portal_cache,portal_content.objects,0,mesh,collision);
    SceneObjects objects(portal_content,0);machines.synchronize(objects);
    const auto definition=portal_cache.load_source_portal_mask_definition(0);
    std::vector<SourcePortalOwnerState> owners;for(const auto &packet:machines.portal_owners())owners.push_back(packet.owner);
    values.source_portal_mask.reset_acoustic();values.source_portal_mask.initialize(definition,owners);
    for(const auto &owner:owners)values.source_portal_owners.emplace(owner.handle,owner);
    require(std::ranges::count_if(owners,[](const auto &owner){return owner.count!=0;})==6,
        "Scene callback copy fixture lost the six actual Gemini portal ranges");
    machines.update_pose(1,1,mesh);
    for(const auto &packet:machines.portal_owners())values.source_portal_mask.step(definition,
        values.source_portal_owners.at(packet.owner.handle),packet.door);
    values.source_portal_tick=1;const auto checkpoint=values;
    auto &bounds_alias=values.prop_initial_state;auto &regions_alias=values.actual_prop_regions;
    auto &mask_alias=values.source_portal_mask;auto &owners_alias=values.source_portal_owners;
    values.prop_initial_state.destroy(prop->handle);values.prop_query_handles.clear();values.actual_prop_regions.reset();
    for(const auto &owner:owners)values.source_portal_mask.destroy(definition,owner);
    values.source_portal_owners.clear();
    auto retired=checkpoint;values.swap(retired);
    require(&bounds_alias==&values.prop_initial_state&&&regions_alias==&values.actual_prop_regions&&
        &mask_alias==&values.source_portal_mask&&&owners_alias==&values.source_portal_owners,
        "Auxiliary scene publication invalidated existing callback field aliases");
    const auto &restored=values.prop_initial_state.retained(prop->handle,authored.definition,nodes.front(),prop->scale,false);
    require(same(restored.sphere.center,initial.sphere.center)&&
        std::bit_cast<std::uint32_t>(restored.sphere.radius)==std::bit_cast<std::uint32_t>(initial.sphere.radius)&&
        restored.frozen==initial.frozen&&restored.query_runtime_flags==initial.query_runtime_flags&&
        values.prop_query_handles==checkpoint.prop_query_handles&&values.actual_prop_regions&&
        values.actual_prop_regions->permutations==selected.permutations&&values.actual_prop_regions->states==selected.states&&
        values.source_portal_tick==1&&values.source_portal_mask.words==checkpoint.source_portal_mask.words&&
        values.source_portal_mask.acoustic==checkpoint.source_portal_mask.acoustic&&
        values.source_portal_owners.size()==owners.size(),"Historical scene callback values lost actual prop bounds/regions or portal banks");
    for(const auto &owner:owners){const auto &saved=values.source_portal_owners.at(owner.handle);
        require(saved.handle==owner.handle&&saved.first==owner.first&&saved.count==owner.count&&
            saved.identity.unique_id==owner.identity.unique_id&&saved.identity.origin_bsp==owner.identity.origin_bsp,
            "Historical portal copy changed the actual full owner/range identity");}
    require(!retired.prop_initial_state.contains(prop->handle)&&retired.prop_query_handles.empty()&&
        !retired.actual_prop_regions&&retired.source_portal_owners.empty(),"Scene value swap destroyed or mixed the retired future banks");
    std::cout<<"PASS owning auxiliary scene values: actual Lockout prop constructor/regions and six Gemini portal ownership/mask/acoustic banks copied and restored without callbacks\n";
}
}
int main(int argc,char **argv) {
    try {
        Counts total;
        const auto add=[&](const auto &path){const auto c=verify(path);total.models+=c.models;total.baked+=c.baked;
            total.dynamic+=c.dynamic;total.unresolved+=c.unresolved;total.streams+=c.streams;total.baked_vertices+=c.baked_vertices;total.idle+=c.idle;};
        if(argc==2)add(std::filesystem::path(argv[1]));else {
            prepared_scenery_birth();lifetime_inputs();animation_input();checkpoint_roundtrip();callback_auxiliary_values();
            for(const auto *map:{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
                "foundation","headlong","lockout","midship","waterworks","zanzibar"})
                add(std::filesystem::path("assets/disc/maps")/(std::string(map)+".map"));
            for(const auto *map:{"containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"})
                add(std::filesystem::path("assets/map-pack/maps")/(std::string(map)+".map"));
            require(total.unresolved==4,"unresolved original SCEN RNG boundary changed");
        }
        std::cout<<"Resolved SCEN union: "<<total.models<<" models, "<<total.streams<<" exact per-view collectors, "
            <<total.baked_vertices<<" literal baked incident/RGB vertices, "<<total.idle<<" admitted initial idle controllers\n";return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
