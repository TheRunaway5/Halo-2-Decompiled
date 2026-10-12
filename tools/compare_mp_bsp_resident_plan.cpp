#define main frozen_vertex_run_pack_main
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "benchmark_mp_vertex_run_pack.cpp"
#pragma GCC diagnostic pop
#undef main
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/hud.hpp"
#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <set>

namespace {
using namespace halo2;
Key tuple(const SourceBspDrawKey &k) {return {k.material,k.lightmap,k.geometry,k.section,k.part};}
void vertices_equal(std::span<const Vertex> a,std::span<const Vertex> b) {
    if(a.size()!=b.size() || (!a.empty() && std::memcmp(a.data(),b.data(),a.size_bytes())))
        throw FormatError("resident plan changed a full vertex stream");
}
bool part_equal(const SourceBspShaderPart &a,const SourceBspShaderPart &b) {
    return std::tie(a.geometry,a.section,a.part,a.shader_lod,a.material,a.coverage_byte,a.instance_alpha)==
        std::tie(b.geometry,b.section,b.part,b.shader_lod,b.material,b.coverage_byte,b.instance_alpha);
}
bool same_texture(const Texture &a,const Texture &b) {
    if(a.width!=b.width || a.height!=b.height || a.format!=b.format || a.layers!=b.layers || a.depth!=b.depth ||
        a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i) {
        const auto &x=a.mips[i],&y=b.mips[i];
        if(std::tie(x.width,x.height,x.offset,x.size,x.depth)!=std::tie(y.width,y.height,y.offset,y.size,y.depth))return false;
    }
    return true;
}
void material_equal(const Material &a,const Material &b) {
    if(a.name!=b.name || a.source_shader_datum!=b.source_shader_datum || a.selected_shader_lod!=b.selected_shader_lod ||
        a.forward_pass_index!=b.forward_pass_index || a.forward_program.words!=b.forward_program.words ||
        a.forward_program.fragment_offsets!=b.forward_program.fragment_offsets || a.lightmap_program.words!=b.lightmap_program.words ||
        a.lightmap_program.fragment_offsets!=b.lightmap_program.fragment_offsets || a.vertex_program!=b.vertex_program ||
        std::bit_cast<std::array<std::uint32_t,128>>(a.program_constants)!=std::bit_cast<std::array<std::uint32_t,128>>(b.program_constants) ||
        a.texture_states!=b.texture_states || a.forward_raster!=b.forward_raster || a.forward_blend!=b.forward_blend ||
        a.forward_vertex!=b.forward_vertex || a.world_pass!=b.world_pass || a.water_pass!=b.water_pass ||
        a.per_vertex_lighting!=b.per_vertex_lighting || a.baked_lightmap_texture!=b.baked_lightmap_texture ||
        a.accumulation_texture!=b.accumulation_texture || !same_texture(a.diffuse,b.diffuse) ||
        !same_texture(a.detail,b.detail) || !same_texture(a.bump,b.bump) || !same_texture(a.auxiliary,b.auxiliary))
        throw FormatError("resident constructor changed program/constants/bitmap/sampler/raster/category bank");
    if(bool(a.environment_pass)!=bool(b.environment_pass))throw FormatError("resident constructor changed environment admission");
    if(a.environment_pass)material_equal(*a.environment_pass,*b.environment_pass);
    for(const auto &lists:std::array<std::pair<const std::vector<std::shared_ptr<Material>>*,const std::vector<std::shared_ptr<Material>>*>,3>{{
        {&a.world_overlays_before,&b.world_overlays_before},{&a.world_overlays_after,&b.world_overlays_after},
        {&a.world_bloom_passes,&b.world_bloom_passes}}}) {
        if(lists.first->size()!=lists.second->size())throw FormatError("resident constructor changed ordered child pass count");
        for(unsigned i=0;i<lists.first->size();++i)material_equal(*lists.first->at(i),*lists.second->at(i));
    }
}
std::vector<Vertex> token_vertices(const SourceBspResidentPlan &resident,const SourceBspMergedDrawPlan &merged,
    const SourceBspDrawToken &token,unsigned dynamic_first) {
    std::span<const Vertex> span;
    if(token.storage==SourceBspVertexStorage::resident)span=std::span{resident.resident_vertices()}.subspan(token.first,token.count);
    else {
        if(token.first<dynamic_first)throw FormatError("dynamic token precedes supplied actual staging prefix");
        span=std::span{merged.dynamic_vertices}.subspan(token.first-dynamic_first,token.count);
    }
    std::vector<Vertex> vertices(span.begin(),span.end());
    // Vertex::material is not a GPU attribute. Restore its explicit draw
    // binding solely for a complete CPU-record comparison, never a GPU result.
    if(token.storage==SourceBspVertexStorage::resident)for(auto &v:vertices)v.material=token.key.material;
    return vertices;
}
void merged_equal(const SourceBspResidentPlan &resident,std::span<const SourceBspPartBinding> bindings,
    std::span<const Vertex> objects,unsigned materials,unsigned lightmaps,unsigned dynamic_first=0,unsigned logical_first=0) {
    Fixture fixture{materials,lightmaps,{}};Output original;
    original.vertices.resize(logical_first);
    auto vertices=resident.expand(bindings);vertices.insert(vertices.end(),objects.begin(),objects.end());
    group(vertices,original,fixture,0,false,Method::Scatter);
    const auto merged=resident.merge(bindings,objects,materials,lightmaps,dynamic_first,logical_first);
    if(merged.draws.size()!=original.draws.size() || merged.descriptors.size()!=original.descriptors.size() ||
        merged.logical_vertices!=vertices.size())throw FormatError("resident plan changed complete draw counts");
    for(unsigned i=0;i<merged.draws.size();++i) {
        const auto &token=merged.draws[i];const auto &old=original.draws[i];
        if(tuple(token.key)!=old.key || token.logical_first!=old.first || token.count!=old.count ||
            merged.descriptors[i].material!=original.descriptors[i].material ||
            merged.descriptors[i].lightmap!=original.descriptors[i].lightmap)
            throw FormatError("resident plan changed sorted full draw/descriptor requests");
        const auto actual=token_vertices(resident,merged,token,dynamic_first);
        vertices_equal(actual,std::span{original.vertices}.subspan(old.first,old.count));
    }
}
void catalogue_equal(const Mesh &mesh,const SourceBspResidentPlan &resident) {
    std::map<Key,unsigned> indices;std::vector<Draw> draws;
    for(const auto &v:mesh.vertices) {
        const auto k=key(v);const auto [it,inserted]=indices.try_emplace(k,unsigned(draws.size()));
        if(inserted)draws.push_back({0,0,0,k});++draws[it->second].count;
    }
    unsigned first{};std::vector<unsigned> cursor;
    for(auto &draw:draws){draw.first=first;cursor.push_back(first);first+=draw.count;}
    std::vector<Vertex> packed(mesh.vertices.size());
    for(const auto &v:mesh.vertices)packed[cursor[indices.at(key(v))]++]=v;
    vertices_equal(packed,resident.resident_vertices());
    if(draws.size()!=resident.parts().size())throw FormatError("resident catalogue changed authored draw count");
    for(unsigned i=0;i<draws.size();++i) {
        const auto &part=resident.parts()[i];
        if(tuple(part.original)!=draws[i].key || part.first!=draws[i].first || part.count!=draws[i].count)
            throw FormatError("resident catalogue changed original physical ranges/order");
    }
}
void selection_equal(SourceBspShaderScene &baseline,const SourceBspResidentPlan &resident,const Mesh &mesh,
    const SourceBspShaderView &input,std::array<std::size_t,3> &decisions,unsigned &suppressed_alpha,unsigned &suppressed_lod) {
    auto before=mesh.vertices;const auto old_parts=baseline.select(before,input);const auto plan=resident.select(input);
    vertices_equal(before,resident.expand(plan.bindings));
    if(old_parts.size()!=plan.selected_parts.size() || plan.bindings.size()!=resident.parts().size() ||
        plan.decisions.size()!=resident.parts().size())throw FormatError("resident selection changed complete record counts");
    for(unsigned i=0;i<old_parts.size();++i)if(!part_equal(old_parts[i],plan.selected_parts[i]))
        throw FormatError("resident selection changed ordered accepted source records");
    for(unsigned i=0;i<plan.decisions.size();++i) {
        const auto &d=plan.decisions[i];const auto &binding=plan.bindings[i];++decisions[unsigned(d.state)];
        if(d.state==SourceBspPartDecision::untouched && (!binding.material ||
            *binding.material!=resident.parts()[i].original.material || binding.stipple || d.selection ||
            d.suppression!=SourceBspPartSuppression::none))throw FormatError("untouched part lost its raw defaults");
        if(d.state==SourceBspPartDecision::accepted && (!binding.material || !binding.stipple || !d.selection ||
            d.suppression!=SourceBspPartSuppression::none))throw FormatError("accepted part lacks actual selection fields");
        if(d.state==SourceBspPartDecision::suppressed) {
            if(binding.material || binding.stipple || d.selection || d.suppression==SourceBspPartSuppression::none)
                throw FormatError("suppressed part retained a fabricated draw binding");
            if(d.suppression==SourceBspPartSuppression::zero_alpha)++suppressed_alpha;else ++suppressed_lod;
        }
    }
    merged_equal(resident,plan.bindings,{},unsigned(mesh.materials.size()),unsigned(mesh.lightmaps.size()+1));
}
}
int main(int argc,char **argv) {
    using namespace halo2;
    try {
        if(argc!=6)throw FormatError("Supply MAP SHARED UPDATE_XBE COMPLETE_GROUP_CAPTURE_STEM OUTPUT_JSON");
        const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
        const auto content=cache.load_content(0,true);
        auto mesh=cache.load_bsp(0),candidate_mesh=mesh;
        SourceBspShaderScene baseline(cache,mesh,0);const SourceBspResidentPlan resident(cache,candidate_mesh,0);
        if(mesh.materials.size()!=candidate_mesh.materials.size() || mesh.resident_material_lightmaps!=candidate_mesh.resident_material_lightmaps)
            throw FormatError("resident plan changed resident LOD/lightmap bank construction");
        for(unsigned i=0;i<mesh.materials.size();++i)material_equal(mesh.materials[i],candidate_mesh.materials[i]);
        catalogue_equal(mesh,resident);
        const auto portals=cache.load_source_portal_bsp(0);const auto geometry=cache.load_source_bsp_admission(0);
        const auto atmosphere=cache.load_bsp_atmosphere(0);
        SourcePortalMaskState mask;mask.initialize(cache.load_source_portal_mask_definition(0),{});
        auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=cache.slayer_starting_weapon(5);
        MultiplayerMatch match(content,0,cache.spawns(),rules);std::array<Controls,multiplayer_capacity> controls{};
        for(unsigned player=0;player<4;++player){const auto id=match.join("Profile "+std::to_string(player));controls[id].yaw=match.unit(id).yaw;}
        std::array<std::size_t,3> decisions{};unsigned alpha{},lod{},authored_cases{},source_controls{};
        for(unsigned test=0;test<16;++test) {
            match.step(controls);const auto state=match.state();
            for(unsigned views:{1u,4u})for(unsigned view=0;view<views;++view) {
                const auto &p=state.players[view];const auto forward=direction(p.yaw,p.pitch);
                const auto aim=source_observer_aim(forward,cross(forward,{-std::sin(p.yaw),std::cos(p.yaw),0}),content.player.camera_field_of_view,content.player_screen_offset);
                const auto position=ordinary_unit_eye(p.position,content.player.camera_height,content.player.crouch_camera_height,p.crouch);
                const auto grid=source_view_grid(views,view,true);
                const auto angles=source_camera_angles(content.player.camera_field_of_view,1.3333333730697632f,grid.grid,grid.span);
                const auto viewport=hud_viewport(views,view,true);
                const auto rectangle=[](UiRect r){return std::array<std::int16_t,4>{std::int16_t(r.top*1.5f),std::int16_t(r.left*2.f),std::int16_t(r.bottom*1.5f),std::int16_t(r.right*2.f)};};
                SourceFrustumProjectionInput projection_input;projection_input.projection_angle=angles.projection_angle;
                projection_input.outer=rectangle(viewport.outer);projection_input.safe=rectangle(viewport.safe);
                projection_input.scenario_present=true;projection_input.scenario_type=cache.load_source_geometry_scenario_type();
                const auto projection=source_frustum_projection(projection_input);
                const auto camera=source_view_projection({position,aim.forward,aim.up,.015625f,1024.f,{},projection.projection_program});
                const auto frustum=source_frustum_camera({position,aim.forward,aim.up,.015625f,1024.f,projection.clipping_rectangle()});
                const auto fog=atmosphere.sample(position,aim.forward);
                const auto visibility=source_portal_visibility(portals,std::span{&frustum,1},fog.cluster,mask.words);
                SourceBspAdmissionInput admission_input;admission_input.containing_fog=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,0});
                const auto admission=source_bsp_admission(geometry,visibility,admission_input);
                const SourceBspShaderView input{camera,admission,projection_input.outer[0],projection_input.outer[2],views,projection.view_modifier};
                selection_equal(baseline,resident,mesh,input,decisions,alpha,lod);++authored_cases;
                if(test==15 && views==4 && view==3) {
                    // Declared algorithm controls, not claimed game requests.
                    SourceBspAdmission empty;
                    selection_equal(baseline,resident,mesh,{camera,empty,input.outer_top,input.outer_bottom,views,input.view_modifier},decisions,alpha,lod);++source_controls;
                    auto duplicate=admission;duplicate.clusters.insert(duplicate.clusters.end(),admission.clusters.begin(),admission.clusters.end());
                    duplicate.instances.insert(duplicate.instances.end(),admission.instances.begin(),admission.instances.end());
                    selection_equal(baseline,resident,mesh,{camera,duplicate,input.outer_top,input.outer_bottom,views,input.view_modifier},decisions,alpha,lod);++source_controls;
                }
            }
        }
        const auto fixture=load(argv[4]);Output reference,logical;std::vector<Vertex> dynamic_staging;
        unsigned reconstructed_parts{},reconstructed_vertices{},captured_calls{},resident_draws{},resident_vertices{},dynamic_draws{};
        unsigned merge_controls{};
        using Geometry=std::tuple<int,unsigned,unsigned,unsigned>;
        const auto identity=[](const Vertex &v)->Geometry{return {v.bsp_cluster,v.source_section,v.source_part,v.lightmap};};
        for(unsigned call=0;call<fixture.calls.size();++call) {
            const auto &input=fixture.calls[call];const auto previous_draws=reference.draws.size();
            group(input.vertices,reference,fixture,call,input.preserve,Method::Scatter);
            const bool has_bsp=std::any_of(input.vertices.begin(),input.vertices.end(),[](const auto &v){return v.bsp_cluster!=-1;});
            if(!has_bsp || input.preserve) {
                const auto before=logical.vertices.size();group(input.vertices,logical,fixture,call,input.preserve,Method::Scatter);
                dynamic_staging.insert(dynamic_staging.end(),logical.vertices.begin()+before,logical.vertices.end());
            } else {
                ++captured_calls;std::map<Geometry,std::vector<Vertex>> captured_parts;std::vector<Vertex> objects,bsp;
                for(const auto &v:input.vertices)if(v.bsp_cluster==-1)objects.push_back(v);else {captured_parts[identity(v)].push_back(v);bsp.push_back(v);}
                // Captured outputs supply only these render bindings. They do
                // not recover accepted-vs-untouched source decisions/cameras.
                std::vector<SourceBspPartBinding> bindings(resident.parts().size());
                for(unsigned i=0;i<resident.parts().size();++i) {
                    const auto &part=resident.parts()[i];
                    const auto found=captured_parts.find({part.original.geometry,part.original.section,part.original.part,part.original.lightmap});
                    if(found==captured_parts.end())continue;
                    if(found->second.size()!=part.count)throw FormatError("captured BSP count differs from catalogue");
                    const auto &v=found->second.front();bindings[i]={v.material,v.source_stipple_level};
                    for(const auto &other:found->second)if(other.material!=v.material || other.source_stipple_level!=v.source_stipple_level)
                        throw FormatError("captured binding is not constant per part");
                    ++reconstructed_parts;reconstructed_vertices+=part.count;
                }
                vertices_equal(bsp,resident.expand(bindings));
                const auto dynamic_first=unsigned(dynamic_staging.size()),logical_first=unsigned(logical.vertices.size());
                const auto merged=resident.merge(bindings,objects,fixture.materials,fixture.lightmaps,dynamic_first,logical_first);
                if(merged.draws.size()!=reference.draws.size()-previous_draws)throw FormatError("captured merged draw count changed");
                for(unsigned i=0;i<merged.draws.size();++i) {
                    const auto &token=merged.draws[i];const auto &old=reference.draws[previous_draws+i];
                    if(tuple(token.key)!=old.key || token.logical_first!=old.first || token.count!=old.count ||
                        merged.descriptors[i].material!=std::get<0>(old.key) || merged.descriptors[i].lightmap!=std::get<1>(old.key))
                        throw FormatError("captured merged sorted requests changed");
                    auto expanded=token_vertices(resident,merged,token,dynamic_first);
                    logical.vertices.insert(logical.vertices.end(),expanded.begin(),expanded.end());
                    logical.draws.push_back(old);logical.descriptors.push_back({call,token.key.material,token.key.lightmap});
                    if(token.storage==SourceBspVertexStorage::resident){++resident_draws;resident_vertices+=token.count;}else ++dynamic_draws;
                }
                dynamic_staging.insert(dynamic_staging.end(),merged.dynamic_vertices.begin(),merged.dynamic_vertices.end());
                if(call==3) {
                    // Explicit same-key object collision, alternating runs,
                    // nonzero prefixes and invalid bounds for the consumer.
                    auto collision=objects;const auto selected=resident.expand(bindings);
                    collision.insert(collision.end(),selected.begin(),selected.begin()+3);
                    merged_equal(resident,bindings,collision,fixture.materials,fixture.lightmaps,117,271);++merge_controls;
                    std::reverse(collision.begin(),collision.end());
                    merged_equal(resident,bindings,collision,fixture.materials,fixture.lightmaps,7,91);++merge_controls;
                    merged_equal(resident,bindings,{},fixture.materials,fixture.lightmaps,17,31);++merge_controls;
                    for(bool material:{false,true}) {
                        auto bad=objects.front();if(material)bad.material=fixture.materials;else bad.lightmap=fixture.lightmaps;
                        bool stopped{};try{resident.merge(bindings,std::span{&bad,1},fixture.materials,fixture.lightmaps);}catch(const FormatError &){stopped=true;}
                        if(!stopped)throw FormatError("resident consumer omitted dynamic bounds validation");++merge_controls;
                    }
                    auto removed=bindings;for(auto &binding:removed)binding.material.reset();
                    merged_equal(resident,removed,objects,fixture.materials,fixture.lightmaps,19,37);++merge_controls;
                    auto invalid=bindings;
                    const auto live=std::find_if(invalid.begin(),invalid.end(),[](const auto &binding){return bool(binding.material);});
                    live->material=fixture.materials;bool stopped{};
                    try{resident.merge(invalid,objects,fixture.materials,fixture.lightmaps);}catch(const FormatError &){stopped=true;}
                    if(!stopped)throw FormatError("resident consumer omitted selected-material bounds validation");++merge_controls;
                    stopped=false;try{resident.merge(std::span{bindings}.first(bindings.size()-1),{},fixture.materials,fixture.lightmaps);}
                    catch(const FormatError &){stopped=true;}
                    if(!stopped)throw FormatError("resident consumer accepted a mismatched catalogue");++merge_controls;
                    // Every old descriptor request can stop independently;
                    // compare exact ordered request/draw prefixes. No handle
                    // or actual provider failure is manufactured by the plan.
                    for(unsigned stop=0;stop<merged.descriptors.size();++stop) {
                        for(unsigned i=0;i<=stop;++i) {
                            const auto &old=reference.draws[previous_draws+i];const auto &request=merged.descriptors[i];
                            if(request.material!=std::get<0>(old.key) || request.lightmap!=std::get<1>(old.key) ||
                                merged.draws[i].logical_first!=old.first || merged.draws[i].count!=old.count)
                                throw FormatError("resident plan changed an unavailable descriptor prefix");
                        }
                        ++merge_controls;
                    }
                }
            }
            equal(reference,logical);
        }
        if(reconstructed_parts!=870 || reconstructed_vertices!=172596)throw FormatError("complete original capture coverage changed");
        std::ofstream out(argv[5]);if(!out)throw FormatError("open resident plan comparison report");
        out<<"{\n  \"authored_selection_cases\": "<<authored_cases<<",\n  \"source_algorithm_controls\": "<<source_controls
            <<",\n  \"untouched_decisions\": "<<decisions[0]<<",\n  \"accepted_decisions\": "<<decisions[1]
            <<",\n  \"suppressed_decisions\": "<<decisions[2]<<",\n  \"zero_alpha_suppressions\": "<<alpha
            <<",\n  \"rejected_lod_suppressions\": "<<lod<<",\n  \"captured_bsp_calls\": "<<captured_calls
            <<",\n  \"captured_bsp_parts\": "<<reconstructed_parts<<",\n  \"captured_bsp_vertices\": "<<reconstructed_vertices
            <<",\n  \"resident_draws\": "<<resident_draws<<",\n  \"resident_vertices\": "<<resident_vertices
            <<",\n  \"dynamic_world_draws\": "<<dynamic_draws<<",\n  \"complete_capture_vertices\": "<<logical.vertices.size()
            <<",\n  \"complete_capture_draws\": "<<logical.draws.size()<<",\n  \"actual_dynamic_staging_vertices\": "<<dynamic_staging.size()
            <<",\n  \"merge_algorithm_controls\": "<<merge_controls<<",\n  \"production_runtime_integrated\": false,\n  \"gpu_output_proven\": false\n}\n";
        if(!out)throw FormatError("write resident plan comparison report");
        std::cout<<"PASS "<<authored_cases<<" authored selection cases, "<<source_controls<<" selection controls; "
            <<reconstructed_parts<<" complete capture parts and all "<<logical.draws.size()<<" sorted mixed draw requests/"
            <<logical.vertices.size()<<" expanded full vertices.\nLIMIT CPU plan/attribute evidence; original capture decisions/cameras not inferred, actual GPU routing unimplemented.\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
