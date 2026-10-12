#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/reference_player.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace {
using namespace halo2;
using Words=std::vector<std::uint32_t>;
void require(bool condition,const std::string &message){if(!condition)throw FormatError(message);}
std::uint32_t word(float f){return std::bit_cast<std::uint32_t>(f);}
using ShaderKey=std::pair<std::string,std::uint32_t>;
using PartKey=std::tuple<std::string,std::uint32_t,unsigned,unsigned>;
struct ModelAsset {std::string map,role;std::uint32_t input{},mode{};std::vector<std::uint32_t> materials;};
struct PartAsset {std::uint32_t shader{};std::array<std::uint32_t,18> words{};};
struct LodAsset {unsigned range{},mask{},height{};std::vector<unsigned> layers;};
struct PassAsset {unsigned lod{},layer{},index{},implementation{};std::string name,vertex;};
struct ShaderAsset {unsigned bias{};std::vector<LodAsset> lods;std::vector<PassAsset> passes;};
struct Assets {std::vector<ModelAsset> models;std::map<PartKey,PartAsset> parts;std::map<ShaderKey,ShaderAsset> shaders;};
Assets load_assets(const std::filesystem::path &path) {
    std::ifstream file(path);require(bool(file),"literal dynamic model fixture missing");
    Assets out;std::string line;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);char kind;std::string map;row>>kind>>map>>std::hex;
        if(kind=='M') {
            ModelAsset model;model.map=map;unsigned count;row>>model.role>>model.input>>model.mode>>count;
            require(bool(row) && count<256,"literal model table invalid");
            for(unsigned i=0;i<count;++i){std::uint32_t shader;row>>shader;model.materials.push_back(shader);}
            out.models.push_back(std::move(model));
        } else if(kind=='R') {
            unsigned mode,section,part;PartAsset asset;row>>mode>>section>>part>>asset.shader;
            for(auto &v:asset.words)row>>v;
            require(out.parts.emplace(PartKey{map,mode,section,part},asset).second,"duplicate literal model part");
        } else if(kind=='L') {
            unsigned shader,lod,bias,count;LodAsset asset;row>>shader>>lod>>bias>>asset.range>>asset.mask>>asset.height>>count;
            require(bool(row) && count<32,"literal shader LOD invalid");
            for(unsigned i=0;i<count;++i){unsigned range;row>>range;asset.layers.push_back(range);}
            auto &bank=out.shaders[{map,shader}];require(lod==bank.lods.size(),"literal shader LOD order changed");
            bank.bias=bias;bank.lods.push_back(std::move(asset));
        } else if(kind=='P') {
            unsigned shader;PassAsset pass;row>>shader>>pass.lod>>pass.layer>>pass.index>>pass.implementation>>pass.name>>pass.vertex;
            out.shaders[{map,shader}].passes.push_back(std::move(pass));
        } else throw FormatError("unknown dynamic model fixture record");
        require(bool(row),"literal dynamic model fixture truncated");
        std::string extra;require(!(row>>extra),"literal dynamic model fixture has trailing fields");
    }
    require(out.models.size()==210,"all21 ten-family dynamic model coverage changed");return out;
}
Words vertex_words(const Vertex &v,bool material) {
    Words out;const auto vec=[&](Vec3 p){out.insert(out.end(),{word(p.x),word(p.y),word(p.z)});};
    vec(v.position);vec(v.color);out.insert(out.end(),{word(v.uv.x),word(v.uv.y),word(v.lightmap_uv.x),word(v.lightmap_uv.y)});
    vec(v.normal);vec(v.tangent);vec(v.binormal);vec(v.incident);
    out.insert(out.end(),{v.lightmap,std::uint32_t(v.bsp_cluster),word(v.prt_cluster)});
    for(auto f:v.prt_weights)out.push_back(word(f));
    out.insert(out.end(),{v.source_section,v.source_part,v.source_stipple_level});
    if(material)out.push_back(v.material);return out;
}
void same_vertices(std::span<const Vertex> a,std::span<const Vertex> b,bool material) {
    require(a.size()==b.size(),"shader selection changed admitted triangle count");
    for(unsigned i=0;i<a.size();++i)require(vertex_words(a[i],material)==vertex_words(b[i],material),
        "shader selection changed geometry/UV/color/baked stream/part identity");
}
Words pass_words(const Material &m) {
    Words out(m.forward_program.words.begin(),m.forward_program.words.end());
    for(auto offset:m.forward_program.fragment_offsets)out.push_back(offset);
    for(const auto&bank:m.program_constants)for(const auto&vector:bank)for(float f:vector)out.push_back(word(f));
    for(auto frame:m.texture_frames)out.push_back(frame);
    return out;
}
template<class F> void each_pass(Material &m,const F &f) {
    f(m);if(m.environment_pass)each_pass(*m.environment_pass,f);
    for(auto &p:m.world_overlays_before)each_pass(*p,f);
    for(auto &p:m.world_overlays_after)each_pass(*p,f);
    for(auto &p:m.world_bloom_passes)each_pass(*p,f);
    for(auto &p:m.world_planar_passes)if(p)each_pass(*p,f);
}
bool same_colors(const ObjectColors &a,const ObjectColors &b) {
    for(unsigned i=0;i<4;++i)if(std::bit_cast<std::array<std::uint32_t,3>>(a[i])!=std::bit_cast<std::array<std::uint32_t,3>>(b[i]))return false;
    return true;
}
bool same_texture(const Texture&a,const Texture&b) {
    if(a.width!=b.width || a.height!=b.height || a.depth!=b.depth || a.layers!=b.layers || a.format!=b.format || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i){const auto &x=a.mips[i],&y=b.mips[i];
        if(std::tie(x.width,x.height,x.depth,x.offset,x.size)!=std::tie(y.width,y.height,y.depth,y.offset,y.size))return false;}
    return true;
}
struct Counts {
    unsigned models{},vertices{},view_cases{},parts{},passes{},lower_passes{},color_passes{},animated_passes{},lower_animated{},bitmap_passes{},low_parts{},high_parts{},light_view_cases{},different_light_banks{};
};
using TextureKey=std::tuple<std::uint32_t,unsigned,unsigned>;
void source_pass(const Cache &cache,const Material &raw,const ShaderAsset &asset,
    Material &emitted,const ObjectColors &colors,std::span<const ObjectLightingState> lights,
    std::map<TextureKey,Texture> &textures,Counts &counts) {
    require(emitted.source_shader_datum==raw.source_shader_datum && emitted.selected_shader_lod<asset.lods.size(),"resident child changed authored shader/LOD identity");
    const auto &lod=asset.lods.at(emitted.selected_shader_lod);
    require(emitted.lods.size()==asset.lods.size() && emitted.lods.at(emitted.selected_shader_lod).available_layers==lod.mask &&
        emitted.lods.at(emitted.selected_shader_lod).layers.packed==lod.range && word(emitted.lods.at(emitted.selected_shader_lod).projected_height)==lod.height,
        "resident selected shader LOD differs from literal cache");
    require(emitted.forward_pass && emitted.world_pass && emitted.forward_pass_index<raw.passes.size(),"resident shader lost authored execution");
    const auto &pass=raw.passes.at(emitted.forward_pass_index);
    const auto original=std::find_if(asset.passes.begin(),asset.passes.end(),[&](const auto&p) {
        return p.lod==emitted.selected_shader_lod && p.index==pass.template_pass && p.implementation==pass.implementation && p.name==pass.name && p.vertex==pass.vertex_shader;
    });
    require(original!=asset.passes.end(),"resident pass is outside literal selected template bank");
    require(pass.implementation==(original->layer==3?2u:0u),"ordinary model selected another source lighting implementation");
    require(emitted.vertex_program==pass.vertex_code && emitted.forward_program.fragment_offsets==pass.program.fragment_offsets,
        "resident pass changed original vertex program/combiner fragment offsets");
    require(emitted.forward_program.words[54]==pass.program.words[54] && emitted.forward_program.words[59]==pass.program.words[59],
        "resident pass changed original texture addressing program");
    require(emitted.object_render_colors && same_colors(*emitted.object_render_colors,colors),"resident pass lost exact packed/unpacked draw colors");
    require(emitted.animation.overlays.size()==raw.animation.overlays.size() && emitted.animation.mappings.size()==raw.animation.mappings.size(),
        "resident pass lost authored animation bank");
    for(unsigned i=0;i<raw.animation.overlays.size();++i) {
        const auto&a=raw.animation.overlays[i],&b=emitted.animation.overlays[i];
        require(a.input==b.input && a.range==b.range && word(a.period)==word(b.period) && a.curve==b.curve,"resident shader changed literal animated curve");
    }
    for(unsigned i=0;i<raw.animation.mappings.size();++i) {
        const auto&a=raw.animation.mappings[i],&b=emitted.animation.mappings[i];
        require(std::tie(a.destination,a.component,a.source)==std::tie(b.destination,b.component,b.source),
            "resident shader changed literal animated remap");
    }
    for(unsigned i=0;i<pass.texture_states.size();++i)require(emitted.texture_states[i]==pass.texture_states[i],"resident shader changed authored sampler state");
    auto expected=pass.program;auto constants=pass.constants;
    std::array<ShaderVector,256> color_externs{};
    for(unsigned i=0;i<4;++i){constexpr unsigned indices[]{3,4,21,22};color_externs[indices[i]]={colors[i].x,colors[i].y,colors[i].z,0};}
    std::vector<ShaderRemap> color_remaps;
    for(const auto&r:pass.pixel_externs)if((r.component&7)==4 && (r.source==3 || r.source==4 || r.source==21 || r.source==22))color_remaps.push_back(r);
    remap_pixel_externs(expected,{},color_externs,color_remaps,pass.fragments,raw.shader_switches);
    bool color=!color_remaps.empty();
    for(const auto&r:pass.vertex_externs)if(r.destination==0x1c8a && r.source==3){
        std::copy_n(color_externs[3].begin(),3,constants[0][10].begin());color=true;}
    // Colors rebuild the authored pixel bank; vertex lighting remains in
    // the retained constants. The renderer binds current pixel lighting
    // after animation, in its per-view copy. These consumer helpers have
    // independent untouched D4080/33980 fixtures.
    auto initial_light_program=expected;
    apply_object_lighting_bindings(initial_light_program,constants,pass,{},raw.shader_switches,raw.specular_brightness,raw.ambient_brightness);
    require(emitted.forward_program.words==expected.words,"resident selected pass has different original bound program");
    require(std::bit_cast<std::array<std::uint32_t,128>>(emitted.program_constants)==std::bit_cast<std::array<std::uint32_t,128>>(constants),
        "resident selected pass has different original bound constants");
    const auto retained=pass_words(emitted);
    std::vector<Words> light_banks;
    for(const auto &light:lights) {
        auto actual_program=emitted.forward_program;auto actual_constants=emitted.program_constants;
        auto source_program=expected;auto source_constants=constants;
        apply_object_lighting_bindings(actual_program,actual_constants,emitted.passes.at(emitted.forward_pass_index),light,
            emitted.shader_switches,emitted.specular_brightness,emitted.ambient_brightness);
        apply_object_lighting_bindings(source_program,source_constants,pass,light,
            raw.shader_switches,raw.specular_brightness,raw.ambient_brightness);
        require(actual_program.words==source_program.words && actual_program.fragment_offsets==source_program.fragment_offsets &&
            std::bit_cast<std::array<std::uint32_t,128>>(actual_constants)==std::bit_cast<std::array<std::uint32_t,128>>(source_constants),
            "per-view model lighting differs from its literal selected pass bank");
        Words bank(actual_program.words.begin(),actual_program.words.end());
        for(const auto& c:actual_constants)for(const auto& v:c)for(const auto f:v)bank.push_back(word(f));
        light_banks.push_back(std::move(bank));++counts.light_view_cases;
    }
    require(pass_words(emitted)==retained,"per-view light binding mutated shared resident material");
    counts.different_light_banks+=light_banks.size()>1 && light_banks.front()!=light_banks.back();
    if(color)++counts.color_passes;
    const Texture*bound[]{&emitted.diffuse,&emitted.detail,&emitted.bump,&emitted.auxiliary};
    for(const auto&binding:pass.textures) {
        if(binding.external==3){require(emitted.accumulation_texture,"resident lighting lost extern3 target");continue;}
        if(!binding.external && binding.parameter==255)continue;
        unsigned datum,index=0;
        if(binding.external==27){require(emitted.scope_texture && same_texture(*bound[binding.stage],prepare_signed_texture(cache.load_scope_default_bitmap(),emitted.texture_states[binding.stage].color_sign)),"resident lower scope default differs from authored target fallback");continue;}
        if(binding.external==1) {
            const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){return t.group=="bitm" && t.name=="rasterizer\\vector_normalization";});
            require(tag!=cache.tags().end(),"source normalization bitmap missing");datum=tag->datum;
        } else {
            require(!binding.external,"unsupported resident model texture extern");
            const auto remap=std::find_if(pass.bitmap_remaps.begin(),pass.bitmap_remaps.end(),[&](const auto&r){return (r.destination&255)==binding.parameter;});
            require(remap!=pass.bitmap_remaps.end(),"resident source bitmap has no remap");
            const auto&bitmap=raw.bitmaps.at(remap->source);datum=bitmap.datum;index=bitmap.index;
        }
        const TextureKey key{datum,index,emitted.texture_states[binding.stage].color_sign};
        if(!textures.contains(key))textures.emplace(key,prepare_signed_texture(cache.load_bitmap(datum,index),std::get<2>(key)));
        require(same_texture(*bound[binding.stage],textures.at(key)),"resident shader variant changed original bitmap bytes/dimensions/mips");
    }
    ++counts.passes;counts.lower_passes+=emitted.selected_shader_lod!=0;
    if(!emitted.animation.overlays.empty()) {
        const auto input=[](std::uint32_t name){return weapon_ammunition_input(name,17).value_or(0.f);};
        auto fresh=emitted;
        animate_forward_material(emitted,.37,input);animate_material_bitmaps(emitted,.37,input);
        const auto animated=pass_words(emitted);
        emitted.forward_program.words.fill(0xfeedface);
        for(auto&bank:emitted.program_constants)for(auto&v:bank)v.fill(-23);
        animate_forward_material(emitted,.37,input);animate_material_bitmaps(emitted,.37,input);
        require(pass_words(emitted)==animated,"resident lower shader retained prior draw animation state");
        require(emitted.object_render_colors && same_colors(*emitted.object_render_colors,colors),"resident animation lost exact object draw bank");
        require(same_colors(*fresh.object_render_colors,colors),"animation mutated separate retained draw bank");
        ++counts.animated_passes;counts.lower_animated+=emitted.selected_shader_lod!=0;
        counts.bitmap_passes+=std::any_of(emitted.animated_textures.begin(),emitted.animated_textures.end(),[](const auto&stage){return !stage.empty();});
        // Keep initial state for later per-view and recolor controls.
        emitted=std::move(fresh);
    }
}
void verify_model(const Cache &cache,const Assets &assets,const ModelAsset &asset,const ObjectColors &colors,
    std::span<const ObjectLightingState> lights,Counts &counts) {
    auto model=cache.load_model(asset.input,true);
    require(!model.mesh.vertices.empty() && model.vertex_origins.size()==model.mesh.vertices.size(),"dynamic model has no retained original geometry");
    require(model.mesh.materials.size()==asset.materials.size()+1,"dynamic model lost literal material table");
    for(unsigned i=0;i<asset.materials.size();++i)require(model.mesh.materials[i].source_shader_datum==asset.materials[i],"dynamic model substituted authored material root");
    for(auto &material:model.mesh.materials)if(!material.lods.empty()) {
        require(prepare_dynamic_material(cache,material,{},true),"initial dynamic material requires unsupported source inputs: "+material.name);
        apply_object_render_colors(material,colors);
    }
    const auto initial=model.mesh.vertices;const auto skin=model.skin;const auto origins=model.vertex_origins;const auto bones=model.bones;
    SourceDynamicModelShaders resident(cache,model,asset.mode);
    same_vertices(initial,model.mesh.vertices,true);
    require(skin.size()==model.skin.size() && origins.size()==model.vertex_origins.size() && bones.size()==model.bones.size(),"resident shader preparation changed model hierarchy/stream sizes");
    for(unsigned i=0;i<bones.size();++i) {
        const auto&a=bones[i],&b=model.bones[i];
        require(a.name==b.name && a.parent==b.parent && std::bit_cast<std::array<std::uint32_t,3>>(a.bind.translation)==std::bit_cast<std::array<std::uint32_t,3>>(b.bind.translation) &&
            std::bit_cast<std::array<std::uint32_t,4>>(a.bind.rotation)==std::bit_cast<std::array<std::uint32_t,4>>(b.bind.rotation) &&
            word(a.bind.scale)==word(b.bind.scale) && word(a.inverse_scale)==word(b.inverse_scale) &&
            std::bit_cast<std::array<std::uint32_t,12>>(a.inverse)==std::bit_cast<std::array<std::uint32_t,12>>(b.inverse),
            "resident shader preparation changed literal bind hierarchy");
    }
    for(unsigned i=0;i<origins.size();++i) {
        const auto&o=origins[i],&n=model.vertex_origins[i];
        require(std::tie(o.section,o.vertex,o.part)==std::tie(n.section,n.vertex,n.part),"resident shader preparation changed source vertex origin");
        require(skin[i].bones==model.skin[i].bones && std::bit_cast<std::array<std::uint32_t,4>>(skin[i].weights)==std::bit_cast<std::array<std::uint32_t,4>>(model.skin[i].weights),"resident shader preparation changed skin influences");
        const auto&part=assets.parts.at({asset.map,asset.mode,o.section,o.part});
        require(part.shader==asset.materials.at(initial[i].material) && std::uint16_t(part.words[1])==initial[i].material,
                "expanded model part differs from literal 72-byte shader identity");
        require(initial[i].source_section==o.section && initial[i].source_part==o.part,"expanded vertex lost authored section/part identity");
    }
    std::map<TextureKey,Texture> textures;
    for(auto &material:model.mesh.materials)if(material.forward_pass) {
        const auto raw=cache.load_material(material.source_shader_datum);
        const auto &literal=assets.shaders.at({asset.map,material.source_shader_datum});
        each_pass(material,[&](auto &pass){source_pass(cache,raw,literal,pass,colors,lights,textures,counts);});
    }
    const auto posed=pose_model(model);std::set<unsigned> observed_lods;
    std::set<std::pair<unsigned,unsigned>> source_parts;
    for(const auto&v:initial)source_parts.insert({v.source_section,v.source_part});
    counts.parts+=unsigned(source_parts.size());
    for(unsigned players:std::array{1u,2u,4u}) {
        const auto policy=source_geometry_lod_policy(players);
        SourceModelShaderView view{{{0,0,-1,0},players==1?240.f:120.f},source_shader_lod_view(0,players==1?480:240,false,false),policy.coverage_multiplier,source_shader_lod_player_multiplier(players),false};
        const auto high=source_model_shader_collector({2,3,-.1f},.44f,view);
        const auto low=source_model_shader_collector({2,3,-1000.f},.44f,view);
        require(high.coverage_byte>0 && low.coverage_byte==0,"integration camera bounds did not exercise actual high/low source coverage");
        auto first=posed,second=posed;resident.select(first,high,view);resident.select(second,low,view);
        same_vertices(posed,first,false);same_vertices(posed,second,false);
        for(const auto &[collector,reference]:std::array{std::pair{high,&first},std::pair{low,&second}}) {
            std::vector<Vertex> appended(posed.begin(),posed.begin()+std::min<std::size_t>(posed.size(),3));
            const auto prefix=appended;const unsigned base=137;
            resident.append_selected(appended,posed,collector,view,base);
            auto expected=prefix;
            for(auto v:*reference) {v.material+=base;expected.push_back(v);}
            same_vertices(expected,appended,true);
        }
        const auto selected=[&](const auto &vertices,const auto &collector,bool lower) {
            for(unsigned i=0;i<vertices.size();++i) {
                const auto&v=vertices[i];const auto&raw=assets.parts.at({asset.map,asset.mode,v.source_section,v.source_part});
                const auto&definition=assets.shaders.at({asset.map,raw.shader});
                SourceShaderLodDefinition source;source.bias=std::uint16_t(definition.bias);
                for(const auto&lod:definition.lods)source.thresholds.push_back(std::bit_cast<float>(lod.height));
                const auto raw_scale=std::bit_cast<float>(raw.words[11]);
                const auto expected=source_model_shader_part(collector,raw_scale>0?raw_scale:1.f,source,view).choice;
                require(expected.accepted && expected.index>=0,"literal dynamic shader bank unexpectedly empty");
                const auto&material=model.mesh.materials.at(v.material);
                require(material.source_shader_datum==raw.shader && material.selected_shader_lod==unsigned(expected.index),
                    "per-view model slot selects a different literal shader bank");
                observed_lods.insert(material.selected_shader_lod);
                if(i==0 || std::tie(v.source_section,v.source_part)!=std::tie(vertices[i-1].source_section,vertices[i-1].source_part)) {
                    if(lower)++counts.low_parts;else ++counts.high_parts;
                }
            }
        };
        selected(first,high,false);selected(second,low,true);
        auto repeated=posed;resident.select(repeated,low,view);same_vertices(second,repeated,true);
        repeated=posed;resident.select(repeated,high,view);same_vertices(first,repeated,true);
        // A copied owner collector must select the same part banks even if
        // an attached mesh has unrelated geometry or its own small bounds.
        resident.select(first,high,view);same_vertices(repeated,first,true);
        counts.view_cases+=2;
    }
    bool multi=false;for(const auto &material:model.mesh.materials)multi|=material.lods.size()>1;
    if(multi)require(observed_lods.size()>1,"high/low actual source view coverage did not select lower authored shader banks");
    auto damaged=posed;damaged.front().source_part=0xffffffffu;bool rejected=false;
    try{resident.select(damaged,{0,0},{});}catch(const FormatError&){rejected=true;}
    require(rejected,"resident shader silently replaced a missing authored part");
    if(!model.mesh.materials.empty() && model.mesh.materials.front().forward_pass) {
        auto retained=model.mesh.materials.front();auto other=retained;
        const auto before=pass_words(retained);ObjectColors changed=colors;changed[0]={.125f,.25f,.75f};
        apply_object_render_colors(other,changed);
        require(pass_words(retained)==before && retained.object_render_colors && same_colors(*retained.object_render_colors,colors),
                "recoloring another view mutated retained resident material");
        each_pass(other,[&](auto &p){require(p.object_render_colors && same_colors(*p.object_render_colors,changed),"recolored view lost its exact draw bank");});
    }
    ++counts.models;counts.vertices+=unsigned(initial.size());
}
} // namespace
int main(int argc,char **argv) {try {
    require(argc==2,"literal dynamic model shader fixture required");
    const auto assets=load_assets(argv[1]);
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    const auto reference=load_reference_player_state(root/"tests/fixtures/retail_zanzibar_player.reference");
    const auto ivory=load_reference_player_state(root/"tests/fixtures/retail_ivory_player.reference");
    const std::array lights{reference.root_lighting,ivory.root_lighting};
    Counts total;std::set<std::string> maps,roles;
    for(const auto &asset:assets.models)maps.insert(asset.map);
    require(maps.size()==21,"dynamic model fixture lost multiplayer maps");
    for(const auto &map:maps) {
        auto path=root/"assets/disc/maps"/(map+".map");if(!std::filesystem::exists(path))path=root/"assets/map-pack/maps"/(map+".map");
        Cache cache(path);unsigned cases=0;
        for(const auto &asset:assets.models)if(asset.map==map) {
            try{verify_model(cache,assets,asset,reference.colors,lights,total);}
            catch(const std::exception&e){throw FormatError(map+" "+asset.role+": "+e.what());}
            roles.insert(asset.role);++cases;
        }
        require(cases==10,"map lost a dynamic body/held/FP family");std::cout<<map<<": 10 dynamic model families verified"<<std::endl;
    }
    require(roles.size()==10 && total.lower_passes && total.lower_animated && total.color_passes && total.low_parts && total.high_parts && total.different_light_banks,
        "dynamic model integration did not exercise lower/color/animation/coverage paths");
    std::cout<<"models="<<total.models<<" vertices="<<total.vertices<<" parts="<<total.parts<<" view_cases="<<total.view_cases
        <<" passes="<<total.passes<<" lower_passes="<<total.lower_passes<<" color_passes="<<total.color_passes
        <<" animated_passes="<<total.animated_passes<<" lower_animated="<<total.lower_animated<<" bitmap_passes="<<total.bitmap_passes
        <<" high_part_cases="<<total.high_parts<<" low_part_cases="<<total.low_parts
        <<" light_view_cases="<<total.light_view_cases<<" different_light_banks="<<total.different_light_banks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
