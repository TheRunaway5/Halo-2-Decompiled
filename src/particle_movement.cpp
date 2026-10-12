#include "halo2/particle_movement.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>

namespace halo2 {
namespace {
std::uint32_t add(std::uint32_t address,std::size_t offset) {
  if(offset>std::numeric_limits<std::uint32_t>::max()-address)
    throw FormatError("Particle movement pointer overflows source address");
  return address+std::uint32_t(offset);
}
float property_value(const ParticleProperty &p,const ParticleValueCache &cache,
    const ShaderFunctionTables &tables) {
  if(p.input<0||p.input>=17||p.range<0||p.range>=17||p.modifier_input>=17)
    throw FormatError("Particle movement property escapes source value cache");
  if(p.curve.size()<2)throw FormatError("Particle movement property has no source curve header");
  const unsigned type=std::to_integer<unsigned>(p.curve[0]);
  const bool ranged=std::to_integer<unsigned>(p.curve[1])&1u;
  const auto require=[&](unsigned index) {
    if(!(cache.evaluated&(1u<<index)))
      throw FormatError("Particle movement consumes a value without its source producer");
  };
  if(type!=1)require(unsigned(p.input));
  if(ranged)require(unsigned(p.range));
  if(p.modifier_input>=0&&(p.modifier==1||p.modifier==2))require(unsigned(p.modifier_input));
  float result=shader_function_value(p.curve,cache.values[p.input],cache.values[p.range],tables);
  if(p.modifier_input>=0) {
    if(p.modifier==1)result=cache.values[p.modifier_input]+result;
    else if(p.modifier==2)result=cache.values[p.modifier_input]*result;
  }
  return result;
}
float clamp_source(float v,float one) {
  // COMISS0,v / JBE and COMISSv,one / JBE retain unordered v.
  if(v<0.f)return 0.f;
  if(v>one)return one;
  return v;
}
}
ParticleMovementDefinition load_particle_movement_definition(
    std::uint32_t datum,const SourceTagMetadataProviders &provider) {
  if(!provider.definition||!provider.read)
    throw FormatError("Particle movement requires retained source metadata");
  const auto &tag=provider.definition(datum);
  if(tag.group()!="pmov")throw FormatError("Particle movement tag group differs");
  const Bytes root(tag.bytes_at(tag.source_address(),0x14));
  const auto count=root.u32(0xc),first=root.u32(0x10);
  if(count>65536)throw FormatError("Excessive source particle movement controllers");
  ParticleMovementDefinition out{datum,root.u32(8),{}};
  for(std::uint32_t i=0;i<count;++i) {
    const Bytes row(provider.read(add(first,std::size_t(i)*0x14),0x14));
    ParticleMovementController controller{row.u16(0),row.u32(0xc),row.u32(0x10),{}};
    const auto n=row.u32(4),parameters=row.u32(8);
    if(n>65536)throw FormatError("Excessive source particle movement parameters");
    for(std::uint32_t j=0;j<n;++j) {
      const Bytes parameter(provider.read(add(parameters,std::size_t(j)*0x14),0x14));
      const auto length=parameter.u32(0xc),pointer=parameter.u32(0x10);
      if(length>4096)throw FormatError("Excessive source particle movement curve");
      ParticleProperty p{std::int16_t(parameter.u16(4)),std::int16_t(parameter.u16(6)),
        std::int16_t(parameter.u16(0xa)),parameter.u16(8),{}};
      if(length) {
        const auto curve=provider.read(pointer,length);
        p.curve.assign(curve.begin(),curve.end());
      }
      controller.parameters.push_back(std::move(p));
    }
    out.controllers.push_back(std::move(controller));
  }
  return out;
}

ParticleMovementDefinition load_particle_movement_definition(
    const Cache &cache,std::uint32_t datum) {
  // Resolve only genuinely contained resident packets. Zero-sized alias tag
  // entries are not storage owners. All returned controller/curve bytes are
  // copied into the owned result before this local packet store is released.
  std::map<std::uint32_t,SourceTagMetadata> owned;
  const auto definition=[&](std::uint32_t d)->const SourceTagMetadata & {
    const auto found=owned.find(d);
    if(found!=owned.end())return found->second;
    const auto alias=cache.query_tag({.handle=TagHandle{d}});
    if(alias && alias->group=="pmov" && alias->address && !alias->size){
      // Stock caches deduplicate identical pmov definitions: an alias keeps
      // the real tag-table pointer and has size0. Only an exact same-address,
      // same-group owned payload may back it. Preserve the requested identity.
      const Tag* backing=nullptr;
      for(const auto&tag:cache.select_tags({.group=alias->type(), .address=alias->address, .minimum_size=1})){
        if(backing && backing->size!=tag.size)throw FormatError("Ambiguous particle movement alias backing");
        backing=&tag;
      }
      if(!backing)throw FormatError("Particle movement alias has no owned metadata backing");
      const auto packet=cache.load_source_tag_metadata(backing->datum);
      return owned.emplace(d,SourceTagMetadata(d,alias->group,alias->address,
          packet.bytes_at(packet.source_address(),packet.size()))).first->second;
    }
    return owned.emplace(d,cache.load_source_tag_metadata(d)).first->second;
  };
  SourceTagMetadataProviders provider;
  provider.definition=definition;
  provider.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
    const Tag *owner=nullptr;
    for(const auto &tag:cache.select_tags({.contains=TagAddressRange{address,extent}, .require_address=true, .minimum_size=1})) {
      if(owner&&extent)throw FormatError("Ambiguous particle movement metadata backing");
      owner=&tag;
    }
    if(!owner)throw FormatError("Particle movement pointer escapes actual cache payloads");
    return definition(owner->datum).bytes_at(address,extent);
  };
  return load_particle_movement_definition(datum,provider);
}

void apply_particle_physics_controller(const ParticleMovementController &controller,
    std::uint32_t head,float elapsed,float gravity,float one,
    const ParticleValueSources &source,const ShaderFunctionTables &tables,
    const ParticleMovementPool &pool,const ParticleMovementObserver &observer) {
  if(controller.type!=0||controller.parameters.size()<3)
    throw FormatError("Original physics0 requires its three authored parameter rows");
  if(!pool.particle||!pool.capacity)
    throw FormatError("Particle physics requires actual retained pool storage");
  // Original stack cache begins with evaluated0; unspecified unused words are
  // inert host storage. All semantically consumed entries must be requested.
  ParticleValueCache cache;
  auto values=source;values.location.reset();values.particle.reset();
  complete_particle_values(cache,controller.required_values&0x107f0u,values);
  std::array<float,3> parameters{};
  for(unsigned i=0;i<3;++i)
    if(controller.constant_parameters&(1u<<i))
      parameters[i]=property_value(controller.parameters[i],cache,tables);
  std::size_t traversed=0;
  while(head!=0xffffffffu) {
    if(++traversed>pool.capacity||std::uint16_t(head)>=pool.capacity)
      throw FormatError("Particle movement list escapes actual pool or cycles");
    const auto datum=head;
    auto &particle=pool.particle(datum);
    head=particle.next_particle; // Source loads next before eligibility.
    if((particle.flags&9u)||!(particle.age<=one))continue;
    cache.evaluated&=0xffff07f0u;
    values.particle=ParticleValueState{particle.age,particle.system_fraction,
      particle.phase,particle.position,particle.random};
    const auto requested=controller.required_values&0xf80fu;
    complete_particle_values(cache,requested,values);
    for(unsigned i=0;i<3;++i)
      if(!(controller.constant_parameters&(1u<<i)))
        parameters[i]=property_value(controller.parameters[i],cache,tables);
    const float drag=clamp_source(parameters[1]*elapsed,one);
    const float angular_drag=clamp_source(parameters[2]*elapsed,one);
    const float acceleration=gravity*parameters[0];
    const float gravity_step=acceleration*elapsed;
    const auto store=[&](float &field,float value,std::uint16_t offset,std::uint32_t instruction) {
      field=value;
      if(observer)observer({datum,instruction,offset,std::bit_cast<std::uint32_t>(value)});
    };
    store(particle.velocity.z,particle.velocity.z-gravity_step,0x30,0x2bf31c);
    const float factor=one-drag;
    store(particle.velocity.x,particle.velocity.x*factor,0x28,0x2bf331);
    store(particle.velocity.y,particle.velocity.y*factor,0x2c,0x2bf33f);
    store(particle.velocity.z,particle.velocity.z*factor,0x30,0x2bf34d);
    store(particle.rotation_rate,(one-angular_drag)*particle.rotation_rate,0x34,0x2bf35b);
  }
}
} // namespace halo2
