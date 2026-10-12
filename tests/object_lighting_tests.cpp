#include "halo2/object_lighting.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/texture.hpp"
#include "halo2/scope_copy.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/scenery.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>

namespace {
using namespace halo2;
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
constexpr std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
  "foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock","deltatap",
  "turf","backwash","dune","elongation","gemini","triplicate"};
std::filesystem::path map_path(const std::string &name) {
  const auto i=std::find(names.begin(),names.end(),name);
  if(i==names.end())throw FormatError("unknown object lighting fixture map");
  return std::filesystem::path(i-names.begin()<12?"assets/disc/maps":"assets/map-pack/maps")/(name+".map");
}
void probe() {
  std::map<std::string,ObjectLightingDefinition> definitions;
  std::string name;unsigned type;ObjectLightmapSample sample;
  std::cout<<std::setprecision(9);
  while(std::cin>>name>>type>>sample.incident.x>>sample.incident.y>>sample.incident.z
      >>sample.diffuse_rgb>>sample.lightmap_rgb) {
    if(!definitions.contains(name))definitions.emplace(name,Cache(map_path(name)).load_object_lighting_definition());
    for(float v:object_lighting(definitions.at(name),sample,type).source_values())std::cout<<v<<' ';
    std::cout<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated original object lighting fixture");
}
void texture_probe() {
  unsigned format,width,height,mips,size,value;Vec2 uv;float detail;
  while(std::cin>>format>>width>>height>>mips>>uv.x>>uv.y>>detail>>size) {
    std::vector<std::byte> raw;
    for(unsigned i=0;i<size;++i){std::cin>>value;raw.push_back(std::byte(value));}
    const auto texture=prepare_texture(Bytes(raw),width,height,format,false,mips);
    const auto result=sample_object_texture(texture,uv,detail);
    if(!result)throw FormatError("empty native CPU texture fixture");
    std::cout<<*result<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated CPU texture fixture");
}
void cache_probe() {
  std::map<std::string,ObjectLightingDefinition> definitions;std::string name;unsigned type;
  std::int32_t frame;int sample_valid,lighting_valid,frozen,available;
  float distance;Vec3 origin,velocity;ObjectLightingCache cache;ObjectLightmapSample query;
  const auto sample=[](ObjectLightmapSample &s) {
    std::cin>>s.incident.x>>s.incident.y>>s.incident.z>>s.diffuse_rgb>>s.lightmap_rgb;
  };
  std::cout<<std::setprecision(9);
  while(std::cin>>name>>type>>frame>>cache.last_query_frame>>cache.last_rendered_frame
      >>sample_valid>>lighting_valid>>frozen>>distance
      >>origin.x>>origin.y>>origin.z>>cache.query_origin.x>>cache.query_origin.y>>cache.query_origin.z
      >>velocity.x>>velocity.y>>velocity.z) {
    sample(cache.current);sample(cache.target);std::cin>>available;sample(query);
    if(!std::cin)throw FormatError("truncated object cache oracle fixture");
    if(!definitions.contains(name))definitions.emplace(name,Cache(map_path(name)).load_object_lighting_definition());
    cache.sample_valid=sample_valid;cache.lighting_valid=lighting_valid;cache.lighting={};
    const auto due=object_lighting_query_due(cache,frame,origin,distance,frozen);
    update_object_lighting_cache(cache,definitions.at(name),type,frame,origin,velocity,due,
      available?std::optional(query):std::nullopt,true,frozen);
    std::cout<<due<<' '<<cache.last_query_frame<<' '<<cache.sample_valid<<' '<<cache.lighting_valid<<' '
      <<cache.query_origin.x<<' '<<cache.query_origin.y<<' '<<cache.query_origin.z<<' ';
    for(const auto &s:{cache.current,cache.target})std::cout<<s.incident.x<<' '<<s.incident.y<<' '<<s.incident.z<<' '
      <<s.diffuse_rgb<<' '<<s.lightmap_rgb<<' ';
    for(float v:cache.lighting.source_values())std::cout<<v<<' ';std::cout<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated object cache oracle fixture");
}
void scope_probe() {
  RenderViewport viewport;Vec2 size,center,extent,uv;float w,near,far;
  std::cout<<std::setprecision(9);
  while(std::cin>>viewport.left>>viewport.top>>viewport.width>>viewport.height
      >>size.x>>size.y>>center.x>>center.y>>extent.x>>extent.y
      >>uv.x>>uv.y>>w>>near>>far) {
    const auto rectangle=scope_source_rectangle(viewport,size,center,extent);
    const auto frame=scope_copy_frame(rectangle);
    const auto position=scope_copy_vertex(uv,w,near,far);
    for(float v:rectangle)std::cout<<v<<' ';
    for(float v:position)std::cout<<v<<' ';
    for(auto word:frame.program.words)std::cout<<word<<' ';std::cout<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated scope copy oracle fixture");
}
void camera_probe() {
  float horizontal,aspect,baseline;std::array<std::int16_t,2> grid,span;
  std::cout<<std::setprecision(9);
  while(std::cin>>horizontal>>aspect>>grid[0]>>grid[1]>>span[0]>>span[1]>>baseline) {
    const auto result=source_camera_angles(horizontal,aspect,grid,span,baseline);
    std::cout<<result.projection_angle<<' '<<result.lighting_distance_scale<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated source camera angle fixture");
}
void grid_probe() {
  unsigned players,view;bool wide;
  while(std::cin>>players>>view>>wide) {
    const auto result=source_view_grid(players,view,wide);
    for(const auto &v:{result.grid,result.span,result.position})for(auto x:v)std::cout<<x<<' ';
    std::cout<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated source split grid fixture");
}
void volume_fixture() {
  // Independent coordinate anchors for NV2A's 8x4x2 bit layout:
  // X bits are 0/3/5, Y bits 1/4, Z bit2. Mip volumes are consecutive.
  // xemu primary swizzle.c uses dimension masks and increment/carry;
  // this checks the decoder's interleaving against fixed known addresses.
  std::vector<std::byte> raw(150+16);
  for(unsigned address=0;address<75;++address) {
    raw[address*2]=std::byte(address);raw[address*2+1]=std::byte(255-address);
  }
  const auto image=prepare_volume_texture(Bytes(raw),8,4,2,3,true,3);
  check(image.depth==2 && image.layers==1 && image.format==11 && image.mips.size()==4,
    "volume image dimensionality changed");
  const auto texel=[&](unsigned x,unsigned y,unsigned z,unsigned address) {
    const auto at=((z*4+y)*8+x)*4;
    check(image.pixels.at(at)==std::byte(address) && image.pixels.at(at+1)==std::byte(address) &&
      image.pixels.at(at+2)==std::byte(address) && image.pixels.at(at+3)==std::byte(255-address),
      "NV2A volume coordinate decode changed");
  };
  texel(0,0,0,0);texel(1,0,0,1);texel(0,1,0,2);texel(0,0,1,4);
  texel(2,0,0,8);texel(0,2,0,16);texel(4,0,0,32);
  texel(3,2,1,29);texel(7,3,1,63);
  const auto &m1=image.mips[1],&m2=image.mips[2],&m3=image.mips[3];
  check(m1.width==4 && m1.height==2 && m1.depth==1 && m1.offset==256 && m1.size==32 &&
    m2.width==2 && m2.height==1 && m2.depth==1 && m2.offset==288 && m2.size==8 &&
    m3.width==1 && m3.height==1 && m3.depth==1 && m3.offset==296 && m3.size==4 &&
    image.pixels.size()==300 && image.pixels.at(m1.offset+4*3)==std::byte(69) &&
    image.pixels.at(m3.offset)==std::byte(74),"volume mip storage or padding changed");
  const std::vector<std::byte> one{std::byte{42},std::byte{128}};
  check(prepare_volume_texture(Bytes(one),1,1,1,3,true).depth==1 &&
    prepare_texture(Bytes(one),1,1,3,true).depth==0,"1x1 volume lost its explicit image type");
}
void volume_header_probe() {
  unsigned width,height,depth,maximum;
  while(std::cin>>width>>height>>depth>>maximum) {
    const auto mips=retail_mip_count(width,height,depth,3,false,maximum);
    std::size_t bytes=0;
    for(unsigned mip=0,w=width,h=height,d=depth;mip<=mips;++mip) {
      bytes+=std::size_t(w)*h*d*2;w=std::max(1u,w/2);h=std::max(1u,h/2);d=std::max(1u,d/2);
    }
    const auto image=prepare_volume_texture(Bytes(std::vector<std::byte>(bytes)),width,height,depth,3,true,mips);
    std::cout<<image.width<<' '<<image.height<<' '<<image.depth<<' '<<image.mips.size()<<'\n';
  }
  if(!std::cin.eof())throw FormatError("truncated source volume header fixture");
}
}
int main(int argc,char **argv) {
  try {
    if(argc>1 && std::string(argv[1])=="--probe"){probe();return 0;}
    if(argc>1 && std::string(argv[1])=="--texture-probe"){texture_probe();return 0;}
    if(argc>1 && std::string(argv[1])=="--cache-probe"){cache_probe();return 0;}
    if(argc>1 && std::string(argv[1])=="--scope-probe"){scope_probe();return 0;}
    if(argc>1 && std::string(argv[1])=="--camera-probe"){camera_probe();return 0;}
    if(argc>1 && std::string(argv[1])=="--grid-probe"){grid_probe();return 0;}
    if(argc>1 && std::string(argv[1])=="--volume-header-probe"){volume_header_probe();return 0;}
    volume_fixture();
    unsigned definitions=0,dynamic=0,accumulation=0,scopes=0,volumes=0;
    unsigned scenery_placements=0,scenery_lit=0,scenery_transparent=0,arms_environment=0;
    for(const auto name:names) {
      const Cache cache(map_path(name));const auto definition=cache.load_object_lighting_definition();
      check(definition.variables.size()==2,"authored multiplayer global lighting variable coverage changed");
      check(definition.variables[0].affected_objects==0x2002 && definition.variables[1].affected_objects==0x1ffc,
        "original object-type global lighting selection changed");
      for(unsigned type=0;type<13;++type) {
        const auto state=object_lighting(definition,{{.2f,.3f,.4f},0x7fc020,0x60a0e0},type);
        for(float v:state.source_values())check(std::isfinite(v),"nonfinite original object lighting conversion");
      }
      ++definitions;
      // Every resident highest-LOD dynamic lighting shader must keep its
      // original implementation and world-space compiled vertex program.
      for(const auto &tag:cache.tags()) {
        if(tag.group!="shad" || !tag.name.starts_with("objects\\") || !tag.address || tag.size<84)continue;
        auto material=cache.load_material(tag.datum);
        if(material.lods.empty() || !(material.lods.front().available_layers&(1u<<3)))continue;
        const auto range=material.layers.at(material.lods.front().layers.first()+3);
        const auto pass=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto &p) {
          return p.template_pass>=range.first() && p.template_pass-range.first()<range.count() &&
            p.implementation==2 && p.vertex_shader.find("\\lightmap_dynamic")!=std::string::npos;
        });
        if(pass==material.passes.end())continue;
        const auto original_code=pass->vertex_code;
        const auto state=object_lighting(definition,{{.2f,.3f,.4f},0x7fc020,0x60a0e0},0);
        bool prepared=false;
        try {prepared=prepare_dynamic_material(cache,material,state);}
        catch(const FormatError &e) {
          // The retained original volume input is a separate image capability.
          // Keep its exact identity visible; no model-color fallback is tested.
          if(tag.name=="objects\\characters\\elite\\fp_arms\\shaders\\elite_fp_glow" &&
             std::string(e.what())=="unsupported bitmap dimensionality") {
            auto unsupported=material;
            for(auto &p:unsupported.passes)
              if(p.name.ends_with("\\plasma_alpha"))p.program.words[59]|=0x100;
            bool rejected=false;
            try {(void)prepare_dynamic_material(cache,unsupported,state,true);}
            catch(const FormatError &error) {rejected=std::string(error.what())=="unsupported volume texture addressing";}
            check(rejected,"volume capability silently admitted unresolved mode adjustment");
            ++volumes;prepared=prepare_dynamic_material(cache,material,state,true);
            check(prepared,"Elite volume capability did not admit authored effect");
            const auto effect=std::find_if(material.world_overlays_after.begin(),material.world_overlays_after.end(),
              [](const auto &p){return p && p->detail.depth && p->bump.depth;});
            check(effect!=material.world_overlays_after.end(),"Elite authored plasma volume layer missing");
            const auto &p=**effect;
            check(p.forward_program.words[54]==0x8420 && p.forward_program.words[59]==0 &&
              p.detail.depth==32 && p.detail.width==32 && p.detail.height==32 && p.detail.layers==1 &&
              p.detail.mips.size()==6 && p.detail.pixels.size()==149796,
              "Elite volume changed authored program or full A8L8 mip chain");
            for(unsigned mip=0,side=32;mip<6;++mip,side=std::max(1u,side/2))
              check(p.detail.mips[mip].width==side && p.detail.mips[mip].height==side &&
                p.detail.mips[mip].depth==side && p.detail.mips[mip].size==std::size_t(side)*side*side*4,
                "Elite volume mip extent changed");
          } else
          throw;
        }
        check(prepared,"resident dynamic material not executable: "+tag.name);
        if(tag.name=="objects\\characters\\masterchief\\fp\\shaders\\fp_arms") {
          const auto effect=std::find_if(material.world_overlays_after.begin(),material.world_overlays_after.end(),
            [](const auto&p){return p && p->passes.at(p->forward_pass_index).vertex_shader.ends_with("\\stencil_env_bumped_tangentspace");});
          check(effect!=material.world_overlays_after.end(),"Chief first person lost original independent environment layer8");
          const auto&source=(*effect)->passes.at((*effect)->forward_pass_index);
          check(source.implementation==0 && (*effect)->vertex_program==source.vertex_code &&
            (*effect)->forward_program.words==source.program.words &&
            (*effect)->forward_program.words[10]==0 && (*effect)->forward_program.words[13]==0x787387 &&
            (*effect)->passes.at((*effect)->forward_pass_index).pixel_externs.size()==1 &&
            (*effect)->passes.at((*effect)->forward_pass_index).pixel_externs.front().source==0,
            "Chief environment changed source program, authored cube color or camera extern");
          ++arms_environment;
        }
        if(tag.name=="objects\\weapons\\rifle\\sniper_rifle\\shaders\\sniper_rifle_screen") {
          check(std::any_of(material.world_overlays_after.begin(),material.world_overlays_after.end(),[](const auto& p) {
            return p && p->scope_texture && p->passes.at(p->forward_pass_index).name==
              "shaders\\shader_passes\\texture_camera\\scope";
          }),"sniper scope lost original layer7 target16 pass");
          const auto scope=std::find_if(material.world_overlays_after.begin(),material.world_overlays_after.end(),
            [](const auto&p){return p && p->scope_texture;});
          const auto fallback=cache.load_scope_default_bitmap();
          check(scope!=material.world_overlays_after.end() && (*scope)->diffuse.width==fallback.width &&
            (*scope)->diffuse.height==fallback.height && (*scope)->diffuse.pixels==fallback.pixels,
            "regular scope draw lost authored matg default bitmap");
          ++scopes;
        }
        const auto &lit=material.environment_pass?*material.environment_pass:material;
        check(lit.forward_vertex==6 && lit.world_pass && lit.forward_pass,
          "dynamic world-space shader path changed");
        check(lit.passes.at(lit.forward_pass_index).implementation==2,
          "moving object substituted baked lighting implementation");
        check(lit.vertex_program==original_code,"dynamic shader replaced source microcode");
        ++dynamic;if(material.environment_pass)++accumulation;
      }
      // Actual Slayer placements include transparent-only emitter components.
      // Their layer15 implementation0 is independent of object lighting mode2.
      const auto content=cache.load_content(0,true);
      const auto bank=cache.load_bsp_lighting(0);
      for(const auto&object:content.objects)if(object.type==6 &&
          scenery_automatic_creation_allowed(object,2) && (object.origin_bsp<0 || object.origin_bsp==0)) {
        const auto scenery_definition=*cache.load_scenery_definition(object.definition);
        auto model=cache.load_model(object.definition,true,object.variant);
        if(scenery_lighting_implementation(bank,model,object,scenery_definition)!=SceneryLightingImplementation::dynamic)continue;
        ++scenery_placements;
        for(auto&material:model.mesh.materials) {
          if(material.lods.empty())continue; // Model's unused NONE material slot.
          const bool lit=(material.lods.front().available_layers&(1u<<3))!=0;
          check(prepare_dynamic_material(cache,material,{},true),"Slayer scenery material not executable: "+material.name);
          const auto&selected=material.environment_pass?*material.environment_pass:material;
          const auto&p=selected.passes.at(selected.forward_pass_index);
          if(lit){check(p.implementation==2,"dynamic scenery substituted another lighting mode");++scenery_lit;}
          else {
            const auto range=material.layers.at(material.lods.front().layers.first()+15);
            check(p.implementation==0 && p.template_pass>=range.first() &&
                p.template_pass-range.first()<range.count() && p.vertex_shader.ends_with("\\transparent_generic_reflection"),
                "transparent scenery changed its independent authored layer");
            check(selected.world_pass && selected.vertex_program==p.vertex_code && selected.forward_program.words==p.program.words,
                "transparent scenery lost source microcode or pixel program");++scenery_transparent;
          }
        }
      }
      std::cout<<name<<": original object lighting definition verified\n";
    }
    check(definitions==21 && dynamic>0,"multiplayer dynamic lighting coverage missing");
    check(scenery_placements==78 && scenery_lit==114 && scenery_transparent==126,
        "all21 Slayer dynamic scenery material inventory changed");
    check(arms_environment==21,"all21 Chief first person environment pass coverage changed");
    // Lighting touches only authored mappings, preserving unrelated view
    // constants and object/fog pixel fields already in the uniform copy.
    MaterialPass pass;pass.vertex_externs={{0x1c98,0,14},{0x1c99,0,16}};
    pass.pixel_externs={{0,4,15},{64,3,23}};
    PixelProgram program;program.words.fill(0xa5a5a5a5);ShaderConstants constants;
    for(auto &bank:constants)for(auto &v:bank)v={.125f,.25f,.5f,.75f};
    ObjectLightingState state;state.ambient={.1f,.2f,.3f};state.primary_color={.4f,.5f,.6f};
    state.primary_direction={1,0,0};state.secondary_direction={0,1,0};
    const auto untouched=constants[1][0];
    apply_object_lighting_bindings(program,constants,pass,state,{},1,0);
    check(constants[1][0]==untouched && constants[1][8]==ShaderVector{1,0,0,.75f} &&
      constants[1][9]==ShaderVector{0,1,0,.75f},"lighting extern changed unrelated shader components");
    check((program.words[10]&0xff000000u)==0xa5000000u &&
      (program.words[11]&0xffffffu)==0xa5a5a5u,"lighting extern changed unrelated pixel channels");
    check(scopes==21 && volumes==21,"original special image capability inventory changed");
    const auto center=scope_sample_center({641,0,640,359});
    check(center.x==961 && center.y==179,"scope integer viewport center changed");
    check(scope_sample_center({641,0,640,359},177.6f).y==178,"scope aiming Y snap changed");
    const auto frame=scope_copy_frame({.1f,.2f,.3f,.4f});
    check(frame.target_extent==64 && frame.texture_state.address_u==3 &&
      frame.texture_state.mag_filter==2 && frame.texture_state.mip_filter==0,
      "original scope extent or copy sampling changed");
    std::cout<<definitions<<" definitions, "<<dynamic<<" dynamic shader passes, "<<accumulation
      <<" source accumulation variants; "<<scopes<<" admitted scope-target and "<<volumes<<" volume-image programs separately inventoried\n";
    return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
