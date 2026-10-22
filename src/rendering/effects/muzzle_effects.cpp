#include "halo2/muzzle_effects.hpp"
#include "halo2/source_particle_sprite_context.hpp"
#include "halo2/particle_presentation.hpp"
#include "halo2/particle_effect_color.hpp"
#include "halo2/math.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/fixed_effect_event.hpp"
#include "halo2/particle_view.hpp"
#include "halo2/particle_movement.hpp"
#include "halo2/material_effects.hpp"
#include "halo2/simulation.hpp"
#include <algorithm>
#include <bit>
#include <map>
#include <utility>
#include <cmath>

namespace halo2 {
AuthoredMuzzleEffect Cache::load_muzzle_effect(std::uint32_t datum) const {
    const auto tag=query_tag({.handle=TagHandle{datum}, .group=tag_groups::effect, .require_address=true});
    if(!tag)throw FormatError("muzzle owner requires authored effe definition");
    const Bytes b(cache_bytes());const auto root=metadata_offset(tag->address,0x1c);
    const auto offset=[&](std::uint32_t address,std::size_t count){return metadata_offset(address,count);};
    //17C140/17C020/17B130 own the full event clock. Bits3/4 additionally
    //require completed17E460 colors at request time.
    const auto events=b.u32(root+0x14);
    if((b.u32(root)&~0x1eu) ||
        !b.u32(root+0xc) || b.u32(root+0xc)>16 || !events || events>64)
      throw FormatError("muzzle effect requires additional original event lifecycle");
    AuthoredMuzzleEffect d;
    d.flags=b.u32(root);d.loop_start=std::bit_cast<std::int16_t>(b.u16(root+4));
    if(d.loop_start < -1 || d.loop_start>=int(events))
      throw FormatError("Muzzle loop escapes its authored event bank");
    d.marker=b.u32(offset(b.u32(root+0x10),4));
    d.initial_advance=(b.u32(root)&2u)?1.f:0.f;
    d.sounds=authored_effect_sound_parts(*this,datum);
    const auto first_event=offset(b.u32(root+0x18),std::size_t(events)*0x38);
    for(unsigned event=0;event<events;++event) {
      const auto e=first_event+event*0x38;
      for(const auto value:{b.f32(e+4),b.f32(e+8),b.f32(e+12),b.f32(e+16),b.f32(e+20)})
        if(!std::isfinite(value) || value<0)throw FormatError("Invalid authored muzzle event range");
      if(b.u32(e+0x20))
        throw FormatError("muzzle event requires original beam owner");
      // Acceleration is a gameplay part. Preserve the event's particle systems
      // only when the request has its explicit original gameplay-part owner.
      d.acceleration_parts|=b.u32(e+0x28)!=0;
      d.events.push_back({b.f32(e+8),b.f32(e+16)});
      d.clocks.push_back({b.f32(e+4),b.f32(e+8),b.f32(e+12),b.f32(e+16),b.f32(e+20)});
      const auto parts=b.u32(e+0x18);
      const auto part=parts?offset(b.u32(e+0x1c),std::size_t(parts)*0x38):0;
      for(unsigned n=0;n<parts;++n)if(b.u32(part+n*0x38+0x10)!=0xffffffffu) {
        const auto group=b.u32(part+n*0x38+8);
        if(group==0x6c696768u)d.light_parts=true;
        else if(group==0x6a707421u)d.damage_parts=true;
        else if(group!=0x736e6421u)throw FormatError("muzzle part requires its original non-sound owner");
      }
      const auto count=b.u32(e+0x30);
      const auto first=count?offset(b.u32(e+0x34),std::size_t(count)*0x38):0;
      for(unsigned n=0;n<count;++n) {
        const auto at=first+n*0x38;
        if(!source_effect_system_admitted(b.u16(at+0x10),b.u16(at+0x12),false,1,true))continue;
        if(b.u32(at+8)>=b.u32(root+0xc) || b.u16(at+0xe)>1 || b.u16(at+0x10)>2)
          throw FormatError("muzzle system requires original location/environment admission");
        const auto marker=b.u32(offset(b.u32(root+0x10)+b.u32(at+8)*4,4));
        auto system=load_particle_system_render_definition(datum,event,n);
        if((system.coordinates!=0 && system.coordinates!=1) || (system.flags&~0x50u))
          throw FormatError("muzzle system requires additional coordinate/lifecycle owner");
        std::vector<ParticleMovementDefinition> movement;
        const auto emitters=offset(b.u32(at+0x34),system.emitters.size()*0xb8);
        for(unsigned j=0;j<system.emitters.size();++j) {
          const auto physics=b.u32(emitters+j*0xb8+4);
          auto definition=physics==0xffffffffu?ParticleMovementDefinition{}:load_particle_movement_definition(*this,physics);
          for(const auto &controller:definition.controllers)if(controller.type>1)
            throw FormatError("muzzle movement requires original swarm/wind owner");
          movement.push_back(std::move(definition));
        }
        auto sprite=load_particle_sprite(system.particle_datum);
        auto material=std::make_shared<const ParticleMaterial>(load_particle_material(system.particle_datum,15,0,0));
        // 224DB0 selects billboard style+7. The renderer already executes
        // the original perpendicular (style2/class9) vertex program too.
        if(material->vertex.classification!=7 && material->vertex.classification!=8 &&
            material->vertex.classification!=9)
          throw FormatError("muzzle particle requires its original vertex classification");
        d.systems.push_back({marker,event,n,std::move(system),std::move(movement),std::move(sprite),std::move(material)});
        const auto particle=load_source_tag_metadata(d.systems.back().definition.particle_datum);
        const Bytes prt(particle.bytes_at(particle.source_address(),0x7c));
        if(prt.u32(0x70)!=0xffffffffu) {
          auto &out=d.systems.back();out.collision_group=prt.u32(0x6c);out.collision_child=prt.u32(0x70);
          if(out.collision_group==0x666f6f74u) {
            const auto foot=load_source_tag_metadata(out.collision_child);
            const Bytes root_(foot.bytes_at(foot.source_address(),8));
            if(root_.u32(0)>10) {
              const auto *row=query_tag({.contains=TagAddressRange{root_.u32(4)+10u*0x18u,0x18},.require_address=true});
              if(!row)throw FormatError("Casing foot has no resident action10 owner");
              const auto tag_=load_source_tag_metadata(row->datum);
              if(Bytes(tag_.bytes_at(root_.u32(4)+10u*0x18u,0x18)).u32(0x10))
                throw FormatError("Casing collision requires authored visual children");
            }
          }else if(out.collision_group!=0x736e6421u)
            throw FormatError("Muzzle collider requires its authored effect child owner");
        }
      }
    }
    if(d.systems.empty())throw FormatError("effect has no authored first-person muzzle system");
    return d;
}
struct NativeMuzzleEffects::Impl {
  const Cache &cache;
  std::uint32_t owned_seed;
  std::uint32_t &seed;
  SourceEffectRandom random_streams;
  DatumPool effect_pool{"effect",256,400},system_pool{"particle_system",128,84},
      location_pool{"particle_location",256,52},emitter_pool{"particle_emitter",256,76},
      particle_pool{"particles",1024,64};
  using Definition=AuthoredMuzzleEffect;
  struct Emitter {
    std::uint32_t handle{};
    ParticleEmitterState state;
  };
  struct Location {
    std::uint32_t handle{};
    unsigned marker_index{};
    std::uint8_t random{};
    ParticleRenderValueCache values;
    std::vector<Emitter> emitters;
  };
  struct System {
    std::uint32_t handle{};
    unsigned definition_index{};
    float random_a{},random_b{},fraction{};
    std::vector<Location> locations;
  };
  struct Effect {
    std::uint32_t handle{},definition{},weapon{},tick{},a{},b{};
    bool pending{},retiring{};
    std::vector<System> systems;
    SourceEffectEventState event;
    SourceEffectColors colors;
    Vec3 velocity;
    struct Slice {unsigned event{};float before{},seconds{},duration{};};
    std::vector<Slice> slices;
    struct Pose {std::map<std::uint32_t,std::vector<Transform>> markers;MuzzleEffectView view;};
    std::optional<Pose> pose;
    bool attachment{};
    std::uint32_t marker_alias{};
  };
  std::map<std::uint32_t,Definition> definitions;
  std::map<std::uint32_t,std::optional<SourceParticleSpriteContext>> sprite_contexts;
  std::map<std::uint32_t,Effect> effects;
  std::map<std::uint32_t,ParticleSimulationState> particles;
  std::map<std::uint32_t,ParticleSimulationState> previous_particles;
  std::optional<std::uint32_t> advanced;
  std::vector<NativeMuzzleEffects::Sound> sounds;
  ParticleCollisionQuery collision;
  Impl(const Cache &c,std::uint32_t s):cache(c),owned_seed(s),seed(owned_seed),random_streams{seed,seed}{}
  Impl(const Cache &c,SourceEffectRandom streams):cache(c),owned_seed{},seed(streams.auxiliary),random_streams(streams){}
  Impl(const Impl &o,SourceEffectRandom streams,ParticleCollisionQuery query)
      :cache(o.cache),owned_seed{},seed(streams.auxiliary),random_streams(streams),
       effect_pool(o.effect_pool),system_pool(o.system_pool),location_pool(o.location_pool),
       emitter_pool(o.emitter_pool),particle_pool(o.particle_pool),definitions(o.definitions),
       sprite_contexts(o.sprite_contexts),effects(o.effects),particles(o.particles),
       previous_particles(o.previous_particles),advanced(o.advanced),sounds(o.sounds),collision(std::move(query)){}
  std::uint32_t next_random(){seed=seed*1664525u+1013904223u;return seed;}
  float random_fraction(){return float(double(next_random()>>16)*double(1.f/65535.f));}
  Definition &definition(std::uint32_t datum) {
    if(auto i=definitions.find(datum);i!=definitions.end())return i->second;
    auto loaded=cache.load_muzzle_effect(datum);
    for(auto &system:loaded.systems) {
      const auto [context,inserted]=sprite_contexts.try_emplace(system.definition.particle_datum,
          source_particle_sprite_context(cache,system.definition.particle_datum,system.sprite));
      (void)inserted;
      if(context->second && !context->second->has_bitmap)system.sprite.bitmap_type=-1;
    }
    return definitions.emplace(datum,std::move(loaded)).first->second;
  }
  ParticleValueSources sources(const Effect &effect,const System &s,const Location &location,const MuzzleEffectView &view,std::uint32_t tick,float seconds) {
    ParticleValueSources out;
    out.system=ParticleSystemValueState{s.fraction,s.random_a,s.random_b,
        std::array<float,2>{std::bit_cast<float>(effect.a),std::bit_cast<float>(effect.b)}};
    out.location=ParticleLocationValueState{location.random,1.f};
    out.game_tick=std::int32_t(tick);out.game_rate=seconds;
    out.camera_position=view.position;out.camera_forward=view.forward;out.camera_up=view.up;
    return out;
  }
  void event_step(Effect &effect,float seconds) {
    const auto &d=definition(effect.definition);
    const SourceEffectEventServices services{[] {},[&] {
      for(auto cue:d.sounds)if(cue.event==unsigned(effect.event.event)) {
        cue.gain=cue.scale_a?std::bit_cast<float>(effect.a):1.f;
        if(cue.scale_b)cue.gain=std::bit_cast<float>(effect.b)*cue.gain;
        sounds.push_back({cue,effect.weapon});
      }
    },[&](float before,float dt){effect.slices.push_back({unsigned(effect.event.event),before,dt,effect.event.target});},
    [] {},[&]{effect.retiring=true;effect.event.flags|=0x44u;return false;}};
    source_effect_event_advance(effect.event,d.clocks,d.loop_start,seconds,
        random_streams.event(d.flags),services);
  }
  void birth(Effect &effect,const NativeMuzzleEffects::Markers &marker,const MuzzleEffectView &view,
      std::uint32_t tick,float seconds,const Effect::Slice &slice) {
    auto &d=definition(effect.definition);
    for(unsigned n=0;n<d.systems.size();++n)if(d.systems[n].event==slice.event &&
        std::none_of(effect.systems.begin(),effect.systems.end(),[&](const auto &s){return s.definition_index==n;})) {
      const auto &authored=d.systems[n];
      const auto handle=system_pool.allocate();
      if(handle==DatumPool::none)throw FormatError("native muzzle system pool exhausted");
      System s;s.handle=handle;s.definition_index=n;
      //173FD0 owns two RNG fields once per system;248620 owns a separate
      //byte and emitter bank for each actual marker location.
      s.random_a=random_fraction();s.random_b=random_fraction();
      const auto placements=marker(effect.attachment && effect.marker_alias?effect.marker_alias:authored.marker);
      for(unsigned n=0;n<placements.size();++n) {
        const auto location=location_pool.allocate();
        if(location==DatumPool::none)throw FormatError("native muzzle location pool exhausted");
        Location l;l.handle=location;l.marker_index=n;
        l.random=std::uint8_t(((next_random()>>16)*255u)>>16);
        for(std::size_t j=0;j<authored.definition.emitters.size();++j) {
          const auto emitter=emitter_pool.allocate();
          if(emitter==DatumPool::none)throw FormatError("native muzzle emitter pool exhausted");
          l.emitters.push_back({emitter,{}});
        }
        s.locations.push_back(std::move(l));
      }
      effect.systems.push_back(std::move(s));
    }
    for(unsigned n=0;n<effect.systems.size();++n) {
      auto &s=effect.systems[n];const auto &authored=d.systems[s.definition_index];
      if(authored.event!=slice.event)continue;
      const auto placements=marker(effect.attachment && effect.marker_alias?effect.marker_alias:authored.marker);
      for(auto &location:s.locations) {
      if(location.marker_index>=placements.size())throw FormatError("Muzzle location lost its original marker ordinal");
      auto placement=placements[location.marker_index];
      if(authored.definition.coordinates==0) {
        // Original FP location conversion, before world-coordinate birth.
        const Transform camera{1,{view.forward,cross(view.up,view.forward),view.up},view.position};
        placement=compose_transform(camera,placement);
      }
      const ParticleEmitterFrame frame{placement.scale,
          {placement.basis[0],placement.basis[1],placement.basis[2]},placement.position};
      s.fraction=slice.duration>0?(slice.before+slice.seconds)/slice.duration:1.f;
      auto owners=sources(effect,s,location,view,tick,seconds);
      location.values.values.evaluated=0;
      complete_particle_values(location.values.values,0x1ffffu,owners);
      for(unsigned j=0;j<location.emitters.size();++j) {
        auto &emitter=location.emitters[j];
        update_particle_emitter(authored.definition,j,emitter.state,0x109,slice.seconds,&frame,
            location.values.values.values,source_particle_effect_color(0xff808080,
                effect.colors.a,effect.colors.b,authored.sprite.flags),*cache.shader_function_tables(),
            [&](const ParticleEmissionRequest &request,ParticleEmitterState &state) {
          const auto datum=particle_pool.allocate();if(datum==DatumPool::none)return;
          auto &p=particles[datum];p.salt=std::uint16_t(datum>>16);
          create_emitter_particle(authored.definition,j,state,p,
              {datum,datum,s.fraction,request,effect.velocity},location.values,owners,seed,
              *cache.original_data(),*cache.shader_function_tables());
        });
      }
      }
    }
    // The fixed event owner controls emission. Existing particles retain
    // their own authored lifetime after the event retires.
    effect.pending=false;
  }
};
NativeMuzzleEffects::NativeMuzzleEffects(const Cache &cache,std::uint32_t seed):impl_(std::make_unique<Impl>(cache,seed)){}
NativeMuzzleEffects::NativeMuzzleEffects(const Cache &cache,SourceEffectRandom streams):impl_(std::make_unique<Impl>(cache,streams)){}
NativeMuzzleEffects::~NativeMuzzleEffects()=default;
struct NativeMuzzleEffects::Checkpoint::Data {
  std::shared_ptr<const int> owner;
  std::array<std::uint32_t,2> random;
  bool aliased;
  std::unique_ptr<Impl> state;
  Data(const Impl &from,std::shared_ptr<const int> token)
      :owner(std::move(token)),random{from.random_streams.primary,from.random_streams.auxiliary},
       aliased(&from.random_streams.primary==&from.random_streams.auxiliary),
       state(std::make_unique<Impl>(from,SourceEffectRandom{random[0],random[1]},ParticleCollisionQuery{})){}
};
std::optional<std::uint64_t> NativeMuzzleEffects::Checkpoint::tick()const noexcept {
  if(!data_ || !data_->state->advanced)return {};
  return std::uint64_t(*data_->state->advanced);
}
std::array<std::uint32_t,2> NativeMuzzleEffects::Checkpoint::random()const {
  if(!data_)throw FormatError("Missing muzzle presentation checkpoint");return data_->random;
}
NativeMuzzleEffects::PreparedRestore::PreparedRestore()=default;
NativeMuzzleEffects::PreparedRestore::~PreparedRestore()=default;
NativeMuzzleEffects::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
NativeMuzzleEffects::PreparedRestore &NativeMuzzleEffects::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
NativeMuzzleEffects::Checkpoint NativeMuzzleEffects::checkpoint()const {
  Checkpoint result;result.data_=std::make_shared<Checkpoint::Data>(*impl_,owner_);return result;
}
NativeMuzzleEffects::PreparedRestore NativeMuzzleEffects::prepare_restore(const Checkpoint &checkpoint,
    SourceEffectRandom streams,ParticleCollisionQuery collision)const {
  if(!checkpoint.data_ || checkpoint.data_->owner!=owner_)
    throw FormatError("Muzzle presentation checkpoint belongs to another owner");
  if(checkpoint.data_->aliased!=(&streams.primary==&streams.auxiliary))
    throw FormatError("Muzzle presentation RNG aliasing changed");
  PreparedRestore result;result.replacement_=std::make_unique<Impl>(*checkpoint.data_->state,streams,std::move(collision));
  result.owner_=owner_;return result;
}
bool NativeMuzzleEffects::restore_ready(const PreparedRestore &prepared)const noexcept {
  return prepared.replacement_ && prepared.owner_==owner_;
}
bool NativeMuzzleEffects::commit_restore(PreparedRestore &&prepared)noexcept {
  if(!restore_ready(prepared))return false;
  impl_.swap(prepared.replacement_);prepared.owner_.reset();return true;
}
std::uint32_t NativeMuzzleEffects::request(const WeaponEffectInput &input,std::uint32_t tick) {
  if(impl_->definition(input.effect).flags&0x18u)
    throw FormatError("Muzzle effect requires completed original17E460 lighting colors");
  return request(input,tick,SourceEffectColors{});
}
std::uint32_t NativeMuzzleEffects::request(const WeaponEffectInput &input,std::uint32_t tick,SourceEffectColors colors) {
  return request(input,tick,colors,{});
}
std::uint32_t NativeMuzzleEffects::request(const WeaponEffectInput &input,std::uint32_t tick,SourceEffectColors colors,Vec3 velocity) {
  return request(input,tick,colors,velocity,false);
}
std::uint32_t NativeMuzzleEffects::request(const WeaponEffectInput &input,std::uint32_t tick,SourceEffectColors colors,Vec3 velocity,bool light_parts_owned) {
  return request(input,tick,colors,velocity,light_parts_owned,false);
}
std::uint32_t NativeMuzzleEffects::request(const WeaponEffectInput &input,std::uint32_t tick,SourceEffectColors colors,Vec3 velocity,bool light_parts_owned,bool source_gameplay_parts_owned) {
  auto &i=*impl_;const auto &d=i.definition(input.effect);
  if((d.damage_parts || d.acceleration_parts) && !source_gameplay_parts_owned)throw FormatError("FP effect requires its original gameplay-part owner");
  if(d.light_parts && !light_parts_owned)throw FormatError("Muzzle light parts require their live world owner");
  if(auto found=i.effects.find(input.previous_handle);found!=i.effects.end() && found->second.definition==input.effect) {
    auto &e=found->second;e.pending=true;e.retiring=false;e.tick=tick;
    //17DFF0 ->17A1A0 reselects0 without rewriting A/B/colors or running
    //the creator's one-second initial update. Existing particles survive.
    e.event={};
    source_effect_event_select(e.event,d.clocks,0,i.random_streams.event(d.flags),{[] {}});
    return e.handle;
  }
  const auto handle=i.effect_pool.allocate();if(handle==DatumPool::none)return handle;
  auto &e=i.effects.emplace(handle,Impl::Effect{handle,input.effect,input.weapon,tick,input.a_bits,input.b_bits,true,false,{}}).first->second;
  e.colors=colors;e.velocity=velocity;
  source_effect_event_select(e.event,d.clocks,0,i.random_streams.event(d.flags),{[] {}});
  i.event_step(e,d.initial_advance);
  if(i.advanced)for(auto t=tick;t<*i.advanced && !e.retiring;++t)i.event_step(e,1.f/30.f);
  return handle;
}
std::uint32_t NativeMuzzleEffects::marker_name(std::uint32_t effect) const {return impl_->definition(effect).marker;}
std::vector<std::uint32_t> NativeMuzzleEffects::marker_names(std::uint32_t effect) const {
  std::vector<std::uint32_t> out;for(const auto &system:impl_->definition(effect).systems)
    if(std::find(out.begin(),out.end(),system.marker)==out.end())out.push_back(system.marker);
  return out;
}
std::vector<NativeMuzzleEffects::Sound> NativeMuzzleEffects::take_sounds(){return std::exchange(impl_->sounds,{});}
void NativeMuzzleEffects::collision_query(ParticleCollisionQuery query){impl_->collision=std::move(query);}
std::uint32_t NativeMuzzleEffects::attachment(std::uint32_t definition,std::uint32_t weapon,
    std::uint32_t tick,SourceEffectControls controls,bool enabled) {
  return attachment(definition,weapon,tick,controls,enabled,0);
}
std::uint32_t NativeMuzzleEffects::attachment(std::uint32_t definition,std::uint32_t weapon,
    std::uint32_t tick,SourceEffectControls controls,bool enabled,std::uint32_t marker_alias) {
  return attachment(definition,weapon,tick,controls,enabled,marker_alias,false);
}
std::uint32_t NativeMuzzleEffects::attachment(std::uint32_t definition,std::uint32_t weapon,
    std::uint32_t tick,SourceEffectControls controls,bool enabled,std::uint32_t marker_alias,bool source_gameplay_parts_owned) {
  auto &i=*impl_;const auto &d=i.definition(definition);
  if((d.damage_parts || d.acceleration_parts) && !source_gameplay_parts_owned)throw FormatError("FP attachment requires its original gameplay-part owner");
  if(d.flags&0x18u || d.light_parts)throw FormatError("FP attachment requires its completed lighting owner");
  for(auto &[handle,e]:i.effects)if(e.attachment && !e.retiring && e.weapon==weapon && e.definition==definition && e.marker_alias==marker_alias) {
    e.a=std::bit_cast<std::uint32_t>(controls.a);e.b=std::bit_cast<std::uint32_t>(controls.b);
    // Complete17C370: a false A function stops through17A0F0; its next
    //true result restarts through17A1A0. Linked particles retain lifetime.
    if(!enabled)e.event.flags=(e.event.flags&~0x10u)|4u;
    else if(e.event.flags&4u) {
      e.event.flags&=~4u;
      source_effect_event_select(e.event,d.clocks,0,i.random_streams.event(d.flags),{[] {}});
    }
    return handle;
  }
  const auto handle=i.effect_pool.allocate();if(handle==DatumPool::none)return handle;
  auto &e=i.effects.emplace(handle,Impl::Effect{handle,definition,weapon,tick,
      std::bit_cast<std::uint32_t>(controls.a),std::bit_cast<std::uint32_t>(controls.b),true,false,{}}).first->second;
  e.attachment=true;e.marker_alias=marker_alias;e.event.flags=enabled?2u:6u;
  source_effect_event_select(e.event,d.clocks,0,i.random_streams.event(d.flags),{[] {}});
  //175FA0 ->179850 starts retained attachments with zero, regardless of
  //the one-shot creator's definition bit1 initial interval.
  i.event_step(e,0.f);return handle;
}
void NativeMuzzleEffects::retire_attachments_except(std::span<const std::uint32_t> weapons) {
  for(auto &[handle,e]:impl_->effects)if(e.attachment &&
      std::find(weapons.begin(),weapons.end(),e.weapon)==weapons.end()) {
    e.retiring=true;e.event.flags|=0x44u;
  }
}
void NativeMuzzleEffects::pose(std::uint32_t weapon,const Marker &marker,const MuzzleEffectView &view,
    std::uint32_t tick,float seconds) {
  pose(weapon,Markers{[&](auto name){return std::vector<Transform>{marker(name)};}},view,tick,seconds);
}
void NativeMuzzleEffects::pose(std::uint32_t weapon,const Markers &marker,const MuzzleEffectView &view,
    std::uint32_t tick,float seconds) {
  auto &i=*impl_;
  for(auto &[handle,e]:i.effects)if(e.weapon==weapon) {
    e.pose.emplace();e.pose->view=view;
    for(const auto &system:i.definition(e.definition).systems) {
      const auto name=e.attachment && e.marker_alias?e.marker_alias:system.marker;
      e.pose->markers.try_emplace(name,marker(name));
    }
    const auto retained=[&](std::uint32_t name){return e.pose->markers.at(name);};
    for(const auto &slice:e.slices)i.birth(e,retained,view,tick,seconds,slice);
    e.slices.clear();
  }
}
void NativeMuzzleEffects::advance(std::uint32_t tick,float seconds) {
  auto &i=*impl_;if(!i.advanced){i.advanced=tick;return;}
  if(tick<*i.advanced)throw FormatError("muzzle clock reversed");
  for(auto t=*i.advanced;t<tick;++t) {
   i.previous_particles=i.particles;
   for(auto &[handle,e]:i.effects)for(auto &s:e.systems)for(auto &location:s.locations)for(unsigned j=0;j<location.emitters.size();++j) {
    auto &emitter=location.emitters[j];
    auto *link=&emitter.state.first_particle;
    while(*link!=DatumPool::none) {
      auto p=i.particles.find(*link);if(p==i.particles.end())throw FormatError("muzzle particle chain lost allocation");
      auto &v=p->second;v.age=v.inverse_lifetime*seconds+v.age;
      if(v.age>1.f || (v.flags&1u)) {
        const auto dead=p->first;*link=v.next_particle;--emitter.state.particle_count;
        i.particle_pool.release_unchecked(dead);i.particles.erase(p);
      }else {
        v.position=v.position+v.velocity*seconds;v.phase=v.rotation_rate*seconds+v.phase;link=&v.next_particle;
      }
    }
    const auto &movement=i.definition(e.definition).systems[s.definition_index].movement[j];
    const ParticleMovementPool pool{[&](auto datum)->ParticleSimulationState&{return i.particles.at(datum);},1024};
    //249210 integrates age/position/phase before its controller dispatch.
    //These admitted definitions contain only the independently recovered physics0.
    for(const auto &controller:movement.controllers) {
      const auto owners=i.sources(e,s,location,MuzzleEffectView{},t,seconds);
      if(controller.type==0)apply_particle_physics_controller(controller,emitter.state.first_particle,seconds,retail_gravity,1,
          owners,*i.cache.shader_function_tables(),pool);
      else {
        const auto &authored=i.definition(e.definition).systems[s.definition_index];
        apply_particle_collider_controller(controller,movement.flags,authored.sprite.flags,
            emitter.state.first_particle,seconds,owners,*i.cache.shader_function_tables(),pool,i.collision,
            [&](const auto &particle,const auto &hit) {
          const auto gain=source_particle_collision_child_gain(particle.velocity);
          const auto publish=[&](std::uint32_t sound) {
            if(sound!=0xffffffffu)i.sounds.push_back({{0,gain,sound},e.weapon,particle.position});
          };
          if(authored.collision_group==0x736e6421u)publish(authored.collision_child);
          else if(authored.collision_group==0x666f6f74u)
            for(const auto sound:source_material_sound_effects(i.cache,authored.collision_child,-1,hit.material,
                10,(authored.sprite.flags&0x4000u)?0:-1))publish(sound);
        });
      }
    }
   }
   for(auto &[handle,e]:i.effects)if(t+1>e.tick && !e.retiring)i.event_step(e,seconds);
   for(auto &[handle,e]:i.effects)if(e.pose) {
     const auto marker=[&](std::uint32_t name){return e.pose->markers.at(name);};
     for(const auto &slice:e.slices)i.birth(e,marker,e.pose->view,t+1,seconds,slice);
     e.slices.clear();
   }
   for(auto effect=i.effects.begin();effect!=i.effects.end();) {
    auto &e=effect->second;
    // 1764B0/176E20 retains a retired system while any emitter owns live
    // particles. 175C00 ->176E70 ->24A7B0 releases its location/emitter
    // allocations; 17C9F0 unlinks it and1788B0 ->178F60 frees the effect.
    // This closed path has no child systems or detached part/audio owners.
    const bool live=std::any_of(e.systems.begin(),e.systems.end(),[](const auto &system) {
      return std::any_of(system.locations.begin(),system.locations.end(),[](const auto &location) {
        return std::any_of(location.emitters.begin(),location.emitters.end(),[](const auto &emitter) {
          return emitter.state.particle_count!=0;
        });
      });
    });
    if(e.slices.empty() && e.retiring && !live) {
      for(const auto &system:e.systems) {
        for(const auto &location:system.locations) {
          for(const auto &emitter:location.emitters)i.emitter_pool.release_unchecked(emitter.handle);
          i.location_pool.release_unchecked(location.handle);
        }
        i.system_pool.release_unchecked(system.handle);
      }
      i.effect_pool.release_unchecked(e.handle);effect=i.effects.erase(effect);
    }else ++effect;
   }
  }
  i.advanced=tick;
}
std::vector<ParticleDraw> NativeMuzzleEffects::draw(std::uint32_t weapon,const Transform &marker,
    const MuzzleEffectView &view,std::uint32_t tick,float fraction,float seconds) {
  return draw(weapon,[&](std::uint32_t){return marker;},view,tick,fraction,seconds);
}
std::vector<ParticleDraw> NativeMuzzleEffects::draw(std::uint32_t weapon,const Marker &,
    const MuzzleEffectView &view,std::uint32_t tick,float fraction,float seconds) {
  return draw(weapon,view,tick,fraction,seconds);
}
std::vector<ParticleDraw> NativeMuzzleEffects::draw(std::uint32_t weapon,const MuzzleEffectView &view,
    std::uint32_t tick,float fraction,float seconds) {
  auto &i=*impl_;std::vector<ParticleDraw> result;
  for(auto &[handle,e]:i.effects)if(e.weapon==weapon) {
    const auto &d=i.definition(e.definition);
    for(unsigned n=0;n<e.systems.size();++n) {
      auto &s=e.systems[n];const auto &a=d.systems[s.definition_index];
      for(auto &location:s.locations) {
      ParticleRenderState state;state.system_identity=s.handle;state.location_identity=location.handle;
      state.sources=i.sources(e,s,location,view,tick,seconds);state.first_person=true;
      for(const auto &emitter:location.emitters) {
        auto &out=state.emitters.emplace_back();out.rotation=emitter.state.rotation;out.position=emitter.state.position;
        for(auto datum=emitter.state.first_particle;datum!=DatumPool::none;) {
          const auto identity=datum;const auto &current=i.particles.at(datum);datum=current.next_particle;
          const auto old=i.previous_particles.find(identity);
          const auto p=particle_presentation(old==i.previous_particles.end()?nullptr:&old->second,current,fraction);
          const float age=p.age;if(age>1.f)continue;
          out.particles.push_back({identity,
              {age,p.system_fraction,p.phase,p.position,p.random},p.velocity,p.seed_color});
        }
      }
      ParticleRenderValueCache values; // Re-evaluate actual interpolated render state each frame.
      const auto &context=i.sprite_contexts.at(a.definition.particle_datum);
      if(!context)continue;
      values.system_identity=state.system_identity;
      values.location_identity=state.location_identity;
      complete_particle_values(values.values,a.sprite.required_values&0x107f0u,state.sources);
      for_each_particle_render_input(a.definition,state,values,*i.cache.shader_function_tables(),
          [&](std::uint64_t,const ParticleSpriteInput &input,ParticleValueCache &cache,const ParticleValueSources &sources) {
        auto completed=input;completed.width=context->width;
        completed.selected_sequence=context->selected_sequence;
        const auto sprite=particle_sprite_submission(a.sprite,completed,cache,sources,*i.cache.shader_function_tables());
        ParticleDraw draw;draw.material=a.material;draw.effect_definition=e.definition;draw.effect_owner=e.weapon;
        std::copy(a.material->pass.constants[1].begin(),a.material->pass.constants[1].end(),draw.vertex.constants.begin()+18);
        std::copy(view.camera.begin(),view.camera.end(),draw.vertex.constants.begin());
        std::copy(view.fog.begin(),view.fog.end(),draw.vertex.constants.begin()+12);
        const auto orientation=particle_view_constants(view.orientation);
        std::copy(orientation.begin(),orientation.end(),draw.vertex.constants.begin()+148);
        // 240B80 submits the current particle rendering identity frame here;
        // 177090 has already placed this sprite in world coordinates.
        draw.vertex.constants[174]={1,0,0,0};draw.vertex.constants[175]={0,1,0,0};draw.vertex.constants[176]={0,0,1,0};
        draw.vertex.attributes=particle_sprite_vertex_attributes(sprite);
        draw.vertex.clip_range=view.clip_range;draw.vertex.surface_fog={float(view.width),float(view.height),1,1};
        draw.depth={view.clip_range,{0,0,1,1}};
        result.push_back(std::move(draw));
      });
      }
    }
  }
  return result;
}
std::size_t NativeMuzzleEffects::particle_count() const {return impl_->particles.size();}
std::size_t NativeMuzzleEffects::location_count() const {
  std::size_t count{};for(const auto &[handle,e]:impl_->effects)for(const auto &s:e.systems)count+=s.locations.size();return count;
}
} // namespace halo2
