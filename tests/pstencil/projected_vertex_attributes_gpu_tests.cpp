// Reuse the actual raw graphics-input/readback harness. Its existing ABI2
// cases remain independent; this executable selects only the ABI3 fixture.
#define main retained_menu_world_vertex_catalog_tests_main
#include "../menu_world_vertex_catalog_gpu_tests.cpp"
#undef main
#include "halo2/source_bsp_admission.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include <unistd.h>

namespace {
ShaderVector fetch(const MenuWorldSourceBinding& binding,unsigned ordinal,unsigned vertex) {
  const auto&input=binding.inputs().at(ordinal);const auto&s=input.stream;
  const auto data=binding.resource_bytes(ordinal);const auto at=s.byte_offset+vertex*s.byte_stride;
  ShaderVector value{0,0,0,1};
  const auto load=[&]<class T>(unsigned offset){T v;std::memcpy(&v,data.data()+at+offset,sizeof(v));return v;};
  switch(s.format) {
  case MenuWorldSourceFormat::float32x3:for(unsigned i=0;i<3;++i)value[i]=load.operator()<float>(i*4);break;
  case MenuWorldSourceFormat::float32x2:for(unsigned i=0;i<2;++i)value[i]=load.operator()<float>(i*4);break;
  case MenuWorldSourceFormat::snorm16x3:for(unsigned i=0;i<3;++i)value[i]=std::max(-1.f,float(load.operator()<std::int16_t>(i*2))/32767.f);break;
  case MenuWorldSourceFormat::snorm16x2:for(unsigned i=0;i<2;++i)value[i]=std::max(-1.f,float(load.operator()<std::int16_t>(i*2))/32767.f);break;
  case MenuWorldSourceFormat::unorm8x1:case MenuWorldSourceFormat::unorm8x2:
  case MenuWorldSourceFormat::unorm8x3:case MenuWorldSourceFormat::unorm8x4: {
    const unsigned count=s.format==MenuWorldSourceFormat::unorm8x1?1:s.format==MenuWorldSourceFormat::unorm8x2?2:s.format==MenuWorldSourceFormat::unorm8x3?3:4;
    for(unsigned i=0;i<count;++i)value[i]=float(load.operator()<std::uint8_t>(i))/255.f;
    break;
  }
  case MenuWorldSourceFormat::packed_signed_11_11_10: {
    const auto p=load.operator()<std::uint32_t>(0);
    const auto signed_field=[&](unsigned offset,unsigned bits){return float(std::int32_t((p>>offset)<<(32-bits))>>(32-bits));};
    value={signed_field(0,11)/1023.f,signed_field(11,11)/1023.f,signed_field(22,10)/511.f,1};break;
  }
  default:throw FormatError("Attribute fixture encountered an unproved input fetch");
  }
  return value;
}
float scalar(const Bytes& bytes,unsigned at) {return std::bit_cast<float>(word(bytes,at*4));}
}
int main(int argc,char**argv){try {
  require(argc==5 || (argc==6 && std::string_view(argv[5])=="--cpu-inputs-only") ||
      (argc==7 && std::string_view(argv[5])=="--references"),
      "attribute GPU test needs opening.map TU1.5.xbe capture-GS.spv output [--cpu-inputs-only]");
  for(unsigned i=1;i<4;++i)if(!fs::exists(argv[i]))return 77;
  const fs::path output=argv[4];fs::create_directories(output);Cache cache(argv[1]);
  std::uint32_t shader=0xffffffffu,definition=0xffffffffu;
  for(const auto&t:cache.tags()) {
    if(t.group=="shad" && t.name.ends_with("\\panel_mech_gunbarrel"))shader=t.datum;
    if(t.group=="ligh" && t.name.ends_with("\\x01_light_mc"))definition=t.datum;
  }
  require(shader!=0xffffffffu,"Actual failed gun-barrel material is absent");
  if(definition==0xffffffffu)for(const auto&t:cache.tags())if(t.group=="ligh" && t.name.find("\\x01_light_")!=std::string::npos){definition=t.datum;break;}
  require(definition!=0xffffffffu,"Actual opening projected light definition is absent");
  auto material=cache.load_material(shader);const auto pass=load_menu_world_source_pass(cache,shader,{0,12,0,0});
  const auto matching=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto&p){return p.template_pass==pass.template_pass() && p.implementation==0;});
  require(matching!=material.passes.end(),"Actual selected material constants absent");
  std::shared_ptr<const MenuWorldSourceSection> geometry;unsigned part=0,classification=0;MenuWorldSourceOwner owner;std::vector<std::array<float,12>> rows;
  for(unsigned bsp=0;bsp<cache.bsps().size() && !geometry;++bsp) {
    const auto admission=cache.load_source_bsp_admission(bsp);
    const auto metadata=cache.load_source_bsp_metadata(bsp);
    const halo2::Bytes bytes(metadata.bytes_at(metadata.source_address(),metadata.size()));
    const auto clusters=bytes.u32(16+156+4)-metadata.source_address();
    const auto definitions=bytes.u32(16+312+4)-metadata.source_address();
    for(unsigned cluster=0;cluster<admission.clusters.size() && !geometry;++cluster) {
      if(!bytes.u16(clusters+cluster*176))continue;
      auto section=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,bsp,MenuWorldSourceSectionKind::bsp_cluster,cluster));
      for(unsigned i=0;i<section->parts().size();++i)if(section->parts()[i].shader==shader && section->parts()[i].index_count) {
        geometry=section;part=i;owner={MenuWorldSourceOwnerKind::bsp_cluster,section->owner_datum(),cluster,{}};break;
      }
    }
    for(unsigned instance=0;instance<admission.instances.size() && !geometry;++instance) {
      if(admission.instances[instance].definition<0 ||
          !bytes.u16(definitions+unsigned(admission.instances[instance].definition)*200))continue;
      const auto actual=load_menu_world_source_bsp_owner(cache,bsp,instance);
      auto section=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,bsp,MenuWorldSourceSectionKind::bsp_instance_definition,actual.section));
      for(unsigned i=0;i<section->parts().size();++i)if(section->parts()[i].shader==shader && section->parts()[i].index_count) {
        geometry=section;part=i;classification=1;owner={MenuWorldSourceOwnerKind::bsp_instance,actual.bsp_datum,instance,{}};rows.push_back(actual.rows);break;
      }
    }
  }
  // The gun-barrel material belongs to the cinematic armory render model.
  // Isolate its class0 attribute arithmetic over unchanged BSP inputs. This
  // fixture proves fetch/bank/output behavior, not a live draw-owner choice.
  if(!geometry)for(unsigned bsp=0;bsp<cache.bsps().size() && !geometry;++bsp) {
    const auto admission=cache.load_source_bsp_admission(bsp);
    const auto metadata=cache.load_source_bsp_metadata(bsp);
    const halo2::Bytes bytes(metadata.bytes_at(metadata.source_address(),metadata.size()));
    const auto clusters=bytes.u32(16+156+4)-metadata.source_address();
    for(unsigned cluster=0;cluster<admission.clusters.size() && !geometry;++cluster) {
      if(!bytes.u16(clusters+cluster*176))continue;
      auto section=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,bsp,MenuWorldSourceSectionKind::bsp_cluster,cluster));
      for(unsigned i=0;i<section->parts().size();++i)if(section->parts()[i].index_count) {
        geometry=section;part=i;classification=0;owner={MenuWorldSourceOwnerKind::bsp_cluster,section->owner_datum(),cluster,{}};break;
      }
    }
  }
  require(bool(geometry),"Opening fixture has no nonempty raw section");
  const auto scratch=output/"native-world";PhysicsLiveWorldTu15 world(cache,argv[2],scratch);world.configure_session(source_game_variant_default(0),1,30,0x12345678u);world.initialize_map();
  const Vec3 position{0,0,3},forward{1,0,0},up{0,0,1};auto call=world.prepare_source_call();const auto input=call.gp[4]-0x200;
  const auto put=[&](unsigned p,const auto&v){world.write_source_call_input(p,std::as_bytes(std::span(&v,1)));};put(input,position);put(input+16,forward);put(input+32,up);
  call=world.prepare_source_call(std::array{definition,0xffffffffu,0xffffu,input+32});call.gp[3]=input;call.gp[7]=input+16;call.xmm[0][0]=std::bit_cast<unsigned>(1.f);world.run_actual(0xc2380,call);
  require(call.gp[0]!=0xffffffffu,"Original light constructor declined fixture");
  const auto light=pstencil::native_light_constants_tu15(world,call.gp[0],{1,1,1,1,1},1,{});
  MenuWorldSourceConstants bank{};std::copy(matching->constants[1].begin(),matching->constants[1].end(),bank.begin()+18);std::copy(light.constants.begin(),light.constants.end(),bank.begin()+31);
  SourceFrustumProjectionInput projection;projection.projection_angle=1.2217305f;projection.outer=projection.safe={0,0,480,640};projection.scenario_present=true;projection.scenario_type=1;
  const auto frustum=source_frustum_projection(projection);const auto camera=source_view_projection({position,forward,up,.015625f,1024.f,{},frustum.projection_program});const auto view=source_view_shader_bank(camera,1024.f,position,{0,0,640,480},false);std::copy(view.begin(),view.end(),bank.begin());
  auto draw=classification?menu_world_source_draw(geometry,part,MenuWorldSourceTopology::triangle_list,rows):menu_world_source_cluster_draw(geometry,part,MenuWorldSourceTopology::triangle_list);
  auto binding=menu_world_source_binding_class(cache,owner,std::move(draw),pass.vertex_shader(),classification,bank);
  require(binding.external_attribute_registers()==std::vector<unsigned>{10,11},"Failed material's V10/V11 read set changed");
  const auto masks=menu_world_vertex_input_components(binding.program(),false);
  require(masks[10]==15 && masks[11]==15,"Material attribute shader's RGBA read set changed");
  const auto&entry=menu_world_vertex_catalog_lookup(generated_menu_world_vertex_catalog::entries,binding.program_bytes(),binding.declaration());require(entry.abi==3,"GPU fixture did not select ABI3");
  MenuWorldVertexAttributeUniforms values;values.view.constants=binding.constants();values.view.clip_range={.015625f,1024.f,.015625f,1024.f};values.view.fog_parameters={1,1};values.view.surface_size={640,480};values.attributes=matching->constants[0];
  if(argc==6) {
    save(output/"native-uniform-input.bin",std::vector<std::byte>(reinterpret_cast<const std::byte*>(&values),reinterpret_cast<const std::byte*>(&values)+sizeof(values)));
    std::cout<<"CPU INPUTS ONLY: gun-barrel program with original opening BSP section "<<geometry->section()<<" part "<<part
        <<" class "<<classification<<"; V10/V11 read masks "<<unsigned(masks[10])<<','<<unsigned(masks[11])<<"; GPU not invoked\n";
    return 0;
  }
  std::vector<MenuWorldSourceBinding> bindings{binding};std::set<unsigned> classes{0};
  std::array<std::array<float,12>,40> palette;
  for(unsigned i=0;i<palette.size();++i)palette[i]={1,0,0,float(i)*.02f,0,1,0,-float(i)*.01f,0,0,1,float(i)*.03f};
  for(const auto&tag:cache.tags())if(tag.group=="mode") {
    const auto metadata=cache.load_source_tag_metadata(tag.datum);
    const halo2::Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
    for(unsigned section=0;section<root.u32(0x24);++section) {
      const halo2::Bytes definition(metadata.bytes_at(root.u32(0x28)+section*0x5c,0x5c));
      const auto mode=definition.u16(0);if(mode<1 || mode>3 || !definition.u16(4))continue;
      const auto raw=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,tag.datum,section));
      for(unsigned ordinal=0;ordinal<raw->parts().size();++ordinal) {
        const auto&p=raw->parts()[ordinal];if(!p.type || p.type==5 || p.index_count<3)continue;
        const auto cls=source_model_vertex_class(mode,std::to_integer<unsigned>(definition.slice(0x14,1)[0]),std::to_integer<unsigned>(definition.slice(0x15,1)[0]),p.type);
        if(cls<1 || cls>6 || classes.contains(cls))continue;
        // Explicit test palettes/identities. These raw source streams are not
        // claimed as currently visible actors or a live game pose.
        const MenuWorldSourceOwner fixture_owner{MenuWorldSourceOwnerKind::dynamic_object,tag.datum,1,0x12340001u};
        const auto count=cls==1?1u:40u;
        auto model_draw=menu_world_source_draw(raw,ordinal,MenuWorldSourceTopology::triangle_strip,std::span(palette).first(count));
        bindings.push_back(menu_world_source_binding_class(cache,fixture_owner,std::move(model_draw),pass.vertex_shader(),cls,bank));classes.insert(cls);
      }
    }
  }
  require(classes.contains(1) && classes.contains(2) && classes.contains(4),"Opening projected fixture lost rigid/skinned class coverage");
  Gpu gpu;if(!gpu.initialize(argv[3]))return 77;MenuWorldSourceStorage storage({gpu.device,gpu.physical});const auto stored=storage.store(bindings);
  struct ReferenceModule {std::vector<std::uint32_t> words;MenuWorldVertexCatalogEntry entry;};
  std::array<ReferenceModule,7> references;
  std::size_t compared=0,controls=0,original_words=0;float largest_error=0,largest_relative_error=0;
  for(const auto&draw:stored) {
    const auto&source=draw.source;const auto cls=source.declaration().class_index;
    const auto&catalog_entry=menu_world_vertex_catalog_lookup(generated_menu_world_vertex_catalog::entries,source.program_bytes(),source.declaration());
    auto uniform=values;uniform.view.constants=source.constants();
    const auto actual=gpu.run(draw,catalog_entry,uniform);const auto stem="class"+std::to_string(cls);save(output/(stem+"-gpu-authored.bin"),actual);
    if(argc==7) {
      const auto raw=read(fs::path(argv[6])/(stem+"-"+std::string(catalog_entry.program_sha256.substr(0,16))+".spv"));
      require(raw.size()%4==0,"Original-emitter module extent changed");auto& reference=references.at(cls);
      reference.words.resize(raw.size()/4);std::memcpy(reference.words.data(),raw.data(),raw.size());
      reference.entry=catalog_entry;reference.entry.spirv=reference.words;const auto original=gpu.run(draw,reference.entry,uniform);save(output/(stem+"-original-emitter.bin"),original);
      require(actual==original,"Projected GPU output differs from retained original-emitter arithmetic");original_words+=original.size()/4;
    }
    for(unsigned triangle=0;triangle<source.draw().triangle_count();++triangle)for(unsigned corner=0;corner<3;++corner) {
      const auto vertex=source.draw().triangle(triangle)[corner];VertexProgramState state;state.constants=uniform.view.constants;state.attributes=uniform.attributes;
      for(unsigned i=0;i<source.inputs().size();++i)state.attributes[source.inputs()[i].vertex_register]=fetch(source,i,vertex);
      execute_vertex_program(source.program(),state);
      for(unsigned group=0;group<6;++group)for(unsigned axis=0;axis<4;++axis) {
        const auto reg=group<2?3+group:9+group-2,offset=group<2?5+group*4:22+(group-2)*4;
        auto expected=state.outputs[reg][axis];if(group<2)expected=std::isnan(expected)?1.f:std::clamp(expected,0.f,1.f);
        const auto observed=scalar(actual,(triangle*3+corner)*51+offset+axis);
        const auto error=std::abs(observed-expected);largest_error=std::max(largest_error,error);
        largest_relative_error=std::max(largest_relative_error,error/std::max(1.f,std::abs(expected)));
        require((std::isnan(observed)&&std::isnan(expected)) || observed==expected || error<=.0002f*std::max(1.f,std::abs(expected)),"GPU material attribute result differs from independent token arithmetic");++compared;
      }
    }
    for(unsigned reg:{10u,11u}) {auto changed=uniform;changed.attributes[reg]={0,0,0,0};const auto zero=gpu.run(draw,catalog_entry,changed);save(output/(stem+"-gpu-zero-v"+std::to_string(reg)+".bin"),zero);require(zero!=actual,"Constant attribute negative control did not affect GPU output");++controls;}
  }
  require(validation_errors==0,"GPU validation failed");std::ofstream report(output/"result.json");report<<"{\"status\":0,\"abi\":3,\"shader\":"<<shader<<",\"classes\":[";bool first=true;for(auto cls:classes){if(!first)report<<',';report<<cls;first=false;}report<<"],\"compared_components\":"<<compared<<",\"largest_float_error\":"<<largest_error<<",\"largest_relative_error\":"<<largest_relative_error<<",\"attribute_controls\":"<<controls<<",\"original_emitter_exact_words\":"<<original_words<<",\"original_emitter_differing_words\":0,\"validation_errors\":0}\n";
  std::cout<<"PASS projected program/material RGBA over "<<classes.size()<<" original raw input classes; "<<compared<<" GPU components, "<<original_words<<" original-emitter exact words, "<<controls<<" attribute controls, validation0; isolated fixtures, not live draw-owner proof\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
