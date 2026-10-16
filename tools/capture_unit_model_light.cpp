#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/scenery.hpp"
#include "halo2/spawn.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/source_shadow_raw_plan.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_live_effect_light_commands_tu15.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_effect_light_geometry_frame.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include "halo2/renderer.hpp"
#include <memory>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* why){if(!ok)throw FormatError(why);}
Json light_state(const SourceDrawUnitState& state) {
  Json units=Json::array();for(const auto& unit:state.units)units.push_back(unit.words);
  return {{"main_prefix",state.main_prefix},{"draw_count",state.draw_count},{"override_count",state.override_count},
    {"section_highwater",state.section_highwater},{"units",units},{"coverage",state.coverage},
    {"override_slots",state.override_slots},{"overrides",state.overrides},
    {"sections",std::bit_cast<std::array<std::uint32_t,288>>(state.sections)},
    {"arena_used",state.arena.used},{"arena_words",state.arena.words}};
}
Json light_rows(std::span<const SourceBspAdmissionRecord> source) {
  Json rows=Json::array();for(const auto& r:source)rows.push_back({r.id,r.flags,r.association,r.other});return rows;
}
std::string material_sha256(std::span<const std::byte> bytes) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
  SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),digest.data());
  std::ostringstream text;for(auto byte:digest)text<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(byte);
  return text.str();
}
Json material_texture_observation(const Texture& texture) {
  Json mips=Json::array();for(const auto& mip:texture.mips)
    mips.push_back({mip.width,mip.height,mip.depth,mip.offset,mip.size});
  std::uint64_t fingerprint=14695981039346656037ull;
  for(auto byte:texture.pixels){fingerprint^=std::to_integer<unsigned char>(byte);fingerprint*=1099511628211ull;}
  return {{"width",texture.width},{"height",texture.height},{"depth",texture.depth},{"layers",texture.layers},
    {"format",texture.format},{"pixel_bytes",texture.pixels.size()},{"pixel_sha256",material_sha256(texture.pixels)},
    {"pixel_fnv1a",fingerprint},{"mips",mips}};
}
Json material_sampler_observation(const ShaderTextureState& state) {
  return {{"address",{state.address_u,state.address_v,state.address_w}},
    {"filter",{state.mag_filter,state.min_filter,state.mip_filter}},
    {"lod_bias",state.lod_bias},{"max_mip",state.max_mip},{"anisotropy",state.anisotropy},
    {"color_key_op",state.color_key_op},{"color_sign",state.color_sign},{"alpha_kill",state.alpha_kill},
    {"border_color",state.border_color},{"key_color",state.key_color}};
}
Json completed_material_observation(const Material& material,unsigned root,unsigned expanded,const std::string& path) {
  const auto bytes=[](std::span<const std::byte> source) {
    Json result=Json::array();for(auto byte:source)result.push_back(std::to_integer<unsigned>(byte));return result;
  };
  const auto remaps=[](std::span<const ShaderRemap> source) {
    Json result=Json::array();for(const auto& r:source)result.push_back({r.destination,r.component,r.source});return result;
  };
  Json result={{"root_slot",root},{"expanded_slot",expanded},{"child_path",path},
    {"source_shader",material.source_shader_datum},{"selected_shader_lod",material.selected_shader_lod},
    {"name",material.name},{"template_name",material.template_name},
    {"forward_pass",material.forward_pass},{"forward_pass_index",material.forward_pass_index},
    {"forward_vertex",material.forward_vertex},{"forward_blend",material.forward_blend},
    {"forward_blend_color",material.forward_blend_color},
    {"program_words",material.forward_program.words},{"fragment_offsets",material.forward_program.fragment_offsets},
    {"lightmap_program_words",material.lightmap_program.words},
    {"constants_words",std::bit_cast<std::array<std::uint32_t,128>>(material.program_constants)},
    {"vertex_program_bytes",bytes(material.vertex_program)},{"vertex_program_sha256",material_sha256(material.vertex_program)},
    {"world_vertex_input_transform",unsigned(material.world_vertex_input_transform)},
    {"shader_switches",material.shader_switches},{"texture_frames",material.texture_frames},
    {"texture_transform_words",std::bit_cast<std::array<std::uint32_t,12>>(material.texture_transforms)},
    {"forward_transform_words",std::bit_cast<std::array<std::uint32_t,32>>(material.forward_transforms)},
    {"forward_diffuse_words",std::bit_cast<std::array<std::uint32_t,4>>(material.forward_diffuse)},
    {"forward_specular_words",std::bit_cast<std::array<std::uint32_t,4>>(material.forward_specular)},
    {"specular_brightness_word",std::bit_cast<std::uint32_t>(material.specular_brightness)},
    {"ambient_brightness_word",std::bit_cast<std::uint32_t>(material.ambient_brightness)},
    {"static_lightmap_pass",material.static_lightmap_pass},{"per_vertex_lighting",material.per_vertex_lighting},
    {"world_pass",material.world_pass},{"baked_lightmap_texture",material.baked_lightmap_texture},
    {"accumulation_texture",material.accumulation_texture},{"scope_texture",material.scope_texture},
    {"water_reflection_texture",material.water_reflection_texture},{"water_refraction_texture",material.water_refraction_texture},
    {"rendering_gaps",material.rendering_gaps}};
  result["bitmap_parameters"]=Json::array();for(const auto& bitmap:material.bitmaps)
    result["bitmap_parameters"].push_back({{"datum",bitmap.datum},{"index",bitmap.index},
      {"log_dimension_word",std::bit_cast<std::uint32_t>(bitmap.log_dimension)}});
  if(material.object_render_colors)result["object_render_color_words"]=
      std::bit_cast<std::array<std::uint32_t,12>>(*material.object_render_colors);
  const auto& raster=material.forward_raster;
  result["raster"]={{"color_write",raster.color_write},{"depth_enable",raster.depth_enable},
    {"depth_write",raster.depth_write},{"depth_function",raster.depth_function},{"cull",raster.cull},
    {"alpha_test",raster.alpha_test},{"alpha_function",raster.alpha_function},{"alpha_reference",raster.alpha_reference}};
  if(material.vertex_declaration)result["vertex_declaration"]={{"class",material.vertex_declaration->class_index},
    {"semantics",material.vertex_declaration->semantics}};
  result["vertex_bank_present"]=bool(material.vertex_bank);
  if(material.vertex_bank) {
    const auto bank=pack_vertex_program_bank(*material.vertex_bank);
    result["vertex_bank_words"]=std::bit_cast<std::array<std::uint32_t,sizeof(bank)/4>>(bank);
  }
  if(material.forward_pass) {
    check(material.forward_pass_index<material.passes.size(),"Completed Mesh material lost its actual selected pass");
    const auto& pass=material.passes[material.forward_pass_index];
    Json states=Json::array();for(const auto& state:pass.states)states.push_back({state.index,state.value});
    result["selected_pass"]={{"template_pass",pass.template_pass},{"implementation",pass.implementation},
      {"pass_name",pass.name},{"vertex_shader",pass.vertex_shader},{"states",states},
      {"bitmap_remaps",remaps(pass.bitmap_remaps)},{"pixel_remaps",remaps(pass.pixel_remaps)},
      {"vertex_remaps",remaps(pass.vertex_remaps)},{"pixel_externs",remaps(pass.pixel_externs)},
      {"vertex_externs",remaps(pass.vertex_externs)},{"state_externs",remaps(pass.state_externs)}};
    result["selected_pass"]["textures"]=Json::array();for(const auto& t:pass.textures)
      result["selected_pass"]["textures"].push_back({t.parameter,t.external,t.stage,t.flags});
  }
  result["textures"]=Json::array();const Texture* images[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
  for(unsigned stage=0;stage<4;++stage) {
    auto texture=material_texture_observation(*images[stage]);texture["stage"]=stage;
    texture["sampler_state"]=material_sampler_observation(material.texture_states[stage]);
    texture["animated_frames"]=Json::array();for(const auto& frame:material.animated_textures[stage])
      texture["animated_frames"].push_back(material_texture_observation(frame));
    result["textures"].push_back(std::move(texture));
  }
  return result;
}
Json recorded_bsp_pipeline(const RendererObservedPipeline& pipeline) {
  Json shaders=Json::array();for(const auto& shader:pipeline.shaders) {
    Json spec=Json::array();for(auto byte:shader.specialization_bytes)spec.push_back(std::to_integer<unsigned>(byte));
    shaders.push_back({{"stage",shader.stage},{"module_identity",shader.module},{"spirv_words",shader.words},
      {"spirv_sha256",material_sha256(std::as_bytes(std::span{shader.words}))},
      {"specialization_entries",shader.specialization_entries},{"specialization_bytes",spec}});
  }
  return {{"handle",pipeline.handle},{"layout",pipeline.layout},{"render_pass",pipeline.render_pass},
    {"raster",pipeline.raster},{"raster_floats",pipeline.raster_floats},{"depth",pipeline.depth},
    {"depth_bounds",pipeline.depth_bounds},{"sample_mask",pipeline.sample_mask},{"dynamic_states",pipeline.dynamic_states},
    {"stencil",pipeline.stencil},{"blend",pipeline.blend},{"samples",pipeline.samples},
    {"minimum_sample_shading",pipeline.minimum_sample_shading},{"assembly",pipeline.assembly},
    {"vertex_bindings",pipeline.vertex_bindings},{"vertex_attributes",pipeline.vertex_attributes},{"shaders",shaders}};
}
Json recorded_bsp_descriptor(const RendererObservedDescriptor& descriptor) {
  Json tables=Json::array();for(const auto& table:descriptor.images) {
    Json stages=Json::array();for(const auto& image:table)stages.push_back({
      {"present",image.present},{"image_owner_known",image.image_owner_known},{"sampler",image.sampler},
      {"view",image.view},{"image",image.image},{"layout",image.layout},{"width",image.width},
      {"height",image.height},{"depth",image.depth},{"layers",image.layers},{"mip_count",image.mip_count},{"format",image.format}});
    tables.push_back(std::move(stages));
  }
  return {{"handle",descriptor.handle},{"image_tables",tables},{"buffers",descriptor.buffers}};
}
Json recorded_bsp_draw(const RendererObservedBspDraw& draw,unsigned view,std::uint32_t tick,
    const std::pair<unsigned,std::string>& logical,const std::string& push_file) {
  check(draw.pipeline && draw.descriptor && draw.resident_part && draw.source_token && draw.source_binding,"Recorded BSP draw lacks actual borrowed creation/binding/source owners");
  check(draw.camera_push.size()==112 && draw.vertex_stride==sizeof(Vertex) && draw.count==draw.source_vertices.size(),
    "Recorded BSP draw changed its camera/vertex ABI");
  check(draw.root_slot==logical.first,"Recorded BSP draw lost its completed logical material owner");
  const auto& source=*draw.resident_part;std::array<std::uint32_t,28> push;std::memcpy(push.data(),draw.camera_push.data(),112);
  Json transitions=Json::array();for(const auto& event:draw.target_transitions)transitions.push_back({
    {"phase",event.phase},{"transitions",event.transitions},{"complete",event.complete},
    {"render_pass",event.render_pass},{"framebuffer",event.framebuffer},{"render_area",event.render_area},{"clear_values",event.clear_values}});
  return {{"view",view},{"tick",tick},{"serial",draw.serial},{"phase",unsigned(draw.phase)},
    {"actual_shadow_target_transitions",transitions},
    {"root_slot",draw.root_slot},{"child_path",logical.second},{"expanded_slot",draw.material_slot},
    {"parent_material",draw.parent_material},{"source_material",draw.source_material},
    {"target_generation",draw.target_generation},{"render_pass",draw.render_pass},{"framebuffer",draw.framebuffer},
    {"pipeline",draw.pipeline->handle},{"pipeline_index",draw.pipeline_index},{"descriptor",draw.descriptor->handle},
    {"dynamic_offset",draw.dynamic_offset},{"lightmap",draw.lightmap},{"blend_constants",draw.blend_constants},{"camera_push_file",push_file},{"camera_push_words",push},
    {"camera_push_sha256",material_sha256(draw.camera_push)},{"viewport",draw.viewport},{"scissor",draw.scissor},
    {"vertex_buffer",draw.vertex_buffer},{"vertex_offset",draw.vertex_offset},{"vertex_stride",draw.vertex_stride},
    {"first",draw.first},{"count",draw.count},{"instances",draw.instances},{"first_instance",draw.first_instance},
    {"geometry",draw.geometry},{"section",draw.section},{"part",draw.part},{"source_resident",draw.source_resident},
    {"source_token",{{"storage",unsigned(draw.source_token->storage)},{"first",draw.source_token->first},
      {"count",draw.source_token->count},{"logical_first",draw.source_token->logical_first},
      {"run_count",draw.source_token->run_count}}},
    {"selection_binding",{{"material",draw.source_binding->material?Json(*draw.source_binding->material):Json(nullptr)},
      {"stipple",draw.source_binding->stipple?Json(*draw.source_binding->stipple):Json(nullptr)}}},
    {"resident_part",{{"first",source.first},{"count",source.count},{"material",source.original.material},
      {"lightmap",source.original.lightmap},{"geometry",source.original.geometry},{"section",source.original.section},
      {"part",source.original.part}}},{"source_vertex_bytes",draw.source_vertices.size_bytes()},
    {"source_vertex_sha256",material_sha256(std::as_bytes(draw.source_vertices))}};
}
void light_native_bank(const SourceRuntimeMemory& memory,const std::filesystem::path& path) {
  std::ofstream file(path,std::ios::binary);
  memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> bytes) {
    const auto size=std::uint32_t(bytes.size());file.write(reinterpret_cast<const char*>(&address),4);
    file.write(reinterpret_cast<const char*>(&size),4);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
  });
  check(bool(file),"Could not retain untouched native per-light input bank");
}
}
int main(int argc,char** argv)try {
  check(argc==8,"Supply MAP SHARED_MAPS TU15_XBE WIDTH HEIGHT FRESH_OUTPUT PINNED_SHADER_DIRECTORY");
  const auto original=OriginalData::load_xbe(argv[3]);
  check(original->image==OriginalData::Image::TitleUpdate15,"Unit light capture requires OG Xbox TU1.5");
  const Cache cache(argv[1],argv[2],original);const auto content=cache.load_content(0,true);
  const auto out=std::filesystem::absolute(argv[6]);
  check(!std::filesystem::exists(out),"Unit capture output already exists");
  std::filesystem::create_directories(out);
  const CollisionWorld terrain(content.collision);
  // The authored Lockout module anchors a declared diagnostic spawn search.
  // Floors and clear sightlines come from the original BSP. The native spawn
  // selector chooses which participant receives each entry in this pool.
  const ScenarioObject* anchor=nullptr;
  for(const auto& object:content.objects)if(object.type==11 && scenery_automatic_creation_allowed(object,2)) {
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
    if(tag && tag->name=="scenarios\\objects\\multi\\lockout\\power_module\\power_module") {anchor=&object;break;}
  }
  check(anchor,"Unit capture needs the authored Lockout module placement");
  const auto basis=authored_basis(anchor->rotation);
  const auto floor_position=[&](Vec3 p)->std::optional<Vec3> {
    const auto floor=terrain.raycast(p+Vec3{0,0,1},{0,0,-1},16);
    if(!floor)return {};
    p.z=floor->position.z+content.player.radius;return p;
  };
  const auto a=floor_position(anchor->position+basis.forward*4.f+basis.left*2.f);
  const auto b=floor_position(anchor->position+basis.forward*1.f+basis.left*2.f);
  check(a && b,"Declared unit capture spawn pool has no original BSP floor");
  check(length(*a-*b)>2.f,"Declared unit spawn entries overlap");
  const auto eye_a=*a+Vec3{0,0,content.player.camera_height};
  const auto eye_b=*b+Vec3{0,0,content.player.camera_height};
  const auto delta=eye_b-eye_a;
  check(!terrain.raycast(eye_a,normalized(delta),length(delta)-.05f),"Declared unit spawn pool lacks a clear BSP sightline");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=std::stoul(argv[4]);options.height=std::stoul(argv[5]);options.local_players=1;
  options.capture_shader_directory=std::filesystem::absolute(argv[7]);
  options.source_object_world=true;options.source_object_savegame_directory=out/"savegame";
  options.hidden=true;options.validation=true;options.mute=true;options.starting_weapon=5;
  options.capture_tick=200;options.reference_tick_cadence=true;options.reference_render_fraction=0;
  const auto* requested_draw_bindings=std::getenv("HALO2_CAPTURE_BSP_DRAW_BINDINGS");
  check(!requested_draw_bindings || !*requested_draw_bindings || std::strcmp(requested_draw_bindings,"0")==0 || std::strcmp(requested_draw_bindings,"1")==0,
    "HALO2_CAPTURE_BSP_DRAW_BINDINGS must be explicitly0 or1");
  const bool bsp_draw_bindings_requested=requested_draw_bindings && std::strcmp(requested_draw_bindings,"1")==0;
  check(!bsp_draw_bindings_requested || (options.width==640 && options.height==480),"Recorded BSP draw capture is bounded to actual640x480");
  const auto* requested_gpu=std::getenv("HALO2_CAPTURE_LIVE_SHADOW_GPU");
  check(!requested_gpu || !*requested_gpu || std::strcmp(requested_gpu,"0")==0 || std::strcmp(requested_gpu,"1")==0,
      "HALO2_CAPTURE_LIVE_SHADOW_GPU must be explicitly0 or1");
  const bool live_shadow_gpu_requested=requested_gpu && std::strcmp(requested_gpu,"1")==0;
  if(bsp_draw_bindings_requested) {
    const auto* tick=std::getenv("HALO2_CAPTURE_SHADOW_SELECTION_TICK");
    check(live_shadow_gpu_requested || (tick && std::strcmp(tick,"61")==0),"Recorded BSP draws require an explicit tick61 capture");
  }
  if(live_shadow_gpu_requested) {
    check(options.width==640 && options.height==480,"Live shadow GPU reference is bounded to actual640x480");
    if(const auto* tick=std::getenv("HALO2_CAPTURE_SHADOW_SELECTION_TICK");tick && *tick)
      check(std::strcmp(tick,"61")==0,"Live shadow GPU reference requires matching CPU snapshot tick61");
    options.capture_tick=61;options.reference_live_shadow_gpu_tick=61;
  }
  options.reference_auxiliary_seed=0x12345678u;
  Spawn spawn_a,spawn_b;spawn_a.position=*a;spawn_b.position=*b;spawn_a.bsp=spawn_b.bsp=0;
  spawn_a.facing=std::atan2(delta.y,delta.x);spawn_b.facing=spawn_a.facing+3.14159265358979323846f;
  options.native_spawn_inputs={spawn_a,spawn_b};
  // This is a declared input-seed search using the ordinary source chooser,
  // not a replacement selection or a body-position write. A spawn pool alone
  // does not assign its entries to participant slots.
  check(content.multiplayer_spawn.has_value(),"Unit capture lacks original spawn settings");
  const auto& settings=*content.multiplayer_spawn;
  const auto static_influences=settings.source_initial_zones.empty()?settings.initial_zones:
      source_spawn_zone_bank(settings.source_initial_zones,2,false,0,{},[](const auto&){return true;});
  const auto rules=slayer_rules(options.variant,true);
  const std::array points{*a,*b};std::array<int,2> expected_spawns{-1,-1};
  for(std::uint32_t candidate=0;candidate<65536;++candidate) {
    auto first_seed=spawn_private_seed(0,candidate,rules.spawn_salt,0);
    const auto first=choose_spawn(points,static_influences,settings.random_bias,first_seed,[](auto,auto){return true;});
    if(first.index<0)continue;
    auto influences=static_influences;const auto& enemy=settings.types[0];
    if(influences.size()<128)influences.push_back({points[first.index],enemy.inner,enemy.outer,
        settings.upper,settings.lower,enemy.weight});
    auto second_seed=spawn_private_seed(1,candidate,rules.spawn_salt,0);
    const auto second=choose_spawn(points,influences,settings.random_bias,second_seed,[](auto,auto){return true;});
    if(second.index>=0 && second.index!=first.index) {
      options.session_random_seed=candidate;expected_spawns={first.index,second.index};break;
    }
  }
  check(options.session_random_seed.has_value(),"Original chooser found no separated diagnostic spawn seed");
  options.input_script=out/"inputs.txt";options.screenshot=out/"unit.ppm";
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  const auto target_slot=options.prepared_world->match.join("Source light receiver");
  const auto initial=options.prepared_world->match.state();
  const auto camera_slot=options.prepared_world->local_slots.at(0);
  const auto& camera_player=initial.players.at(camera_slot);
  const auto& target_player=initial.players.at(target_slot);
  check(camera_player.source_unit!=null_datum && target_player.source_unit!=null_datum,
      "Native participant construction did not publish both real biped owners");
  check(target_player.source_weapon!=null_datum,"Native receiver has no actual held-weapon owner");
  check(length(camera_player.position-target_player.position)>2.f,"Native selector placed both participants together");
  for(unsigned slot=0;slot<2;++slot) {
    const auto& p=initial.players.at(slot);const auto expected=points.at(expected_spawns[slot]);
    check((p.source_player&65535u)==slot && p.position.x==expected.x && p.position.y==expected.y && p.position.z==expected.z,
        "Actual native constructor disagrees with the declared source-chooser seed");
  }
  const auto aim=normalized(target_player.position+Vec3{0,0,content.player.camera_height*.55f}-
      (camera_player.position+Vec3{0,0,content.player.camera_height}));
  const auto yaw=std::atan2(aim.y,aim.x),pitch=std::atan2(aim.z,std::hypot(aim.x,aim.y));
  {std::ofstream f(options.input_script);f.precision(9);
    for(unsigned tick=0;tick<options.capture_tick;++tick)
      f<<tick<<" 0 0 0 "<<(tick==20?1:0)<<' '<<yaw<<' '<<pitch<<'\n';}
  const std::set<std::uint32_t> owners{target_player.source_unit,target_player.source_weapon};
  const auto audited_owner=std::to_string(target_player.source_unit);
  check(::setenv("HALO2_CAPTURE_UNIT_ADMISSION_OWNER",audited_owner.c_str(),1)==0,"Could not enable read-only unit admission audit");
  std::map<std::uint32_t,std::uint32_t> definitions;
  for(const auto owner:owners)definitions.emplace(owner,options.prepared_world->source_objects->object_definition(owner));
  Json report={{"authority","OG Xbox TU1.5"},
    {"boundary","Actual native biped/held-weapon base/light join and grenade delivery. Declared offline spawn pool; no substituted poses, lights or original-frame parity claim."},
    {"source_owners",owners},{"camera_slot",camera_slot},{"target_slot",target_slot},
    {"session_random_seed",*options.session_random_seed},{"source_selected_spawn_indices",expected_spawns},
    {"pinned_shader_directory",options.capture_shader_directory.string()},
    {"initial_players",Json::array()},{"target_frames",Json::array()},{"draws",Json::array()}};
  for(const auto slot:{camera_slot,target_slot}) {const auto& p=initial.players[slot];
    report["initial_players"].push_back({{"slot",slot},{"unit",p.source_unit},{"weapon",p.source_weapon},
      {"position",{p.position.x,p.position.y,p.position.z}}});}
  report["spawn_pool"]=Json::array();for(const auto& p:options.native_spawn_inputs)
    report["spawn_pool"].push_back({{"position",{p.position.x,p.position.y,p.position.z}},{"facing",p.facing},{"bsp",p.bsp}});
  report["bsp_draw_bindings_requested"]=bsp_draw_bindings_requested;
  report["live_shadow_gpu_requested"]=live_shadow_gpu_requested;
  report["live_shadow_gpu_enabled"]=false;
  report["live_shadow_gpu_submission"]=Json::object();
  report["shadow_visual_parity_verified"]=false;
  if(live_shadow_gpu_requested)options.reference_live_shadow_gpu_observer=[&](unsigned view,std::uint32_t tick,
      bool submitted,const SourceShadowRawPlanStats& stats) {
    check(!view && tick==61,"Live shadow GPU observer lost its explicitly requested tick/view");
    report["live_shadow_gpu_submission"]={{"view",view},{"tick",tick},{"submitted",submitted},
      {"roots",stats.roots},{"caster_draws",stats.caster_draws},{"receiver_draws",stats.receiver_draws},{"indices",stats.indices},
      {"boundary","Actual copied synchronous source roots and guarded native640attachment lease submitted through Renderer. Ordinary base/rawApply D24 compatibility and matched Xbox framebuffer parity remain separate gates."}};
    report["live_shadow_gpu_enabled"]=submitted && stats.roots>0;
  };
  std::map<std::uint64_t,NativeGrenadeDetonation> events;
  std::set<std::uint32_t> other_unit_lights;
  report["other_unit_lights"]=Json::array();
  std::uint32_t source_tick{},light_tick{};unsigned light_packets{},draws{},lit{},visible{};
  Json current_inputs=Json::array();std::vector<Vertex> geometry;
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& state) {
    source_tick=state.tick;current_inputs=Json::array();
    const auto& target=state.players.at(target_slot);
    report["target_frames"].push_back({{"tick",state.tick},{"connected",target.connected},{"dead",target.dead},
      {"unit",target.source_unit},{"previous_unit",target.source_previous_unit},{"weapon",target.source_weapon},
      {"flags",target.source_flags},{"health",target.health},{"shield",target.shield}});
    for(const auto& event:state.grenade_detonations)events.emplace(event.serial,event);
    auto& source=*options.prepared_world->source_objects;
    for(const auto owner:owners) {
      const auto nodes=source.world_nodes(owner);const auto sphere=source.object_sphere(owner);
      Json bank=Json::array();static_assert(sizeof(Transform)==52);
      for(const auto& node:nodes)bank.push_back(std::bit_cast<std::array<std::uint32_t,13>>(node));
      current_inputs.push_back({{"object",owner},{"nodes",bank},
        {"sphere",{sphere.center.x,sphere.center.y,sphere.center.z,sphere.radius}},
        {"velocity",std::bit_cast<std::array<std::uint32_t,3>>(source.object_linear_velocity(owner))}});
    }
  };
  options.effect_light_capture_request=[&](unsigned view,std::uint32_t tick,
      std::span<const EffectLightRawDraw> packets,std::span<const WorldEffectLight> lights) {
    if(view || light_tick || events.empty())return false;
    std::map<std::uint32_t,std::set<std::uint32_t>> selected;
    for(const auto& packet:packets) {const auto& owner=packet.source.owner();
      if(owner.kind!=MenuWorldSourceOwnerKind::dynamic_object || !owner.generation || !owners.contains(*owner.generation))continue;
      const auto light=std::find_if(lights.begin(),lights.end(),[&](const auto& l){return l.identity==packet.light_identity;});
      check(light!=lights.end(),"Unit light packet lost its original occupied light identity");
      const bool grenade=std::any_of(events.begin(),events.end(),[&](const auto& pair) {
        return pair.second.projectile==light->effect_owner && pair.second.effect==light->effect_definition;
      });
      if(!grenade) {
        if(other_unit_lights.insert(light->identity).second)
          report["other_unit_lights"].push_back({{"identity",light->identity},{"definition",light->definition},
            {"effect_owner",light->effect_owner},{"effect_definition",light->effect_definition},{"first_observed_tick",tick}});
        continue;
      }
      selected[packet.light_identity].insert(*owner.generation);
    }
    std::set<std::uint32_t> receivers;
    for(const auto& [identity,bank]:selected)receivers.insert(bank.begin(),bank.end());
    if(receivers!=owners)return false;
    check(source_tick==tick,"Unit light capture lost its completed source frame");
    report["model_light_sources"]=Json::array();
    for(const auto& [identity,receivers]:selected) {
      const auto at=std::find_if(lights.begin(),lights.end(),[&](const auto& light){return light.identity==identity;});
      check(at!=lights.end(),"Unit light packet lost its original occupied light identity");
      Json matches=Json::array();for(const auto& [serial,event]:events)
        if(event.projectile==at->effect_owner && event.effect==at->effect_definition)
          matches.push_back({{"serial",serial},{"tick",event.tick},{"projectile",event.projectile},{"effect",event.effect},{"source_effect_instance",event.source_effect_instance}});
      check(!matches.empty(),"Unit illumination does not belong to actual grenade effect delivery");
      unsigned count{};for(const auto& packet:packets)if(packet.light_identity==identity &&
          packet.source.owner().kind==MenuWorldSourceOwnerKind::dynamic_object &&
          packet.source.owner().generation && owners.contains(*packet.source.owner().generation))++count;
      light_packets+=count;
      report["model_light_sources"].push_back({{"identity",identity},{"receivers",receivers},
        {"definition",at->definition},{"effect_owner",at->effect_owner},{"effect_definition",at->effect_definition},
        {"delivery_tick",at->tick},{"model_packets",count},{"original_effect_matches",matches}});
    }
    check(!live_shadow_gpu_requested || tick==61,"Actual grenade-light capture did not reach the requested live shadow GPU tick61");
    light_tick=tick;options.capture_tick=tick;report["source_inputs"]=current_inputs;return true;
  };
  if(const auto* requested=live_shadow_gpu_requested?"61":std::getenv("HALO2_CAPTURE_SHADOW_SELECTION_TICK");requested && *requested) {
    std::size_t end{};const auto requested_tick=std::stoul(requested,&end);
    check(end==std::strlen(requested) && requested_tick>0 && requested_tick<200,"Invalid shadow snapshot tick");
    // These callbacks outlive this setup block. Keep the before/after
    // descriptor-layer index alive with them until the actual capture ends.
    const auto descriptor_layers=std::make_shared<std::map<std::pair<std::uint32_t,std::uint32_t>,std::uint32_t>>();
    const auto retain=[&,requested_tick,descriptor_layers](bool before,unsigned view,std::uint32_t tick,
        std::uint32_t owner,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi& abi,const SourceRuntimeMemory& memory) {
      if(view || tick!=requested_tick)return;
      auto destination=entry==0x333f0u?out/"shadow-selection":
          out/"shadow-admission"/std::to_string(owner)/std::to_string(entry);
      if(entry==0x3baa0u) {
        const auto key=std::pair{owner,entry};
        if(before)(*descriptor_layers)[key]=Bytes(memory.view(abi.gp[4]+12u,4)).u32(0);
        destination/=std::to_string(descriptor_layers->at(key));
      }
      std::filesystem::create_directories(destination);
      const auto name=before?"before":"after";
      check(abi.x87_depth==0 && !abi.active_mmx && abi.floating_point,"Shadow snapshot requires the actual complete native FP state");
      const Json state={{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags},{"fs_base",abi.fs_base},
        {"x87_depth",abi.x87_depth},{"x87_status",abi.x87_status},
        {"x87_control",abi.floating_point->x87_control_word},{"mxcsr",abi.floating_point->mxcsr}};
      std::ofstream(destination/(std::string(name)+"-abi.json"))<<state.dump(2)<<'\n';
      std::ofstream file(destination/(std::string(name)+".bin"),std::ios::binary);
      memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> bytes) {
        const auto size=std::uint32_t(bytes.size());
        file.write(reinterpret_cast<const char*>(&address),4);file.write(reinterpret_cast<const char*>(&size),4);
        file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
      });
      check(bool(file),"Could not retain actual shadow call memory");
      std::ofstream(destination/"call.json")<<Json{{"entry",entry},{"owner",owner},{"tick",tick},{"view",view},
        {"authority","OG Xbox TU1.5"},{"boundary","Actual completed native before/after source call, with retained full ABI and banks. No supplied return or queue result; no shadow graphics or matched framebuffer claim."}}.dump(2)<<'\n';
    };
    options.shadow_selection_observer=[retain](bool before,unsigned view,std::uint32_t tick,
        const SourcePhysicsWorldRuntimeAbi& abi,const SourceRuntimeMemory& memory) {
      retain(before,view,tick,0u,0x333f0u,abi,memory);
    };
    options.shadow_admission_observer=retain;
    const auto light_birth_sequence=std::make_shared<unsigned>(0);
    report["native_light_births"]=Json::array();
    options.prepared_world->source_objects->tu15_world().observe_effect_light_births(
      [&,requested_tick,light_birth_sequence](bool before,const SourceEffectLightBirthTu15& birth,
          const SourcePhysicsWorldRuntimeAbi& abi,const SourceRuntimeMemory& memory) {
        if(before)++*light_birth_sequence;
        check(*light_birth_sequence>0,"Native light birth lacks its actual before-call owner");
        const auto destination=out/"effect-light-births"/std::to_string(*light_birth_sequence-1);
        std::filesystem::create_directories(destination);
        const auto phase=before?"before":"after";
        Json call={{"schema","tu15-native-effect-light-birth-v1"},{"authority","OG Xbox TU1.5"},
          {"phase",phase},{"entry",0xc2380u},{"callsite",birth.callsite},{"source_tick",birth.source_tick},
          {"last_observed_input_tick",source_tick},{"definition",birth.definition},{"light",birth.light},
          {"parent_effect",birth.parent_effect},{"parent_effect_body",birth.parent_effect_body},
          {"event_ordinal",birth.event_ordinal},{"part_ordinal",birth.part_ordinal},{"part_address",birth.part_address},
          {"location",birth.location?Json(*birth.location):Json(nullptr)},
          {"location_address",birth.location_address?Json(*birth.location_address):Json(nullptr)},
          {"owner",birth.owner},{"node",birth.node},{"control_word",birth.control_word},
          {"position_address",birth.position_address},{"forward_address",birth.forward_address},{"up_address",birth.up_address},
          {"light_record",birth.light_record},{"light_words",birth.light_words},
          {"native_pool_remap_enabled",false},{"cached_command_publication_enabled",false},
          {"boundary","Read-only literal17DB3B/17DB6D before/after complete nativeC2380. Parent effect/part/location are validated actual source ownership; no position/tag identity inference or typed light mapping."}};
        // Retain both actual source ticks adjacent to the requested displayed
        // tick. The exported tick is literal; no fixed-step offset is changed.
        const bool bank=birth.source_tick==requested_tick || birth.source_tick+1==requested_tick;
        call["native_bank_exported"]=bank;
        if(bank) {
          check(abi.x87_depth==0 && !abi.active_mmx && abi.floating_point,"Native light birth snapshot requires complete empty x87 state");
          const Json state={{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags},{"fs_base",abi.fs_base},
            {"x87_depth",abi.x87_depth},{"x87_status",abi.x87_status},
            {"x87_control",abi.floating_point->x87_control_word},{"mxcsr",abi.floating_point->mxcsr}};
          std::ofstream(destination/(std::string(phase)+"-abi.json"))<<state.dump(2)<<'\n';
          light_native_bank(memory,destination/(std::string(phase)+".bin"));
        }
        std::ofstream(destination/(std::string(phase)+".json"))<<call.dump(2)<<'\n';
        report["native_light_births"].push_back(std::move(call));
      });
    options.diagnostic_native_light_seed=true;
    report["effect_light_command_input_capture"]={{"schema","tu15-live-effect-light-inputs-v1"},
      {"tick",requested_tick},{"diagnostic_native_seed",true},{"native_state_seed_enabled",true},{"native_command_join_enabled",true},
      {"live_shadow_gpu_enabled",false},{"boundary","Actual retained native-to-typed seed before genuine secondary query reset/collection, then complete native40E80 after cull. PREseed inputs and actual producer ABI/banks are retained; no supplied callee result or shadow GPU activation."}};
    const auto light_calls=std::make_shared<std::map<std::pair<unsigned,std::uint32_t>,unsigned>>();
    options.effect_light_command_observer=[&,requested_tick,light_calls](unsigned view,std::uint32_t tick,
        const SourceEffectLightCommandObservationTu15& observed) {
      if(view || tick!=requested_tick)return;
      auto& sequence=(*light_calls)[{view,observed.light}];
      if(observed.phase==SourceEffectLightCommandPhaseTu15::before_seed)++sequence;
      check(sequence>0,"Light input phase lacks its matching before-seed owner");
      const auto phase=std::array{"before-seed","before-collection","collected","culled"}.at(unsigned(observed.phase));
      const auto destination=out/"effect-light-command-inputs"/std::to_string(view)/std::to_string(observed.light)/std::to_string(sequence-1)/phase;
      std::filesystem::create_directories(destination);
      const auto& memory=observed.world.memory();const auto native=source_live_effect_light_state_tu15(memory);
      Json queues=Json::array();for(const auto& q:native.queues)queues.push_back({{"header",q.header},{"capacity",q.capacity},{"streams",q.streams}});
      Json transforms=Json::array();for(const auto& transform:observed.instance_transforms)
        transforms.push_back(std::bit_cast<std::array<std::uint32_t,13>>(transform));
      const auto& context=observed.context;
      Json call={{"schema","tu15-live-effect-light-inputs-v1"},{"authority","OG Xbox TU1.5"},
        {"view",view},{"tick",tick},{"light",observed.light},{"sequence",sequence-1},{"phase",phase},
        {"diagnostic_native_seed",observed.diagnostic_native_seed},{"native_state_seed_enabled",true},{"native_command_join_enabled",observed.native_command_join_enabled},{"live_shadow_gpu_enabled",false},
        {"typed_state",light_state(observed.state)},{"retained_native_state",light_state(native.state)},
        {"retained_native_main_arena",native.main_arena.words},{"retained_native_main_arena_used",native.main_arena.used},
        {"native_bsp",native.bsp},{"native_current_context",native.current_context},{"native_queue_allocations",queues},
        {"bsp",{{"clusters",light_rows(observed.bsp.clusters)},{"instances",light_rows(observed.bsp.instances)},
          {"cluster_entries",observed.bsp.cluster_entries},{"marked_clusters",observed.bsp.marked_clusters},{"associations",observed.bsp.associations}}},
        {"objects",{{"rows",light_rows(observed.objects.objects)},{"associations",observed.objects.associations}}},
        {"bindings",{{"main_arena_token",observed.main_arena_token},{"light_arena_token",observed.light_arena_token},
          {"instance_token",observed.instance_token},{"instance_transform_words",transforms}}},
        {"context",{{"pass_mode",context.draw.mode},{"query_mode",context.cull.query_mode},{"command_selector",context.command_selector},
          {"override_byte",context.draw.override_byte},{"sphere_center_words",std::bit_cast<std::array<std::uint32_t,3>>(context.cull.center)},
          {"sphere_radius_word",std::bit_cast<std::uint32_t>(context.cull.radius)},
          {"coverage_camera_words",std::bit_cast<std::array<std::uint32_t,5>>(context.draw.camera)},
          {"lod_view_words",std::bit_cast<std::array<std::uint32_t,7>>(context.commands.lod_view)},
          {"player_multiplier_word",std::bit_cast<std::uint32_t>(context.commands.player_multiplier)},
          {"view_modifier",context.commands.view_modifier}}},
        {"boundary","Synchronous actual typed light phase with real native world/resources. PREseed typed and retained actual states are both retained; no source memory writes or command/GPU execution by the observer."}};
      const bool save_bank=observed.phase==SourceEffectLightCommandPhaseTu15::before_seed || observed.phase==SourceEffectLightCommandPhaseTu15::culled;
      call["native_bank_exported"]=save_bank;
      if(save_bank)light_native_bank(memory,destination/"native.bin");
      std::ofstream(destination/"call.json")<<call.dump(2)<<'\n';
    };
    // Shared synchronous metadata joins the actual Mesh flatten order to
    // the later final per-view upload. It supplies no material or draw input.
    const auto completed_slots=std::make_shared<std::map<unsigned,std::pair<unsigned,std::string>>>();
    const auto completed_roots=std::make_shared<std::set<unsigned>>();
    const auto completed_uploads=std::make_shared<Json>(Json::array());
    const auto recorded_draws=std::make_shared<Json>(Json::array());
    const auto recorded_pipelines=std::make_shared<Json>(Json::object());
    const auto recorded_descriptors=std::make_shared<Json>(Json::object());
    if(bsp_draw_bindings_requested) {
      check(requested_tick==61,"Recorded BSP draw capture requires explicitly requested tick61");
      options.completed_bsp_draw_tick=requested_tick;
      options.completed_bsp_draw_observer=[&,requested_tick,completed_slots,completed_roots,recorded_draws,recorded_pipelines,recorded_descriptors](
          unsigned view,std::uint32_t tick,const RendererObservedBspDraw& draw) {
        check(!view && tick==requested_tick,"Recorded BSP draw lost its requested actual view/tick");
        if(!completed_roots->contains(draw.root_slot))return;
        const auto identity=completed_slots->find(draw.material_slot);
        check(identity!=completed_slots->end(),"Recorded BSP child has no actual completed topology owner");
        const auto stem="view"+std::to_string(view)+"-tick"+std::to_string(tick)+"-draw"+std::to_string(draw.serial)+".push.bin";
        const auto relative="bsp-recorded-draws/"+stem;const auto destination=out/"bsp-recorded-draws";
        std::filesystem::create_directories(destination);
        std::ofstream file(destination/stem,std::ios::binary);
        file.write(reinterpret_cast<const char*>(draw.camera_push.data()),draw.camera_push.size());
        check(bool(file),"Could not retain actual recorded camera push bytes");
        const auto row=recorded_bsp_draw(draw,view,tick,identity->second,relative);
        (*recorded_pipelines)[std::to_string(draw.pipeline->handle)]=recorded_bsp_pipeline(*draw.pipeline);
        (*recorded_descriptors)[std::to_string(draw.descriptor->handle)]=recorded_bsp_descriptor(*draw.descriptor);
        recorded_draws->push_back(row);
        const Json data{{"authority","OG Xbox TU1.5"},{"draws",*recorded_draws},{"pipelines",*recorded_pipelines},
          {"descriptors",*recorded_descriptors},{"boundary","Actual source-resident and uniquely joined dynamic BSP draws and selected children, immediately before unchanged vkCmdDraw. Exact current push/set/bind arguments and creation/write facts. Recorded commands are not surviving samples, GPU payload readback or original-frame parity."}};
        std::ofstream(out/"bsp-recorded-draws.json")<<data.dump(2)<<'\n';
        report["bsp_recorded_draws"]={{"file","bsp-recorded-draws.json"},{"count",recorded_draws->size()}};
      };
    }
    options.completed_material_upload_tick=requested_tick;
    options.completed_material_upload_observer=[&,requested_tick,completed_slots,completed_roots,completed_uploads](
        unsigned view,std::uint32_t tick,const RendererCompletedMaterialUpload& upload) {
      if(view || tick!=requested_tick)return;
      if(!completed_roots->contains(unsigned(upload.root_slot)))return;
      const auto identity=completed_slots->find(unsigned(upload.material_slot));
      check(identity!=completed_slots->end() && identity->second.first==upload.root_slot,
          "Actual completed upload lost its observed Mesh child identity");
      check(upload.material_uniform.size()==3712 && upload.vertex_bank_uniform.size()==2544,
          "Actual completed material upload changed its observed ABI");
      const auto destination=out/"bsp-final-material-uploads";std::filesystem::create_directories(destination);
      const auto stem="view"+std::to_string(view)+"-tick"+std::to_string(tick)+"-slot"+
          std::to_string(upload.material_slot)+"-ordinal"+std::to_string(completed_uploads->size());
      const auto retain_bytes=[&](const std::string& name,std::span<const std::byte> bytes) {
        std::ofstream file(destination/name,std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        check(bool(file),"Could not retain actual final material upload bytes");
      };
      retain_bytes(stem+".material.bin",upload.material_uniform);
      retain_bytes(stem+".vertex-bank.bin",upload.vertex_bank_uniform);
      Json pages=Json::array(),samplers=Json::array();
      for(unsigned stage=0;stage<4;++stage) {
        const auto& page=upload.cached_material_pages[stage];Json mips=Json::array();
        for(const auto& mip:page.source_mips)mips.push_back({mip.width,mip.height,mip.depth,mip.offset,mip.size});
        pages.push_back({{"stage",stage},{"width",page.width},{"height",page.height},{"depth",page.depth},
          {"layers",page.layers},{"mip_count",page.mip_count},{"format",page.format},
          {"source_pixel_fnv1a",page.source_pixel_fingerprint?Json(*page.source_pixel_fingerprint):Json(nullptr)},{"source_image",page.source_image},
          {"source_mutated",page.source_mutated?Json(*page.source_mutated):Json(nullptr)},{"source_mips",mips},
          {"image_identity",page.image_identity},{"view_identity",page.view_identity},
          {"cube_view_identity",page.cube_view_identity},{"sampler_identity",page.sampler_identity}});
        samplers.push_back(material_sampler_observation(upload.texture_states[stage]));
      }
      completed_uploads->push_back({{"view",view},{"tick",tick},{"root_slot",upload.root_slot},
        {"expanded_slot",upload.material_slot},{"child_path",identity->second.second},
        {"source_shader",upload.source_shader},{"selected_shader_lod",upload.selected_shader_lod},
        {"root_material_count",upload.root_material_count},{"expanded_material_count",upload.expanded_material_count},
        {"target_generation",upload.target_generation},{"source_frames_submitted",upload.source_frames_submitted?Json(*upload.source_frames_submitted):Json(nullptr)},
        {"material_file","bsp-final-material-uploads/"+stem+".material.bin"},
        {"material_bytes",upload.material_uniform.size()},{"material_sha256",material_sha256(upload.material_uniform)},
        {"vertex_bank_file","bsp-final-material-uploads/"+stem+".vertex-bank.bin"},
        {"vertex_bank_bytes",upload.vertex_bank_uniform.size()},{"vertex_bank_sha256",material_sha256(upload.vertex_bank_uniform)},
        {"wanted_frames",upload.wanted_frames},{"bound_frames",upload.bound_frames},
        {"cached_material_pages",pages},{"sampler_states",samplers},
        {"per_draw_baked_lightmap",upload.baked_lightmap},{"external_accumulation",upload.accumulation},
        {"external_scope",upload.scope},{"external_water",upload.water_targets}});
      const Json data{{"authority","OG Xbox TU1.5"},{"uploads",*completed_uploads},
        {"boundary","Actual slots already collected by Renderer, after final per-view CPU completion and before upload. Bytes borrowed synchronously and copied. Cached image metadata is not a per-draw descriptor/lightmap/external target/pipeline observation; GPU execution and original framebuffer parity remain separate."}};
      std::ofstream(out/"bsp-final-material-uploads.json")<<data.dump(2)<<'\n';
      report["bsp_final_material_uploads"]={{"file","bsp-final-material-uploads.json"},{"count",completed_uploads->size()}};
    };
    options.bsp_submission_observer=[&,requested_tick,completed_slots,completed_roots](unsigned view,std::uint32_t tick,
        const SourceBspResidentViewPlan& plan,std::span<const Material> materials) {
      if(view || tick!=requested_tick)return;
      Json parts=Json::array(),bindings=Json::array(),bank=Json::array();std::set<unsigned> slots;
      for(const auto& part:plan.selected_parts) {
        parts.push_back({{"geometry",part.geometry},{"section",part.section},{"part",part.part},
          {"shader_lod",part.shader_lod},{"material_slot",part.material},
          {"coverage",part.coverage_byte},{"instance_alpha",part.instance_alpha}});
        slots.insert(part.material);
      }
      for(const auto& binding:plan.bindings)bindings.push_back({
        {"material_slot",binding.material?Json(*binding.material):Json(nullptr)},
        {"stipple",binding.stipple?Json(*binding.stipple):Json(nullptr)}});
      const auto byte_values=[](const std::vector<std::byte>& bytes) {
        Json values=Json::array();for(auto value:bytes)values.push_back(std::to_integer<unsigned>(value));return values;
      };
      const auto words=[](const ShaderConstants& values) {
        return std::bit_cast<std::array<std::uint32_t,sizeof(ShaderConstants)/4>>(values);
      };
      for(auto slot:slots) {
        check(slot<materials.size(),"Live BSP selection lost its submitted material");
        const auto& material=materials[slot];Json passes=Json::array();
        for(const auto& pass:material.passes) {
          Json states=Json::array();for(const auto& state:pass.states)states.push_back({state.index,state.value});
          passes.push_back({{"template_pass",pass.template_pass},{"implementation",pass.implementation},
            {"vertex_shader",pass.vertex_shader},{"vertex_code_bytes",byte_values(pass.vertex_code)},
            {"program_words",pass.program.words},{"constants_words",words(pass.constants)},{"states",states}});
        }
        bank.push_back({{"slot",slot},{"source_shader",material.source_shader_datum},
          {"selected_shader_lod",material.selected_shader_lod},{"static_lightmap_pass",material.static_lightmap_pass},
          {"forward_pass",material.forward_pass},{"forward_pass_index",material.forward_pass_index},
          {"forward_vertex",material.forward_vertex},{"per_vertex_lighting",material.per_vertex_lighting},
          {"world_pass",material.world_pass},{"forward_program_words",material.forward_program.words},
          {"lightmap_program_words",material.lightmap_program.words},{"constants_words",words(material.program_constants)},
          {"vertex_program_bytes",byte_values(material.vertex_program)},{"passes",passes}});
      }
      // Mirror the actual Renderer::make_textures flatten topology across
      // ALL roots; selected BSP roots alone cannot determine child slots.
      std::vector<const Material*> expanded;std::vector<std::pair<unsigned,std::string>> identities;
      Json topology=Json::array();
      const auto add=[&](const Material& material,unsigned root,const std::string& path) {
        expanded.push_back(&material);identities.emplace_back(root,path);
      };
      for(unsigned n=0;n<materials.size();++n) {
        const auto& m=materials[n];add(m,n,"root");
        topology.push_back({{"slot",n},{"environment",bool(m.environment_pass)},
          {"before",m.world_overlays_before.size()},{"after",m.world_overlays_after.size()},
          {"bloom",m.world_bloom_passes.size()},{"planar",bool(m.planar_fog_pass)},
          {"world_planar",{bool(m.world_planar_passes[0]),bool(m.world_planar_passes[1]),bool(m.world_planar_passes[2])}}});
      }
      for(unsigned n=0;n<materials.size();++n)if(materials[n].environment_pass)add(*materials[n].environment_pass,n,"environment");
      for(unsigned n=0;n<materials.size();++n) {
        const auto& m=materials[n];
        for(unsigned k=0;k<m.world_overlays_before.size();++k)add(*m.world_overlays_before[k],n,"before"+std::to_string(k));
        for(unsigned k=0;k<m.world_overlays_after.size();++k)add(*m.world_overlays_after[k],n,"after"+std::to_string(k));
        for(unsigned k=0;k<m.world_bloom_passes.size();++k)add(*m.world_bloom_passes[k],n,"bloom"+std::to_string(k));
      }
      for(unsigned n=0;n<materials.size();++n)if(materials[n].planar_fog_pass)add(*materials[n].planar_fog_pass,n,"planar");
      for(unsigned n=0;n<materials.size();++n)for(unsigned k=0;k<3;++k)
        if(materials[n].world_planar_passes[k])add(*materials[n].world_planar_passes[k],n,"world_planar"+std::to_string(k));
      *completed_roots=slots;completed_slots->clear();Json flattened=Json::array();
      for(unsigned n=0;n<expanded.size();++n)if(slots.contains(identities[n].first)) {
        check(completed_slots->emplace(n,identities[n]).second,"Duplicate actual flattened material observation");
        flattened.push_back(completed_material_observation(*expanded[n],identities[n].first,n,identities[n].second));
      }
      const Json data{{"authority","OG Xbox TU1.5"},{"view",view},{"tick",tick},
        {"root_material_count",materials.size()},{"expanded_material_count",expanded.size()},
        {"root_topology",topology},{"flattened_materials",flattened},
        {"selected_parts",parts},{"bindings",bindings},{"materials",bank},
        {"boundary","Read-only actual ordinary BSP selected parts, completed animated Mesh root/child banks and actual full flatten topology immediately before Renderer::draw. Per-view Renderer completion and actual per-draw bindings are separate observations. Shadow LOD is not substituted for main LOD; no visibility/material/source result is supplied."}};
      std::ofstream(out/"bsp-main-submission.json")<<data.dump(2)<<'\n';
      report["bsp_main_submission"]={{"file","bsp-main-submission.json"},{"view",view},{"tick",tick},
        {"selected_parts",parts.size()},{"materials",bank.size()}};
    };
    report["shadow_completed_roots"]=Json::array();
    options.shadow_root_completed=[&,requested_tick](unsigned view,std::uint32_t tick,
        PhysicsLiveWorldTu15& world,const SourceLiveShadowRootTu15& root) {
      if(view || tick!=requested_tick || !root.ordered_draws)return;
      check(root.collected.has_value(),"Completed shadow descriptors have no collected units");
      Json records=Json::array();
      for(const auto& phase:*root.ordered_draws) {
        const auto& bank=(*root.commands)[phase.phase==SourceShadowPhase::Generate?1:0];
        for(unsigned n=0;n<phase.records.size();++n) {
          const auto& descriptor=phase.records[n];
          const auto& command=bank.commands.at(descriptor[0]&65535u);
          const auto ordinal=command[3]>>8&65535u;
          const auto& unit=root.collected->state.units.at(ordinal);
          Json row={{"phase",unsigned(phase.phase)},{"sorted_ordinal",n},
            {"unit_ordinal",ordinal},{"descriptor",descriptor},{"command",command},{"unit",unit.words}};
          if(unit.words[1]!=0xffffffffu) {
            const auto owner=Bytes(world.memory().view(unit.words[4],4)).u32(0);
            (void)world.object_body(owner);
            const auto geometry=source_live_model_command_geometry_tu15(world,unit);
            const auto rows=source_live_model_palette_rows_tu15(world,unit,geometry);
            row["geometry_owner"]=owner;
            Json matrices=Json::array();for(const auto& matrix:rows)
              matrices.push_back(std::bit_cast<std::array<std::uint32_t,12>>(matrix));
            row["palette_words"]=std::move(matrices);
          } else if(unit.words[0]&0x1000u) {
            const auto& memory=world.memory();const auto bsp=Bytes(memory.view(0x4ecc18u,4)).u32(0);
            const auto count=Bytes(memory.view(bsp+0x140u,4)).u32(0),instances=Bytes(memory.view(bsp+0x144u,4)).u32(0);
            const auto instance=(unit.words[3]>>18)&2047u;
            check(instance<count && unit.words[7]==instances+instance*88u,"Completed shadow instance lost its actual Transform owner");
            Transform transform;std::memcpy(&transform,memory.view(unit.words[7],sizeof(transform)).data(),sizeof(transform));
            const auto rows=source_live_model_palette_rows_tu15(world,unit,{});
            row["instance_transform_words"]=std::bit_cast<std::array<std::uint32_t,13>>(transform);
            row["palette_words"]=Json::array();for(const auto& matrix:rows)
              row["palette_words"].push_back(std::bit_cast<std::array<std::uint32_t,12>>(matrix));
          }
          records.push_back(std::move(row));
        }
      }
      report["shadow_completed_roots"].push_back({{"view",view},{"tick",tick},
        {"owner",root.owner},{"records",records},
        {"boundary","Synchronous actual completed root before later roots/light queries reuse native contexts or palettes; no supplied source result or GPU submission."}});
    };
    options.shadow_synchronization_observer=[&,requested_tick](unsigned view,std::uint32_t tick,const PhysicsLiveWorldTu15& world) {
      if(view || tick!=requested_tick)return;
      // Read actual quiescent native synchronization ownership. Export
      // failure is retained and never converted into a successful wait.
      const auto destination=out/"shadow-selection"/"native-synchronization.json";
      std::filesystem::create_directories(destination.parent_path());
      try {
        const auto owner=world.authority_checkpoint();
        const auto& synchronization=owner.arena.synchronization;
        Json objects=Json::array();for(const auto& object:synchronization.objects)
          objects.push_back({{"handle",object.handle},{"kind",unsigned(object.kind)},
            {"count",object.count},{"limit",object.limit}});
        std::ofstream(destination)<<Json{{"authority","Actual native game-arena synchronization ownership"},
          {"version",synchronization.version},{"first",synchronization.first},{"next",synchronization.next},
          {"limit",synchronization.limit},{"objects",objects},{"lock_events",owner.lock_events},
          {"main_thread_token",owner.main_thread_token},{"exported",true}}.dump(2)<<'\n';
      }catch(const std::exception& e) {
        std::ofstream(destination)<<Json{{"exported",false},{"error",e.what()}}.dump(2)<<'\n';
      }
    };
  }
  report["shadow_root_frames"]=Json::array();
  options.shadow_root_frame_observer=[&](unsigned view,std::uint32_t tick,
      std::span<const SourceLiveShadowRootTu15> roots) {
    static constexpr std::array names{"ineligible","parent","disabled","blocked","local_light","no_lighting","opacity","projected"};
    for(const auto& root:roots) {
      Json row={{"view",view},{"tick",tick},{"owner",root.owner},
        {"route",names.at(unsigned(root.route))},{"first_person",root.first_person},
        {"lighting_address",root.lighting_address},{"owner_fade",root.owner_fade},
        {"history_fade",root.history_fade},{"cached_opacity",root.cached_opacity},
        {"extents",{root.extents.x,root.extents.y}}};
      if(root.frame) {
        row["frame_values"]=root.frame->source_values();
        row["frame_words"]=std::bit_cast<std::array<std::uint32_t,29>>(root.frame->source_values());
      }
      if(root.view_volume) {
        row["view_admitted"]=root.view_volume->admitted;
        row["projected_bounds_words"]=std::bit_cast<std::array<std::uint32_t,6>>(root.view_volume->projected_bounds);
        row["world_endpoints_words"]=std::bit_cast<std::array<std::uint32_t,72>>(root.view_volume->world_endpoints);
      }
      if(root.projection)row["projection_words"]=root.projection->words;
      if(root.uniforms) {
        row["shadow_depth_feature"]=root.shadow_depth_feature;
        row["uniform_projection_words"]=std::bit_cast<std::array<std::uint32_t,24>>(root.uniforms->projection);
        row["uniform_constants_words"]=std::bit_cast<std::array<std::uint32_t,36>>(root.uniforms->constants);
      }
      if(root.secondary_camera) {
        row["secondary_camera_words"]=std::bit_cast<std::array<std::uint32_t,111>>(root.secondary_camera->camera);
        row["secondary_plane_inconsistent"]=root.secondary_camera->plane_inconsistent;
      }
      if(root.secondary_queues) {
        const auto& q=*root.secondary_queues;
        const auto rows=[](const auto& source) {
          Json result=Json::array();for(const auto& r:source)
            result.push_back({r.id,r.flags,r.association,r.other});return result;
        };
        row["secondary_queues"]={{"context",q.context},{"visibility_bank",q.visibility_bank},{"pass_mode",q.pass_mode},{"query_mode",q.query_mode},
          {"clusters",rows(q.clusters)},{"objects",rows(q.objects)},{"instances",rows(q.instances)}};
        row["query_radius_word"]=std::bit_cast<std::uint32_t>(root.query_radius);
      }
      if(root.collected) {
        const auto& c=*root.collected;const auto& state=c.state;
        Json units=Json::array();for(unsigned n=0;n<state.draw_count;++n)units.push_back(state.units[n].words);
        row["collected"]={{"main_prefix",state.main_prefix},{"draw_count",state.draw_count},{"units",units},
          {"coverage",std::vector(state.coverage.begin(),state.coverage.begin()+state.draw_count)},
          {"override_slots",std::vector(state.override_slots.begin(),state.override_slots.begin()+state.draw_count)},
          {"overrides",std::vector(state.overrides.begin(),state.overrides.begin()+state.override_count)},
          {"section_highwater",state.section_highwater},{"sections",std::bit_cast<std::array<std::uint32_t,288>>(state.sections)},
          {"main_arena_address",c.main_arena_address},{"main_arena_used",c.main_arena.used},{"main_arena",c.main_arena.words},
          {"secondary_arena_address",c.secondary_arena_address},{"secondary_arena_used",state.arena.used},{"secondary_arena",state.arena.words}};
      }
      if(root.commands) {
        row["commands"]=Json::array();
        for(const auto& bank:*root.commands)
          row["commands"].push_back({{"address",bank.address},{"header",bank.header},{"commands",bank.commands}});
      }
      if(root.ordered_draws) {
        row["ordered_draws"]=Json::array();
        for(const auto& ordered:*root.ordered_draws)
          row["ordered_draws"].push_back({{"phase",unsigned(ordered.phase)},{"owner",ordered.owner},
            {"bank",ordered.bank},{"category",ordered.category},{"layer",ordered.layer},
            {"caller_stack",ordered.caller_stack},{"source_address",ordered.source_address},
            {"capacity",ordered.capacity},{"records",ordered.records}});
      }
      report["shadow_root_frames"].push_back(std::move(row));
    }
  };
  options.unit_source_draw_observer=[&](unsigned view,std::uint32_t owner,std::uint32_t definition,
      const ObjectLightingState& light,bool valid,unsigned lod,std::span<const Vertex> vertices) {
    if(!owners.contains(owner))return;check(view==0,"Single-view unit capture received another viewport");
    check(definitions.at(owner)==definition,"Unit base draw changed its full original definition identity");
    ++draws;lit+=valid;geometry.insert(geometry.end(),vertices.begin(),vertices.end());
    report["draws"].push_back({{"tick",source_tick},{"object",owner},{"definition",definition},
      {"geometry_lod",lod},{"lighting_valid",valid},{"lighting",light.source_values()},{"vertices",vertices.size()}});
    if(light_tick && source_tick==light_tick) {
      std::ofstream file(out/("base-"+std::to_string(owner)+".bin"),std::ios::binary);
      file.write(reinterpret_cast<const char*>(vertices.data()),vertices.size_bytes());
      check(bool(file),"Could not retain the actual captured unit base vertices");
    }
  };
  options.view_observer=[&](unsigned view,const Camera& camera,float horizontal_fov,std::int16_t) {
    check(!view,"Single-view unit capture received another camera");visible=0;
    const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
    const auto right=normalized(cross(forward,camera.oriented?camera.up:Vec3{0,0,1})),up=cross(right,forward);
    const float horizontal=std::tan(horizontal_fov*.5f),aspect=float(options.width)/options.height;
    for(std::size_t n=0;n+2<geometry.size();n+=3) {
      const auto center=(geometry[n].position+geometry[n+1].position+geometry[n+2].position)/3.f;
      const auto delta=center-camera.position;const auto ahead=dot(delta,forward);
      if(ahead<=0 || std::abs(dot(delta,right))>ahead*horizontal || std::abs(dot(delta,up))>ahead*horizontal/aspect)continue;
      const auto distance=length(delta);if(distance>.01f && !terrain.raycast(camera.position,delta/distance,distance-.01f))++visible;
    }
    const auto raw_up=camera.oriented?camera.up:Vec3{0,0,1};
    geometry.clear();report["final_view_geometry"]={{"oriented",camera.oriented},
      {"raw_submitting_up",{raw_up.x,raw_up.y,raw_up.z}},{"raw_rendering_forward",{forward.x,forward.y,forward.z}},
      {"eye",{camera.position.x,camera.position.y,camera.position.z}},
      {"unoccluded_triangle_centers",visible}};
  };
  const auto result=run_multiplayer(options);
  report["runtime_exit"]=result;report["model_light_tick"]=light_tick;report["model_light_packets"]=light_packets;
  report["draw_counts"]=draws;report["lit_draws"]=lit;
  report["grenade_events"]=Json::array();
  for(const auto& [serial,event]:events)report["grenade_events"].push_back({{"serial",serial},{"tick",event.tick},
      {"projectile",event.projectile},{"effect",event.effect},{"source_effect_instance",event.source_effect_instance},
      {"position",{event.position.x,event.position.y,event.position.z}}});
  std::ofstream(out/"report.json")<<report.dump(2)<<'\n';
  check(result==0 && std::filesystem::exists(options.screenshot),"Live unit capture did not complete");
  check(!live_shadow_gpu_requested || report["live_shadow_gpu_enabled"].get<bool>(),
      "Requested live shadow GPU reference did not submit any actual command-mask root");
  check(!bsp_draw_bindings_requested || (report.contains("bsp_recorded_draws") && report["bsp_recorded_draws"]["count"].get<unsigned>()>0),
      "Requested recorded BSP draw capture observed no actual BSP draw");
  check(light_tick && light_packets,"Actual grenade produced no joined unit light packets");
  check(draws && lit && visible,"Capture lacks visible joined unit geometry and valid lighting");
  std::cout<<"PASS actual native biped/held-weapon base and grenade model light; original-frame parity remains separate\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
