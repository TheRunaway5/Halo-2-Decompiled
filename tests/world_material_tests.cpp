#include "halo2/world_material.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/texture.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/source_prt.hpp"
#include "halo2/pstencil/light_response_material.hpp"
#include "halo2/pstencil/texture_accumulation_2003.hpp"
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
static_assert(source_alpha_add_blend!=pstencil::light_utility_blend_2003 &&
    source_alpha_add_blend!=pstencil::dependent_light_blend_2003 &&
    source_alpha_add_blend!=pstencil::normal_accumulation_blend_2003 &&
    source_alpha_add_blend!=pstencil::normal_light_utility_blend_2003 &&
    source_alpha_add_blend!=pstencil::texture_accumulation_blend_2003);
void check(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}
std::vector<std::byte> read(std::ifstream &stream, std::size_t at, std::size_t size) {
  std::vector<std::byte> bytes(size);
  stream.seekg(at);
  if (!stream.read(reinterpret_cast<char *>(bytes.data()), size))
    throw FormatError("truncated multiplayer material fixture");
  return bytes;
}
std::set<std::uint32_t> bsp_materials(const Cache &cache, const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary);
  check(bool(stream), "missing multiplayer cache fixture");
  std::set<std::uint32_t> materials;
  for (const auto &bsp : cache.bsps()) {
    const auto header = read(stream, bsp.file_offset + 16 + 164, 8);
    const Bytes block(header);
    const auto count = block.u32(0);
    check(count < 65536, "excessive BSP material fixture count");
    const auto offset = std::int64_t(block.u32(4)) - bsp.memory_address;
    check(offset >= 0 && std::uint64_t(offset) + std::uint64_t(count) * 32 <= bsp.size,
          "BSP material fixture outside allocation");
    const auto entries = read(stream, bsp.file_offset + std::size_t(offset), count * 32);
    const Bytes records(entries);
    for (unsigned i = 0; i < count; ++i) materials.insert(records.u32(i * 32 + 12));
  }
  return materials;
}
bool first_layer(const Material &m, unsigned layer) {
  return !m.lods.empty() && (m.lods.front().available_layers & (1u << layer));
}
using LodKey=std::tuple<std::string,std::uint32_t,unsigned>;
struct LiteralLod {
  unsigned range{},mask{},height{};
  std::vector<unsigned> layers;
};
std::map<LodKey,LiteralLod> literal_lods() {
  const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_bsp_shader_lods.tsv";
  std::ifstream file(path);check(bool(file),"missing literal BSP shader LOD fixture");
  std::map<LodKey,LiteralLod> result;std::string line;
  while(std::getline(file,line)) {
    if(line.empty() || line.front()=='#')continue;
    std::istringstream row(line);std::string map;unsigned datum,index,count;LiteralLod lod;
    row>>map>>std::hex>>datum>>index>>lod.range>>lod.mask>>lod.height>>count;
    check(bool(row) && count<=32,"truncated literal BSP shader LOD fixture");
    for(unsigned n=0;n<count;++n){unsigned v;row>>v;check(bool(row),"truncated literal layer range");lod.layers.push_back(v);}
    std::string extra;check(!(row>>extra),"extra literal BSP shader LOD words");
    check(result.emplace(LodKey{map,datum,index},std::move(lod)).second,
          "duplicate literal BSP shader LOD fixture");
  }
  check(result.size()==1362,"literal21-map shader LOD coverage changed");return result;
}
void exact_lod(const Material &m,const LiteralLod &raw,unsigned index) {
  const auto &lod=m.lods.at(index);
  check(lod.layers.packed==raw.range && lod.available_layers==raw.mask &&
        std::bit_cast<std::uint32_t>(lod.projected_height)==raw.height &&
        lod.layers.count()==raw.layers.size(),"decoded shader LOD differs from literal cache words");
  for(unsigned layer=0;layer<raw.layers.size();++layer)
    check(m.layers.at(lod.layers.first()+layer).packed==raw.layers[layer],
          "decoded ordered shader layer range differs from literal cache");
}
std::vector<unsigned> selected_passes(const Material &m,unsigned layer,unsigned implementation) {
  const auto &lod=m.lods.at(m.selected_shader_lod);
  std::vector<unsigned> result;
  if(!(lod.available_layers&(1u<<layer)))return result;
  check(layer<lod.layers.count(),"authored shader mask outside layer range");
  const auto range=m.layers.at(lod.layers.first()+layer);
  for(unsigned i=0;i<m.passes.size();++i) {
    const auto &p=m.passes[i];
    if(p.template_pass>=range.first() && p.template_pass-range.first()<range.count() &&
       p.implementation==implementation)result.push_back(i);
  }
  return result;
}
bool same_texture(const Texture&a,const Texture&b) {
  if(a.width!=b.width || a.height!=b.height || a.format!=b.format ||
     a.layers!=b.layers || a.depth!=b.depth || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
  for(unsigned i=0;i<a.mips.size();++i) {
    const auto &x=a.mips[i],&y=b.mips[i];
    if(x.width!=y.width || x.height!=y.height || x.offset!=y.offset || x.size!=y.size || x.depth!=y.depth)return false;
  }
  return true;
}
using TextureKey=std::tuple<std::uint32_t,std::uint32_t,std::uint32_t>;
void exact_textures(const Cache&cache,const Material &original,const Material &emitted,const MaterialPass&pass,
                    std::map<TextureKey,Texture>&resident) {
  const Texture*stages[]{&emitted.diffuse,&emitted.detail,&emitted.bump,&emitted.auxiliary};
  for(const auto &binding:pass.textures) {
    check(binding.stage<4,"selected source texture stage exceeds NV2A slots");
    if(binding.external==3){check(binding.stage==0 && emitted.accumulation_texture,"missing accumulation source texture");continue;}
    if(binding.external==11){check(binding.stage==3 && emitted.baked_lightmap_texture,"missing baked-lightmap source texture");continue;}
    if(binding.external==19){check(binding.stage==1 && emitted.water_reflection_texture,"missing source water reflection texture");continue;}
    if(binding.external==20){check(binding.stage==2 && emitted.water_refraction_texture,"missing source water refraction texture");continue;}
    if(binding.external==27){check(emitted.scope_texture &&
        same_texture(*stages[binding.stage],prepare_signed_texture(cache.load_scope_default_bitmap(),emitted.texture_states[binding.stage].color_sign)),
        "model scope default differs from source rasterizer bitmap");continue;}
    if(binding.parameter==255 && !binding.external)continue; // Authored coordinate/PASSTHRU stage.
    unsigned datum=0xffffffffu,index=0;
    if(binding.external==1) {
      const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){return t.group=="bitm" && t.name=="rasterizer\\vector_normalization";});
      check(tag!=cache.tags().end(),"literal normalization cubemap missing");datum=tag->datum;
    } else {
      check(!binding.external,"unresolved selected source texture extern");
      const auto remap=std::find_if(pass.bitmap_remaps.begin(),pass.bitmap_remaps.end(),[&](const auto&r){return (r.destination&255)==binding.parameter;});
      check(remap!=pass.bitmap_remaps.end(),"selected source bitmap parameter has no remap");
      const auto &b=original.bitmaps.at(remap->source);datum=b.datum;index=b.index;
    }
    const auto sign=emitted.texture_states[binding.stage].color_sign;
    const TextureKey key{datum,index,sign};
    if(!resident.contains(key))resident.emplace(key,prepare_signed_texture(cache.load_bitmap(datum,index),sign));
    check(same_texture(*stages[binding.stage],resident.at(key)),
          "selected shader LOD bound different bitmap bytes/mips/dimensions");
  }
}
void exact_pass(const Material &emitted, const Material &original, unsigned layer,
                unsigned implementation,bool object_context=false,bool dynamic_context=false) {
  check(emitted.world_pass && emitted.forward_pass, "world pass lost authored execution");
  const auto &source = original.passes.at(emitted.forward_pass_index);
  const auto &lod = original.lods.at(original.selected_shader_lod);
  const auto range = original.layers.at(lod.layers.first() + layer);
  check(source.template_pass >= range.first() &&
        source.template_pass - range.first() < range.count() &&
        source.implementation == implementation,
        "native world scheduler selected a different authored LOD/layer/implementation");
  auto constants = source.constants;
  for (const auto &r : source.vertex_externs) {
    if(dynamic_context && ((r.source>=14 && r.source<=17) || r.source==23 || r.source==24 || r.source==35))continue;
    check(r.destination == 0x1c8a && r.source == 3,
          "world vertex extern differs from original ObjectChangePrimary fixture");
    std::fill_n(constants[0][10].begin(), 3, 1.f);
  }
  auto program=source.program;
  if(object_context) {
    std::array<ShaderVector,256> colors{};colors[3]=colors[4]=colors[21]=colors[22]={1,1,1,0};std::vector<ShaderRemap> mappings;
    //33980 color-bank indices3/4/21/22 are the four change-color channels;
    //scalar21/22 remain the separately authored brightness inputs.
    for(const auto &r:source.pixel_externs)if((r.source==3 || r.source==4 || r.source==21 || r.source==22) && (r.component&7)==4)mappings.push_back(r);
    remap_pixel_externs(program,{},colors,mappings,source.fragments,original.shader_switches);
  }
  if(dynamic_context)apply_object_lighting_bindings(program,constants,source,{},original.shader_switches,
        original.specular_brightness,original.ambient_brightness);
  check(emitted.source_shader_datum==original.source_shader_datum &&
        emitted.selected_shader_lod==original.selected_shader_lod,
        "world child pass lost selected source shader identity/LOD");
  if(emitted.forward_program.words!=program.words) {
    std::ostringstream detail;detail<<"selected layer"<<layer<<" pixel words differ:";
    for(unsigned i=0;i<program.words.size();++i)if(emitted.forward_program.words[i]!=program.words[i])detail<<' '<<i<<'='<<std::hex<<emitted.forward_program.words[i]<<'/'<<program.words[i]<<std::dec;
    throw FormatError(detail.str());
  }
  check(emitted.forward_program.fragment_offsets==source.program.fragment_offsets && emitted.vertex_program==source.vertex_code,
        "native world pass changed original program offset/vertex code");
  if(std::bit_cast<std::array<std::uint32_t,128>>(emitted.program_constants)!=std::bit_cast<std::array<std::uint32_t,128>>(constants)) {
    std::ostringstream detail;detail<<"selected layer"<<layer<<" vertex constants differ:";
    for(unsigned bank=0;bank<2;++bank)for(unsigned reg=0;reg<16;++reg)for(unsigned component=0;component<4;++component)
      if(std::bit_cast<std::uint32_t>(emitted.program_constants[bank][reg][component])!=std::bit_cast<std::uint32_t>(constants[bank][reg][component]))
        detail<<' '<<bank<<':'<<reg<<':'<<component<<'='<<std::hex<<std::bit_cast<std::uint32_t>(emitted.program_constants[bank][reg][component])<<'/'<<std::bit_cast<std::uint32_t>(constants[bank][reg][component])<<std::dec;
    throw FormatError(detail.str());
  }
  check(texture_program_supported(emitted.forward_program),
        "authored multiplayer world texture program is not executable");
  check(decode_vertex_program(emitted.vertex_program).count != 0,
        "authored multiplayer world vertex program is empty");
  check(emitted.animation.overlays.size() == original.animation.overlays.size(),
        "world child pass lost authored animation overlay data");
  for (unsigned i = 0; i < source.texture_states.size(); ++i)
    check(emitted.texture_states[i] == source.texture_states[i],
          "world pass changed original sampler state");
  for (const auto &state : source.states) {
    const auto &r = emitted.forward_raster;
    switch (state.index) {
    case 57: check(r.depth_function == state.value, "world depth function changed"); break;
    case 58: check(r.alpha_function == state.value, "world alpha function changed"); break;
    case 60: check(r.alpha_test == state.value, "world alpha-test enable changed"); break;
    case 61: check(r.alpha_reference == state.value, "world alpha reference changed"); break;
    case 64: check(r.depth_write == state.value, "world depth write changed"); break;
    case 67: check(r.color_write == state.value, "world channel write mask changed"); break;
    case 143: check(r.depth_enable == state.value, "world depth enable changed"); break;
    case 147: check(r.cull == state.value, "world culling changed"); break;
    default: break;
    }
  }
  unsigned source_blend=1,destination_blend=0;bool blend=false;
  for(const auto &state:source.states) {
    if(state.index==59)blend=state.value!=0;
    if(state.index==62)source_blend=state.value;
    if(state.index==63)destination_blend=state.value;
  }
  const unsigned expected=!blend?0:source_blend==1 && destination_blend==1?3:
    source_blend==0x306 && destination_blend==0x300?6:
    source_blend==1 && destination_blend==0x303?2:
    source_blend==0x302 && destination_blend==0x303?1:
    source_blend==0x302 && destination_blend==1?source_alpha_add_blend:
    source_blend==0x305 && destination_blend==0x304?7:
    source_blend==0x304 && destination_blend==1?8:0xffffffffu;
  check(emitted.forward_blend==expected,"world pass changed original blend factors");
  ForwardRasterState raster;
  for(const auto &state:source.states)switch(state.index) {
    case 57:raster.depth_function=state.value;break;
    case 58:raster.alpha_function=state.value;break;
    case 60:raster.alpha_test=state.value;break;
    case 61:raster.alpha_reference=state.value;break;
    case 64:raster.depth_write=state.value;break;
    case 67:raster.color_write=state.value;break;
    case 143:raster.depth_enable=state.value;break;
    case 147:raster.cull=state.value;break;
    default:break;
  }
  check(emitted.forward_raster==raster,"world raster state lost original reset or selected pass writes");
}

void lower_lods(const std::filesystem::path &disc,const std::filesystem::path &dlc) {
  constexpr std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
    "foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock","deltatap",
    "turf","backwash","dune","elongation","gemini","triplicate"};
  constexpr std::array lower_counts{20u,33u,11u,27u,28u,48u,36u,72u,26u,49u,20u,37u,
    27u,23u,33u,53u,21u,18u,20u,50u,53u};
  const auto literal=literal_lods();unsigned shaders=0,lower=0,variants=0,passes=0,planar=0,water=0,different=0;
  std::array<unsigned,32> layer_counts{};std::vector<std::string> failures;
  for(unsigned map=0;map<names.size();++map) {
    const auto path=(map<12?disc:dlc)/(std::string(names[map])+".map");
    const Cache cache(path);const auto ids=bsp_materials(cache,path);shaders+=unsigned(ids.size());
    const auto atmosphere=cache.load_bsp_atmosphere(0);unsigned map_lower=0;
    std::map<TextureKey,Texture> resident;
    for(const auto datum:ids) {
      const auto source=cache.load_material(datum);
      check(source.source_shader_datum==datum,"Cache lost actual BSP source shader datum");
      for(unsigned lod=0;lod<source.lods.size();++lod) {
        const auto key=LodKey{names[map],datum,lod};
        check(literal.contains(key),"native BSP shader LOD absent from literal fixture");
        exact_lod(source,literal.at(key),lod);
        if(!lod)continue;
        ++map_lower;++lower;auto original=source;original.selected_shader_lod=lod;
        for(unsigned layer=0;layer<32;++layer)if(source.lods[lod].available_layers&(1u<<layer))++layer_counts[layer];
        if(source.lods[lod].layers.packed!=source.lods[0].layers.packed)++different;
        for(const bool per_vertex:{false,true}) {
          try {
            auto emitted=original;
            check(prepare_world_material(cache,emitted,per_vertex),"world material preparation rejected selected authored LOD");
            const auto &lit=emitted.environment_pass?*emitted.environment_pass:emitted;
            std::set<unsigned> expected,actual;
            const auto add=[&](unsigned layer,unsigned implementation){for(auto p:selected_passes(original,layer,implementation))expected.insert(p);};
            if(emitted.water_pass) {
              const unsigned layer=emitted.forward_vertex==7?15:6;
              exact_pass(emitted,original,layer,0);add(layer,0);++water;
            } else {
              check(selected_passes(original,3,per_vertex?1:0).size()==1,
                    "lower LOD has no unique authored static-lighting implementation");
              exact_pass(lit,original,3,per_vertex?1:0);add(3,per_vertex?1:0);
              check(lit.per_vertex_lighting==per_vertex && lit.baked_lightmap_texture!=per_vertex,
                    "selected lower lighting LOD lost original lighting inputs");
              if(emitted.environment_pass){exact_pass(emitted,original,1,0);add(1,0);}
              for(const unsigned layer:{2u,7u,8u,17u})add(layer,0);
            }
            const auto compare=[&](const Material&m) {
              const auto &p=original.passes.at(m.forward_pass_index);
              exact_textures(cache,original,m,p,resident);actual.insert(m.forward_pass_index);++passes;
            };
            compare(emitted);if(emitted.environment_pass)compare(*emitted.environment_pass);
            for(const auto&p:emitted.world_overlays_before){exact_pass(*p,original,2,0);compare(*p);}
            for(const auto&p:emitted.world_overlays_after){exact_pass(*p,original,p->forward_vertex==4?8:7,0);compare(*p);}
            for(const auto&p:emitted.world_bloom_passes){exact_pass(*p,original,17,0);compare(*p);check(p->world_bloom_pass,"selected LOD lost alpha bloom admission");}
            check(actual==expected,"selected lower shader LOD omitted or added an authored scheduled pass");
            check(emitted.rendering_gaps.empty(),emitted.rendering_gaps.empty()?"":emitted.rendering_gaps.front());
            ++variants;
            if(!per_vertex) {
              Mesh fog_mesh;fog_mesh.materials.push_back(emitted);
              prepare_bsp_planar_fog(cache,fog_mesh,atmosphere);
              for(unsigned implementation=0;implementation<3;++implementation) {
                const auto &p=fog_mesh.materials[0].world_planar_passes[implementation];
                const auto selected=selected_passes(original,16,implementation);
                if(atmosphere.fog_palette.empty()){check(!p,"planar pass admitted without source fog palette");continue;}
                check(selected.size()==1 && bool(p),"selected lower LOD lost unique planar implementation");
                const auto &a=original.passes.at(selected[0]);
                check(p->forward_vertex==5 && p->forward_program.words==a.program.words &&
                      p->forward_program.fragment_offsets==a.program.fragment_offsets &&
                      p->vertex_program==a.vertex_code &&
                      std::bit_cast<std::array<std::uint32_t,128>>(p->program_constants)==
                        std::bit_cast<std::array<std::uint32_t,128>>(a.constants),
                      "lower LOD planar pass differs from selected source implementation");
                for(unsigned stage=0;stage<a.texture_states.size();++stage)
                  check(p->texture_states[stage]==a.texture_states[stage],"lower LOD planar sampler differs");
                check(p->forward_raster.depth_function==0x202 && !p->forward_raster.depth_write &&
                      p->forward_blend==5,"lower LOD planar raster differs");++planar;
              }
            }
          } catch(const std::exception&e) {
            std::ostringstream report;report<<names[map]<<" datum"<<std::hex<<datum<<" mask"<<source.lods[lod].available_layers
              <<std::dec<<" LOD"<<lod<<" impl"<<(per_vertex?1:0)<<" "<<source.name<<": "<<e.what();
            failures.push_back(report.str());
          }
        }
      }
    }
    check(map_lower==lower_counts[map],std::string(names[map])+" literal lower shader LOD count changed");
    std::cout<<names[map]<<": "<<map_lower<<" literal lower shader LODs\n";
  }
  for(const auto&failure:failures)std::cerr<<failure<<'\n';
  std::cout<<shaders<<" BSP shaders; "<<lower<<" lower LODs; "<<variants<<" static-lighting variants; "
    <<passes<<" selected passes; "<<planar<<" planar variants; "<<water<<" lower water variants; "
    <<different<<" ranges different from highest LOD\n";
  std::cout<<"literal lower layers:";for(unsigned layer=0;layer<32;++layer)if(layer_counts[layer])std::cout<<' '<<layer<<'='<<layer_counts[layer];std::cout<<'\n';
  check(shaders==657 && lower==705 && different==705,"lower BSP shader LOD audit has empty or changed coverage");
  check(failures.empty(),std::to_string(failures.size())+" lower shader LOD variants require source-supported fixes");
}

struct ModelContext {
  std::string map,role;
  unsigned shader{},implementation{},prt_model{},definition{},geometry_lod{},section{};
};
struct ModelLiteral {
  std::vector<ModelContext> contexts;
  std::map<LodKey,LiteralLod> lods;
};
ModelLiteral model_literals() {
  std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_model_shader_lods.tsv");
  check(bool(file),"literal admitted model shader LOD fixture missing");ModelLiteral result;
  for(std::string line;std::getline(file,line);) {
    if(line.empty() || line.front()=='#')continue;std::istringstream row(line);std::string kind,map;
    row>>kind>>map>>std::hex;
    if(kind=="C") {
      ModelContext c;c.map=map;row>>c.shader>>c.implementation>>c.prt_model>>c.definition>>c.geometry_lod>>c.section>>c.role;
      check(bool(row) && (c.implementation==1 || c.implementation==2),"model source lighting context truncated");
      result.contexts.push_back(std::move(c));
    } else if(kind=="L") {
      unsigned shader,index,count;LiteralLod lod;row>>shader>>index>>lod.range>>lod.mask>>lod.height>>count;
      check(bool(row) && count<=32,"model literal shader LOD header truncated");
      for(unsigned n=0;n<count;++n){unsigned v;row>>v;check(bool(row),"model literal shader layers truncated");lod.layers.push_back(v);}
      check(result.lods.emplace(LodKey{map,shader,index},std::move(lod)).second,"duplicate model literal shader LOD");
    } else throw FormatError("unknown literal model bank record");
    std::string extra;check(!(row>>extra),"model literal shader bank has trailing fields");
  }
  check(result.contexts.size()==1071 && result.lods.size()==2335,"21-map model shader fixture census changed");return result;
}
void model_lower_lods(const std::filesystem::path &disc,const std::filesystem::path &dlc) {
  constexpr std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar",
    "containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
  const auto literal=model_literals();unsigned shaders=0,lower=0,variants=0,baked=0,dynamic=0,prt_owner_lower=0,passes=0;
  std::array<unsigned,32> layers{};std::vector<std::string> failures;std::map<std::string,unsigned> roles;
  for(unsigned map=0;map<names.size();++map) {
    const Cache cache((map<12?disc:dlc)/(std::string(names[map])+".map"));std::map<TextureKey,Texture> resident;
    unsigned map_variants=0,map_baked=0,map_dynamic=0,map_prt=0;std::set<unsigned> unique;
    std::map<std::pair<unsigned,unsigned>,SourceTransparentModelParts> parts;
    for(const auto &context:literal.contexts)if(context.map==names[map]) {
      unique.insert(context.shader);const auto source=cache.load_material(context.shader);
      check(source.source_shader_datum==context.shader,"model material lost literal source shader datum");
      if(context.section!=0xffffffffu) {
        const auto definition=cache.load_initial_model_definition(context.definition);
        const auto key=std::pair{definition.render_model,context.section};
        if(!parts.contains(key))parts.emplace(key,cache.load_source_transparent_model_parts(key.first,std::span(&context.section,1)));
        const auto &actual=parts.at(key).sections;
        check(actual.size()==1 && std::any_of(actual[0].parts.begin(),actual[0].parts.end(),[&](const auto&p){return p.shader==context.shader;}),
              "model shader fixture is absent from literal source collector section");
        if(context.prt_model!=0xffffffffu)check(definition.render_model==context.prt_model && bool(cache.load_source_prt_definition(context.prt_model)),
              "model fixture's PRT capability differs from actual geometry bank");
      } else {
        const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.datum==context.definition && t.address;});
        check(tag!=cache.tags().end() && (tag->group=="bipd" || tag->group=="weap" || tag->group=="mode"),"player/FP literal model reference missing");
      }
      for(unsigned lod=0;lod<source.lods.size();++lod) {
        const auto it=literal.lods.find({context.map,context.shader,lod});check(it!=literal.lods.end(),"native model shader LOD absent from literal fixture");
        exact_lod(source,it->second,lod);if(!lod)continue;
        // Every supplied lower MODEL record removes layer5, even on a PRT
        // owner. A future record retaining5 needs its actual section/LOD bank;
        // do not substitute a synthetic bank merely to pass preparation.
        check(!(it->second.mask&(1u<<5)),"new lower model PRT layer requires actual paired geometry bank fixture");
        try {
          auto original=source;original.selected_shader_lod=lod;auto emitted=original;const bool is_dynamic=context.implementation==2;
          check(is_dynamic?prepare_dynamic_material(cache,emitted,{},true):prepare_world_material(cache,emitted,true),
                "selected lower model material preparation rejected authored source inputs");
          const auto &lit=emitted.environment_pass?*emitted.environment_pass:emitted;std::set<unsigned> want,got;
          const auto add=[&](unsigned layer,unsigned impl){for(auto id:selected_passes(original,layer,impl))want.insert(id);};
          check(selected_passes(original,3,context.implementation).size()==1,"lower model shader has no unique original lighting pass");
          exact_pass(lit,original,3,context.implementation,true,is_dynamic);add(3,context.implementation);
          check(lit.per_vertex_lighting==!is_dynamic && !lit.baked_lightmap_texture,"lower model lighting classification changed");
          if(emitted.environment_pass){exact_pass(emitted,original,1,0,true,is_dynamic);add(1,0);}
          for(unsigned layer:{2u,7u,8u,17u})add(layer,0);
          const auto compare=[&](const Material&m){got.insert(m.forward_pass_index);exact_textures(cache,original,m,original.passes.at(m.forward_pass_index),resident);++passes;};
          compare(emitted);if(emitted.environment_pass)compare(*emitted.environment_pass);
          for(const auto&p:emitted.world_overlays_before){exact_pass(*p,original,2,0,true,is_dynamic);compare(*p);}
          for(const auto&p:emitted.world_overlays_after){exact_pass(*p,original,p->forward_vertex==4?8:7,0,true,is_dynamic);compare(*p);}
          for(const auto&p:emitted.world_bloom_passes){exact_pass(*p,original,17,0,true,is_dynamic);compare(*p);}
          check(want==got,"lower model shader preparation omitted or added an authored scheduled pass");
          check(emitted.rendering_gaps.empty(),emitted.rendering_gaps.empty()?"":emitted.rendering_gaps.front());
          ++map_variants;++variants;++roles[context.role];
          if(is_dynamic){++dynamic;++map_dynamic;}else{++baked;++map_baked;}
          if(context.prt_model!=0xffffffffu){++prt_owner_lower;++map_prt;}
        } catch(const std::exception&e) {
          std::ostringstream report;report<<context.map<<" datum"<<std::hex<<context.shader<<" mask"<<it->second.mask<<std::dec<<" LOD"<<lod
              <<" impl"<<context.implementation<<" role"<<context.role<<" "<<source.name<<": "<<e.what();failures.push_back(report.str());
        }
      }
    }
    for(const auto &[key,lod]:literal.lods)if(std::get<0>(key)==names[map] && std::get<2>(key)) {
      ++lower;for(unsigned layer=0;layer<32;++layer)if(lod.mask&(1u<<layer))++layers[layer];
    }
    shaders+=unsigned(unique.size());std::cout<<names[map]<<": "<<unique.size()<<" literal MODEL shaders, "<<map_variants<<" lower lighting contexts ("
      <<map_baked<<" baked, "<<map_dynamic<<" dynamic; "<<map_prt<<" on PRT owners)\n";
  }
  for(const auto &failure:failures)std::cerr<<failure<<'\n';
  std::cout<<shaders<<" MODEL shaders; "<<lower<<" unique lower records; "<<variants<<" prepared lower contexts ("<<baked<<" baked, "<<dynamic
    <<" dynamic), "<<prt_owner_lower<<" lower records on PRT owners; "<<passes<<" selected passes\n";
  std::cout<<"literal MODEL lower layers:";for(unsigned i=0;i<layers.size();++i)if(layers[i])std::cout<<' '<<i<<'='<<layers[i];std::cout<<'\n';
  std::cout<<"prepared MODEL roles:";for(const auto &[role,count]:roles)std::cout<<' '<<role<<'='<<count;std::cout<<'\n';
  check(shaders==1068 && lower==1267 && variants==1270 && baked==42 && dynamic==1228 && prt_owner_lower==11,
        "lower MODEL audit has empty or changed source coverage");
  check(failures.empty(),std::to_string(failures.size())+" lower MODEL variants require source-backed fixes");
}
}

int main(int argc, char **argv) {
  try {
    if(argc>1 && std::string(argv[1])=="--campaign-late") {
      const auto directory=argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/disc/maps");
      const Cache cache(directory/"01b_spacestation.map");
      const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &value) {
        return value.group=="shad" && value.name=="scenarios\\shaders\\solo\\spacestation\\glass_hangar_breach_char";
      });
      check(tag!=cache.tags().end(),"Cairo hangar breach glass source owner missing");
      const auto authored=cache.load_material(tag->datum);
      const auto selected=selected_passes(authored,15,0);
      check(selected==std::vector<unsigned>{0,1},"Cairo glass authored layer15 order changed");
      auto emitted=authored;
      check(prepare_world_material(cache,emitted),"Cairo two-pass glass was not admitted");
      check(emitted.forward_pass_index==selected[0] && emitted.world_overlays_after.size()==1 &&
            emitted.world_overlays_after[0]->forward_pass_index==selected[1],
            "Cairo glass dropped or reordered its masked environment pass");
      const auto &environment=*emitted.world_overlays_after[0];
      exact_pass(emitted,authored,15,0);
      exact_pass(environment,authored,15,0);
      std::map<TextureKey,Texture> resident;
      exact_textures(cache,authored,emitted,authored.passes[selected[0]],resident);
      exact_textures(cache,authored,environment,authored.passes[selected[1]],resident);
      check(emitted.rendering_gaps.empty() && environment.rendering_gaps.empty(),
            "Cairo glass retained a collapsed-pass scheduling gap");
      check(emitted.forward_blend==2 && emitted.forward_raster.color_write==0x00010101 &&
            environment.forward_blend==source_alpha_add_blend && environment.forward_raster.color_write==0x01010101 &&
            !emitted.forward_raster.depth_write && !environment.forward_raster.depth_write,
            "Cairo glass changed its authored two-pass blend, channels, or depth writes");
      // An unresolved later pass must reject the whole preparation, retaining
      // the original owner instead of publishing an incomplete first pass.
      auto incomplete=authored;
      incomplete.passes[selected[1]].vertex_externs.push_back({0,0,255});
      check(!prepare_world_material(cache,incomplete) && !incomplete.world_pass &&
            incomplete.world_overlays_after.empty(),"Cairo glass accepted an unresolved later pass");
      std::cout<<"Cairo hangar glass: two ordered source passes, exact programs/constants/textures/raster; unresolved child rejected\n";
      return 0;
    }
    if(argc>1 && std::string(argv[1])=="--model-lods") {
      std::cout<<std::unitbuf;
      model_lower_lods(argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/disc/maps"),
                 argc>3?std::filesystem::path(argv[3]):std::filesystem::path("assets/map-pack/maps"));return 0;
    }
    if(argc>1 && std::string(argv[1])=="--lower-lods") {
      lower_lods(argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/disc/maps"),
                 argc>3?std::filesystem::path(argv[3]):std::filesystem::path("assets/map-pack/maps"));
      return 0;
    }
    const auto directory = argc > 1 ? std::filesystem::path(argv[1]) :
                                     std::filesystem::path("assets/disc/maps");
    constexpr std::array names{"ascension", "beavercreek", "burial_mounds", "coagulation",
                               "colossus", "cyclotron", "foundation", "headlong", "lockout",
                               "midship", "waterworks", "zanzibar", "containment", "warlock",
                               "deltatap", "turf", "backwash", "dune", "elongation", "gemini", "triplicate"};
    // Counts come from the original BSP shader tables and first LOD masks,
    // independently recorded in authored-bsp-layers.json by the raw cache audit.
    constexpr std::array unique{19u,25u,9u,25u,21u,48u,29u,69u,25u,28u,18u,39u,
                                20u,25u,35u,57u,32u,17u,21u,47u,48u};
    constexpr std::array lighting{17u,21u,9u,19u,20u,42u,24u,63u,19u,26u,15u,34u,
                                  20u,18u,26u,53u,22u,15u,20u,37u,46u};
    unsigned all_lighting = 0, all_accumulation = 0, all_emissive = 0;
    unsigned before = 0, overlays = 0,environments=0,late_passes=0,bloom_passes=0;
    for (unsigned i = 0; i < names.size(); ++i) {
      const auto dlc_directory=argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/map-pack/maps");
      const auto path = (i<12?directory:dlc_directory) / (std::string(names[i]) + ".map");
      const Cache cache(path);
      const auto ids = bsp_materials(cache, path);
      check(ids.size() == unique[i], std::string(names[i]) + " authored material count changed");
      unsigned admitted = 0;
      for (const auto datum : ids) {
        const auto original = cache.load_material(datum);
        if (!first_layer(original, 3)) {
          if (first_layer(original, 7)) {
            auto emitted = original;
            check(prepare_world_material(cache, emitted), "emissive-only world shader was skipped");
            exact_pass(emitted, original, 7, 0);
            ++all_emissive;
            for(const auto &p:emitted.world_bloom_passes) {
              exact_pass(*p,original,17,0);++bloom_passes;
              check(p->world_bloom_pass && p->forward_raster.color_write==0x01000000,
                  "emissive-only world bloom mask lost source alpha output");
            }
          }
          if (first_layer(original, 2)) {
            auto emitted = original;
            check(prepare_world_material(cache, emitted), "standalone world overlay was skipped");
            exact_pass(emitted, original, 2, 0);
            check(emitted.forward_blend == 6 && !emitted.forward_raster.depth_write,
                  "standalone world overlay lost original double-multiply blend");
            ++overlays;
          }
          if(first_layer(original,15)) {
            auto emitted=original;
            check(prepare_world_material(cache,emitted),std::string(names[i])+" skipped highest-LOD layer15: "+original.name);
            exact_pass(emitted,original,15,0);++late_passes;
          }
          continue;
        }
        ++admitted;
        for (const bool per_vertex : {false, true}) {
          auto emitted = original;
          check(prepare_world_material(cache, emitted, per_vertex),
                std::string(names[i]) + " failed static world admission: " + original.name);
          const auto &lit = emitted.environment_pass ? *emitted.environment_pass : emitted;
          exact_pass(lit, original, 3, per_vertex ? 1 : 0);
          check(lit.per_vertex_lighting == per_vertex &&
                lit.baked_lightmap_texture != per_vertex,
                "world lighting policy changed its per-pixel/per-vertex input bindings");
          if (emitted.environment_pass) {
            exact_pass(emitted, original, 1, 0);
            check(lit.accumulation_texture && lit.forward_raster.depth_function == 0x202 &&
                  !lit.forward_raster.depth_write && emitted.forward_raster.depth_write,
                  "two-pass world lighting lost authored accumulation/depth ordering");
            if (!per_vertex) ++all_accumulation;
          }
          if (!per_vertex) {
            for (const auto &p : emitted.world_overlays_before) {
              exact_pass(*p, original, 2, 0);
              check(p->forward_blend == 6, "world decal lost double-multiply blend");
              ++before;
            }
            for (const auto &p : emitted.world_overlays_after) {
              if(p->forward_vertex==4) {
                exact_pass(*p,original,8,0);++environments;
                check(p->forward_blend==8,"world environment lost DST_ALPHA/ONE blend");
                check(original.passes[p->forward_pass_index].pixel_externs.size()==1,
                    "world environment lost source camera-direction binding");
              } else {exact_pass(*p, original, 7, 0);++all_emissive;}
            }
            for(const auto &p:emitted.world_bloom_passes) {
              exact_pass(*p,original,17,0);++bloom_passes;
              check(p->world_bloom_pass && p->forward_raster.color_write==0x01000000 &&
                p->forward_blend==3,"world bloom lost authored alpha-only additive mask");
            }
          }
        }
      }
      check(admitted == lighting[i], std::string(names[i]) + " static lighting shader count changed");
      all_lighting += admitted;
      std::cout << names[i] << ": " << admitted << " original highest-LOD static shaders\n";
    }
    std::cout << "coverage: " << all_lighting << " lighting, " << all_accumulation
              << " accumulation, " << all_emissive << " emissive, " << before
              << " before overlays, " << overlays << " standalone overlays\n";
    check(all_lighting == 566 && all_accumulation == 164 && all_emissive == 88 &&
          before == 0 && overlays == 11,
          "original multiplayer world layer coverage differs from audited payloads");
    check(environments==2 && late_passes==71 && bloom_passes==10,
          "21-map highest-LOD environment/transparent coverage differs from original tables");
    std::cout << "21 maps: 566 per-pixel and 566 per-vertex variants; 164 accumulation passes; "
              << "88 emissive passes; 11 world overlays; 2 environment passes; "
              << "71 layer15 passes; 10 bloom masks\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
