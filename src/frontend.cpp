#include "halo2/frontend.hpp"
#include "halo2/tags.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/frontend_audio.hpp"
#include "halo2/frontend_extension_catalog.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/debug_panel.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/frontend_timing.hpp"
#include "halo2/frontend_model.hpp"
#include "halo2/frontend_model_pose.hpp"
#include "halo2/frontend_model_lighting.hpp"
#include "halo2/frontend_model_shader.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/atmosphere.hpp"
#include "halo2/audio.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/frontend_keyboard.hpp"
#include "halo2/math.hpp"
#include "halo2/movie.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/renderer.hpp"
#include "halo2/mainmenu_world_source.hpp"
#include "halo2/scripts.hpp"
#include "halo2/sky.hpp"
#include "halo2/world_material.hpp"
#include "halo2/menu_projection.hpp"
#include "halo2/mainmenu_bsp_frame.hpp"
#include "halo2/mainmenu_portal_state.hpp"
#include "halo2/light_volume.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/model_selection.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <chrono>
#include <charconv>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <set>
namespace halo2 {
namespace {
struct AnimatedObjectMaterials {
  std::size_t placement{}, first{}, count{};
  std::vector<ObjectFunction> functions;
  std::uint32_t handle = null_datum;
};
struct FrontendHeldWeapon {
  RenderModel model;
  std::optional<AnimationClip> animation;
  WeaponAttachmentMarkers attachment;
  ObjectAttachmentFrame local;
  unsigned material_base{};
  std::optional<SourceDynamicModelShaders> shaders;
  SourceGeometryLodDefinition geometry;
  std::int16_t scenario_type{};
  std::int8_t geometry_level{};
};
struct FrontendPreparedModel {
  RenderModel model;
  Transform root;
  std::optional<AnimationClip> animation, ui_animation;
  std::optional<FrontendModelPrimary> primary,ui_primary;
  FrontendModelClock clock;
  std::array<std::uint32_t,8> deferred{};
  std::optional<FrontendModelGrip> grip;
  FrontendModelWeaponIk combat_ik,ui_ik;
  FirstPersonAimBlend aim;
  bool original_world_callback=false,source_biped_pose=false;
  std::optional<FrontendHeldWeapon> weapon;
  std::optional<FrontendAppearance> appearance;
  float initial_shield_fraction{};
  std::optional<SourceDynamicModelShaders> shaders;
  ShadowCasterDefinition bounds;
  ShadowCasterSphere sphere;
  bool cinematic_lighting{};
  std::array<ObjectLightingBinding,2> lighting;
  unsigned material_base{};
  std::vector<Vertex> vertices;
  std::size_t body_vertices{};
  bool pose_dirty = true;
  void select_aim() {
    if(!original_world_callback || !animation)return;
    FirstPersonAimEligibility eligibility;
    eligibility.base_state=0x0400000cu; // Actual menu primary idle action.
    eligibility.base_animation_flags=animation->authored_flags;
    source_aim_blend_select(aim,source_weapon_aim_eligible(eligibility),30);
  }
  void update_pose() {
    if (!pose_dirty) return;
    if(!source_biped_pose || !animation) {
      vertices=pose_model_world_frame(model,root,animation?&*animation:nullptr,clock.frame);
      for(auto&vertex:vertices)vertex.material+=material_base;
      pose_dirty=false;
      return;
    }
    if(!primary)throw FormatError("frontend biped lacks verified primary source codec");
    const bool ui=ui_animation && animation->source_graph==ui_animation->source_graph &&
        animation->source_index==ui_animation->source_index;
    const auto posed=frontend_model_pose(model,root,*animation,clock.frame,deferred,
        grip?&*grip:nullptr,weapon?&weapon->model:nullptr,
        weapon && weapon->animation?&*weapon->animation:nullptr,
        weapon?&weapon->attachment:nullptr,ui?&ui_ik:&combat_ik,
        source_aim_blend_weight(aim),&*primary,weapon?&weapon->local:nullptr);
    vertices=skin_model_world_nodes(model,posed.body);
    sphere=source_shadow_sphere(bounds,posed.body.front(),root.scale);
    body_vertices=vertices.size();
    for(auto&vertex:vertices)vertex.material+=material_base;
    if(weapon) {
      // Attached133E30 records inherit this owner's current sphere distance.
      // The source single-held BR/PR flags have neither bit19 nor10A bit2.
      const auto level=frontend_model_geometry_lod(weapon->geometry,sphere.center,
          0,0,weapon->scenario_type,weapon->geometry_level);
      if(level.level!=weapon->geometry_level)
        throw FormatError("frontend held weapon geometry residency changed");
      auto held=skin_model_world_nodes(weapon->model,posed.weapon);
      for(auto&vertex:held)vertex.material+=weapon->material_base;
      vertices.insert(vertices.end(),held.begin(),held.end());
    }
    pose_dirty=false;
  }
};
std::vector<AnimatedObjectMaterials> append_scene(const Cache &cache, const TagSystem& tags,
    const LevelContent &level, Mesh &mesh,
    std::vector<LightVolumeInstance> &light_volumes) {
  std::vector<AnimatedObjectMaterials> animated;
  std::map<std::uint32_t,std::shared_ptr<const LightVolumeDefinition>> volume_definitions;
  std::optional<unsigned> glow_material;
  for (const auto &object : level.objects) {
    if (object.type == 10 || object.type == 0 ||
        object.definition == 0xffffffffu || (object.flags & 1))
      continue;
    const auto tag = tags.try_get({object.definition},tag_groups::scenery);
    if (!tag)
      continue;
    try {
      auto model = cache.load_model(object.definition, true, object.variant);
      if (tag->info().name == "scenarios\\objects\\covenant\\military\\capital_ship\\capital_ship") {
        // Captured original draws 1448/1453 use implementation 3. Both
        // opaque A/B programs match its 18 packed lighting constants.
        for (auto &material : model.mesh.materials) {
          if (prepare_authored_lighting_material(cache, material))
            continue;
          // capital_ship_c has only layer7. Its authored additive/alpha
          // states and disabled depth writes must replace the collapsed
          // surface binding; it has no layer3 lighting implementation.
          if (material.name == "scenarios\\objects\\covenant\\military\\capital_ship\\shaders\\capital_ship_c" &&
              !prepare_default_object_material(cache, material))
            throw FormatError("original carrier emissive pass was not admitted");
        }
        // Original 1880/1885 draw the same two body sections with the
        // layer-16 planar program also captured on BSP draw 2409.
        prepare_menu_bsp_fog(cache, model.mesh);
      }
      const auto base = unsigned(mesh.materials.size());
      if(std::any_of(model.mesh.materials.begin(),model.mesh.materials.end(),
                     [](const auto&m){return !m.animation.overlays.empty();}))
        animated.push_back({std::size_t(&object-level.objects.data()),base,
                           model.mesh.materials.size(),cache.load_object_functions(object.definition)});
      for (auto &m : model.mesh.materials)
        mesh.materials.push_back(std::move(m));
      const auto rotation = authored_basis(object.rotation);
      for (auto v : pose_scenery_model(cache, model)) {
        const auto p = rotation.rotate(v.position * object.scale);
        v.position = {p.x + object.position.x, p.y + object.position.y,
                      p.z + object.position.z};
        v.material += base;
        v.normal = rotation.rotate(v.normal);
        v.tangent = rotation.rotate(v.tangent);
        v.binormal = rotation.rotate(v.binormal);
        v.incident = rotation.rotate(v.incident);
        mesh.vertices.push_back(v);
      }
      const Transform world{object.scale,{rotation.forward,rotation.left,rotation.up},object.position};
      for(const auto &attachment:cache.load_light_volume_attachments(object.definition,model,0,&world)) {
        if(!volume_definitions.contains(attachment.datum))
          volume_definitions[attachment.datum]=std::make_shared<LightVolumeDefinition>(cache.load_light_volume(attachment.datum));
        const auto &definition=volume_definitions.at(attachment.datum);
        if(std::any_of(definition->volumes.begin(),definition->volumes.end(),
            [](const auto &volume){return volume.bitmap!=null_datum || volume.flags&8;}))
          throw FormatError("non-default or fuzzy title light-volume rendering is not implemented");
        if(!glow_material) {
          glow_material=unsigned(mesh.materials.size());
          Material glow;glow.name="rasterizer/glow";glow.diffuse=cache.load_light_volume_default_bitmap();
          glow.forward_pass=true;glow.light_volume_pass=true;
          glow.forward_program=light_volume_program();glow.forward_blend=2;
          glow.forward_raster.color_write=0x10101;glow.forward_raster.cull=0;
          mesh.materials.push_back(std::move(glow));
        }
        for(const auto &marker:attachment.markers)
          light_volumes.push_back({definition,{marker},*glow_material});
      }
    } catch (const FormatError &e) {
      std::cerr << "Menu scenery: " << tag->info().name << ": " << e.what() << '\n';
    }
  }
  return animated;
}
} // namespace
std::optional<FrontendLaunch> run_frontend(const FrontendOptions &options) {
  if (options.capture_ui_step_ms && (!options.frames || !options.capture_ui_ms))
    throw FormatError("UI capture clock progression requires --frames and --menu-ui-ms");
  if ((!options.capture_actions.empty() || options.capture_ui_ms) && !options.frames)
    throw FormatError("menu capture actions and UI time require --frames");
  const auto session = options.session ? options.session : std::make_shared<FrontendSession>();
  const bool initial_entry=options.entry==FrontendOptions::Entry::initial;
  if(initial_entry && session->has_content())
    throw FormatError("Used frontend requires an explicit new screen factory");
  if(!initial_entry && (!session->has_state() || !session->has_content() ||
      options.screen!=9 || options.audit || !options.sweep.empty()))
    throw FormatError("Retained frontend factory requires its existing configuration and content");
  const auto cache_owner=std::make_shared<const Cache>(options.maps / "mainmenu.map");
  const auto& cache=*cache_owner;
  TagSystem tags(cache_owner);
  tags.load_bsp(0);
  auto &content = initial_entry ? session->retain_content(cache.load_frontend()) : session->content();
  auto &audio_state=session->audio_state();
  if(options.desktop && audio_state.desktop && options.desktop!=audio_state.desktop)
    throw FormatError("Retained frontend audio requires its original desktop owner");
  const auto desktop = options.desktop ? options.desktop : audio_state.desktop
      ?audio_state.desktop:std::make_shared<DesktopSession>();
  audio_state.desktop=desktop;
  auto vista_map=options.vista_settings;
  if(vista_map.empty())
    vista_map=options.maps.parent_path().parent_path().parent_path()/
        "Halo 2 Vista installer/maps/mainmenu.map";
  if(initial_entry && std::filesystem::is_regular_file(vista_map)) {
    content.vista=load_vista_settings(vista_map);
    const FrontendExtensionCatalog catalog(content);
    for(const auto &[id,screen]:content.vista->screens)
      catalog.validate_definition({FrontendMenuSource::vista,std::uint16_t(id)},screen);
    std::cout << "Vista settings: " << content.vista->screens.size()
              << " source-qualified definitions from " << vista_map << '\n';
  } else if(initial_entry && !options.vista_settings.empty())
    throw FormatError("requested Vista settings cache is unavailable");
  const auto ui_assets = load_frontend_ui_assets(content, options.ui_assets);
  if (!options.ui_assets.empty())
    std::cout << "Optional frontend 2D texture pack: " << ui_assets.size()
              << " image banks; Xbox layout, font metrics and 3D assets retained\n";
  std::size_t panes = 0, bitmaps = 0;
  for (const auto &[id, s] : content.screens) {
    panes += s.panes.size();
    for (const auto &p : s.panes)
      bitmaps += p.bitmaps.size();
  }
  std::cout << "Retail frontend: " << content.screens.size() << " screens, "
            << panes << " panes, " << bitmaps << " bitmap widgets, "
            << content.parameters.size() << " variant parameters, "
            << content.variants.size() << " built-in variants\n";
  if (options.audit) {
    session->create_state();
    const FrontendExtensionCatalog extensions(content);
    for (const auto &[id, s] : content.screens)
      std::cout << "Screen " << id << " panes=" << s.panes.size() << " "
                << s.name << '\n';
    std::size_t vista_variants = 0, vista_additions = 0;
    for (const auto &entry : extensions.entries()) {
      if (entry.key.source != FrontendMenuSource::vista) continue;
      vista_variants += entry.relation == FrontendMenuRelation::vista_variant;
      vista_additions += entry.relation == FrontendMenuRelation::vista_addition;
      if (entry.relation == FrontendMenuRelation::vista_addition)
        std::cout << "Reserved Vista screen " << entry.key.id
                  << " panes=" << entry.pane_count << " " << entry.path << '\n';
    }
    std::cout << "Vista menu reservations: " << vista_additions << " additional screens, "
              << vista_variants << " separate shared variants; metadata only\n";
    for (const auto &[id, p] : content.parameters)
      std::cout << "Parameter " << p.parameter
                << " choices=" << p.choices.size() << " " << p.title << '\n';
    return {};
  }
  auto level = cache.load_content(0);
  auto mesh = cache.load_bsp(0);
  // Import source LOD alternatives before scenery/UI allocate material slots.
  // The plan leaves the original packed BSP vertices unchanged.
  SourceBspResidentPlan mainmenu_resident(cache,mesh,0,
      SourceBspResidentLighting::classification3);
  MainmenuWorldSourcePlan mainmenu_world_source(cache,mainmenu_resident,0);
  // Mainmenu's absent lightmap bitmap/checksum makes 2D000 return mode 3
  // for all 70 clusters and 104 instances. 4CBF0 retains implementation 3.
  unsigned authored_lighting = 0;
  for (auto &material : mesh.materials)
    authored_lighting += prepare_authored_lighting_material(cache, material);
  std::cout << "Authored menu lighting materials: " << authored_lighting << '\n';
  prepare_menu_bsp_fog(cache,mesh);
  prepare_bsp_planar_fog(cache,mesh,cache.load_bsp_atmosphere(0));
  std::cout << "Authored BSP planar-fog materials: " << std::count_if(mesh.materials.begin(),mesh.materials.end(),
      [](const auto &material){return bool(material.planar_fog_pass);}) << '\n';
  std::vector<LightVolumeInstance> light_volumes;
  auto animated_objects = append_scene(cache, tags, level, mesh,light_volumes);
  std::cout << "Authored title light-volume attachments: " << light_volumes.size() << '\n';
  // Retail146240 seeds the secondary stream from wall time, uptime and a
  // CRT rand draw. Host startup and other consumer ordering remain separate.
  std::uint32_t menu_crt_state = 1;
  const auto menu_secondary_seed = source_secondary_random_seed(
      std::uint32_t(std::time(nullptr)),
      std::uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch()).count()),
      source_crt_random(menu_crt_state));
  auto effect_seed = menu_secondary_seed;
  if (const auto *seed = std::getenv("HALO2_MENU_EFFECT_SEED")) {
    const auto *end = seed + std::char_traits<char>::length(seed);
    const auto result = std::from_chars(seed, end, effect_seed);
    if (result.ec != std::errc{} || result.ptr != end)
      throw FormatError("HALO2_MENU_EFFECT_SEED must be a uint32 decimal value");
  }
  CollisionWorld title_collision(level.collision);
  NativeWorldEffects title_effects(cache, mesh, title_collision, effect_seed);
  std::cout << "Authored title particle attachments: " << title_effects.attach_scenery(level)
            << " seed=" << effect_seed << '\n';
  const auto sky = cache.load_visible_sky(0);
  std::optional<RenderModel> sky_model;
  unsigned sky_material_base = 0;
  if (sky) {
    sky_model = cache.load_model(sky->model);
    sky_material_base = unsigned(mesh.materials.size());
    for (auto &material : sky_model->mesh.materials)
      mesh.materials.push_back(std::move(material));
    std::cout << "Visible retail menu sky: " << sky->name << '\n';
  }
  std::map<std::string, FrontendPreparedModel> models;
  ObjectLightingState cinematic_lighting;
  // Original lifecycle entry53:13BF00 allocates/zeros the interface before
  // loaded mainmenu scripts. Its actual byte5 follows13BFF0/13CA80 below.
  SourceBspShaderLodAuxiliary cinematic{true,false};
  std::set<std::string> requested;
  for (const auto &[id, screen] : content.screens)
    for (const auto &pane : screen.panes)
      for (const auto &scene : pane.scenes)
        for (const auto &name : scene.objects)
          requested.insert(name);
  for (const auto &object : level.objects) {
    if (object.name < 0 ||
        !requested.contains(level.object_names.at(object.name).name))
      continue;
    auto model = cache.load_model(object.definition, true, object.variant);
    for(auto &material:model.mesh.materials) {
      const auto colors=material.object_render_colors;
      // Actual ten menu units retain+2B0=0;33670 inherits that scalar for
      // attached weapons. The source context is separate from lighting.
      // 4CBF0 overrides15D70's preliminary3 with collector classification2.
      if(!material.lods.empty() && !prepare_dynamic_material(cache,material,{},true,0.f))
        throw FormatError("frontend original model shader is unresolved: "+material.name);
      if(colors)apply_object_render_colors(material,*colors);
    }
    SourceDynamicModelShaders shaders(cache,model,
        cache.load_initial_model_definition(object.definition).render_model,2,0.f);
    const auto base = unsigned(mesh.materials.size());
    for (auto &m : model.mesh.materials)
      mesh.materials.push_back(std::move(m));
    const auto tag = tags.try_get({object.definition});
    const bool biped = tag && tag->info().type() == tag_groups::biped;
    const bool progress_ring = tag && tag->info().type() == tag_groups::scenery &&
        level.object_names.at(object.name).name == "halo_ring2";
    // Original SCEN10A1B0 ->10A3F0 starts progress_symbol's authored idle
    // at frame0 with looping enabled. It is a model, not bitmap rotation.
    const auto animation = biped
        ? cache.load_frontend_model_animation(object.definition, false)
        : progress_ring ? cache.load_initialized_object_idle(object.definition)
                        : std::optional<AnimationClip>{};
    if(progress_ring && !animation)
      throw FormatError("matchmaking progress ring lacks its original idle animation");
    const auto rotation = authored_basis(object.rotation);
    const auto frame = source_object_transform(object.position, rotation.forward,
        rotation.up, object.scale, false);
    FrontendPreparedModel prepared;
    prepared.model = std::move(model); prepared.root = frame;
    prepared.shaders=std::move(shaders);
    prepared.bounds=cache.load_object_shadow_definition(object.definition);
    prepared.animation = animation; prepared.material_base = base;
    if (biped) {
      prepared.source_biped_pose=true;
      prepared.initial_shield_fraction=cache.load_frontend_model_initial_shield_fraction(object.definition);
      if(animation)prepared.primary=cache.load_frontend_model_primary_animation(
          animation->source_graph,animation->source_index);
      prepared.deferred=cache.load_frontend_model_deferred_mask(object.definition);
      prepared.grip=cache.load_frontend_model_grip(object.definition);
      prepared.combat_ik=cache.load_frontend_model_weapon_ik(object.definition,0x06000086u);
      prepared.ui_ik=cache.load_frontend_model_weapon_ik(object.definition,0x020001b3u);
      prepared.original_world_callback=prepared.combat_ik.world_callback;
      prepared.select_aim(); //113870 clear, then initial1130F0 selection at epoch0.
      const auto requested_ui = cache.load_frontend_model_animation(object.definition);
      // A failed request retains the original combat channel, including phase.
      if (requested_ui && animation &&
          (requested_ui->source_graph != animation->source_graph ||
           requested_ui->source_index != animation->source_index)) {
        prepared.ui_animation = requested_ui;
        prepared.ui_primary=cache.load_frontend_model_primary_animation(
            requested_ui->source_graph,requested_ui->source_index);
      }
      if (const auto weapon = cache.load_frontend_model_weapon(object.definition)) {
        const auto attachment = select_weapon_attachment(
            cache.load_weapon_attachment(object.definition, *weapon, 0), {}, {});
        if (attachment.named_child || attachment.parent.region != -1)
          throw FormatError("frontend held weapon requires unresolved animated marker/permutation state");
        const auto startup=cache.load_initialized_object_idle(object.definition);
        if(!startup)throw FormatError("frontend startup attachment pose is unresolved");
        const auto startup_primary=cache.load_frontend_model_primary_animation(
            startup->source_graph,startup->source_index);
        const auto startup_locals=frontend_model_locals(prepared.model,*startup,0.f,{},&startup_primary);
        const auto startup_nodes=pose_model_locals_nodes_world(prepared.model,prepared.root,startup_locals);
        const auto owner_sphere=source_shadow_sphere(prepared.bounds,startup_nodes.front(),prepared.root.scale);
        const auto geometry=cache.load_source_geometry_lod_definition(*weapon);
        const auto scenario_type=cache.load_source_geometry_scenario_type();
        const auto geometry_level=frontend_model_geometry_lod(geometry,
            owner_sphere.center,0,0,scenario_type);
        // 2B0C08 forces the visible biped to5. Its held child separately
        // selects its own literal bank from the copied whole-owner distance.
        auto weapon_model=load_frontend_model_geometry(cache,*weapon,0,
            owner_sphere.center,0,0);
        for(auto &material:weapon_model.mesh.materials) {
          const auto colors=material.object_render_colors;
          // Original held BR and plasma-rifle collectors retain class3,
          // independently of their class2 parent biped.
          if(!material.lods.empty() && !prepare_default_object_material(cache,material,0.f))
            throw FormatError("frontend original weapon shader is unresolved: "+material.name);
          if(colors)apply_object_render_colors(material,*colors);
        }
        SourceDynamicModelShaders weapon_shaders(cache,weapon_model,
            cache.load_initial_model_definition(*weapon).render_model,3,0.f);
        const auto weapon_idle = cache.load_initialized_object_idle(*weapon);
        if (weapon_idle && weapon_idle->frame_count > 1)
          throw FormatError("frontend held weapon requires unresolved animation clock");
        const auto local=frontend_model_initialize_weapon_attachment(prepared.model,
            prepared.root,*startup,startup_primary,attachment);
        const auto weapon_base = unsigned(mesh.materials.size());
        for (auto &material : weapon_model.mesh.materials)
          mesh.materials.push_back(std::move(material));
        prepared.weapon = FrontendHeldWeapon{std::move(weapon_model), weapon_idle,
            attachment, local, weapon_base,std::move(weapon_shaders),geometry,
            scenario_type,std::int8_t(geometry_level.level)};
      }
    }
    prepared.update_pose();
    models.emplace(level.object_names.at(object.name).name, std::move(prepared));
  }
  auto model_tick = [&]() {
    for (auto &[name, model] : models)
      if (model.animation) {
        // Original111650 advances113910 before the animation channel.
        if(model.original_world_callback)source_aim_blend_advance(model.aim);
        frontend_model_clock(model.clock, model.animation->frame_count,
            model.animation->loop_frame, 1.f / 30.f);
        model.select_aim();
        model.pose_dirty = true;
      }
  };
  std::vector<std::pair<unsigned, unsigned>> model_screens;
  auto model_screen_entries = [&](const MenuState &menu) {
    std::vector<std::pair<unsigned, unsigned>> current;
    for (const auto &frame : menu.stack()) current.emplace_back(frame.screen, frame.context);
    std::size_t common = 0;
    while (common < current.size() && common < model_screens.size() &&
           current[common] == model_screens[common]) ++common;
    for (std::size_t i = common; i < current.size(); ++i) {
      const auto screen = content.screens.find(current[i].first);
      if (screen == content.screens.end() || screen->second.panes.empty()) continue;
      // Every biped scene on this disc is in pane0. Creation requests all
      // model widgets even when the signed-slot/species policy hides them.
      for (const auto &scene : screen->second.panes.front().scenes)
        for (const auto &name : scene.objects)
          if (auto at = models.find(name); at != models.end()) {
            auto &model = at->second;
            if (!model.animation || !model.ui_animation) continue;
            frontend_model_preserve_phase(model.clock, model.animation->frame_count,
                model.ui_animation->frame_count, 1.f / 30.f);
            model.animation = model.ui_animation;
            model.primary = model.ui_primary;
            model.pose_dirty = true;
          }
    }
    model_screens = std::move(current);
  };
  std::deque<std::vector<Vertex>> model_view_vertices;
  SDL_Window *window{};
  auto views_for = [&](const UiDraw &draw) {
    model_view_vertices.clear();
    std::vector<ModelView> views;
    int drawable_width{},drawable_height{};
    SDL_Vulkan_GetDrawableSize(window,&drawable_width,&drawable_height);
    for (const auto &spec : draw.scenes) {
      const auto &definition = *spec.definition;
      const auto camera = frontend_model_camera(definition, spec.animation_offset);
      std::optional<std::array<std::int32_t,4>> vista_viewport;
      if(spec.vista_viewport) {
        if(drawable_width<=0 || drawable_height<=0 || drawable_width>32767 || drawable_height>32767)
          throw FormatError("Vista model drawable exceeds its original signed rectangle");
        const auto &r=definition.viewport;
        vista_viewport=frontend_vista_model_viewport({std::int16_t(r.top),std::int16_t(r.left),
            std::int16_t(r.bottom),std::int16_t(r.right)},
            {0,0,std::int16_t(drawable_height),std::int16_t(drawable_width)});
      }
      // Original2C560 uses identical outer/safe short rectangles. Its
      // direct model camera does not use2F800's observer conversion or
      // the title's standard-video safe-area clip scale.
      SourceFrustumProjectionInput projection_input;
      projection_input.projection_angle = *camera.projection_vertical_angle;
      projection_input.outer = projection_input.safe = {
          std::int16_t(240 - definition.viewport.top),
          std::int16_t(320 + definition.viewport.left),
          std::int16_t(240 - definition.viewport.bottom),
          std::int16_t(320 + definition.viewport.right)};
      if(vista_viewport) {
        for(unsigned i=0;i<4;++i) {
          if((*vista_viewport)[i]<-32768 || (*vista_viewport)[i]>32767)
            throw FormatError("Vista model viewport exceeds its original projection rectangle");
          projection_input.outer[i]=projection_input.safe[i]=std::int16_t((*vista_viewport)[i]);
        }
      }
      projection_input.layout_state_present = true;
      projection_input.layout_state_flag = 1;
      projection_input.layout_state_index = 0;
      projection_input.layout_state_mode = 3;
      const auto projection = source_frustum_projection(projection_input);
      const auto render_camera = source_view_projection({camera.position,
          camera.forward, camera.up, .015625f, 1024.f, {},
          projection.projection_program});
      // 2B0DDC prepares collectors using2232A0's enclosing frontend view,
      // before2B0E12 installs this widget's2C560 render camera. Attached
      // weapons copy that owner coverage. 2C560 then forces4BA019 for
      // per-part shader selection in its own viewport.
      const SourceModelShaderView shader_view{
          frontend_model_coverage_camera(),
          source_shader_lod_view(projection_input.outer[0],projection_input.outer[2],cinematic.present,cinematic.byte5),
          native_detail_policy(desktop->display_settings.detail).coverage_multiplier,
          native_detail_policy(desktop->display_settings.detail).effect_scale,true};
      for (const auto &name : definition.objects)
        if (const auto object = models.find(name); object != models.end()) {
          auto &model = object->second;
          if (model.source_biped_pose && spec.appearance && model.appearance != spec.appearance) {
            // Original2B10A3 ->2B0F76 overrides the biped's own four banks;
            // held weapons and other model objects keep their authored colors.
            apply_frontend_appearance(std::span(mesh.materials).subspan(
                model.material_base, model.model.mesh.materials.size()),
                *spec.appearance, content.appearance_palette);
            model.appearance = spec.appearance;
          }
          object->second.update_pose();
          auto &view_vertices=model_view_vertices.emplace_back(model.vertices);
          if(model.shaders && model.source_biped_pose) {
            const auto collector=source_model_shader_collector(model.sphere.center,model.sphere.radius,shader_view);
            std::vector<Vertex> body(view_vertices.begin(),view_vertices.begin()+model.body_vertices);
            model.shaders->select(body,collector,shader_view);
            for(auto &v:body)v.material+=model.material_base;
            if(model.weapon) {
              std::vector<Vertex> held(view_vertices.begin()+model.body_vertices,view_vertices.end());
              model.weapon->shaders->select(held,collector,shader_view);
              for(auto &v:held)v.material+=model.weapon->material_base;
              body.insert(body.end(),held.begin(),held.end());
            }
            view_vertices=std::move(body);
          }
          ModelView view{camera, 320 + definition.viewport.left,
                           240 - definition.viewport.top,
                           definition.viewport.right - definition.viewport.left,
                           definition.viewport.top - definition.viewport.bottom,
                           spec.before_vertex, view_vertices};
          view.source_ui_viewport=vista_viewport;
          if(model.cinematic_lighting) {
            model.lighting[0]={model.material_base,unsigned(model.model.mesh.materials.size()),cinematic_lighting};
            // 3DB00/BAF80 resolves an attached weapon through its parent
            // lighting cache;4BA36 copies that same bank into its draw context.
            if(model.weapon)model.lighting[1]={model.weapon->material_base,
                unsigned(model.weapon->model.mesh.materials.size()),cinematic_lighting};
            view.object_lighting=std::span(model.lighting).first(model.weapon?2:1);
          }
          view.projection_clip_planes = Vec2{.015625f, 1024.f};
          SourceViewShaderConstants source;
          source.world = source_view_shader_constants(render_camera, 1024.f, false);
          source.reflection = source.world;
          view.source_view_shader_constants = source;
          views.push_back(view);
        }
    }
    return views;
  };
  FrontendPainter painter(content, options.maps / "fonts", mesh,
                          options.maps / "ui/frame_edges", ui_assets,
                          options.ui_assets.empty()?std::filesystem::path{}:options.ui_assets/"fonts");
  prepare_frontend_materials(cache, mesh);
  const auto movie_material = unsigned(mesh.materials.size());
  mesh.materials.push_back(
      {"bink/frame",
       Texture{640, 480, 11, std::vector<std::byte>(640u * 480u * 4u), {}}});
  window = desktop->window(options.width, options.height, options.hidden);
  const auto renderer_owner=desktop->make_renderer(mesh,options.validation);
  auto& renderer=*renderer_owner;
  renderer.debug_panel().share_display_settings(desktop->display_settings);
  if(options.debug_panel)renderer.debug_panel().set_visible(true);
  renderer.scene_light_volumes(std::move(light_volumes),cache.shader_function_tables());
  if (content.scene_clear) renderer.scene_clear_color(*content.scene_clear);
  renderer.scene_atmosphere(cache.load_menu_atmosphere(),cache.load_bsp_atmosphere(0));
  MainmenuBspFrameProducer mainmenu_frames(cache,0);
  SourcePortalMaskState mainmenu_portals;
  initialize_mainmenu_portal_state(mainmenu_portals,cache,0);
  BspAtmosphere::CameraLocation mainmenu_location;
  const auto source_mainmenu_frame=[&](const Camera &camera) {
    auto frame=mainmenu_frames.prepare({camera,mainmenu_location,
        mainmenu_portals,cinematic,{}});
    // Original2B790 commits successful location before rendering/presentation.
    mainmenu_location=frame.retained_location;
    return frame;
  };
  const auto scene_filter=cache.load_menu_scene_filter();
  std::cout << "Frontend GPU: " << renderer.device_name() << '\n';
  std::unique_ptr<char, decltype(&SDL_free)> pref(
      SDL_GetPrefPath("Halo2Native", "Halo2"), SDL_free);
  auto &state = initial_entry ? session->create_state(
      options.sweep.empty() && options.capture_actions.empty() && pref
          ? std::filesystem::path(pref.get()) / "menu.cfg"
          : std::filesystem::path{}) : options.entry==FrontendOptions::Entry::session_return
              ? session->state() : session->enter(options.entry,options.postgame_report);
  if(options.entry==FrontendOptions::Entry::session_return) {
    if(!options.session_mainmenu_ready)
      throw FormatError("Native mainmenu return requires the real session factory owner");
    const auto revision=state.revision;
    state.ui_clock(session->ui_clock());
    options.session_mainmenu_ready(state);
    if(state.revision==revision)
      throw FormatError("Native mainmenu return did not request a new screen factory");
  }
  FrontendRuntimeAdapterScope runtime_adapters(state);
  if(!initial_entry) {
    int width{},height{};SDL_GetWindowSize(window,&width,&height);
    state.adopt_controller_settings(desktop->controller_settings);
    state.adopt_pc_settings(desktop->keyboard_settings,desktop->display_settings,
        {unsigned(width),unsigned(height),unsigned(bool(SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN))});
  }
  // OAuth runs outside the render loop; only browser launch uses the UI thread.
  if(!session->discord_login)session->discord_login=std::make_shared<DiscordLogin>([](const std::string &url) {
    return SDL_OpenURL(url.c_str()) == 0;
  });
  auto discord_login=session->discord_login;
  state.set_discord_login(discord_login);
  if(!session->discord_social)session->discord_social=std::make_shared<DiscordSocial>(discord_login,[](const std::string &url) {
    return SDL_OpenURL(url.c_str())==0;
  });
  state.set_discord_social(session->discord_social);
  state.set_online_account(std::make_shared<OnlineAccount>(discord_login, online_account_environment()));
  state.set_online_directory(std::make_shared<OnlineDirectory>(online_directory_environment()));
  state.set_online_matchmaking(std::make_shared<OnlineMatchmaking>(discord_login, online_matchmaking_environment(options.maps)));
  state.set_pc_display_adapter([&](const DisplaySettings &value) {
    const auto error=renderer.validate_display_settings(value);
    if(error.empty()) {
      if(audio_state.engine)if(auto failure=audio_state.engine->speaker_layout(value.speakers);!failure.empty())return failure;
      const bool crt=desktop->display_settings.crt_filter;
      desktop->display_settings=value;
      desktop->display_settings.crt_filter=crt;
    }
    return error;
  });
  state.set_pc_video_adapter([&](unsigned width,unsigned height,bool fullscreen) {
    auto candidate=desktop->display_settings;
    if(!candidate.internal_width){candidate.internal_width=width;candidate.internal_height=height;}
    if(const auto invalid=renderer.validate_display_settings(candidate);!invalid.empty())return invalid;
    const auto error=desktop->apply_video(width,height,fullscreen);
    if(error.empty())renderer.resized();
    return error;
  });
  state.set_pc_video_query([&] {
    int w{},h{};SDL_GetWindowSize(window,&w,&h);
    return std::array<unsigned,3>{unsigned(w),unsigned(h),unsigned((SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0)};
  });
  // The PC executable's windowed table at8680C0 contains these11 sizes.
  std::vector<std::pair<unsigned,unsigned>> modes{{800,600},{1024,768},{1152,720},
    {1280,720},{1280,960},{1280,1024},{1440,900},{1600,1200},{1680,1050},
    {1920,1080},{1920,1200},{2560,1440},{2560,1600},{3440,1440},
    {3840,2160},{5120,1440},{5120,2880},{7680,4320}};
  const auto display=SDL_GetWindowDisplayIndex(window);
  for(int i=0;i<SDL_GetNumDisplayModes(display);++i) {
    SDL_DisplayMode mode{};
    if(SDL_GetDisplayMode(display,i,&mode)==0&&mode.w>0&&mode.h>0)
      modes.emplace_back(unsigned(mode.w),unsigned(mode.h));
  }
  int window_width{},window_height{};SDL_GetWindowSize(window,&window_width,&window_height);
  state.set_pc_video_modes(std::move(modes),unsigned(window_width),unsigned(window_height),
                          (SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN)!=0);
  state.set_multiplayer_map_validator([&](const FrontendLaunch &launch) {
    const auto rules=slayer_rules(launch.variant,true);
    if(rules.primary_light_vehicle) {
      const Cache selected_map(options.maps/launch.map);
      validate_slayer_map_options(selected_map.load_content(0,true),rules);
    }
  });
  if (options.screen != 9)
    state.open(options.screen);
  std::vector<int> capture_wheel_events;
  std::vector<FrontendCaptureAction> capture_pointer_events;
  for (const auto &action : options.capture_actions) {
    using Kind = FrontendCaptureAction::Kind;
    switch (action.kind) {
    case Kind::Accept: state.accept(); break;
    case Kind::Back: state.back(); break;
    case Kind::Select:
      if (action.index >= state.frame().entries.size())
        throw FormatError("menu capture selection is outside current entries");
      state.select(action.index);
      break;
    case Kind::Text: state.text(action.text); break;
    case Kind::Commit: state.commit_text(); break;
    case Kind::WheelUp:
    case Kind::WheelDown: {
      capture_wheel_events.push_back(action.kind==Kind::WheelUp ? 1 : -1);
      break;
    }
    case Kind::Click: capture_pointer_events.push_back(action); break;
    }
    if (state.launch || state.quit)
      throw FormatError("menu capture route must remain in the frontend");
    std::cout << "Menu capture route screen=" << state.frame().screen << '\n';
  }
  model_screen_entries(state);
  Camera fallback;
  if (!level.camera_points.empty()) {
    const auto &c = level.camera_points.front();
    fallback = {c.position, c.rotation.x, c.rotation.y};
  }
  Presentation presentation(level);
  presentation.control(true, fallback);
  // Resolve the original construction predicate at each actual native root
  // birth. SceneObjects retains only that generation's historical bit31 fact;
  // it is not a substitute for current source body flags or child ownership.
  SceneObjects scene(level, 0, {}, [&](std::uint32_t definition) {
    return cache.load_object_light_contact_definition(definition);
  });
  std::uint64_t tick = 0;
  bool bloom_override=false;
  float bloom_threshold=.9f;
  ScriptVm vm(
      level.scripts,
      [&](const std::string &name, const std::vector<ScriptValue> &args) {
        if (name == "camera_control") {
          presentation.control(args.at(0).truth(), fallback);
          return ScriptValue{};
        }
        if (name == "camera_set") {
          presentation.camera_point(std::int16_t(args.at(0).bits),
                                    int(args.at(1).number()));
          return ScriptValue{};
        }
        if (name == "print") {
          std::cout << "Menu script: " << args.at(0).text << '\n';
          return ScriptValue{};
        }
        if(name == "object_set_function_variable") {
          scene.set_function_variable(args.at(0).bits,args.at(1).bits,
              float(args.at(2).number()),float(args.at(3).number()),int(tick));
          return ScriptValue{};
        }
        if(name=="cinematic_start") {
          cinematic.byte5=true;
          renderer.scene_bloom_threshold(.9f);return ScriptValue{};
        }
        if(name=="cinematic_stop") {
          cinematic.byte5=false;return ScriptValue{};
        }
        if(name=="rasterizer_bloom_override") {
          bloom_override=args.at(0).truth();
          renderer.scene_bloom_threshold(bloom_override?bloom_threshold:.9f);return ScriptValue{};
        }
        if(name=="rasterizer_bloom_override_threshold") {
          bloom_threshold=float(args.at(0).number());
          if(bloom_override)renderer.scene_bloom_threshold(bloom_threshold);
          return ScriptValue{};
        }
        if(name=="cinematic_lighting_set_primary_light" || name=="cinematic_lighting_set_secondary_light") {
          const Vec3 color{float(args.at(2).number()),float(args.at(3).number()),float(args.at(4).number())};
          if(name=="cinematic_lighting_set_primary_light")
            frontend_cinematic_primary(cinematic_lighting,float(args.at(0).number()),float(args.at(1).number()),color);
          else frontend_cinematic_secondary(cinematic_lighting,float(args.at(0).number()),float(args.at(1).number()),color);
          return ScriptValue{};
        }
        if(name=="cinematic_lighting_set_ambient_light") {
          frontend_cinematic_ambient(cinematic_lighting,{float(args.at(0).number()),float(args.at(1).number()),float(args.at(2).number())});
          return ScriptValue{};
        }
        if(name=="object_uses_cinematic_lighting") {
          const auto &object=level.objects.at(scene.get(args.at(0).bits).placement);
          if(object.name>=0)if(auto it=models.find(level.object_names.at(object.name).name);it!=models.end())
            it->second.cinematic_lighting=args.at(1).truth();
          return ScriptValue{};
        }
        if(name == "cinematic_lightmap_shadow_enable")
          return ScriptValue{};
        throw FormatError("unsupported menu script service: " + name);
      },
      [&](int name, unsigned) { return scene.resolve(name); });
  vm.start_level();
  vm.tick(0);
  title_effects.advance(0, 1.f/30.f);
  for(auto &binding:animated_objects)
    for(const auto &object:scene.objects())
      if(object.placement==binding.placement && object.handle!=null_datum)
        binding.handle=object.handle;
  std::set<unsigned> animation_faults;
  auto update_animations = [&](double seconds) {
    // tick identifies the next VM update; object functions use the last
    // completed 30 Hz game tick, while shader scrolling samples render time.
    const int object_tick = std::max(0,int(tick)-1);
    std::vector<const AnimatedObjectMaterials*> owners(mesh.materials.size());
    std::vector<const FrontendPreparedModel*> model_owners(mesh.materials.size());
    for(const auto &binding:animated_objects)
      for(std::size_t i=0;i<binding.count;++i) owners[binding.first+i]=&binding;
    for(const auto &[name,model]:models)if(model.source_biped_pose || model.appearance)
      for(std::size_t i=0;i<model.model.mesh.materials.size();++i)
        model_owners[model.material_base+i]=&model;
    for(unsigned i=0;i<mesh.materials.size();++i) {
      auto &material=mesh.materials[i];
      if(material.ui_emblem_pass)continue; // Painter supplies original appearance function inputs.
      if(!material.forward_pass || (!model_owners[i] && material.animation.overlays.empty()) || animation_faults.contains(i)) continue;
      try {
        const auto input=[&](std::uint32_t name) {
          // 2B102F writes the biped's emblem bytes; its model shader uses
          // BCC20/BC970, independently of the UI emblem's22ACE7 inputs.
          if(model_owners[i])
            if(const auto value=frontend_model_material_input(
                model_owners[i]->appearance?&*model_owners[i]->appearance:nullptr,
                model_owners[i]->initial_shield_fraction,name))
              return *value;
          if(!owners[i] || owners[i]->handle==null_datum) return 0.f;
          return evaluate_object_function(owners[i]->functions,*material.animation.tables,
              name,float(object_tick)*(1.f/30.f),[&](std::uint32_t input) {
                return scene.function_variable(owners[i]->handle,input,object_tick);
              }).value;
        };
        if(model_owners[i])animate_frontend_model_material(material,seconds,input);
        else animate_forward_material(material,seconds,input);
      } catch(const FormatError &e) {
        animation_faults.insert(i);
        std::cerr << "Menu shader animation: " << material.name << ": " << e.what() << '\n';
      }
    }
    renderer.update_material_constants(mesh.materials);
  };
  update_animations(0);
  for (const auto &diagnostic : vm.diagnostics())
    std::cerr << "Menu script fault: " << diagnostic << '\n';
  auto &audio=audio_state.engine;
  auto &music_transitions=audio_state.transitions;
  auto &music_voices=audio_state.music_voices;
  auto &menu_music=audio_state.music;
  std::map<std::uint32_t, std::shared_ptr<const PcmSound>> sounds;
  auto pcm = [&](std::uint32_t datum) -> std::shared_ptr<const PcmSound> {
    if (datum == 0xffffffffu)
      return {};
    if (!sounds.contains(datum))
      sounds[datum] =
          std::make_shared<PcmSound>(decode_sound(cache.load_sound(datum)));
    return sounds.at(datum);
  };
  auto start_music = [&] {
    if (!audio || !music_voices.empty())
      return;
    if (!menu_music) {
      if(content.vista) {
        // Keep PC datums and WMA resource bytes in their actual cache owner;
        // the Xbox cache still owns every 3D backdrop asset and UI sound.
        Cache music_cache(vista_map,vista_map.parent_path());
        menu_music=std::make_unique<MenuMusic>(music_cache,content.vista->music,
                                              menu_secondary_seed);
        std::cout << "Vista menu music: " << music_cache.load_looping_sound(content.vista->music).name << '\n';
      } else {
        menu_music = std::make_unique<MenuMusic>(cache, content.music,
                                                menu_secondary_seed);
        std::cout << "Retail menu music: " << cache.load_looping_sound(content.music).name << '\n';
      }
    }
    music_voices = menu_music->play(*audio);
  };
  if (!options.mute) {
    try {
      if(!audio)audio = std::make_unique<AudioEngine>();
      audio->master_volume(float(desktop->display_settings.volume)/10.f);
      if(auto error=audio->speaker_layout(desktop->display_settings.speakers);!error.empty())std::cerr<<"Menu audio layout: "<<error<<'\n';
      for (auto id : content.sounds)
        if (id != 0xffffffffu)
          pcm(id);
    } catch (const std::exception &e) {
      std::cerr << "Menu audio: " << e.what() << '\n';
    }
  }
  if(audio && options.mute)audio->master_volume(0.f);
  auto sound = [&](FrontendUiSound slot) {
    const auto index = static_cast<unsigned>(slot);
    if (audio && index < content.sounds.size()) {
      try {
        if (const auto prepared = pcm(content.sounds[index]))
          audio->play(prepared, frontend_ui_sound_request_scale);
      } catch (const FormatError &e) {
        std::cerr << "Menu sound: " << e.what() << '\n';
      }
    }
  };
  using Controller=std::unique_ptr<SDL_GameController,decltype(&SDL_GameControllerClose)>;
  std::vector<Controller> controllers;
  auto connect = [&]() {
    std::erase_if(controllers,[](const auto& pad){return !SDL_GameControllerGetAttached(pad.get());});
    for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)) {
      const auto id=SDL_JoystickGetDeviceInstanceID(i);
      if(std::none_of(controllers.begin(),controllers.end(),[&](const auto& pad) {
        return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad.get()))==id;
      }))if(auto* pad=SDL_GameControllerOpen(i))controllers.emplace_back(pad,SDL_GameControllerClose);
    }
  };
  connect();
  auto background_for = [&](const Camera &camera) {
    std::vector<Vertex> vertices;
    if (sky_model) {
      vertices = pose_sky(*sky_model, *sky, camera.position);
      for (auto &vertex : vertices) vertex.material += sky_material_base;
    }
    return vertices;
  };
  const auto draw_title_effects = [&](const Camera &camera, const BspFogSample &fog, float fraction) {
    int width{}, height{};
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    if (width <= 0 || height <= 0) { renderer.scene_particles({}); return; }
    const auto projection = menu_full_frame_projection(camera, unsigned(width), unsigned(height));
    const auto depth = menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
    MuzzleEffectView view;
    view.camera = source_view_shader_bank(projection.bank,1024.f,camera.position,
        full_surface_viewport({0,0,640,480},unsigned(width),unsigned(height)),false);
    view.fog = bsp_fog_constants(fog,camera.position,camera.forward);
    const auto &basis = projection.bank.camera_to_world.basis;
    view.orientation = {basis[0].x,basis[0].y,basis[0].z,basis[1].x,basis[1].y,basis[1].z,basis[2].x,basis[2].y,basis[2].z};
    view.position=camera.position; view.forward=camera.forward; view.up=camera.up;
    view.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
    view.width=unsigned(width); view.height=unsigned(height);
    renderer.scene_particles(title_effects.draw(view,fraction,1.f/30.f));
  };
  if (!options.sweep.empty()) {
    std::filesystem::create_directories(options.sweep);
    std::ofstream report(options.sweep / "screens.tsv");
    report << "screen\tpane\tselection\tfile\n";
    unsigned rendered = 0;
    auto capture = [&](MenuState &sample, int pane, const std::string &name) {
      model_screen_entries(sample);
      int w{}, h{};
      SDL_Vulkan_GetDrawableSize(window, &w, &h);
      const auto ui = painter.draw(sample, 5, pane, 0, {unsigned(w), unsigned(h)});
      renderer.scene_color_filter(ui.scene_color_filter
          ?std::optional{scene_filter}:std::nullopt);
      const auto path = options.sweep / (name + ".ppm");
      const auto views = views_for(ui);
      update_animations(0);
      const auto camera = menu_render_camera(presentation.camera(fallback));
      const auto source_frame=source_mainmenu_frame(camera);
      const MainmenuWorldDraw source_world{source_frame,mainmenu_world_source,mainmenu_resident,mesh.materials};
      const auto background = background_for(camera);
      draw_title_effects(camera,source_frame.fog,0);
      if (!renderer.draw(camera, path, {}, false, {}, {},
                         ui.vertices, views, background, ui.backdrops,&source_frame.fog,&source_world,
                         ui.full_surface_bounds))
        throw FormatError("menu sweep draw unavailable");
      report << sample.frame().screen << '\t' << pane << '\t'
             << sample.frame().selected << '\t' << path.filename().string()
             << '\n';
      ++rendered;
    };
    for (const auto &[id, s] : content.screens) {
      MenuState sample(content);
      if (id != 9)
        sample.open(id);
      for (unsigned pane = 0; pane < std::max(1u, unsigned(s.panes.size()));
           ++pane) {
        sample.select(pane);
        capture(sample, int(pane),
                "screen-" + std::to_string(id) + "-pane-" +
                    std::to_string(pane));
      }
    }
    for (const auto &[category, params] : content.categories) {
      MenuState sample(content);
      sample.open(221, category);
      capture(sample, 0, "category-" + std::to_string(category));
    }
    for (const auto &[id, p] : content.parameters) {
      MenuState sample(content);
      sample.open(223, id);
      capture(sample, 0,
              "parameter-" + std::to_string(p.parameter) + "-" +
                  std::to_string(id));
    }
    for (unsigned model = 0; model < 2; ++model) {
      MenuState sample(content);
      sample.open(47);
      sample.open(52);
      sample.select(model);
      sample.accept();
      capture(sample, 0, model ? "appearance-elite" : "appearance-spartan");
    }
    for (unsigned page = 0; page < 4; ++page) {
      MenuState sample(content);
      sample.open(167);
      sample.select(41 + page);
      sample.accept();
      capture(sample, 0, "keyboard-page-" + std::to_string(page));
    }
    std::cout << "Rendered " << rendered
              << " retail menu fixtures; GPU execution only, retail visual "
                 "comparison pending\n";
    return {};
  }
  using Clock = std::chrono::steady_clock;
  auto previous = Clock::now(), entered = previous;
  std::uint32_t ui_clock_ms = session->ui_clock();
  const bool fixed_capture_clock = options.frames && initial_entry;
  FrontendIdleTimer idle(previous);
  auto timing_screen = [&] {
    std::vector<unsigned> screens;
    screens.reserve(state.stack().size());
    for (const auto &frame : state.stack())
      screens.push_back(frame.screen);
    return resolved_frontend_channel5_screen(screens)
        .value_or(state.frame().screen);
  };
  idle.screen(timing_screen(), previous);
  auto &music_enabled=audio_state.enabled;
  std::unique_ptr<Movie> movie;
  AudioEngine::VoiceId movie_voice{};
  struct MovieVoiceScope {
    AudioEngine *engine;
    AudioEngine::VoiceId &voice;
    ~MovieVoiceScope() { if(engine && voice)engine->stop(voice,true); }
  } movie_voice_scope{audio.get(),movie_voice};
  auto movie_started = previous;
  auto begin_movie = [&](const std::string &name) {
    const auto path = options.maps.parent_path() / "bink" / (name + "_60.bik");
    movie = std::make_unique<Movie>(path);
    if (movie->image().width != 640 || movie->image().height != 480)
      throw FormatError("retail movie dimensions differ from frontend canvas");
    for (auto voice : music_voices)
      if (audio)
        audio->stop(voice, true);
    music_voices.clear();
    renderer.update_image(movie_material, movie->image());
    if (audio) {
      auto track = std::make_shared<PcmSound>(decode_movie_audio(path));
      movie_voice = audio->play(track);
      std::cout << "Movie audio: " << track->name
                << " frames=" << track->frames() << '\n';
    }
    movie_started = Clock::now();
    previous = movie_started;
    std::cout << "Retail movie: " << name << " frames=" << movie->frame_count()
              << " fps=" << movie->frame_rate() << '\n';
  };
  auto end_movie = [&] {
    if (audio && movie_voice)
      audio->stop(movie_voice, true);
    movie_voice = 0;
    movie.reset();
    entered = previous = Clock::now();
    idle.activity(entered);
    idle.await_presentation();
  };
  if (!options.movie.empty())
    begin_movie(options.movie);
  else if (initial_entry && !options.skip_intro && options.screen == 9)
    begin_movie("intro");
  // Existing source transition state is retained across renderer/resource
  // changes. The ordinary update below decides from the actual current menu;
  // entering a renderer is not a request to restart title music.
  unsigned screen = state.frame().screen, frames = 0;
  tick = 1;
  while (tick <= options.scene_tick) {
    vm.tick(tick++);
    title_effects.advance(tick-1,1.f/30.f);
    presentation.tick();
    model_tick();
  }
  float accumulator = 0, axis_timer = 0;
  int axis_direction = 0;
  bool running = true, text_input = false;
  struct PointerCursor {
    SDL_Cursor *previous = SDL_GetCursor();
    SDL_Cursor *hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    ~PointerCursor() {
      SDL_SetCursor(previous ? previous : SDL_GetDefaultCursor());
      if (hand) SDL_FreeCursor(hand);
    }
    void update(bool clickable) const {
      auto *cursor = clickable && hand ? hand : previous;
      if (cursor && SDL_GetCursor() != cursor) SDL_SetCursor(cursor);
    }
  } pointer_cursor;
  std::optional<unsigned> painted_revision;
  std::array<unsigned, 2> painted_viewport{};
  std::uint64_t pointer_press_instance{};
  std::uint8_t pointer_press_button{};
  const auto drawable_size = [&] {
    int w{}, h{};
    SDL_Vulkan_GetDrawableSize(window, &w, &h);
    return std::array<unsigned, 2>{unsigned(std::max(1, w)), unsigned(std::max(1, h))};
  };
  auto pointer_target = [&](int window_x, int window_y) {
    int w{}, h{};
    SDL_GetWindowSize(window, &w, &h);
    const float scale = std::min(float(w) / 640, float(h) / 480);
    if (scale <= 0) return std::optional<FrontendPointerTarget>{};
    return painter.pointer_hit((window_x - (w - 640 * scale) * .5f) / scale,
                               (window_y - (h - 480 * scale) * .5f) / scale);
  };
  auto refresh_pointer_layout = [&] {
    const auto viewport = drawable_size();
    if (painted_revision == state.revision && painted_viewport == viewport) return;
    const auto clock = options.capture_ui_ms
        ? *options.capture_ui_ms + frames * options.capture_ui_step_ms
        : fixed_capture_clock ? 5000u : ui_clock_ms;
    const float elapsed = options.capture_ui_ms ? float(clock) / 1000.f
        : fixed_capture_clock ? 5.f : std::chrono::duration<float>(Clock::now() - entered).count();
    painter.draw(state, elapsed, -1, clock, viewport);
    painted_revision = state.revision;
    painted_viewport = viewport;
  };
  desktop->renderer_ready(renderer_owner);
  previous=Clock::now();
  while (running && !state.quit && !state.launch &&
         (!options.frames || frames < options.frames)) {
    const auto now = Clock::now();
    ui_clock_ms = session->ui_clock();
    state.ui_clock(ui_clock_ms);
    state.update_online_login();
    state.voice_push_to_talk(SDL_GetKeyboardFocus()==window && SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_V]);
    state.update_online_directory();
    state.update_online_matchmaking();
    if(audio)audio->master_volume(options.mute?0.f:float(desktop->display_settings.volume)/10.f);
    const float dt =
        std::min(.1f, std::chrono::duration<float>(now - previous).count());
    previous = now;
    // Submit diagnostic input only after startup has initialized its input
    // modes, and attach it to the actual SDL window like a platform event.
    for (const auto direction:capture_wheel_events) {
      SDL_Event wheel{};
      wheel.type=SDL_MOUSEWHEEL;
      wheel.wheel.windowID=SDL_GetWindowID(window);
      wheel.wheel.y=direction;
      wheel.wheel.preciseY=float(direction);
      wheel.wheel.direction=SDL_MOUSEWHEEL_NORMAL;
      if(SDL_PushEvent(&wheel)!=1) throw FormatError("menu capture wheel event rejected");
    }
    capture_wheel_events.clear();
    for (const auto &action : capture_pointer_events) {
      SDL_Event click{};
      click.type = SDL_MOUSEBUTTONDOWN;
      click.button.windowID = SDL_GetWindowID(window);
      click.button.button = SDL_BUTTON_LEFT;
      click.button.state = SDL_PRESSED;
      click.button.x = action.x;
      click.button.y = action.y;
      if (SDL_PushEvent(&click) != 1) throw FormatError("menu capture click event rejected");
    }
    capture_pointer_events.clear();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      const bool debug_event = renderer.debug_panel().process_event(event);
      // SDL2-compat can retain only preciseY when translating wheel events.
      // A platform notch and a precise host event use the same menu step.
      const float wheel_vertical=event.type==SDL_MOUSEWHEEL
          ? (event.wheel.y ? float(event.wheel.y) : event.wheel.preciseY) : 0.f;
      if (event.type == SDL_QUIT) {
        running = false;
        break;
      }
      if (event.type == SDL_WINDOWEVENT &&
          (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
           event.window.event == SDL_WINDOWEVENT_RESIZED))
        renderer.resized();
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
        idle.activity(now);
      if (event.type == SDL_CONTROLLERDEVICEADDED)
        connect();
      if(event.type==SDL_CONTROLLERDEVICEREMOVED)connect();
      const bool input = event.type == SDL_KEYDOWN ||
                         event.type == SDL_CONTROLLERBUTTONDOWN ||
                         event.type == SDL_MOUSEBUTTONDOWN ||
                         wheel_vertical != 0 ||
                         (event.type == SDL_MOUSEMOTION &&
                          (event.motion.xrel || event.motion.yrel)) ||
                         (event.type == SDL_CONTROLLERAXISMOTION &&
                          std::abs(event.caxis.value) > 18000);
      if (input)
        idle.activity(now);
      // Opening/closing F1 and captured input never skip a movie or activate a
      // menu item. Window/device housekeeping above still receives its events.
      if (debug_event)
        continue;
      if (movie) {
        const bool button =
            event.type == SDL_CONTROLLERBUTTONDOWN &&
            event.cbutton.button < SDL_CONTROLLER_BUTTON_DPAD_UP;
        const bool trigger =
            event.type == SDL_CONTROLLERAXISMOTION &&
            (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
             event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) &&
            event.caxis.value > 18000;
        if ((event.type == SDL_KEYDOWN && !event.key.repeat) ||
            event.type == SDL_MOUSEBUTTONDOWN || button || trigger)
          end_movie();
        // The skip press belongs to playback and never activates a menu item.
        continue;
      }
      if(event.type==SDL_MOUSEBUTTONDOWN) {
        // SDL reports every press of a double click separately. Keep that
        // click sequence with its original screen: Done can replace PCR16
        // with lobby14 between presses, whose Back prompt means Leave.
        if(event.button.clicks>1 && pointer_press_button==event.button.button &&
            pointer_press_instance && pointer_press_instance!=state.frame().instance)
          continue;
        if(event.button.clicks<=1 || pointer_press_button!=event.button.button || !pointer_press_instance) {
          pointer_press_instance=state.frame().instance;
          pointer_press_button=event.button.button;
        }
      }
      if(const auto report=state.frame().vista_pcr;report && report->voice_control &&
          event.type==SDL_KEYDOWN && !event.key.repeat) {
        const auto key=event.key.keysym.sym;
        if(key==SDLK_F6 || key==SDLK_F7 || key==SDLK_m) {
          report->voice_control(key==SDLK_F6?0u:key==SDLK_F7?1u:2u,state.frame().context,state.frame().selected);
          continue;
        }
      }
      if(state.capturing_binding()) {
        if(event.type==SDL_KEYDOWN && !event.key.repeat) {
          if(event.key.keysym.sym==SDLK_ESCAPE)state.back();
          else if(event.key.keysym.sym==SDLK_BACKSPACE)state.bind_pc_input(0);
          else state.bind_pc_input(event.key.keysym.scancode!=SDL_SCANCODE_UNKNOWN?
              int(event.key.keysym.scancode):int(SDL_GetScancodeFromKey(event.key.keysym.sym)));
        } else if(event.type==SDL_MOUSEBUTTONDOWN) {
          refresh_pointer_layout();
          const auto target = pointer_target(event.button.x, event.button.y);
          if (event.button.button == SDL_BUTTON_LEFT && target &&
              (target->action == FrontendPointerAction::Back ||
               target->action == FrontendPointerAction::ClearBinding)) {
            state.activate_pointer(*target);
            sound(target->action == FrontendPointerAction::Back ? FrontendUiSound::Back : FrontendUiSound::Forward);
            if (!options.capture_actions.empty())
              std::cout << "Menu click action=" << unsigned(target->action)
                        << " screen=" << state.frame().screen
                        << " binding_capture=" << state.capturing_binding() << '\n';
          } else state.bind_pc_input(pc_mouse_button(event.button.button));
        }
        else if(event.type==SDL_MOUSEWHEEL &&
                (event.wheel.x||event.wheel.preciseX||wheel_vertical))
          state.bind_pc_input(pc_wheel_up);
        else if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)
          state.back();
        continue;
      }
      auto accept = [&] {
        state.accept();
        model_screen_entries(state);
        sound(FrontendUiSound::Forward);
      };
      auto back = [&] {
        state.back();
        model_screen_entries(state);
        sound(FrontendUiSound::Back);
      };
      if (wheel_vertical != 0) {
        const auto previous = state.frame().selected;
        const bool up=(wheel_vertical > 0) !=
                      (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED);
        state.mouse_wheel(up ? 1 : -1);
        if (state.frame().selected != previous)
          sound(FrontendUiSound::Cursor);
        if (!options.capture_actions.empty())
          std::cout << "Menu wheel screen=" << state.frame().screen
                    << " selected=" << state.frame().selected << '\n';
      }
      if(event.type==SDL_KEYDOWN && state.joining_controller() && state.frame().screen!=167)continue;
      if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        const auto key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE)
          back();
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
          if (state.frame().screen == 167)
            state.commit_text();
          else
            accept();
        } else if (key == SDLK_UP)
          state.navigate(0, -1);
        else if (key == SDLK_DOWN)
          state.navigate(0, 1);
        else if (key == SDLK_LEFT)
          state.navigate(-1, 0);
        else if (key == SDLK_RIGHT)
          state.navigate(1, 0);
        else if (key == SDLK_BACKSPACE)
          state.erase_text();
        else if (state.frame().screen != 167 && key == SDLK_x) {
          state.auxiliary();
          sound(FrontendUiSound::Forward);
        } else if (state.frame().screen != 167 && key == SDLK_y) {
          state.players();
          sound(FrontendUiSound::Forward);
        } else if (key == SDLK_SPACE && state.frame().screen == 9)
          accept();
      }
      if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_UP ||
                                        event.key.keysym.sym == SDLK_DOWN ||
                                        event.key.keysym.sym == SDLK_LEFT ||
                                        event.key.keysym.sym == SDLK_RIGHT))
        sound(FrontendUiSound::Cursor);
      if (event.type == SDL_TEXTINPUT)
        state.text(event.text.text);
      if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        const auto before=state.frame().instance;
        const bool join=event.cbutton.button==SDL_CONTROLLER_BUTTON_A || event.cbutton.button==SDL_CONTROLLER_BUTTON_START;
        const auto slot=state.controller_slot(event.cbutton.which,join);
        if(!slot || before!=state.frame().instance)continue;
        if(state.joining_controller() && slot!=state.joining_controller())continue;
        if(*slot!=0 && !state.joining_controller()) {
          if((state.frame().screen==30 || state.frame().screen==14) &&
              event.cbutton.button==SDL_CONTROLLER_BUTTON_B)state.leave_local_controller(*slot);
          continue;
        }
        switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
          accept();
          break;
        case SDL_CONTROLLER_BUTTON_START:
          if (state.frame().screen == 167)
            state.commit_text();
          else
            accept();
          break;
        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_BACK:
          back();
          break;
        case SDL_CONTROLLER_BUTTON_X:
          state.auxiliary();
          sound(FrontendUiSound::Forward);
          break;
        case SDL_CONTROLLER_BUTTON_Y:
          state.players();
          sound(FrontendUiSound::Forward);
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
          state.navigate(0, -1);
          sound(FrontendUiSound::Cursor);
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
          state.navigate(0, 1);
          sound(FrontendUiSound::Cursor);
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
          state.navigate(-1, 0);
          sound(FrontendUiSound::Cursor);
          break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
          state.navigate(1, 0);
          sound(FrontendUiSound::Cursor);
          break;
        default:
          break;
        }
      }
      if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN) {
        refresh_pointer_layout();
        const auto target = pointer_target(
            event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x,
            event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y);
        pointer_cursor.update(target.has_value());
        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_RIGHT)
          back();
        else if (target) {
          if (event.type == SDL_MOUSEMOTION && target->action == FrontendPointerAction::Entry) {
            const auto previous = state.frame().selected;
            state.select(target->index);
            if (state.frame().selected != previous) sound(FrontendUiSound::Cursor);
          } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT &&
                     state.activate_pointer(*target)) {
            model_screen_entries(state);
            sound(target->action == FrontendPointerAction::Back
                      ? FrontendUiSound::Back : FrontendUiSound::Forward);
            if (!options.capture_actions.empty())
              std::cout << "Menu click action=" << unsigned(target->action)
                        << " screen=" << state.frame().screen
                        << " binding_capture=" << state.capturing_binding() << '\n';
          }
        } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                   event.button.button == SDL_BUTTON_LEFT && state.frame().screen == 9)
          accept();
      }
    }
    if(options.session_update && options.session_update(state,ui_clock_ms,sound))break;
    const bool debug_capture = renderer.debug_panel().captures_game_input();
    if (debug_capture) {
      // The optional workbench is active use, including frames without events.
      // Keep both the music-silence and attract timers from expiring under it.
      idle.activity(now);
      axis_direction = 0;
      axis_timer = 0;
    }
    SDL_GameController* navigation_controller{};
    for(const auto& pad:controllers) {
      const auto slot=state.controller_slot(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad.get())),false);
      if(slot && *slot==state.joining_controller().value_or(0))navigation_controller=pad.get();
    }
    if (navigation_controller && !movie && !debug_capture) {
      const int value = SDL_GameControllerGetAxis(navigation_controller,SDL_CONTROLLER_AXIS_LEFTY);
      const int direction = value < -18000 ? -1 : value > 18000 ? 1 : 0;
      axis_timer -= dt;
      if (direction && (direction != axis_direction || axis_timer <= 0)) {
        idle.activity(now);
        state.navigate(0, direction);
        sound(FrontendUiSound::Cursor);
        axis_timer = direction != axis_direction ? .35f : .12f;
      }
      axis_direction = direction;
    }
    model_screen_entries(state);
    if(const auto report=state.frame().vista_pcr;report && report->voice_update)
      report->voice_update((SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0 && !renderer.debug_panel().visible());
    if (screen != state.frame().screen) {
      screen = state.frame().screen;
      entered = now;
      idle.screen(timing_screen(), now);
      std::cout << "Menu screen " << screen << '\n';
    }
    const bool menu_text_input = screen == 167 && !debug_capture;
    if (!debug_capture && menu_text_input != text_input) {
      text_input = menu_text_input;
      if (text_input)
        SDL_StartTextInput();
      else
        SDL_StopTextInput();
    }
    if (!options.freeze_scene) accumulator += dt;
    while (!movie && accumulator >= 1.f / 30) {
      vm.tick(tick++);
      title_effects.advance(tick-1,1.f/30.f);
      presentation.tick();
      model_tick();
      accumulator -= 1.f / 30;
    }
    if (movie)
      accumulator = 0;
    if (!movie) {
      // Retail 235BFA retains the previous music preference in other menus.
      music_enabled = frontend_music_enabled(screen, music_enabled);
      const bool wanted = frontend_music_wanted(
          music_enabled, idle, now, content.vista?content.vista->music_silence_ms:content.music_silence_ms);
      const auto transition = audio ? music_transitions.request(wanted, ui_clock_ms)
                                    : std::optional<bool>{};
      if (transition) {
        if (*transition) start_music();
        else {
          if (menu_music) menu_music->stop(*audio,music_voices);
          else for (auto voice : music_voices) audio->stop(voice);
          music_voices.clear();
          std::cout << "Menu music fade requested screen=" << screen
                    << " idle_ms=" << idle.elapsed(now).count() << '\n';
        }
      }
    }
    if (!movie && !debug_capture && idle.due(now)) {
      std::cout << "Attract idle_ms=" << idle.elapsed(now).count() << '\n';
      begin_movie("attract");
    }
    if (options.resize_test && frames == options.frames / 2) {
      SDL_SetWindowSize(window, 1000, 600);
      renderer.resized();
    }
    UiDraw ui;
    if (movie) {
      const double seconds =
          options.movie_frame
              ? double(options.movie_frame) / movie->frame_rate()
              : std::max(
                    0.0,
                    std::chrono::duration<double>(now - movie_started).count());
      if (movie->advance(seconds))
        renderer.update_image(movie_material, movie->image());
      if (movie->finished(seconds))
        end_movie();
    }
    if (movie) {
      const Vertex a{{0, 0, 1}, {1, 1, 1}, {0, 0}, movie_material},
          b{{640, 0, 1}, {1, 1, 1}, {1, 0}, movie_material},
          c{{0, 480, 1}, {1, 1, 1}, {0, 1}, movie_material},
          d{{640, 480, 1}, {1, 1, 1}, {1, 1}, movie_material};
      ui.vertices = {a, b, c, b, d, c};
    } else {
      const auto capture_clock = options.capture_ui_ms
          ? std::optional<std::uint32_t>{*options.capture_ui_ms +
              frames * options.capture_ui_step_ms} : std::nullopt;
      const float elapsed = capture_clock ? float(*capture_clock) / 1000.f : fixed_capture_clock ? 5.f :
          std::chrono::duration<float>(Clock::now() - entered).count();
      painted_viewport = drawable_size();
      ui = painter.draw(state, elapsed, -1, capture_clock.value_or(fixed_capture_clock ? 5000u : ui_clock_ms),
                        painted_viewport);
      painted_revision = state.revision;
    }
    // A newly opened menu or an animated prompt can move under a stationary
    // pointer. Keep the platform hand cursor in sync without changing focus.
    int pointer_x{}, pointer_y{};
    SDL_GetMouseState(&pointer_x, &pointer_y);
    const auto hovered = movie || debug_capture ? std::nullopt : pointer_target(pointer_x, pointer_y);
    pointer_cursor.update(hovered && (!state.capturing_binding() ||
                                    hovered->action == FrontendPointerAction::Back ||
                                    hovered->action == FrontendPointerAction::ClearBinding));
    const bool capture =
        !options.screenshot.empty() && frames + 1 == options.frames;
    renderer.scene_color_filter(!movie && ui.scene_color_filter
        ? std::optional{scene_filter} : std::nullopt);
    const auto views = views_for(ui);
    const auto camera = menu_render_camera(presentation.camera(fallback, accumulator * 30.f));
    const auto source_frame=movie?std::nullopt:
        std::optional(source_mainmenu_frame(camera));
    const auto source_world=source_frame?std::optional(MainmenuWorldDraw{
        *source_frame,mainmenu_world_source,mainmenu_resident,mesh.materials}):std::nullopt;
    if(!movie) update_animations((double(tick-1)+accumulator*30.f)/30.0);
    const auto background = movie ? std::vector<Vertex>{} : background_for(camera);
    if (source_frame) draw_title_effects(camera,source_frame->fog,accumulator*30.f);
    else renderer.scene_particles({});
    DebugGameObservation debug_observation;
    debug_observation.mode = movie ? "Movie" : "Frontend";
    debug_observation.tick = tick;
    renderer.debug_panel().observe(std::move(debug_observation));
    if (renderer.draw(camera,
                      capture ? options.screenshot : std::filesystem::path{},
                      {}, false, {}, {}, ui.vertices, views, background, ui.backdrops,
                      source_frame?&source_frame->fog:nullptr,
                      source_world?&*source_world:nullptr, ui.full_surface_bounds)) {
      ++frames;
      if (!movie && idle.presented(Clock::now()))
        std::cout << "Menu ready screen=" << screen << " idle_ms=0\n";
    } else
      SDL_Delay(10);
  }
  if (audio)
    std::cout << "Menu audio mixed_frames=" << audio->mixed_frames()
              << " peak=" << audio->peak() << '\n';
  std::cout << "Rendered " << frames << " frontend frames\n";
  const auto effect_diagnostics=title_effects.diagnostics();
  std::cout << "Title particles: " << effect_diagnostics.particles << " in "
            << effect_diagnostics.systems << " systems\n";
  for(const auto &boundary:effect_diagnostics.unavailable)
    std::cerr << "Title effect boundary: " << boundary << '\n';
  if (state.launch)
    state.launch->map = options.maps / state.launch->map;
  desktop->keyboard_settings=state.pc_keyboard_settings();
  desktop->controller_settings=state.controller_settings();
  return state.launch;
}
} // namespace halo2
