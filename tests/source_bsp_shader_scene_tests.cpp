#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

using namespace halo2;
namespace {
using Key=std::pair<int,unsigned>;
void require(bool v,const std::string&s){if(!v)throw FormatError(s);}
float real(std::uint32_t x){return std::bit_cast<float>(x);}
std::uint32_t word(float x){return std::bit_cast<std::uint32_t>(x);}
struct RawBsp {
    std::vector<std::array<float,6>> bounds;
    std::map<unsigned,std::vector<SourceTransparentPart>> clusters,definitions;
    std::vector<SourceTransparentInstance> instances;
};
struct Literal {
    std::map<std::string,RawBsp> maps;
    std::map<std::pair<std::string,std::uint32_t>,SourceShaderLodDefinition> shaders;
};
Literal literal_banks(){
    const auto folder=std::filesystem::path(__FILE__).parent_path()/"fixtures";Literal result;
    std::ifstream shader(folder/"retail_source_shader_lod_assets.tsv");require(bool(shader),"original shader bank fixture missing");
    for(std::string line;std::getline(shader,line);){
        if(line.empty() || line[0]=='#')continue;std::istringstream in(line);std::string name;unsigned datum,bias,count;
        in>>name>>std::hex>>datum>>bias>>count;require(bool(in),"original shader bank header truncated");
        SourceShaderLodDefinition bank;bank.bias=std::uint16_t(bias);
        for(unsigned i=0;i<count;++i){unsigned bits;in>>bits;require(bool(in),"original shader threshold truncated");bank.thresholds.push_back(real(bits));}
        require(result.shaders.emplace(std::pair{name,datum},std::move(bank)).second,"duplicate original shader bank");
    }
    std::ifstream parts(folder/"retail_source_transparent_parts_assets.tsv");require(bool(parts),"original BSP part fixture missing");
    for(std::string line;std::getline(parts,line);){
        if(line.empty() || line[0]=='#')continue;std::istringstream in(line);std::string name;unsigned kind,a,b;
        in>>name>>kind>>a>>b;require(bool(in),"original BSP part fixture header truncated");
        std::vector<unsigned> values;for(unsigned v;in>>v;)values.push_back(v);auto &map=result.maps[name];
        if(kind==0 || kind==1){
            require(values.size()>=27,"original BSP part words truncated");SourceTransparentPart p;p.ordinal=std::uint16_t(b);
            std::copy_n(values.begin(),18,p.authored_words.begin());p.type=std::uint16_t(values[0]);p.flags=std::uint16_t(values[0]>>16);
            p.material=std::int16_t(values[1]);p.index_first=std::uint16_t(values[1]>>16);p.index_count=std::uint16_t(values[2]);
            p.center={real(values[4]),real(values[5]),real(values[6])};p.lod_scale=real(values[11]);p.shader=values[18];
            p.shader_priority_flags=std::uint16_t(values[19]);p.shader_command_flags=std::uint16_t(values[20]);
            const unsigned count=values[21];require(values.size()==27+count,"original part mask/plane fixture length differs");
            p.template_lod_layers.assign(values.begin()+22,values.begin()+22+count);
            if(values[22+count]){std::array<float,4> plane;for(unsigned i=0;i<4;++i)plane[i]=real(values[23+count+i]);p.plane=plane;}
            auto &section=(kind==0?map.clusters:map.definitions)[a];require(section.size()==b,"original part order differs");section.push_back(p);
        }else if(kind==3){
            require(values.size()==18 && map.instances.size()==a,"original instance fixture order/length differs");
            SourceTransparentInstance instance;instance.ordinal=a;instance.definition=values[0];instance.transform.scale=real(values[1]);
            for(unsigned i=0;i<3;++i)instance.transform.basis[i]={real(values[2+i*3]),real(values[3+i*3]),real(values[4+i*3])};
            instance.transform.position={real(values[11]),real(values[12]),real(values[13])};
            instance.center={real(values[14]),real(values[15]),real(values[16])};instance.radius=real(values[17]);map.instances.push_back(instance);
        }else if(kind==6){
            require(values.size()==6 && map.bounds.size()==a,"original cluster bound fixture order/length differs");
            std::array<float,6> bounds;for(unsigned i=0;i<6;++i)bounds[i]=real(values[i]);map.bounds.push_back(bounds);
        }
    }
    require(result.maps.size()==21 && result.shaders.size()==6450,"original21-map fixture census differs");return result;
}
std::array<std::uint32_t,33> vertex_words(const Vertex&v){
    // Compare fields rather than struct padding, without heap allocation for
    // each vertex in the Debug/sanitizer all-map run.
    return {word(v.position.x),word(v.position.y),word(v.position.z),
        word(v.color.x),word(v.color.y),word(v.color.z),word(v.uv.x),word(v.uv.y),v.material,
        word(v.lightmap_uv.x),word(v.lightmap_uv.y),word(v.normal.x),word(v.normal.y),word(v.normal.z),
        word(v.tangent.x),word(v.tangent.y),word(v.tangent.z),word(v.binormal.x),word(v.binormal.y),word(v.binormal.z),
        word(v.incident.x),word(v.incident.y),word(v.incident.z),v.lightmap,
        std::bit_cast<std::uint32_t>(v.bsp_cluster),word(v.prt_cluster),
        word(v.prt_weights[0]),word(v.prt_weights[1]),word(v.prt_weights[2]),word(v.prt_weights[3]),
        v.source_section,v.source_part,v.source_stipple_level};
}
bool same_texture(const Texture&a,const Texture&b){
    if(a.width!=b.width || a.height!=b.height || a.format!=b.format || a.layers!=b.layers || a.depth!=b.depth || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i){const auto &x=a.mips[i],&y=b.mips[i];if(x.width!=y.width || x.height!=y.height || x.offset!=y.offset || x.size!=y.size || x.depth!=y.depth)return false;}return true;
}
void same_material(const Material&a,const Material&b){
    require(a.source_shader_datum==b.source_shader_datum && a.selected_shader_lod==b.selected_shader_lod && a.forward_pass_index==b.forward_pass_index,
            "resident shader identity/index/pass differs from independently prepared source bank");
    require(a.forward_program.words==b.forward_program.words && a.forward_program.fragment_offsets==b.forward_program.fragment_offsets &&
        a.lightmap_program.words==b.lightmap_program.words && a.lightmap_program.fragment_offsets==b.lightmap_program.fragment_offsets &&
        a.vertex_program==b.vertex_program &&
        std::bit_cast<std::array<std::uint32_t,128>>(a.program_constants)==std::bit_cast<std::array<std::uint32_t,128>>(b.program_constants) &&
        a.texture_states==b.texture_states && a.forward_raster==b.forward_raster && a.forward_blend==b.forward_blend &&
        a.forward_vertex==b.forward_vertex && a.world_pass==b.world_pass && a.water_pass==b.water_pass &&
        a.per_vertex_lighting==b.per_vertex_lighting && a.baked_lightmap_texture==b.baked_lightmap_texture && a.accumulation_texture==b.accumulation_texture,
        "resident selected shader LOD changed program/constants/sampler/raster/category bank");
    require(same_texture(a.diffuse,b.diffuse) && same_texture(a.detail,b.detail) && same_texture(a.bump,b.bump) && same_texture(a.auxiliary,b.auxiliary),
        "resident selected shader LOD changed bitmap bytes/mips/dimensions");
    require(bool(a.environment_pass)==bool(b.environment_pass),"resident selected LOD changed accumulation admission");
    if(a.environment_pass)same_material(*a.environment_pass,*b.environment_pass);
    for(const auto &list:std::array<std::pair<const std::vector<std::shared_ptr<Material>>*,const std::vector<std::shared_ptr<Material>>*>,3>{{
        {&a.world_overlays_before,&b.world_overlays_before},{&a.world_overlays_after,&b.world_overlays_after},{&a.world_bloom_passes,&b.world_bloom_passes}}}){
        require(list.first->size()==list.second->size(),"resident selected LOD changed ordered child passes");
        for(unsigned i=0;i<list.first->size();++i)same_material(*list.first->at(i),*list.second->at(i));
    }
}
struct FixtureCamera {SourceViewProjectionBank bank;SourceFrustumCamera frustum;std::int16_t top{},bottom{};bool modifier{};};
FixtureCamera camera(Vec3 eye,Vec3 forward,unsigned players,unsigned index){
    const auto grid=source_view_grid(players,index,false);const auto angles=source_camera_angles(1.2217304706573486f,1.3333333730697632f,grid.grid,grid.span);
    const int width=640/grid.grid[0],height=480/grid.grid[1];
    SourceFrustumProjectionInput p;p.projection_angle=angles.projection_angle;
    p.outer={std::int16_t(grid.position[1]*height),std::int16_t(grid.position[0]*width),
        std::int16_t((grid.position[1]+grid.span[1])*height),std::int16_t((grid.position[0]+grid.span[0])*width)};
    // Explicit source-camera fixture: safe==outer, feature/global overrides off.
    // This tests the original producer; it does not infer live UI overscan.
    p.safe=p.outer;const auto projection=source_frustum_projection(p);
    const Vec3 up{0,0,1};const auto bank=source_view_projection({eye,forward,up,.1f,1024.f,{},projection.projection_program});
    const auto frustum=source_frustum_camera({eye,forward,up,.1f,1024.f,projection.clipping_rectangle()});
    return {bank,frustum,p.outer[0],p.outer[2],projection.view_modifier};
}
struct Expected {unsigned section{},lod{},material{};std::uint8_t coverage{},alpha=255;bool omit{};};
struct Stats {unsigned views{},cluster_parts{},instance_parts{},lower{},producer_lower{},producer_alpha_zero{},alpha_zero_instances{},alpha_zero_vertices{},partial_alpha{},changed_worlds{},resident_pairs{};};
void map_test(const Cache&cache,const std::string&name,const Literal&literal,Stats&total){
    const auto &raw=literal.maps.at(name);const auto geometry=cache.load_source_bsp_admission(0);const auto atmosphere=cache.load_bsp_atmosphere(0);
    require(raw.bounds.size()==geometry.clusters.size() && raw.instances.size()==geometry.instances.size(),"original BSP fixture owner census differs");
    for(unsigned i=0;i<raw.bounds.size();++i)require(std::bit_cast<std::array<std::uint32_t,6>>(raw.bounds[i])==
        std::bit_cast<std::array<std::uint32_t,6>>(geometry.clusters[i].bounds),"selected cluster bounds differ from literal original fixture");
    for(unsigned i=0;i<raw.instances.size();++i){const auto &a=raw.instances[i];const auto &b=geometry.instances[i];require(a.definition==unsigned(b.definition) &&
        word(a.center.x)==word(b.center.x) && word(a.center.y)==word(b.center.y) && word(a.center.z)==word(b.center.z) && word(a.radius)==word(b.radius),
        "selected instance sphere/definition differs from literal original fixture");}
    auto mesh=cache.load_bsp(0);const auto roots=unsigned(mesh.materials.size());const auto original_vertices=mesh.vertices;
    std::map<Key,unsigned> source_roots;
    for(const auto&v:original_vertices){auto[it,fresh]=source_roots.emplace(Key{v.bsp_cluster,v.source_part},v.material);
        require(fresh || it->second==v.material,"authored part has multiple original material identities");}
    std::vector<std::vector<unsigned>> slots(roots);std::map<std::tuple<unsigned,unsigned,bool>,Material> prepared;unsigned next=roots;
    for(unsigned root=0;root<roots;++root){const auto &m=mesh.materials[root];
        if(m.source_shader_datum==0xffffffffu || (m.water_pass && m.forward_vertex==8))continue;
        const auto &bank=literal.shaders.at({name,m.source_shader_datum});const bool per_vertex=m.per_vertex_lighting || (m.environment_pass && m.environment_pass->per_vertex_lighting);
        slots[root].push_back(root);for(unsigned lod=1;lod<bank.thresholds.size();++lod)slots[root].push_back(next++);
        for(unsigned lod=0;lod<bank.thresholds.size();++lod){const auto key=std::tuple{m.source_shader_datum,lod,per_vertex};
            if(!prepared.contains(key)){auto material=cache.load_material(m.source_shader_datum);material.selected_shader_lod=lod;
                require(prepare_world_material(cache,material,per_vertex),"independent selected source material requires unsupported pass");prepared.emplace(key,std::move(material));}
        }
    }
    SourceBspShaderScene scene(cache,mesh,0);require(mesh.materials.size()==next,"resident shader LOD bank changed root/alternative allocation contract");
    for(unsigned root=0;root<roots;++root)for(unsigned lod=0;lod<slots[root].size();++lod){
        const auto &m=mesh.materials.at(root);const bool per_vertex=m.per_vertex_lighting || (m.environment_pass && m.environment_pass->per_vertex_lighting);
        same_material(mesh.materials.at(slots[root][lod]),prepared.at({m.source_shader_datum,lod,per_vertex}));
    }
    std::set<std::array<std::uint32_t,2>> pairs;
    for(const auto&v:original_vertices)if(v.lightmap)for(unsigned material:slots.at(v.material))pairs.insert({material,v.lightmap});
    require(std::vector<std::array<std::uint32_t,2>>(pairs.begin(),pairs.end())==mesh.resident_material_lightmaps,
        "selected shader LOD omitted or cross-bound a resident authored baked-lightmap descriptor pair");
    total.resident_pairs+=unsigned(pairs.size());
    const auto expected=[&](const FixtureCamera &cam,const SourceBspAdmission &admission,unsigned players){
        std::map<Key,Expected> out;const auto coverage_camera=source_shader_coverage_camera(cam.bank);
        const auto view=source_shader_lod_view(cam.top,cam.bottom,false,false);const auto multiplier=source_shader_lod_player_multiplier(players);
        const auto section=[&](int owner,unsigned section_id,const std::vector<SourceTransparentPart>&parts,float coverage,unsigned alpha,
                               const SourceTransparentInstance*instance){
            for(const auto&p:parts){const Key key{owner,p.ordinal};const auto root=source_roots.find(key);if(root==source_roots.end())continue;
                const auto &bank=slots.at(root->second);if(!alpha){out.emplace(key,Expected{section_id,0,0,0,0,true});continue;}
                if(bank.empty())continue;const auto &shader=literal.shaders.at({name,p.shader});
                const auto part_geometry=instance?source_transparent_instance_geometry(p,*instance):source_transparent_cluster_geometry(p);
                const auto collector=source_shader_coverage_byte(coverage);const auto metric=source_shader_part_metric(collector,part_geometry.lod_scale,multiplier);
                const auto choice=source_shader_lod_choice(shader.thresholds,shader.bias,metric,view,cam.modifier);
                const bool omit=!choice.accepted || choice.index<0;
                out.emplace(key,Expected{section_id,unsigned(std::max(choice.index,0)),omit?0:bank.at(unsigned(choice.index)),collector,std::uint8_t(alpha),omit});
            }
        };
        for(const auto&r:admission.clusters){const auto parts=raw.clusters.find(r.id);if(parts==raw.clusters.end())continue;
            section(int(r.id),r.id,parts->second,source_shader_cluster_coverage(raw.bounds.at(r.id),coverage_camera),255,nullptr);}
        for(const auto&r:admission.instances){const auto&i=raw.instances.at(r.id);const auto parts=raw.definitions.find(i.definition);if(parts==raw.definitions.end())continue;
            const auto coverage=source_shader_instance_coverage(i.center,i.radius,coverage_camera);section(-2-int(r.id),i.definition,parts->second,coverage,source_shader_instance_alpha(coverage),&i);}
        return out;
    };
    Stats map;std::vector<Vertex> retained_a;bool different_view=false;
    const auto exercise=[&](const FixtureCamera &cam,const SourceBspAdmission &admission,unsigned players,const char*label,bool producer){
        std::vector<Vertex> input;for(const auto&v:original_vertices)if(admission.geometry_visible(v.bsp_cluster))input.push_back(v);
        const auto untouched=input;const auto want=expected(cam,admission,players);const auto selected=scene.select(input,{cam.bank,admission,cam.top,cam.bottom,players,cam.modifier});
        std::map<Key,const SourceBspShaderPart*> actual;for(const auto&s:selected)require(actual.emplace(Key{s.geometry,s.part},&s).second,"selector repeated authored part command");
        unsigned accepted=0;std::set<int> alpha_zero;
        for(const auto&[key,e]:want){if(e.omit){require(!actual.contains(key),"selector emitted source alpha-zero/rejected instance command");if(key.first<=-2 && !e.alpha)alpha_zero.insert(key.first);continue;}
            ++accepted;require(actual.contains(key),"selector omitted accepted original part command");const auto&s=*actual.at(key);
            require(s.section==e.section && s.shader_lod==e.lod && s.material==e.material && s.coverage_byte==e.coverage && s.instance_alpha==e.alpha,
                name+" "+label+" part"+std::to_string(key.first)+":"+std::to_string(key.second)+" differs from literal source helper composition");
            if(key.first>=0)++map.cluster_parts;else ++map.instance_parts;if(e.lod){++map.lower;if(producer)++map.producer_lower;}if(e.alpha && e.alpha<255)++map.partial_alpha;
        }
        require(accepted==actual.size(),"selector emitted unauthored or unadmitted part command");unsigned at=0;
        const std::string vertex_error=name+" "+label+" vertex remap/stipple/order/retained attributes differ";
        for(const auto&v:untouched){auto expected_vertex=v;const auto it=want.find({v.bsp_cluster,v.source_part});
            if(it!=want.end()){const auto&e=it->second;if(e.omit){if(!e.alpha)++map.alpha_zero_vertices;continue;}
                expected_vertex.material=e.material;expected_vertex.source_stipple_level=source_shader_instance_stipple(e.alpha).level;}
            require(at<input.size() && vertex_words(input[at])==vertex_words(expected_vertex),
                vertex_error);++at;
        }
        require(at==input.size(),"selector appended or failed to omit original vertices");map.alpha_zero_instances+=unsigned(alpha_zero.size());
        if(producer)map.producer_alpha_zero+=unsigned(alpha_zero.size());++map.views;
        if(retained_a.empty())retained_a=input;
        else if(!input.empty() && (input.size()!=retained_a.size() || vertex_words(input.front())!=vertex_words(retained_a.front())))different_view=true;
        if(producer)require(!admission.clusters.empty(),name+" original camera/portal fixture admitted no cluster geometry");
        return input;
    };
    std::vector<Vec3> eyes;for(const auto&s:cache.spawns()){
        auto p=s.position;p.z+=.35f;if(atmosphere.cluster_at(p)>=0)eyes.push_back(p);if(eyes.size()==8)break;
    }
    require(!eyes.empty(),name+" has no authored spawn with valid explicit source-camera fixture");
    const auto portals=cache.load_source_portal_bsp(0);const std::array<std::uint32_t,16> open=[](){std::array<std::uint32_t,16>a;a.fill(0xffffffffu);return a;}();
    unsigned eye=0;for(unsigned players:{1u,2u,4u})for(unsigned view=0;view<players;++view){
        const Vec3 forward=view%2?Vec3{0,1,0}:Vec3{1,0,0};const auto cam=camera(eyes.at(eye++%eyes.size()),forward,players,view);
        const auto visible=source_portal_visibility(portals,std::span(&cam.frustum,1),atmosphere.cluster_at(cam.frustum.camera_to_world.position),open);
        const auto fog=atmosphere.sample(cam.frustum.camera_to_world.position,forward);
        const auto containing=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,0});SourceBspAdmissionInput context;context.containing_fog=containing;
        const auto admission=source_bsp_admission(geometry,visible,context);exercise(cam,admission,players,"source camera/portal/queue",true);
    }
    // Bounded consumer fixtures retain the complete authored command bank.
    // Their far/partial-alpha cameras are explicit inputs, not a claim that
    // main-mode portal visibility admits an outside-map gameplay camera.
    SourceBspAdmission all;for(unsigned i=0;i<geometry.clusters.size();++i)if(geometry.clusters[i].geometry_count)all.clusters.push_back({i});
    for(unsigned i=0;i<geometry.instances.size();++i){const auto definition=geometry.instances[i].definition;
        if(definition>=0 && geometry.definitions.at(unsigned(definition)).geometry_count)all.instances.push_back({i});}
    auto far_eye=eyes.front();far_eye.x-=100000;const auto far=camera(far_eye,{1,0,0},4,3);
    const auto near=camera(eyes.front(),{1,0,0},1,0);const auto a=exercise(near,all,1,"all-command near consumer",false);
    const auto a_copy=a;exercise(far,all,4,"all-command far consumer",false);
    require(a.size()==a_copy.size(),"second view rewrote first view vertex storage");for(unsigned i=0;i<a.size();++i)require(vertex_words(a[i])==vertex_words(a_copy[i]),"second view mutated prior view");
    const auto repeat=exercise(near,all,1,"repeat independent near consumer",false);require(repeat.size()==a.size(),"later view altered source selection history");
    for(unsigned i=0;i<a.size();++i)require(vertex_words(repeat[i])==vertex_words(a[i]),"later view changed independent first-view shader/material/stipple output");
    for(const auto &instance:raw.instances){if(!(instance.radius>0) || !source_roots.contains({-2-int(instance.ordinal),0}))continue;
        const auto base=camera(instance.center,{1,0,0},1,0);float distance=instance.radius*base.bank.viewport_half_metrics.y*2/28.f;
        if(instance.radius<.13f)distance*=1.75f;auto eye=instance.center;eye.x-=distance;
        const auto partial=camera(eye,{1,0,0},1,0);const auto coverage=source_shader_instance_coverage(instance.center,instance.radius,source_shader_coverage_camera(partial.bank));
        const auto alpha=source_shader_instance_alpha(coverage);if(!alpha || alpha==255)continue;
        exercise(partial,all,1,"authored instance partial-alpha consumer",false);break;
    }
    require(map.lower>0 && map.alpha_zero_vertices>0 && different_view,name+" did not exercise real lower-LOD/alpha-zero/independent view geometry");
    for(unsigned root=0;root<roots;++root)for(unsigned lod=0;lod<slots[root].size();++lod){
        const auto &m=mesh.materials.at(root);const bool per_vertex=m.per_vertex_lighting || (m.environment_pass && m.environment_pass->per_vertex_lighting);
        same_material(mesh.materials.at(slots[root][lod]),prepared.at({m.source_shader_datum,lod,per_vertex}));
    }
    ++map.changed_worlds;
    total.views+=map.views;total.cluster_parts+=map.cluster_parts;total.instance_parts+=map.instance_parts;total.lower+=map.lower;
    total.producer_lower+=map.producer_lower;total.producer_alpha_zero+=map.producer_alpha_zero;
    total.alpha_zero_instances+=map.alpha_zero_instances;total.alpha_zero_vertices+=map.alpha_zero_vertices;total.partial_alpha+=map.partial_alpha;total.changed_worlds+=map.changed_worlds;
    std::cout<<name<<": "<<map.views<<" views, "<<map.lower<<" lower selected parts ("<<map.producer_lower<<" through source portal/admission), "<<map.alpha_zero_instances<<" alpha-zero instance submissions ("<<map.producer_alpha_zero<<" through source portal/admission) / "
        <<map.alpha_zero_vertices<<" omitted vertices, "<<map.partial_alpha<<" partial-alpha parts, "<<pairs.size()<<" resident material/lightmap pairs\n";
}
}
int main(int argc,char**argv){try{
    std::cout<<std::unitbuf;
    const auto disc=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("assets/disc/maps");
    const auto dlc=argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/map-pack/maps");const auto literal=literal_banks();
    constexpr std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout",
        "midship","waterworks","zanzibar","containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};Stats total;
    for(unsigned i=0;i<names.size();++i){const Cache cache((i<12?disc:dlc)/(std::string(names[i])+".map"));map_test(cache,names[i],literal,total);}
    require(total.changed_worlds==21 && total.lower>100 && total.producer_lower>100 && total.producer_alpha_zero>0 &&
        total.partial_alpha>0 && total.alpha_zero_instances>100 && total.resident_pairs>100,
        "all21 source selector regression lacks nonempty admission/LOD/alpha/residency controls");
    std::cout<<"21 maps: "<<total.views<<" views, "<<total.cluster_parts<<" cluster and "<<total.instance_parts<<" instance accepted parts, "<<total.lower
        <<" selected lower LODs ("<<total.producer_lower<<" through source portal/admission), "<<total.alpha_zero_instances<<" alpha-zero instances ("<<total.producer_alpha_zero<<" through source portal/admission) / "<<total.alpha_zero_vertices<<" omitted vertices, "
        <<total.partial_alpha<<" partial-alpha parts, "<<total.resident_pairs<<" literal required resident pairs\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
