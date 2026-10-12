#include "halo2/world_effects.hpp"
#include "halo2/source_effect_event.hpp"
#include "halo2/particle_presentation.hpp"
#include "halo2/material_effects.hpp"
#include "halo2/math.hpp"
#include "halo2/particle_collision.hpp"
#include "halo2/particle_movement.hpp"
#include "halo2/particle_view.hpp"
#include "halo2/original_data.hpp"
#include "halo2/projectile_material_response.hpp"
#include "halo2/simulation.hpp"
#include "halo2/effect_light_lifetime.hpp"
#include "halo2/effect_part_control.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <deque>
#include <map>
#include <set>
#include <utility>

namespace halo2 {
namespace {
constexpr auto none = 0xffffffffu;
struct Packet {
  const Cache &cache;
  Tag tag;
  Bytes root;
  static Tag load(const Cache &c, std::uint32_t d) {
    const auto t = c.query_tag({.handle=TagHandle{d}});
    if (t && !t->size)
      for (const auto &backing : c.select_tags({.group=t->type(), .address=t->address, .minimum_size=1})) {
          (void)c.tag_payload(backing.handle());
          auto alias=*t;alias.size=backing.size;
          return alias;
        }
    (void)c.tag_payload({d});
    return *t;
  }
  Packet(const Cache &c, std::uint32_t d)
      : cache(c), tag(load(c, d)),
        root(c.read_tag_bytes(tag.address,tag.size)) {}
  std::span<const std::byte> read(std::uint32_t address,
                                  std::size_t extent) const {
    if (TagQuery{.contains=TagAddressRange{address,extent}}.matches(tag))
      return cache.read_tag_bytes(address,extent);
    for (const auto &t : cache.select_tags({.contains=TagAddressRange{address,extent}, .require_address=true})) {
        (void)cache.tag_payload(t.handle());
        return cache.read_tag_bytes(address,extent);
      }
    throw FormatError("World effect pointer has no resident metadata owner");
  }
  Bytes block(const Bytes &b, std::size_t at, std::size_t stride) const {
    const auto descriptor=read_tag_block_descriptor(b,at);
    const auto extent=descriptor.extent(stride,65536);
    return Bytes(descriptor.count ? read(descriptor.address,extent)
                       : std::span<const std::byte>{});
  }
};
Vec3 unit(Vec3 v) {
  const float n = length(v);
  if (!(n > 0))
    throw FormatError("World effect has no direction producer");
  return v / n;
}
AuthoredBasis basis(Vec3 forward, Vec3 up) {
  forward = unit(forward);
  if (length(cross(up, forward)) < .0001f)
    up = std::abs(forward.z) < .9f ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
  const auto left = unit(cross(up, forward));
  return {forward, left, cross(forward, left)};
}
AuthoredBasis location(const std::string &name, const WorldEffectFrame &f) {
  if (name == "normal")
    return basis(f.normal, f.up);
  if (name == "forward")
    return basis(f.incoming, f.up);
  if (name == "backward")
    return basis(f.incoming * -1.f, f.up);
  if (name == "reflection")
    return basis(f.incoming - f.normal * (2.f * dot(f.incoming, f.normal)),
                 f.up);
  if (name == "up")
    return basis({0, 0, 1}, f.forward);
  if (name == "gravity")
    return basis({0, 0, -1}, f.forward);
  if (name == "root" || name == "impact" || name == "marker")
    return basis(f.forward, f.up);
  throw FormatError("World effect location has no native producer: " + name);
}
} // namespace
struct NativeWorldEffects::Impl {
  const Cache &cache;
  CollisionWorld collision;
  std::uint32_t seed;
  SourceEffectRandom random_streams;
  std::uint64_t tick{}, identity{};
  std::vector<std::int16_t> parents;
  std::map<std::uint32_t, std::uint32_t> projectiles;
  std::set<std::string> unavailable;
  std::vector<WorldEffectSound> sounds;
  std::map<std::uint32_t,EffectLightDefinition> light_definitions;
  struct LightWorld {
    BspAtmosphere locations;
    ObjectLightSpatialBsp memberships;
    TerrainCollision terrain;
    SourceEffectLightDatumPool pool;
    SourceEffectLightSpatialPool spatial;
    struct Delivery {std::uint32_t effect_owner=none;std::uint64_t tick{};};
    std::array<Delivery,SourceEffectLightDatumPool::capacity> deliveries;
    explicit LightWorld(const Cache& c)
        :locations(c.load_bsp_atmosphere(0)),memberships(c.load_object_light_spatial_bsp(0)),
         terrain(c.load_raw_terrain_collision(0)),spatial(memberships.clusters.size()) {
      if(terrain.bsps.size()!=1)throw FormatError("World effect light requires actual first BSP collision bank");
    }
  };
  std::unique_ptr<LightWorld> light_world;
  SourceEffectLightQueryState light_queries;
  std::array<SourceShadowSelectionCache,4> light_selection;
  SourceEffectLightLifetimeWorld light_inputs(std::uint64_t t) const {
    return {light_world->locations,light_world->memberships,light_world->terrain.bsps[0],
      cache.original_data()->fade_curve,std::uint32_t(t),light_queries.epoch,30,0,true};
  }
  struct DecalDefinition {
    struct Sprite {
      float u0{}, u1 = 1, v0{}, v1 = 1, pivot_x{}, pivot_y{};
    };
    unsigned material{}, layer{}, overlap{};
    std::uint16_t flags{};
    std::uint32_t next = none;
    float low{}, high{}, rejection{}, life_low{}, life_high{}, decay_low{},
        decay_high{};
    float maximum_sprite_extent{};
    std::vector<Sprite> sprites;
    Texture texture;
  };
  std::map<std::uint32_t, DecalDefinition> decal_definitions;
  std::map<std::uint32_t, std::string> decal_boundaries;
  struct Decal {
    std::uint32_t definition{};
    Vec3 center;
    float radius{}, lifetime{}, decay{};
    std::uint64_t tick{};
    std::vector<Vertex> vertices;
  };
  std::vector<Decal> decal_bank;
  struct SystemDefinition {
    ParticleSystemRenderDefinition render;
    ParticleSpriteDefinition sprite;
    std::shared_ptr<const ParticleMaterial> material;
    std::vector<ParticleMovementDefinition> movement;
    std::string location;
    std::uint32_t collision_sound = none;
    std::uint32_t death_sound = none;
    std::uint32_t death_effect = none;
    std::int16_t sort_bias{};
  };
  struct Part {
    std::uint32_t decal{};
    std::string location;
    float radius_low{}, radius_high{};
    std::uint32_t sound = none;
    std::uint32_t light = none;
    std::uint32_t light_scale_a{},light_scale_b{};
  };
  struct EventDefinition : SourceEffectEventDefinition {
    std::vector<SystemDefinition> systems;
    std::vector<Part> parts;
  };
  struct Definition {
    std::uint32_t flags{};
    std::int16_t loop_start=-1;
    std::vector<EventDefinition> events;
    std::vector<SourceEffectEventDefinition> clocks;
  };
  std::map<std::uint32_t, Definition> definitions;
  struct Emitter {
    ParticleEmitterState state;
    std::map<std::uint32_t, ParticleSimulationState> particles;
    std::map<std::uint32_t, ParticleSimulationState> previous_particles;
  };
  struct System {
    const SystemDefinition *definition{};
    std::uint64_t identity{};
    float random_a{}, random_b{}, fraction{};
    std::uint8_t random_location{};
    ParticleRenderValueCache values;
    std::vector<Emitter> emitters;
  };
  struct Event {
    const EventDefinition *definition{};
    float duration{}, emitted{};
    bool begun{};
    std::vector<System> systems;
  };
  struct Effect {
    std::uint64_t identity{}, tick{};
    std::uint32_t owner{};
    WorldEffectFrame frame;
    const Definition *definition{};
    bool scheduled{};
    std::vector<Event> events;
    bool attached{},finished{};
    SourceEffectEventState clock;
  };
  std::deque<Effect> effects;
  Impl(const Cache &c, Mesh &mesh, const CollisionWorld &w, std::uint32_t s,
       std::optional<SourceEffectRandom> streams={})
      : cache(c), collision(std::vector<CollisionTriangle>(
                      w.triangles().begin(), w.triangles().end())),
        seed(s),random_streams{streams?streams->primary:seed,streams?streams->auxiliary:seed} {
    const auto globals =
        c.query_tag({.group=tag_groups::globals});
    if (!globals)
      throw FormatError("World effects require global materials");
    Packet p(c, globals->datum);
    const auto rows = p.block(p.root, 0x150, 0xb4);
    for (unsigned i = 0; i < p.root.u32(0x150); ++i)
      parents.push_back(std::int16_t(rows.u16(i * 0xb4 + 8)));
    for (const auto &t : c.select_tags({.group=tag_groups::decal, .require_address=true, .minimum_size=1}))
        try {
          Packet d(c, t.datum);
          DecalDefinition out;
          out.flags = d.root.u16(0);
          out.layer = d.root.u16(4);
          out.overlap = d.root.u16(6);
          out.next = d.root.u32(0xc);
          out.low = d.root.f32(0x10);
          out.high = d.root.f32(0x14);
          out.rejection = d.root.f32(0x18);
          out.life_low = d.root.f32(0x34);
          out.life_high = d.root.f32(0x38);
          out.decay_low = d.root.f32(0x3c);
          out.decay_high = d.root.f32(0x40);
          if (out.layer != 2 && out.layer != 3)
            throw FormatError(
                "decal layer requires original light/add/max pass: " + t.name);
          if (out.flags & 8)
            throw FormatError(
                "Directional decal tangent requires source impact frame: " +
                t.name);
          out.maximum_sprite_extent = d.root.f32(0xa4);
          Packet bitmap(c, d.root.u32(0x8c));
          if (bitmap.root.u16(0) == 3) {
            const auto sequences = bitmap.block(bitmap.root, 0x3c, 0x3c);
            for (unsigned s = 0; s < bitmap.root.u32(0x3c); ++s) {
              if (!sequences.u32(s * 0x3c + 0x34))
                throw FormatError("Decal sprite sequence is empty: " + t.name);
              const auto sprites =
                  bitmap.block(sequences, s * 0x3c + 0x34, 0x20);
              if (sprites.u16(0) != 0)
                throw FormatError(
                    "Decal sprite needs a different bitmap image: " + t.name);
              out.sprites.push_back({sprites.f32(8), sprites.f32(12),
                                     sprites.f32(16), sprites.f32(20),
                                     sprites.f32(24), sprites.f32(28)});
            }
            if (!(out.maximum_sprite_extent > 0))
              throw FormatError("Decal sprite has no authored extent: " +
                                t.name);
          }
          out.texture = c.load_bitmap(d.root.u32(0x8c), 0);
          Material m;
          m.name = t.name;
          m.diffuse = out.texture;
          m.forward_pass = true;
          m.forward_vertex = 11;
          m.forward_diffuse = {out.layer == 2 ? .5f : 1.f, 0, 0, 0};
          m.forward_blend = out.layer == 2 ? 6 : 9;
          m.forward_raster.cull = 0;
          m.forward_raster.depth_write = 0;
          out.material = unsigned(mesh.materials.size());
          mesh.materials.push_back(std::move(m));
          decal_definitions.emplace(t.datum, std::move(out));
        } catch (const FormatError &e) {
          decal_boundaries.emplace(t.datum, e.what());
        }
    // The same authored global grenade/projectile references used by the
    // gameplay owner. Resolve resources before renderer creation; this does
    // not instantiate an effect or advance either random stream.
    const auto grenades=p.block(p.root,0x100,0x2c);
    for(unsigned n=0;n<p.root.u32(0x100);++n) {
      Packet projectile(c,grenades.u32(n*0x2c+0x28));
      if(projectile.tag.group!="proj")throw FormatError("Grenade preload is not a projectile");
      const auto effect=projectile.root.u32(0xfc);
      if(effect!=none)definition(effect);
    }
  }
  std::uint32_t random() {
    auto &auxiliary=random_streams.auxiliary;
    auxiliary = auxiliary * 1664525u + 1013904223u;
    return auxiliary;
  }
  static float fraction(std::uint32_t &stream) {
    stream=stream*1664525u+1013904223u;
    return float(double(stream>>16)*double(1.f/65535.f));
  }
  static float range(std::uint32_t &stream,float low,float high) {
    return low==high?low:(high-low)*fraction(stream)+low;
  }
  float fraction() {
    return float(double(random() >> 16) * double(1.f / 65535.f));
  }
  float range(float low, float high) {
    return low == high ? low : (high - low) * fraction() + low;
  }
  Definition &definition(std::uint32_t datum) {
    if (auto i = definitions.find(datum); i != definitions.end())
      return i->second;
    Packet p(cache, datum);
    if (p.tag.group != "effe")
      throw FormatError("World visual request is not an effect");
    auto locs = p.block(p.root, 0xc, 4), events = p.block(p.root, 0x14, 0x38);
    Definition d;
    d.flags=p.root.u32(0);
    d.loop_start=std::int16_t(p.root.u16(4));
    if(d.loop_start < -1 || d.loop_start >= int(p.root.u32(0x14)))
      throw FormatError("World effect loop escapes its authored event bank");
    const auto marker = [&](std::int16_t index) {
      if (index < 0 || unsigned(index) >= p.root.u32(0xc))
        throw FormatError("World effect location escapes original bank");
      return cache.string_id(locs.u32(unsigned(index) * 4));
    };
    for (unsigned n = 0; n < p.root.u32(0x14); ++n) {
      const auto q = n * 0x38;
      auto &event = d.events.emplace_back();
      event.skip = events.f32(q + 4);
      event.delay_low = events.f32(q + 8);
      event.delay_high = events.f32(q + 12);
      event.duration_low = events.f32(q + 16);
      event.duration_high = events.f32(q + 20);
      //179190 reads skip+4; event flags do not replace that probability.
      d.clocks.push_back(event);
      const auto parts = p.block(events, q + 0x18, 0x38);
      for (unsigned j = 0; j < events.u32(q + 0x18); ++j) {
        const auto a = j * 0x38;
        if (parts.u32(a + 8) == 0x736e6421u && parts.u32(a + 0x10) != none &&
            parts.u16(a) != 2 && !(parts.u16(a + 6) & 8)) {
          Part part;
          part.decal = none;
          part.sound = parts.u32(a + 0x10);
          part.location = marker(std::int16_t(parts.u16(a + 4)));
          event.parts.push_back(std::move(part));
        }
        if (parts.u32(a + 8) == 0x6c696768u && parts.u32(a + 0x10) != none &&
            parts.u16(a) != 2 && !(parts.u16(a + 6) & 8)) {
          try {
            const auto tag=parts.u32(a+0x10);
            light_definitions.try_emplace(tag,load_effect_light(cache,tag));
            Part part;part.decal=none;part.light=tag;
            part.light_scale_a=parts.u32(a+0x30);part.light_scale_b=parts.u32(a+0x34);
            part.location=marker(std::int16_t(parts.u16(a+4)));
            event.parts.push_back(std::move(part));
            unavailable.insert("Dynamic object light receiver/model submission and lens-flare owner remain unavailable");
          } catch(const FormatError &e) { unavailable.insert(e.what()); }
        }
        if (parts.u32(a + 8) != 0x64656361u || parts.u32(a + 0x10) == none ||
            parts.u16(a) == 2 || (parts.u16(a + 6) & 8))
          continue;
        if (parts.u32(a + 0x30) || parts.u32(a + 0x34)) {
          unavailable.insert("Decal radius needs authored effect A/B scaling");
          continue;
        }
        event.parts.push_back({parts.u32(a + 0x10),
                               marker(std::int16_t(parts.u16(a + 4))),
                               parts.f32(a + 0x28), parts.f32(a + 0x2c)});
      }
      const auto systems = p.block(events, q + 0x30, 0x38);
      for (unsigned j = 0; j < events.u32(q + 0x30); ++j)
        try {
          const auto a = j * 0x38;
          if (systems.u16(a + 0xe) == 2 || systems.u16(a + 0x12) == 1 ||
              (systems.u16(a + 0x16) & 0x28))
            continue;
          SystemDefinition out;
          out.render =
              cache.load_particle_system_render_definition(datum, n, j);
          out.sort_bias = std::int16_t(systems.u16(a + 0x14));
          if (out.render.coordinates != 0)
            throw FormatError(
                "World effect coordinates require a parent frame owner");
          out.location = marker(std::int16_t(systems.u16(a + 8)));
          out.sprite = cache.load_particle_sprite(out.render.particle_datum);
          Packet particle(cache, out.render.particle_datum);
          if (particle.root.u32(0x78) != none) {
            const auto group=particle.root.u32(0x74);
            if(group==0x736e6421u)
              out.death_sound=particle.root.u32(0x78);
            else if(group==0x65666665u)
              out.death_effect=particle.root.u32(0x78);
            else
              throw FormatError("Particle death requires a non-effect dispatcher");
          }
          out.material = std::make_shared<const ParticleMaterial>(
              cache.load_particle_material(out.render.particle_datum, 15, 0,
                                           0));
          if (out.material->vertex.classification != 7 &&
              out.material->vertex.classification != 8)
            throw FormatError(
                "World effect requires particle vertex class" +
                std::to_string(out.material->vertex.classification));
          const auto emitters = p.block(systems, a + 0x30, 0xb8);
          for (unsigned k = 0; k < out.render.emitters.size(); ++k) {
            const auto move = emitters.u32(k * 0xb8 + 4);
            out.movement.push_back(
                move == none ? ParticleMovementDefinition{}
                             : load_particle_movement_definition(cache, move));
            for (const auto &controller : out.movement.back().controllers)
              if (controller.type != 0 && controller.type != 1)
                throw FormatError(
                    "World effect requires collider/swarm/wind controller" +
                    std::to_string(controller.type));
            if (std::any_of(out.movement.back().controllers.begin(),
                            out.movement.back().controllers.end(),
                            [](const auto &v) { return v.type == 1; })) {
              Packet particle(cache, out.render.particle_datum);
              if (particle.root.u32(0x70) != none) {
                // 2BF380 dispatches an independent sound leaf2490D0 after
                // bounce arithmetic, then sets the same first-contact bit.
                // Missing audio must not discard the particle's visual owner.
                if (particle.root.u32(0x6c) != 0x736e6421u)
                  throw FormatError("Particle collider requires its authored "
                                    "child-effect dispatcher");
                out.collision_sound = particle.root.u32(0x70);
              }
            }
            const auto shape = out.render.emitters[k].birth.shape;
            if (shape > 5)
              throw FormatError("World effect requires particle shape" +
                                std::to_string(shape));
          }
          event.systems.push_back(std::move(out));
        } catch (const FormatError &e) {
          unavailable.insert(cache.string_id(locs.u32(0)) + ": " + e.what());
        }
    }
    auto &retained = definitions.emplace(datum, std::move(d)).first->second;
    // Load child definitions once, before live particles can reach their
    // death boundary. Map ownership also closes recursive definition chains.
    for (const auto &event : retained.events)
      for (const auto &system : event.systems)
        if (system.death_effect != none) definition(system.death_effect);
    return retained;
  }
  ParticleValueSources sources(const System &s, std::uint64_t t,
                               float seconds, const WorldEffectFrame &frame) const {
    ParticleValueSources out;
    out.system = ParticleSystemValueState{s.fraction, s.random_a, s.random_b,
                                          std::array<float, 2>{frame.scale_a, frame.scale_b}};
    out.location = ParticleLocationValueState{s.random_location, 1};
    out.game_tick = std::int32_t(t);
    out.game_rate = seconds;
    return out;
  }
  void decal(std::uint32_t datum, const WorldEffectFrame &f, float modifier,
             std::uint64_t at, std::set<std::uint32_t> &chain) {
    if (!f.terrain || datum == none)
      return;
    if (!chain.insert(datum).second)
      throw FormatError("Cyclic authored decal chain");
    auto i = decal_definitions.find(datum);
    if (i == decal_definitions.end()) {
      const auto gap = decal_boundaries.find(datum);
      unavailable.insert(gap == decal_boundaries.end()
                             ? "Decal material unavailable: " +
                                   std::to_string(datum)
                             : gap->second);
      return;
    }
    const auto &d = i->second;
    const auto normal = unit(f.normal);
    unsigned overlaps{};
    for (const auto &existing : decal_bank)
      if (existing.definition == datum &&
          length(existing.center - f.position) <=
              existing.radius + d.high * (d.rejection == 0 ? 1 : d.rejection))
        ++overlaps;
    if (d.overlap && overlaps >= d.overlap)
      return;
    // Original180980 builds a tangent using the smallest normal component,
    // then rotates both normalized axes by its visual-bank random angle.
    Vec3 tangent;
    if (std::abs(normal.x) > std::abs(normal.y) ||
        std::abs(normal.x) > std::abs(normal.z))
      tangent = std::abs(normal.z) < std::abs(normal.y)
                    ? Vec3{normal.y, -normal.x, 0}
                    : Vec3{-normal.z, 0, normal.x};
    else
      tangent = {0, normal.z, -normal.y};
    tangent = unit(tangent);
    const auto second = unit(cross(normal, tangent));
    const float angle = fraction() * 6.2831855f;
    const auto x = second * std::cos(angle) - tangent * std::sin(angle),
               y = second * std::sin(angle) + tangent * std::cos(angle);
    // 180980 selects sequence zero's first sprite after choosing the sequence
    // with the original high-word multiply. Even a fixed radius consumes RNG.
    DecalDefinition::Sprite sprite;
    unsigned sequence{};
    if (!d.sprites.empty()) {
      sequence = ((random() >> 16) * unsigned(d.sprites.size() - 1)) >> 16;
      sprite = d.sprites[sequence];
    }
    const float radius = ((d.high - d.low) * fraction() + d.low) *
                         (modifier == 0 ? 1 : modifier);
    if (!(radius > 0))
      return;
    float x0 = -radius, x1 = radius, y0 = -radius, y1 = radius;
    if (!d.sprites.empty()) {
      const float scale = radius / d.maximum_sprite_extent;
      const float width = float(d.texture.width) * scale;
      const float aspect =
          (d.flags & 0x100)
              ? ((sprite.u1 - sprite.u0) / (sprite.v1 - sprite.v0)) *
                    (float(d.texture.height) / float(d.texture.width))
              : 1.f;
      const float height = float(d.texture.height) * scale * aspect;
      x0 = -sprite.pivot_x * width;
      x1 = ((sprite.u1 - sprite.pivot_x) - sprite.u0) * width;
      y0 = -sprite.pivot_y * height;
      y1 = ((sprite.v1 - sprite.pivot_y) - sprite.v0) * height;
    } else if (d.flags & 0x100) {
      y0 *= float(d.texture.height) / d.texture.width;
      y1 = -y0;
    }
    if (!(x1 > x0 && y1 > y0))
      throw FormatError("Decal sprite has empty source geometry");
    Decal out{datum,
              f.position,
              radius,
              range(d.life_low, d.life_high),
              range(d.decay_low, d.decay_high),
              at,
              {}};
    struct Point {
      Vec3 world;
      float u, v;
    };
    for (const auto &t : collision.triangles()) {
      const auto tn = unit(cross(t.b - t.a, t.c - t.a));
      if (dot(tn, normal) < .95f)
        continue;
      // Native triangle projection service. The source BSP polygon traversal
      // remains a distinct owner; clipping never spans an empty triangle.
      if (std::abs(dot(t.a - f.position, normal)) > radius)
        continue;
      std::vector<Point> polygon;
      for (auto v : {t.a, t.b, t.c}) {
        const auto delta = v - f.position;
        polygon.push_back({v, dot(delta, x), dot(delta, y)});
      }
      for (unsigned plane = 0; plane < 4 && !polygon.empty(); ++plane) {
        std::vector<Point> clipped;
        const auto distance = [&](const Point &p) {
          return plane == 0   ? p.u - x0
                 : plane == 1 ? x1 - p.u
                 : plane == 2 ? p.v - y0
                              : y1 - p.v;
        };
        for (unsigned a = 0; a < polygon.size(); ++a) {
          const auto &p = polygon[a];
          const auto &q = polygon[(a + 1) % polygon.size()];
          const auto dp = distance(p), dq = distance(q);
          if (dp >= 0)
            clipped.push_back(p);
          if ((dp >= 0) != (dq >= 0)) {
            const float blend = dp / (dp - dq);
            clipped.push_back({p.world + (q.world - p.world) * blend,
                               p.u + (q.u - p.u) * blend,
                               p.v + (q.v - p.v) * blend});
          }
        }
        polygon = std::move(clipped);
      }
      for (unsigned a = 1; a + 1 < polygon.size(); ++a)
        for (unsigned index : {0u, a, a + 1}) {
          const auto &p = polygon[index];
          Vertex v{};
          v.position = p.world + normal * .0001f;
          v.normal = normal;
          v.uv = {
              sprite.u0 + ((p.u - x0) / (x1 - x0)) * (sprite.u1 - sprite.u0),
              sprite.v0 + ((p.v - y0) / (y1 - y0)) * (sprite.v1 - sprite.v0)};
          v.color = {1, 1, 1};
          v.material = d.material;
          out.vertices.push_back(v);
        }
    }
    if (!out.vertices.empty()) {
      if (decal_bank.size() >= 1024)
        decal_bank.erase(decal_bank.begin());
      decal_bank.push_back(std::move(out));
    }
    decal(d.next, f, modifier, at, chain);
  }
  void begin_parts(Effect &e, Event &event, std::uint64_t t) {
      for (const auto &part : event.definition->parts) {
        if(part.light!=none){
          const auto &d=light_definitions.at(part.light);const auto frame=location(part.location,e.frame);
          // 17A8A0's world branch calls C0350 with object/node NONE. C0350
          // also always initializes attached-owner+4C to NONE; an effect's
          // transport projectile is not an attached-light owner.
          if(!light_world)light_world=std::make_unique<LightWorld>(cache);
          const auto id=source_standalone_effect_light_create(light_world->pool,light_world->spatial,
            d,part.light,source_effect_part_control(part.light_scale_a,part.light_scale_b,e.frame.scale_a,e.frame.scale_b),
            e.frame.position,frame.forward,frame.up,light_inputs(t));
          if(id!=none)light_world->deliveries[id&0xffffu]={e.owner,t};
          continue;
        }
        if (part.sound != none) {
          sounds.push_back({part.sound, e.owner, t, e.frame.position, 1});
          continue;
        }
        std::set<std::uint32_t> chain;
        decal(part.decal, e.frame, range(part.radius_low, part.radius_high), t,
              chain);
      }
  }
  void emit(Effect &e, Event &event, float interval, float seconds,
            std::uint64_t t) {
    if (!event.begun) {
      event.begun = true;
      for (const auto &d : event.definition->systems) {
        System s;
        s.definition = &d;
        s.identity = ++identity;
        s.random_a = fraction();
        s.random_b = fraction();
        s.random_location = std::uint8_t(((random() >> 16) * 255u) >> 16);
        s.emitters.resize(d.render.emitters.size());
        event.systems.push_back(std::move(s));
      }
    }
    for (auto &s : event.systems) {
      const auto &d = *s.definition;
      s.fraction =
          event.duration > 0 ? (event.emitted + interval) / event.duration : 1;
      auto owners = sources(s, t, seconds, e.frame);
      s.values.values.evaluated = 0;
      complete_particle_values(s.values.values, 0x107f0, owners);
      const ParticleEmitterFrame frame{1, location(d.location, e.frame),
                                       e.frame.position};
      for (unsigned j = 0; j < s.emitters.size(); ++j) {
        auto &emitter = s.emitters[j];
        update_particle_emitter(
            d.render, j, emitter.state, 0x109, interval, &frame,
            s.values.values.values, 0xff808080, *cache.shader_function_tables(),
            [&](const ParticleEmissionRequest &request,
                ParticleEmitterState &state) {
              if (emitter.particles.size() >= 4096)
                return;
              const auto datum = std::uint32_t(++identity);
              auto &p = emitter.particles[datum];
              create_emitter_particle(
                  d.render, j, state, p,
                  {datum, identity, s.fraction, request, {}}, s.values, owners,
                  random_streams.auxiliary, *cache.original_data(),
                  *cache.shader_function_tables());
            });
      }
    }
    event.emitted += interval;
  }
};
NativeWorldEffects::NativeWorldEffects(const Cache &c, Mesh &m,
                                       const CollisionWorld &w,
                                       std::uint32_t seed)
    : impl_(std::make_unique<Impl>(c, m, w, seed)) {}
NativeWorldEffects::NativeWorldEffects(const Cache &c,Mesh &m,const CollisionWorld &w,
                                      SourceEffectRandom streams)
    :impl_(std::make_unique<Impl>(c,m,w,0,streams)) {}
NativeWorldEffects::~NativeWorldEffects() = default;
std::size_t NativeWorldEffects::attach_scenery(const LevelContent &content) {
  std::size_t count{};
  for (unsigned index = 0; index < content.objects.size(); ++index) {
    const auto &object = content.objects[index];
    if (object.definition == none || (object.flags & 1)) continue;
    const auto tag = impl_->cache.query_tag({.handle=TagHandle{object.definition}});
    if (!tag || tag->group != "scen") continue;
    Packet p(impl_->cache, object.definition);
    const auto attachments = p.block(p.root, 0x94, 0x18);
    for (unsigned n = 0; n < p.root.u32(0x94); ++n) {
      const auto at = n * 0x18;
      if (attachments.u32(at) != 0x65666665u || attachments.u32(at + 4) == none) continue;
      if (attachments.u32(at + 8) || attachments.u32(at + 0x10) || attachments.u32(at + 0x14))
        throw FormatError("Static scenery effect needs a posed marker/function owner");
      const auto datum = attachments.u32(at + 4);
      const auto &definition = impl_->definition(datum);
      // The title's attached flak has one constant, delay-free event. Keep
      // more general retained event graphs outside this bounded owner.
      if (definition.events.size() != 1 || definition.events[0].skip != 0 ||
          definition.events[0].delay_low != 0 || definition.events[0].delay_high != 0 ||
          definition.events[0].duration_low != definition.events[0].duration_high ||
          !(definition.events[0].duration_low > 0) || !definition.events[0].parts.empty())
        throw FormatError("Attached scenery effect requires additional event lifecycle");
      const auto rotation = authored_basis(object.rotation);
      spawn(datum, index, 0, {object.position, rotation.up, rotation.forward,
          rotation.forward, rotation.up, false});
      impl_->effects.back().attached = true;
      ++count;
    }
  }
  return count;
}
void NativeWorldEffects::spawn(std::uint32_t datum, std::uint32_t owner,
                               std::uint64_t tick,
                               const WorldEffectFrame &frame) {
  if (datum == none)
    return;
  auto &i = *impl_;
  auto &definition = i.definition(datum);
  Impl::Effect e{0, tick, owner, frame, &definition, false, {}};
  if (i.effects.size() >= 256)
    throw FormatError("World effect pool exhausted");
  i.effects.push_back(std::move(e));
}
void NativeWorldEffects::impact(const BulletImpact &hit) {
  auto &i = *impl_;
  if (i.projectiles.empty())
    for (const auto &[datum, weapon] : i.cache.load_content(0, true).weapons)
      i.projectiles.emplace(datum, weapon.projectile);
  Packet p(i.cache, i.projectiles.at(hit.weapon));
  const auto rows = p.block(p.root, 0x19c, 0x58);
  const auto raw = rows.slice(0, rows.size());
  const auto response =
      source_projectile_material_response(raw, i.parents, hit.material);
  if (!response) {
    i.unavailable.insert("Projectile effect response has no authored row");
    return;
  }
  const auto type = std::int16_t(rows.u16(*response * 0x58 + 2));
  int index = type == 0   ? 15
              : type == 1 ? 16
              : type == 2 ? 17
              : type == 3 ? 18
              : type == 4 ? 19
                          : 20;
  const auto foot = p.root.u32(0x58);
  const WorldEffectFrame frame{hit.position,        hit.normal, hit.incoming,
                               hit.incoming * -1.f, {0, 0, 1},  hit.terrain};
  // 1018B0 supplies default materialNONE and sweetener variant0. The
  // primary, sweetener and secondary entries are separate ordered requests.
  if (foot != none)
    for (const auto sound : source_material_sound_effects(
             i.cache, foot, -1, hit.material, index, 0))
      if (sound != none)
        i.sounds.push_back({sound, hit.projectile, hit.tick, hit.position, 1,
                            WorldEffectSoundKind::Material});
  if (foot != none)
    for (const auto datum : source_material_visual_effects(
             i.cache, foot, -1, hit.material, index, 0))
      if (datum != none)
        spawn(datum, hit.projectile, hit.tick, frame);
  if (hit.material != -1 && p.root.u32(0x144) != none)
    spawn(p.root.u32(0x144), hit.projectile, hit.tick, frame);
}
void NativeWorldEffects::advance(std::uint64_t tick, float seconds) {
  auto &i = *impl_;
  if (tick + 1 < i.tick)
    throw FormatError("World visual clock reversed");
  // A render frame can deliver several simulation ticks at once. Preserve
  // event chronology and consume visual RNG only at its owning game tick.
  std::stable_sort(
      i.effects.begin(), i.effects.end(),
      [](const auto &a, const auto &b) { return a.tick < b.tick; });
  for (auto t = i.tick; t <= tick; ++t) {
    for (std::size_t effect_index = 0; effect_index < i.effects.size(); ++effect_index) {
      auto &e = i.effects[effect_index];
      if (t >= e.tick) {
        if (!e.scheduled) {
          e.scheduled = true;
          e.identity = ++i.identity;
          for (const auto &d : e.definition->events) e.events.push_back({&d});
          e.clock.flags=e.attached?2u:0u;
          const SourceEffectEventServices select_services{[](){}};
          //177310/178120 select event zero directly, before179190 handles
          //later skips. Native allocation/birth remains a separate owner.
          if(!source_effect_event_select(e.clock,e.definition->clocks,0,
                i.random_streams.event(e.definition->flags),select_services))e.finished=true;
        }
        for (auto &event : e.events) {
          for (auto &s : event.systems)
            for (unsigned j = 0; j < s.emitters.size(); ++j) {
              auto &emitter = s.emitters[j];
              emitter.previous_particles=emitter.particles;
              auto *link = &emitter.state.first_particle;
              while (*link != none) {
                auto p = emitter.particles.find(*link);
                if (p == emitter.particles.end())
                  throw FormatError("World particle chain lost retained owner");
                auto &v = p->second;
                v.age = v.inverse_lifetime * seconds + v.age;
                if (v.age > 1 || (v.flags & 1)) {
                  if(s.definition->death_sound!=none) {
                    // Retail248340 ->246E60 ->247000 dispatches snd! before
                    // unlink.247000 copies particle+1C and uses246EB0's
                    // velocity gain (the same TU1.1 248F80 numeric owner).
                    // Existing native audio owns downstream spatial/admission.
                    i.sounds.push_back({s.definition->death_sound,e.owner,t,
                        v.position,source_particle_collision_child_gain(v.velocity),
                        WorldEffectSoundKind::Death});
                  }
                  if (s.definition->death_effect != none) {
                    // Retail248340 ->246E60/246FA0 dispatches before unlink,
                    // at the particle's last simulated position. 246F10 uses
                    // immutable4687B0=(0,0,1),4687BC=(0,0,-1) as fallbacks.
                    const auto direction = length(v.velocity) >= .0001f
                        ? unit(v.velocity) : Vec3{0,0,-1};
                    const float scale = source_particle_collision_child_gain(v.velocity);
                    spawn(s.definition->death_effect, e.owner, t,
                        {v.position, {0,0,1}, direction, {0,0,-1}, {0,0,1},
                         false, scale, scale});
                  }
                  *link = v.next_particle;
                  --emitter.state.particle_count;
                  emitter.particles.erase(p);
                } else {
                  v.position = v.position + v.velocity * seconds;
                  v.phase = v.rotation_rate * seconds + v.phase;
                  link = &v.next_particle;
                }
              }
              auto owners = i.sources(s, t, seconds, e.frame);
              const auto &movement = s.definition->movement[j];
              const ParticleMovementPool pool{
                  [&](auto d) -> ParticleSimulationState & {
                    return emitter.particles.at(d);
                  },
                  std::size_t(0xffffffffu)};
              for (const auto &controller : movement.controllers) {
                if (controller.type == 0)
                  apply_particle_physics_controller(
                      controller, emitter.state.first_particle, seconds,
                      retail_gravity, 1, owners,
                      *i.cache.shader_function_tables(), pool);
                else
                  apply_particle_collider_controller(
                      controller, movement.flags, s.definition->sprite.flags,
                      emitter.state.first_particle, seconds, owners,
                      *i.cache.shader_function_tables(), pool,
                      [&](auto, Vec3 origin, Vec3 displacement)
                          -> std::optional<ParticleCollisionHit> {
                        const float distance = length(displacement);
                        if (!(distance > 0))
                          return {};
                        const auto hit =
                            i.collision.raycast(origin, displacement, distance);
                        if (!hit)
                          return {};
                        return ParticleCollisionHit{hit->distance / distance,
                                                    hit->position, hit->normal,
                                                    hit->source.instance < 0};
                      },
                      [&](const auto &particle, const auto &) {
                        if (s.definition->collision_sound != none)
                          i.sounds.push_back(
                              {s.definition->collision_sound, e.owner, t,
                               particle.position,
                               source_particle_collision_child_gain(
                                   particle.velocity),
                               WorldEffectSoundKind::Collision});
                      });
              }
            }
        }
        if(!e.finished) {
          const SourceEffectEventServices services{
            [&] {
              auto &event=e.events.at(e.clock.event);
              event.emitted=e.clock.elapsed;
              if(e.clock.flags&1u)event.duration=e.clock.target;
              for(auto &system:event.systems)system.fraction=0;
            },
            [&] { i.begin_parts(e,e.events.at(e.clock.event),t); },
            [&](float before,float interval) {
              auto &event=e.events.at(e.clock.event);
              event.duration=e.clock.target;event.emitted=before;
              i.emit(e,event,interval,seconds,t);
            },
            [] {},
            [&] {
              e.finished=true;
              //177610 keeps a stopped effect alive while its linked systems
              //drain. Native particles below own the corresponding lifetime.
              e.clock.flags|=0x44u;
              return false;
            }};
          source_effect_event_advance(e.clock,e.definition->clocks,e.definition->loop_start,
              seconds,i.random_streams.event(e.definition->flags),services);
        }
      }
    }
    std::erase_if(i.effects, [](const auto &e) {
      return e.finished && e.scheduled &&
             std::all_of(e.events.begin(), e.events.end(), [](const auto &v) {
               return std::all_of(v.systems.begin(), v.systems.end(),
                                  [](const auto &s) {
                                    return std::all_of(
                                        s.emitters.begin(), s.emitters.end(),
                                        [](const auto &e) {
                                          return e.particles.empty();
                                        });
                                  });
             });
    });
    std::erase_if(i.decal_bank, [&](const auto &d) {
      return d.lifetime > 0 && float(t - d.tick) * seconds >= d.lifetime;
    });
    if(i.light_world)source_standalone_effect_lights_advance(i.light_world->pool,i.light_world->spatial,
      [&](auto definition)->const EffectLightDefinition& {return i.light_definitions.at(definition);},i.light_inputs(t));
  }
  i.tick = tick + 1;
}
std::vector<WorldEffectLight> NativeWorldEffects::lights() const {
  const auto& i=*impl_;std::vector<WorldEffectLight> out;if(!i.light_world)return out;
  for(const auto id:i.light_world->pool.identities()) {
    const auto light=source_standalone_effect_light(i.light_world->pool,id);
    const auto& d=i.light_definitions.at(light.definition);const auto& delivery=i.light_world->deliveries[id&0xffffu];
    out.push_back({light.definition,none,d.lens,d.flags,delivery.tick,light.registration.position,
      light.forward,light.up,d.radius,d.specular_radius,light.state,delivery.effect_owner,light.registration});
  }
  return out;
}
std::vector<std::uint32_t> NativeWorldEffects::light_definitions() const {
  std::vector<std::uint32_t> result;
  for(const auto& [identity,definition]:impl_->light_definitions)result.push_back(identity);
  return result;
}
SourceEffectLightView NativeWorldEffects::light_view(Vec3 camera_position,
    const SourceEffectLightCollectionContext& collection,
    const SourceEffectLightCandidateContext& candidates,std::int32_t frame,bool cinematic) {
  auto& i=*impl_;
  SourceEffectLightView view;
  if(i.light_world) {
    const SourceEffectLightDefinitions definition=[&](auto id)->const EffectLightDefinition& {
      return i.light_definitions.at(id);
    };
    source_effect_lights_update_view(i.light_world->pool,definition,camera_position);
    view=source_effect_lights_query(i.light_world->pool,i.light_world->spatial,definition,
        i.light_queries,SourceEffectLightQueryKind::Main,collection);
  } else view.epoch=++i.light_queries.epoch; // Original empty-pool query still advances4E0308.
  source_effect_lights_select(view,i.light_selection,
      std::bit_cast<std::int32_t>(collection.local_player),candidates,frame,cinematic);
  return view;
}
std::vector<WorldEffectLightReceiver> NativeWorldEffects::light_receivers(
    const SourceEffectLightView& lights,const SourceBspAdmissionBsp& geometry,
    std::span<const std::uint8_t> containing_fog,const SourceBspAdmission* main_view) {
  auto& i=*impl_;std::vector<WorldEffectLightReceiver> out;
  if(!i.light_world)return out;
  const SourceEffectLightReceiverWorld world{i.light_world->memberships,geometry,
      i.light_world->locations,0};
  const SourceEffectLightDefinitions definition=[&](auto id)->const EffectLightDefinition& {
    return i.light_definitions.at(id);
  };
  for(const auto& selected:lights.selected) {
    if(!(selected.requested_fade>std::bit_cast<float>(0x3d8f5c29u)))continue; //31C80: .07
    const auto found=std::find_if(lights.records.begin(),lights.records.end(),
        [&](const auto& r){return r.runtime.identity==selected.identity;});
    if(found==lights.records.end() || !found->definition)
      throw FormatError("Selected world light lacks its current source record/definition");
    const auto& r=found->runtime;
    try {
      const auto shape=source_effect_light_sphere(*found->definition,r.position,r.forward,r.up,r.state.size);
      auto receiver=source_effect_light_receiver_view(world,r.cluster,r.spatial_center,
          r.spatial_radius,shape,containing_fog,main_view);
      const auto frustums=receiver.visibility.sphere_bank();
      SourceEffectLightCollectionContext collection{};
      collection.visibility.frustums=&frustums;collection.visibility.query_mode=receiver.query_mode;
      collection.visibility.sphere_center=r.spatial_center;collection.visibility.sphere_radius=r.spatial_radius;
      collection.flags=receiver.flags;collection.pass_mode=receiver.pass_mode;
      collection.containing_cluster=receiver.location.cluster;collection.local_player=0xffffffffu;
      const auto query=source_effect_lights_query(i.light_world->pool,i.light_world->spatial,
          definition,i.light_queries,SourceEffectLightQueryKind::Light,collection);
      out.push_back({selected.identity,query.epoch,selected.requested_fade,std::move(receiver)});
    }catch(const FormatError& error){i.unavailable.insert(std::string("Light receiver: ")+error.what());}
  }
  return out;
}
std::vector<WorldEffectSound> NativeWorldEffects::take_sounds() {
  auto &sounds = impl_->sounds;
  std::stable_sort(
      sounds.begin(), sounds.end(),
      [](const auto &a, const auto &b) { return a.tick < b.tick; });
  return std::exchange(sounds, {});
}
std::vector<ParticleDraw> NativeWorldEffects::draw(const MuzzleEffectView &view,
                                                   float fraction,
                                                   float seconds) const {
  auto &i = *impl_;
  std::vector<ParticleDraw> result;
  for (const auto &e : i.effects)
    for (const auto &event : e.events)
      for (const auto &s : event.systems) {
        const auto &a = *s.definition;
        Vec3 low{}, high{};
        bool bounded{};
        for (const auto &emitter : s.emitters)
          for (const auto &[identity, current] : emitter.particles) {
            const auto old=emitter.previous_particles.find(identity);
            const auto p=particle_presentation(old==emitter.previous_particles.end()?nullptr:&old->second,current,fraction);
            if (!bounded) { low = high = p.position; bounded = true; }
            else {
              low = {std::min(low.x,p.position.x),std::min(low.y,p.position.y),std::min(low.z,p.position.z)};
              high = {std::max(high.x,p.position.x),std::max(high.y,p.position.y),std::max(high.z,p.position.z)};
            }
          }
        if (!bounded) continue;
        // Source175EA0 biases the current location bounds center along the
        // view forward by authored+14 * .05 before1E4C0's depth collector.
        const auto center = (low + high) * .5f + view.forward * (float(a.sort_bias) * .05f);
        const auto delta = center - view.position;
        const float sort_depth = 0.f - ((view.forward.z*delta.z + view.forward.y*delta.y) + view.forward.x*delta.x);
        ParticleRenderState state;
        state.system_identity = s.identity;
        state.location_identity = s.identity;
        state.sources = i.sources(s, i.tick, seconds, e.frame);
        state.sources.camera_position = view.position;
        state.sources.camera_forward = view.forward;
        state.sources.camera_up = view.up;
        for (const auto &emitter : s.emitters) {
          auto &out = state.emitters.emplace_back();
          out.rotation = emitter.state.rotation;
          out.position = emitter.state.position;
          for (auto d = emitter.state.first_particle; d != none;) {
            const auto identity = d;
            const auto &current = emitter.particles.at(d);
            d = current.next_particle;
            const auto old=emitter.previous_particles.find(identity);
            const auto p=particle_presentation(old==emitter.previous_particles.end()?nullptr:&old->second,current,fraction);
            const float age=p.age;
            if (age > 1)
              continue;
            out.particles.push_back(
                {identity,
                 {age, p.system_fraction, p.phase,p.position,p.random},
                 p.velocity,
                 p.seed_color});
          }
        }
        ParticleRenderValueCache values;
        for_each_particle_render_input(
            a.render, state, values, *i.cache.shader_function_tables(),
            [&](auto, const ParticleSpriteInput &input,
                ParticleValueCache &cache,
                const ParticleValueSources &sources) {
              const auto sprite =
                  particle_sprite_submission(a.sprite, input, cache, sources,
                                             *i.cache.shader_function_tables());
              ParticleDraw d;
              d.material = a.material;
              std::copy(a.material->pass.constants[1].begin(),
                        a.material->pass.constants[1].end(),
                        d.vertex.constants.begin() + 18);
              std::copy(view.camera.begin(), view.camera.end(),
                        d.vertex.constants.begin());
              std::copy(view.fog.begin(), view.fog.end(),
                        d.vertex.constants.begin() + 12);
              const auto orientation =
                  particle_view_constants(view.orientation);
              std::copy(orientation.begin(), orientation.end(),
                        d.vertex.constants.begin() + 148);
              d.vertex.constants[174] = {1, 0, 0, 0};
              d.vertex.constants[175] = {0, 1, 0, 0};
              d.vertex.constants[176] = {0, 0, 1, 0};
              d.vertex.attributes = particle_sprite_vertex_attributes(sprite);
              d.vertex.clip_range = view.clip_range;
              d.vertex.surface_fog = {float(view.width), float(view.height), 1,
                                      1};
              d.depth = {view.clip_range, {0, 0, 1, 1}};
              d.scene_sort_depth = sort_depth;
              result.push_back(std::move(d));
            });
      }
  return result;
}
std::vector<Vertex> NativeWorldEffects::decals(std::uint64_t tick,
                                               float fraction,
                                               float seconds) const {
  std::vector<Vertex> result;
  for (const auto &d : impl_->decal_bank) {
    const float elapsed = (float(tick - d.tick) + fraction) * seconds;
    const float fade =
        d.lifetime > 0 && d.decay > 0
            ? std::clamp((d.lifetime - elapsed) / d.decay, 0.f, 1.f)
            : 1;
    for (auto v : d.vertices) {
      v.color = {fade, fade, fade};
      result.push_back(v);
    }
  }
  return result;
}
WorldEffectDiagnostics NativeWorldEffects::diagnostics() const {
  auto &i = *impl_;
  WorldEffectDiagnostics d;
  d.effects = i.effects.size();
  d.decals = i.decal_bank.size();
  d.lights = i.light_world?i.light_world->pool.active_count:0;
  d.unavailable.assign(i.unavailable.begin(), i.unavailable.end());
  for (const auto &e : i.effects)
    for (const auto &v : e.events)
      for (const auto &s : v.systems) {
        ++d.systems;
        for (const auto &p : s.emitters)
          d.particles += p.particles.size();
      }
  return d;
}
} // namespace halo2
