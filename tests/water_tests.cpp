#include "halo2/water.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/texture.hpp"
#include "halo2/original_data.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <set>

namespace {
using namespace halo2;
void check(bool v,const char *text) {if(!v)throw std::runtime_error(text);}
void zero_words() {
  // Literal TU1.5 3D202 and 3C705/3C718/3C85D..3C8DE results for
  // authored zero drift, zero heading and positive bump scales. Numeric
  // equality alone accepts the unary-negation regression (+0 versus -0).
  WaterDefinition definition;
  definition.parameters[26]=2;definition.parameters[27]=3;definition.parameters[28]=4;
  const auto frame=water_frame(definition,{},Vec3{0,1,0},1.2f,4.f/3.f);
  check(std::bit_cast<std::uint32_t>(frame.constants[1][1])==0,
        "TU1.5 water zero drift lost its positive-zero word");
  constexpr std::array<std::array<std::uint32_t,4>,3> expected{{
      {0x40000000u,0,0x40000000u,0},
      {0xc0400000u,0,0xc0400000u,0},
      {0x40800000u,0,0x40800000u,0}}};
  for(unsigned stage=0;stage<3;++stage)for(unsigned axis=0;axis<4;++axis)
    check(std::bit_cast<std::uint32_t>(frame.bump_matrices[stage+1][axis])==expected[stage][axis],
          "TU1.5 water zero-heading matrix lost original operation order");
}
struct LiteralBlock {std::uint32_t count{};std::span<const std::byte> bytes;};
LiteralBlock literal_block(const Cache &cache,const Bytes &parent,unsigned at,unsigned stride) {
  const auto count=parent.u32(at),address=parent.u32(at+4);
  check(count<65536,"excessive literal water shader block");
  return {count,count?cache.read_tag_bytes(address,std::size_t(count)*stride):std::span<const std::byte>{}};
}
void original_program(const Cache &cache,const Material &material) {
  const auto &pass=material.passes.at(material.forward_pass_index);
  const auto *tag=cache.query_tag({.group=TagGroup::from_string("spas"),.name=pass.name});
  check(tag,"water pass lost its original tag owner");
  const auto post=literal_block(cache,Bytes(cache.tag_payload(tag->handle())),28,88);
  check(post.count==1,"water pass lost its literal postprocess bank");
  const auto implementations=literal_block(cache,Bytes(post.bytes),0,306);
  check(pass.implementation<implementations.count,"water implementation outside literal pass");
  const Bytes raw(implementations.bytes.subspan(pass.implementation*306,306));
  for(unsigned word=0;word<60;++word)
    check(pass.definition.words[word]==raw.u32(6+word*4),"water pixel definition differs from literal source words");
  check(material.forward_program.words==pass.program.words &&
        material.forward_program.fragment_offsets==pass.program.fragment_offsets,
        "water binding altered its assembled original pixel program");
  const auto *vertex=cache.find_tag({raw.u32(256)},TagGroup::from_string("vrtx"));
  check(vertex && vertex->name==pass.vertex_shader,"water vertex identity differs from literal pass reference");
  const auto classes=literal_block(cache,Bytes(cache.tag_payload(vertex->handle())),4,28);
  check(classes.count,"water class zero is absent from original vertex bank");
  const Bytes world(classes.bytes.first(28));
  const auto code=cache.read_tag_bytes(world.u32(16),world.u32(12));
  check(!code.empty() && material.vertex_program.size()==code.size() &&
        std::equal(code.begin(),code.end(),material.vertex_program.begin()),
        "water vertex microcode differs from literal class zero");
  check(texture_program_supported(material.forward_program) &&
        decode_vertex_program(material.vertex_program).count>0,"original water program is not executable");
  for(unsigned stage=0;stage<pass.texture_states.size();++stage)
    check(material.texture_states[stage]==pass.texture_states[stage],"water binding altered its original sampler state");
  bool reflection=false,refraction=false;
  for(const auto &binding:pass.textures) {
    if(binding.external==19){check(binding.stage==1,"water reflection changed original stage");reflection=true;}
    if(binding.external==20){check(binding.stage==2,"water refraction changed original stage");refraction=true;}
  }
  check(material.water_reflection_texture==reflection && material.water_refraction_texture==refraction,
        "water external target binding differs from original pass");
}
int materials(int argc,char **argv) {
  check(argc>=4,"Supply --materials, actual TU1.5 image and at least one map");
  if(std::string_view(argv[2]).empty())return 77;
  const auto original=OriginalData::load_xbe(argv[2]);
  check(original->image==OriginalData::Image::TitleUpdate15,"water material audit requires authenticated TU1.5");
  unsigned count=0,infinite=0,pools=0,edges=0,waves=0;std::set<std::string> templates;
  for(int argument=3;argument<argc;++argument) {
    const std::filesystem::path path=argv[argument];
    Cache cache(path,path.parent_path(),original);unsigned map_count=0;
    for(const auto &tag:cache.tags()) {
      if(tag.group!="shad" || !tag.address || tag.size<8)continue;
      const auto *stem=cache.find_tag({Bytes(cache.tag_payload(tag.handle())).u32(4)},TagGroup::from_string("stem"));
      if(!stem || (!stem->name.starts_with("shaders\\shader_templates\\water\\") &&
          stem->name!="shaders\\shader_templates\\transparent\\waves"))continue;
      const auto authored=cache.load_material(tag.datum);templates.insert(authored.template_name);
      Material material;
      // Templates named water/water_static and ocean/ocean_static can both
      // select the independent water vertex family. Ask the actual binders
      // transactionally; template leaf names do not select the source owner.
      const auto bind=[&](const auto &prepare) {
        auto candidate=authored;
        if(!prepare(candidate))return false;
        material=std::move(candidate);return true;
      };
      const bool prepared=bind([&](auto &m){return prepare_water_material(cache,m,false);}) ||
          bind([&](auto &m){return prepare_water_material(cache,m,true);}) ||
          bind([&](auto &m){return prepare_world_material(cache,m,true);}) ||
          bind([&](auto &m){return prepare_world_material(cache,m,false);});
      if(!prepared)throw FormatError("Unresolved original water material: "+path.filename().string()+" "+tag.name);
      original_program(cache,material);
      infinite+=material.water_pass && material.forward_vertex==8;
      pools+=material.water_pass && material.forward_vertex==7;
      edges+=material.passes.at(material.forward_pass_index).vertex_shader.ends_with("\\water_edge_blend");
      waves+=authored.template_name=="shaders\\shader_templates\\transparent\\waves";++count;++map_count;
    }
    std::cout<<path.filename().string()<<": "<<map_count<<" literal original water materials\n";
  }
  check(count>0 && infinite>0 && pools>0,"original water material cohort lost oceans or pools");
  std::cout<<count<<" original water material programs: "<<infinite<<" oceans, "<<pools<<" pools, "
      <<edges<<" edges, "<<waves<<" waves; "<<templates.size()<<" template families\n";
  std::cout<<"Program identity, assembly preservation, class-zero microcode, samplers and target bindings only; GPU pixel parity requires original framebuffer evidence\n";
  return 0;
}
void probe() {
  std::cout<<std::setprecision(9);
  WaterDefinition definition;
  while(std::cin>>definition.parameters[0]) {
    for(unsigned i=1;i<43;++i)std::cin>>definition.parameters[i];
    Vec3 eye,forward;float fov,aspect,anamorphic;
    std::cin>>eye.x>>eye.y>>eye.z>>forward.x>>forward.y>>forward.z>>fov>>aspect>>anamorphic;
    if(!std::cin)throw std::runtime_error("truncated water oracle fixture");
    const auto frame=water_frame(definition,eye,forward,fov,aspect,anamorphic!=0);
    for(const auto &v:frame.constants)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.bump_matrices)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.local_to_world)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.corners)std::cout<<v.x<<' '<<v.y<<' '<<v.z<<' ';
    std::cout<<frame.horizon_argb<<'\n';
  }
}
}
int main(int argc,char **argv) {
  try {
    if(argc>1 && std::string_view(argv[1])=="--materials")return materials(argc,argv);
    if(argc>1 && std::string(argv[1])=="--probe"){probe();return 0;}
    zero_words();
    const std::array<std::byte,10> source{std::byte{0},std::byte{127},std::byte{128},std::byte{255},
      std::byte{31},std::byte{64},std::byte{192},std::byte{254},std::byte{15},std::byte{240}};
    const auto v8u8=prepare_texture(Bytes(source),2,2,22,true,1);
    check(v8u8.format==11 && v8u8.mips.size()==2 && v8u8.pixels.size()==20,
          "V8U8 original mip chain changed");
    for(unsigned i=0;i<5;++i)check(v8u8.pixels[i*4]==source[i*2] && v8u8.pixels[i*4+2]==source[i*2] &&
      v8u8.pixels[i*4+1]==source[i*2+1] && v8u8.pixels[i*4+3]==source[i*2+1],
      "V8U8 original U,V,U,V fetch swizzle changed");
    check(prepare_texture(Bytes(source),2,2,23,true,1).pixels==v8u8.pixels,
          "retail V8U8/G8B8 format aliases decode differently");
    const auto signed_image=prepare_signed_texture(v8u8,0xe0000000u);
    check(signed_image.format==native_rgba32f && signed_image.pixels.size()==80 &&
      signed_image.mips[1].offset==64 && signed_image.mips[1].size==16,
      "signed water mip layout changed");
    for(unsigned i=0;i<20;++i) {
      const unsigned byte=std::to_integer<unsigned>(v8u8.pixels[i]);
      const float expected=i%4==3?float(byte)/255.f:
        float(byte>=128?int(byte)-256:int(byte))/128.f;
      check(Bytes(signed_image.pixels).f32(i*4)==expected,
            "signed water channels were converted after filtering or lost unsigned alpha");
    }
    const auto disc=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("assets/disc/maps");
    const auto dlc=argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/map-pack/maps");
    constexpr std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
      "foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock","deltatap",
      "turf","backwash","dune","elongation","gemini","triplicate"};
    unsigned definitions=0,resident=0,drawn=0,pools=0;
    for(unsigned i=0;i<names.size();++i) {
      const Cache cache((i<12?disc:dlc)/(std::string(names[i])+".map"));
      const auto water=cache.load_bsp_water(0);definitions+=unsigned(water.definitions.size());
      for(const auto &definition:water.definitions) {
        const auto frame=water_frame(definition,{3,-2,5},{.8f,.6f,-.2f},1.2f,4.f/3.f);
        check(frame.constants[0]==ShaderVector{1,-1,-1,.5f},"water c21 changed from retail state");
        check(frame.constants[2][3]==.4970000088214874f && frame.constants[4][3]==1,
              "water sun constants changed from retail state");
        check(!(frame.horizon_argb&0xff000000u),"water horizon clear has nonzero source alpha");
        for(const auto &v:frame.constants)for(float f:v)check(std::isfinite(f),"nonfinite native water constant");
        if(!definition.geometry.vertices.empty()) {
          ++resident;check(definition.geometry.vertices.size()==8064,"authored water quad expansion changed");
          check(frame.infinite,"resident infinite grid lost authored camera basis");
          check(std::all_of(definition.geometry.vertices.begin(),definition.geometry.vertices.end(),
            [](const auto &v){return v.position.z==0 && v.material==0;}),"water source position-only stream changed");
          const auto transformed=water_world_position(frame,definition.geometry.vertices.front().position);
          check(transformed.z==definition.parameters[33]-std::abs(5-definition.parameters[33])*definition.parameters[42],
                "water dynamic plane height changed");
          if(definition.shader==0xffffffffu) {
            check(definition.geometry.materials.empty(),"NONE water shader created an unauthored draw");
          } else {
            ++drawn;check(definition.geometry.materials.size()==1,"water grid lost shader");
            const auto &m=definition.geometry.materials.front();
            check(m.water_pass && m.forward_vertex==8 && texture_program_supported(m.forward_program),
                  "water grid lost original executable texture program");
            check(decode_vertex_program(m.vertex_program).count>0,"water grid vertex program lost");
          }
        }
      }
      for(const auto &tag:cache.tags())if(tag.group=="shad" && tag.name.find("water")!=std::string::npos) {
        // Admission below is checked against the original template identity;
        // the filename filter only bounds this resident-cache fixture scan.
        const auto material=cache.load_material(tag.datum);
        if(material.template_name!="shaders\\shader_templates\\water\\water_pool" &&
           material.template_name!="shaders\\shader_templates\\water\\water_pool_static")continue;
        auto emitted=material;check(prepare_water_material(cache,emitted),"authored pool pass was skipped");
        check(emitted.water_pass && emitted.forward_vertex==7 && texture_program_supported(emitted.forward_program),
              "water pool addressing not executable");++pools;
      }
      std::cout<<names[i]<<": "<<water.definitions.size()<<" original water definitions\n";
    }
    check(definitions==12 && resident==5 && drawn==3,"21-map authored water definition coverage changed");
    std::cout<<definitions<<" water records, "<<resident<<" resident grids, "<<drawn<<" submitted infinite grids, "
      <<pools<<" resident pool shader records\n";return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
