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
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <set>

using namespace halo2;
namespace {
void require(bool condition,const char *message){if(!condition)throw FormatError(message);}
bool same(Vec3 a,Vec3 b){return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
    std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
    std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);}
struct Counts {unsigned models{},baked{},dynamic{},unresolved{},streams{},baked_vertices{},empty{},idle{};};
Counts verify(const std::filesystem::path &path) {
    Cache cache(path);const auto content=cache.load_content(0,true);
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
        });SceneObjects objects(content,0);
    require(scene.stats().unresolved_selections==counts.unresolved &&
        scene.unresolved_selection_owners().size()==counts.unresolved,"SCEN unresolved owner boundary lost content identity");
    scene.synchronize(objects);std::map<std::size_t,unsigned> reads;
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
    std::cout<<"SCEN presentation: actual Ascension teleporter geometry moves at four fractional samples across source loop wrap\n";
    std::cout<<"SCEN animation: two actual moving initial clips require explicit retained owner/time service\n";
}
}
int main(int argc,char **argv) {
    try {
        Counts total;
        const auto add=[&](const auto &path){const auto c=verify(path);total.models+=c.models;total.baked+=c.baked;
            total.dynamic+=c.dynamic;total.unresolved+=c.unresolved;total.streams+=c.streams;total.baked_vertices+=c.baked_vertices;total.idle+=c.idle;};
        if(argc==2)add(std::filesystem::path(argv[1]));else {
            lifetime_inputs();animation_input();
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
