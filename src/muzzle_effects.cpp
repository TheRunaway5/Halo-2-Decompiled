#include "halo2/muzzle_effects.hpp"
#include "halo2/particle_presentation.hpp"
#include "halo2/math.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/fixed_effect_event.hpp"
#include "halo2/particle_view.hpp"
#include "halo2/particle_movement.hpp"
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
    const Bytes b(storage_);const auto root=metadata_offset(tag->address,0x1c);
    const auto offset=[&](std::uint32_t address,std::size_t count){return metadata_offset(address,count);};
    //17AD20/17AC00 fixed sequential branch. Unsupported ownership is an
    //error, not a skipped event or guessed timing.
    const auto events=b.u32(root+0x14);
    if((b.u32(root)&~2u) || b.u16(root+4)!=0xffffu ||
        !b.u32(root+0xc) || b.u32(root+0xc)>16 || !events || events>64)
      throw FormatError("muzzle effect requires additional original event lifecycle");
    AuthoredMuzzleEffect d;
    d.marker=b.u32(offset(b.u32(root+0x10),4));
    d.initial_advance=(b.u32(root)&2u)?1.f:0.f;
    d.sounds=fixed_effect_sound_cues(*this,datum,0,0);
    const auto first_event=offset(b.u32(root+0x18),std::size_t(events)*0x38);
    for(unsigned event=0;event<events;++event) {
      const auto e=first_event+event*0x38;
      if(b.f32(e+4)>0 || b.f32(e+8)!=b.f32(e+12) || b.f32(e+8)<0 ||
          b.f32(e+16)!=b.f32(e+20) || b.f32(e+16)<0 ||
          !std::isfinite(b.f32(e+8)) || !std::isfinite(b.f32(e+16)) ||
          b.u32(e+0x20) || b.u32(e+0x28))
        throw FormatError("muzzle event requires original random/beam/acceleration owner");
      d.events.push_back({b.f32(e+8),b.f32(e+16)});
      const auto parts=b.u32(e+0x18);
      const auto part=parts?offset(b.u32(e+0x1c),std::size_t(parts)*0x38):0;
      for(unsigned n=0;n<parts;++n)if(b.u32(part+n*0x38+0x10)!=0xffffffffu && b.u32(part+n*0x38+8)!=0x736e6421u)
        throw FormatError("muzzle part requires its original non-sound owner");
      const auto count=b.u32(e+0x30);
      const auto first=count?offset(b.u32(e+0x34),std::size_t(count)*0x38):0;
      for(unsigned n=0;n<count;++n) {
        const auto at=first+n*0x38;
        if(!source_effect_system_admitted(b.u16(at+0x10),b.u16(at+0x12),false,1,true))continue;
        if(b.u32(at+8)>=b.u32(root+0xc) || b.u16(at+0xe)>1 || b.u16(at+0x10)>2)
          throw FormatError("muzzle system requires original location/environment admission");
        const auto marker=b.u32(offset(b.u32(root+0x10)+b.u32(at+8)*4,4));
        auto system=load_particle_system_render_definition(datum,event,n);
        if((system.coordinates!=0 && system.coordinates!=1) || system.flags!=0)
          throw FormatError("muzzle system requires additional coordinate/lifecycle owner");
        std::vector<ParticleMovementDefinition> movement;
        const auto emitters=offset(b.u32(at+0x34),system.emitters.size()*0xb8);
        for(unsigned j=0;j<system.emitters.size();++j) {
          const auto physics=b.u32(emitters+j*0xb8+4);
          auto definition=physics==0xffffffffu?ParticleMovementDefinition{}:load_particle_movement_definition(*this,physics);
          for(const auto &controller:definition.controllers)if(controller.type!=0)
            throw FormatError("muzzle movement requires original collider/swarm/wind owner");
          movement.push_back(std::move(definition));
        }
        auto sprite=load_particle_sprite(system.particle_datum);
        auto material=std::make_shared<const ParticleMaterial>(load_particle_material(system.particle_datum,15,0,0));
        if(material->vertex.classification!=7 && material->vertex.classification!=8)
          throw FormatError("muzzle particle requires its original vertex classification");
        d.systems.push_back({marker,event,n,std::move(system),std::move(movement),std::move(sprite),std::move(material)});
      }
    }
    if(d.systems.empty())throw FormatError("effect has no authored first-person muzzle system");
    return d;
}
struct NativeMuzzleEffects::Impl {
  const Cache &cache;
  std::uint32_t owned_seed;
  std::uint32_t &seed;
  DatumPool effect_pool{"effect",256,400},system_pool{"particle_system",128,84},
      location_pool{"particle_location",256,52},emitter_pool{"particle_emitter",256,76},
      particle_pool{"particles",1024,64};
  using Definition=AuthoredMuzzleEffect;
  struct Emitter {
    std::uint32_t handle{};
    ParticleEmitterState state;
  };
  struct System {
    std::uint32_t handle{},location{};
    unsigned definition_index{};
    float random_a{},random_b{},fraction{};
    std::uint8_t random_location{};
    ParticleRenderValueCache values;
    std::vector<Emitter> emitters;
  };
  struct Effect {
    std::uint32_t handle{},definition{},weapon{},tick{},a{},b{};
    bool pending{},retiring{};
    std::vector<System> systems;
    FixedEffectSequenceState event;
    struct Slice {unsigned event{};float before{},seconds{};};
    std::vector<Slice> slices;
  };
  std::map<std::uint32_t,Definition> definitions;
  std::map<std::uint32_t,Effect> effects;
  std::map<std::uint32_t,ParticleSimulationState> particles;
  std::map<std::uint32_t,ParticleSimulationState> previous_particles;
  std::optional<std::uint32_t> advanced;
  std::vector<NativeMuzzleEffects::Sound> sounds;
  Impl(const Cache &c,std::uint32_t s):cache(c),owned_seed(s),seed(owned_seed){}
  Impl(const Cache &c,SourceEffectRandom streams):cache(c),owned_seed{},seed(streams.auxiliary){}
  std::uint32_t next_random(){seed=seed*1664525u+1013904223u;return seed;}
  float random_fraction(){return float(double(next_random()>>16)*double(1.f/65535.f));}
  Definition &definition(std::uint32_t datum) {
    if(auto i=definitions.find(datum);i!=definitions.end())return i->second;
    return definitions.emplace(datum,cache.load_muzzle_effect(datum)).first->second;
  }
  ParticleValueSources sources(const Effect &effect,const System &s,const MuzzleEffectView &view,std::uint32_t tick,float seconds) {
    ParticleValueSources out;
    out.system=ParticleSystemValueState{s.fraction,s.random_a,s.random_b,
        std::array<float,2>{std::bit_cast<float>(effect.a),std::bit_cast<float>(effect.b)}};
    out.location=ParticleLocationValueState{s.random_location,1.f};
    out.game_tick=std::int32_t(tick);out.game_rate=seconds;
    out.camera_position=view.position;out.camera_forward=view.forward;out.camera_up=view.up;
    return out;
  }
  void event_step(Effect &effect,float seconds) {
    const auto &d=definition(effect.definition);
    advance_fixed_effect_sequence(effect.event,d.events,seconds,[&](unsigned event) {
      for(const auto &cue:d.sounds)if(cue.event==event)sounds.push_back({cue,effect.weapon});
    },[&](unsigned event,float before,float dt){effect.slices.push_back({event,before,dt});});
    effect.retiring=effect.event.clock.finished;
  }
  void birth(Effect &effect,const NativeMuzzleEffects::Marker &marker,const MuzzleEffectView &view,
      std::uint32_t tick,float seconds,const Effect::Slice &slice) {
    auto &d=definition(effect.definition);
    for(unsigned n=0;n<d.systems.size();++n)if(d.systems[n].event==slice.event &&
        std::none_of(effect.systems.begin(),effect.systems.end(),[&](const auto &s){return s.definition_index==n;})) {
      const auto &authored=d.systems[n];
      const auto handle=system_pool.allocate(),location=location_pool.allocate();
      if(handle==DatumPool::none || location==DatumPool::none)throw FormatError("native muzzle source pools exhausted");
      System s;s.handle=handle;s.location=location;s.definition_index=n;
      // Original173FD0 then248620: two auxiliary system RNG fields,
      // followed by the auxiliary location byte.
      s.random_a=random_fraction();s.random_b=random_fraction();
      s.random_location=std::uint8_t(((next_random()>>16)*255u)>>16);
      for(std::size_t j=0;j<authored.definition.emitters.size();++j) {
        const auto emitter=emitter_pool.allocate();
        if(emitter==DatumPool::none)throw FormatError("native muzzle emitter pool exhausted");
        s.emitters.push_back({emitter,{}});
      }
      effect.systems.push_back(std::move(s));
    }
    for(unsigned n=0;n<effect.systems.size();++n) {
      auto &s=effect.systems[n];const auto &authored=d.systems[s.definition_index];
      if(authored.event!=slice.event)continue;
      auto placement=marker(authored.marker);
      if(authored.definition.coordinates==0) {
        // Original FP location conversion, before world-coordinate birth.
        const Transform camera{1,{view.forward,cross(view.up,view.forward),view.up},view.position};
        placement=compose_transform(camera,placement);
      }
      const ParticleEmitterFrame frame{placement.scale,
          {placement.basis[0],placement.basis[1],placement.basis[2]},placement.position};
      s.fraction=d.events[slice.event].duration>0?(slice.before+slice.seconds)/d.events[slice.event].duration:1.f;
      auto owners=sources(effect,s,view,tick,seconds);
      s.values.values.evaluated=0;
      complete_particle_values(s.values.values,0x1ffffu,owners);
      for(unsigned j=0;j<s.emitters.size();++j) {
        auto &emitter=s.emitters[j];
        update_particle_emitter(authored.definition,j,emitter.state,0x109,slice.seconds,&frame,
            s.values.values.values,0xff808080,*cache.shader_function_tables(),
            [&](const ParticleEmissionRequest &request,ParticleEmitterState &state) {
          const auto datum=particle_pool.allocate();if(datum==DatumPool::none)return;
          auto &p=particles[datum];p.salt=std::uint16_t(datum>>16);
          create_emitter_particle(authored.definition,j,state,p,
              {datum,datum,s.fraction,request,{}},s.values,owners,seed,
              *cache.original_data(),*cache.shader_function_tables());
        });
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
std::uint32_t NativeMuzzleEffects::request(const WeaponEffectInput &input,std::uint32_t tick) {
  auto &i=*impl_;const auto &d=i.definition(input.effect);
  if(auto found=i.effects.find(input.previous_handle);found!=i.effects.end() && found->second.definition==input.effect) {
    auto &e=found->second;e.pending=true;e.retiring=false;e.tick=tick;e.a=input.a_bits;e.b=input.b_bits;
    e.event={{0,d.events.front().delay,false,false},0};e.slices.clear();i.event_step(e,d.initial_advance);return e.handle;
  }
  const auto handle=i.effect_pool.allocate();if(handle==DatumPool::none)return handle;
  auto &e=i.effects.emplace(handle,Impl::Effect{handle,input.effect,input.weapon,tick,input.a_bits,input.b_bits,true,false,{}}).first->second;
  e.event={{0,d.events.front().delay,false,false},0};i.event_step(e,d.initial_advance);
  if(i.advanced)for(auto t=tick;t<*i.advanced && !e.event.clock.finished;++t)i.event_step(e,1.f/30.f);
  return handle;
}
std::uint32_t NativeMuzzleEffects::marker_name(std::uint32_t effect) const {return impl_->definition(effect).marker;}
std::vector<std::uint32_t> NativeMuzzleEffects::marker_names(std::uint32_t effect) const {
  std::vector<std::uint32_t> out;for(const auto &system:impl_->definition(effect).systems)
    if(std::find(out.begin(),out.end(),system.marker)==out.end())out.push_back(system.marker);
  return out;
}
std::vector<NativeMuzzleEffects::Sound> NativeMuzzleEffects::take_sounds(){return std::exchange(impl_->sounds,{});}
void NativeMuzzleEffects::advance(std::uint32_t tick,float seconds) {
  auto &i=*impl_;if(!i.advanced){i.advanced=tick;return;}
  if(tick<*i.advanced)throw FormatError("muzzle clock reversed");
  for(auto t=*i.advanced;t<tick;++t) {
   i.previous_particles=i.particles;
   for(auto &[handle,e]:i.effects)for(auto &s:e.systems)for(unsigned j=0;j<s.emitters.size();++j) {
    auto &emitter=s.emitters[j];
    auto *link=&emitter.state.first_particle;
    while(*link!=DatumPool::none) {
      auto p=i.particles.find(*link);if(p==i.particles.end())throw FormatError("muzzle particle chain lost allocation");
      auto &v=p->second;v.age=v.inverse_lifetime*seconds+v.age;
      if(v.age>1.f) {
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
    for(const auto &controller:movement.controllers)
      apply_particle_physics_controller(controller,emitter.state.first_particle,seconds,retail_gravity,1,
          i.sources(e,s,MuzzleEffectView{},t,seconds),*i.cache.shader_function_tables(),pool);
   }
   for(auto &[handle,e]:i.effects)if(t+1>e.tick && !e.event.clock.finished)i.event_step(e,seconds);
   for(auto effect=i.effects.begin();effect!=i.effects.end();) {
    auto &e=effect->second;
    // 1764B0/176E20 retains a retired system while any emitter owns live
    // particles. 175C00 ->176E70 ->24A7B0 releases its location/emitter
    // allocations; 17C9F0 unlinks it and1788B0 ->178F60 frees the effect.
    // This closed path has no child systems or detached part/audio owners.
    const bool live=std::any_of(e.systems.begin(),e.systems.end(),[](const auto &system) {
      return std::any_of(system.emitters.begin(),system.emitters.end(),[](const auto &emitter) {
        return emitter.state.particle_count!=0;
      });
    });
    if(e.slices.empty() && e.retiring && !live) {
      for(const auto &system:e.systems) {
        for(const auto &emitter:system.emitters)i.emitter_pool.release_unchecked(emitter.handle);
        i.location_pool.release_unchecked(system.location);
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
std::vector<ParticleDraw> NativeMuzzleEffects::draw(std::uint32_t weapon,const Marker &marker,
    const MuzzleEffectView &view,std::uint32_t tick,float fraction,float seconds) {
  auto &i=*impl_;std::vector<ParticleDraw> result;
  for(auto &[handle,e]:i.effects)if(e.weapon==weapon) {
    for(const auto &slice:e.slices)i.birth(e,marker,view,tick,seconds,slice);
    e.slices.clear();
    const auto &d=i.definition(e.definition);
    for(unsigned n=0;n<e.systems.size();++n) {
      auto &s=e.systems[n];const auto &a=d.systems[s.definition_index];
      ParticleRenderState state;state.system_identity=s.handle;state.location_identity=s.location;
      state.sources=i.sources(e,s,view,tick,seconds);state.first_person=true;
      for(const auto &emitter:s.emitters) {
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
      for_each_particle_render_input(a.definition,state,values,*i.cache.shader_function_tables(),
          [&](std::uint64_t,const ParticleSpriteInput &input,ParticleValueCache &cache,const ParticleValueSources &sources) {
        const auto sprite=particle_sprite_submission(a.sprite,input,cache,sources,*i.cache.shader_function_tables());
        ParticleDraw draw;draw.material=a.material;
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
  return result;
}
std::size_t NativeMuzzleEffects::particle_count() const {return impl_->particles.size();}
} // namespace halo2
