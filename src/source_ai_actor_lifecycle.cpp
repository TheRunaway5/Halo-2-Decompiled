#include "halo2/source_ai_actor_lifecycle.hpp"
#include "halo2/source_ai_actor_membership.hpp"
#include "halo2/source_unit_control.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset,
                   unsigned count = 4) {
  if (offset > bytes.size() || count > bytes.size()-offset)
    throw std::invalid_argument("Actor lifecycle field exceeds its actual record");
  std::uint32_t value = 0;
  for (unsigned i = 0; i < count; ++i)
    value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned count = 4) {
  if (offset > bytes.size() || count > bytes.size()-offset)
    throw std::invalid_argument("Actor lifecycle field exceeds its actual record");
  for (unsigned i = 0; i < count; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
std::span<std::uint8_t> row(DatumPool &pool, std::uint32_t datum) {
  const auto index = datum & 0xffffu;
  if (index >= std::uint32_t(pool.capacity()))
    throw std::invalid_argument("Actor lifecycle index exceeds its actual pool");
  return pool.element_storage().subspan(index*pool.stride(), pool.stride());
}
void shape(const DatumPool &pool, int capacity, int stride) {
  if (pool.capacity() != capacity || pool.stride() != stride)
    throw std::invalid_argument("Actor lifecycle has the wrong source pool owner");
}
template<class F> const F &required(const F &function, const char *name) {
  if (!function)
    throw std::invalid_argument(name);
  return function;
}
const SourceAiActorSlotCallbacks &handler(
    std::span<const SourceAiActorSlotCallbacks> handlers, std::int16_t type) {
  if (type < 0 || std::size_t(type) >= handlers.size() || !handlers[type].handler)
    throw std::invalid_argument("Actor lifecycle slot has no original handler");
  return handlers[type];
}
std::span<std::uint8_t> slot_data(std::span<std::uint8_t> actor, unsigned index) {
  return index < 4 ? actor.subspan(0x90+index*0x40, 0x40)
                   : std::span<std::uint8_t>{};
}
template<class Objects, class Visit>
void visit_perception_objects(std::uint32_t perception,
                              const SourceAiActorLifecycleServices &services,
                              const Objects &object, const Visit &visit) {
  auto next = read(row(services.perceptions,perception),0x18);
  unsigned visited = 0;
  while (next != none) {
    if (++visited > 2048)
      throw std::invalid_argument("Actor lifecycle swarm object chain is cyclic");
    const auto body = object(next,0x13c);
    if (read(body,0x134))
      break;
    const auto offset = std::int16_t(read(body,0x13a,2));
    if (offset < 0 || unsigned(offset) > body.size() || body.size()-offset < 0x54)
      throw std::invalid_argument("Actor lifecycle needs the actual object AI block");
    const auto data = body.subspan(unsigned(offset));
    next = read(data,0xc);
    visit(data);
  }
}
template<class Objects>
void forget_observation(DatumPool &actors, std::uint32_t id,
                         std::uint32_t reference,
                         const SourceAiActorLifecycleServices &services,
                         const Objects &object) {
  auto actor = row(actors, id);
  for (const auto offset : {0x338u,0x32cu,0x344u,0x7e0u,0x3b4u})
    if (read(actor,offset) == reference)
      write(actor,offset,none);
  if (read(actor,0x368) == reference) {
    write(actor,0x368,none);
    write(actor,0x358,0,2);
  }
  if (read(actor,0x722,2) == 1 && read(actor,0x724) == reference) {
    write(actor,0x724,none);
    write(actor,0x722,0,2);
  }
  if (read(actor,0x4ac,2) == 7 && read(actor,0x4b8) == reference) {
    write(actor,0x4ac,0,2);
    write(actor,0x4e4,none);
  }
  for (const auto offset : {0x688u,0x6a0u,0x6b0u})
    if (read(actor,offset,2) == 1 && read(actor,offset+4) == reference)
      write(actor,offset+4,none);
  if (read(actor,7,1) && read(actor,0x1c) != none)
    visit_perception_objects(read(actor,0x1c),services,object,
        [&](std::span<std::uint8_t> data) {
          for (const auto offset : {0x10u,0x50u})
            if (read(data,offset) == reference)
              write(data,offset,none);
        });
  for (unsigned index = 0;
       std::int32_t(index) <= std::int16_t(read(actor,0x190,2)); ++index) {
    const auto &callback = handler(services.slot_handlers,std::int16_t(read(actor,0x90+index*0x40,2)));
    if (callback.forget)
      required(services.release_slot_reference,
               "Actor lifecycle needs the real slot-reference owner")(
          callback.forget,id,slot_data(actor,index),reference,actors);
  }
}
void retire_joint_participant(std::uint32_t joint_id, std::uint32_t actor_id,
                               const SourceAiActorLifecycleServices &services) {
  const auto joint = row(services.joint_commands,joint_id);
  bool remaining = false;
  for (unsigned index = 0; std::int32_t(index) < std::int16_t(read(joint,0xa,2)); ++index) {
    const auto offset = 0x10+index*12;
    const auto command_id = read(joint,offset+4);
    if (command_id == none)
      continue;
    const auto command = row(services.commands,command_id);
    if (read(joint,offset) == actor_id) {
      write(command,0x40,none);
      write(joint,offset,none);
      write(joint,offset+4,none);
    } else if (read(joint,offset) != none) {
      remaining = true;
      write(command,0x7e,1,1);
    }
  }
  if (!remaining) {
    if (const auto thread = read(joint,0xc); thread != none)
      services.script_threads.release_unchecked(thread);
    services.joint_commands.release_unchecked(joint_id);
  }
}
void stop_commands(DatumPool &actors, std::uint32_t id,
                     const SourceAiActorLifecycleServices &services) {
  const auto actor = row(actors,id);
  auto next = read(actor,0x858);
  unsigned visited = 0;
  while (next != none) {
    if (++visited > 40)
      throw std::invalid_argument("Actor lifecycle command chain is cyclic");
    const auto current = next;
    const auto command = row(services.commands,current);
    next = read(command,0x3c);
    if (const auto joint = read(command,0x40); joint != none)
      retire_joint_participant(joint,id,services);
    else if (const auto thread = read(command,0x38); thread != none)
      services.script_threads.release_unchecked(thread);
    auto link = actor;
    unsigned offset = 0x858, links = 0;
    while (read(link,offset) != none) {
      if (++links > 40)
        throw std::invalid_argument("Actor lifecycle command membership is cyclic");
      if (read(link,offset) == current) {
        write(link,offset,read(command,0x3c));
        break;
      }
      link = row(services.commands,read(link,offset));
      offset = 0x3c;
    }
    if (read(actor,0x85c) == current)
      write(actor,0x85c,none);
    services.commands.release_unchecked(current);
  }
}
void remove_clump(DatumPool &actors, std::uint32_t id,
                  const SourceAiActorLifecycleServices &services) {
  const auto actor = row(actors,id);
  const auto clump = row(services.clumps,read(actor,0x7c));
  auto link = clump;
  unsigned offset = 0x18, visited = 0;
  while (read(link,offset) != none) {
    if (++visited > 256)
      throw std::invalid_argument("Actor lifecycle clump chain is cyclic");
    const auto current = read(link,offset);
    const auto member = row(actors,current);
    if (current != id) {
      link = member;
      offset = 0x80;
      continue;
    }
    write(link,offset,read(member,0x80));
    write(member,0x7c,none);
    auto next = read(actor,0x58);
    unsigned references = 0;
    while (next != none) {
      if (++references > 1024)
        throw std::invalid_argument("Actor lifecycle clump references are cyclic");
      const auto node = row(services.prop_references,next);
      next = read(node,0x2c);
      const auto previous = read(node,0x38);
      if (previous == none)
        write(row(services.props,read(node,8)),0x18,read(node,0x34));
      else
        write(row(services.prop_references,previous),0x34,read(node,0x34));
      if (const auto following = read(node,0x34); following != none)
        write(row(services.prop_references,following),0x38,read(node,0x38));
      write(node,8,none);
      write(node,0x38,none);
      write(node,0x34,none);
    }
    write(clump,0x10,std::uint16_t(read(clump,0x10,2)-1),2);
    return;
  }
}
void clear_follow_reference(std::span<std::uint8_t> actor,
                             const SourceAiActorLocationReferenceServices &services) {
  if (read(actor,0x418) != services.empty_reference)
    required(services.release_follow_reference,
             "Actor lifecycle needs the real followed-reference registry")(
        read(actor,0x418));
  write(actor,0x418,services.empty_reference);
}
void reset_location_references(DatumPool &actors, std::uint32_t id, bool conditional,
                                const SourceAiActorLocationReferenceServices &services) {
  const auto actor = row(actors,id);
  clear_follow_reference(actor,services);
  const auto mode = read(actor,0x4ac,2);
  if (mode >= 4 && mode <= 6 && (!conditional || (read(actor,0x4ba,2)&0x8000))) {
    write(actor,0x50c,0,1);
    write(actor,0x5ac,none);
    write(actor,0x5b0,none,2);
    for (const auto offset : {0x5b4u,0x5b6u,0x4acu,0x504u})
      write(actor,offset,0,2);
  }
  for (unsigned index = 0; index < 4; ++index)
    if (!conditional || (read(actor,0x404+index*6,2)&0x8000))
      write(actor,0x402+index*6,services.empty_reference);
  for (unsigned index = 0;
       std::int32_t(index) <= std::int16_t(read(actor,0x190,2)); ++index) {
    const auto &callback = handler(services.slot_handlers,std::int16_t(read(actor,0x90+index*0x40,2)));
    if (callback.notify_location)
      required(services.notify_slot_location,
               "Actor lifecycle needs the real slot-location owner")(
          callback.notify_location,id,slot_data(actor,index),conditional,actors);
  }
}
void release_location(DatumPool &actors, std::uint32_t id,
                       const SourceAiActorLifecycleServices &services) {
  const auto actor = row(actors,id);
  const auto location = read(actor,0x3f4);
  reset_location_references(actors,id,true,source_ai_actor_location_reference_services(services));
  write(actor,0x3f4,none);
  const auto record = row(services.locations,location);
  const auto remaining = std::uint16_t(read(record,0xa,2)-1);
  write(record,0xa,remaining,2);
  if (!remaining)
    services.locations.release_unchecked(location);
}
}

void SourceAiActorBank::release_actor(
    std::uint32_t id, bool keep_squad_count,
    const SourceAiActorLifecycleServices &services) {
  SourceAiActorLifecycle::release(actors_,id,keep_squad_count,services);
}
void SourceAiActorBank::reset_location_references(
    std::uint32_t id, bool conditional, const SourceAiActorLocationReferenceServices &services) {
  SourceAiActorLifecycle::reset_location_references(actors_,id,conditional,services);
}
void SourceAiActorLifecycle::reset_location_references(
    DatumPool &actors, std::uint32_t id, bool conditional,
    const SourceAiActorLocationReferenceServices &services) {
  shape(actors,256,0x888);
  if (actors.lookup(id).empty() || services.slot_handlers.size() != 0x83)
    throw std::invalid_argument("Location reference reset requires actual actor/slot owners");
  halo2::reset_location_references(actors,id,conditional,services);
}
void SourceAiActorLifecycle::release(
    DatumPool &actors, std::uint32_t id, bool keep_squad_count,
    const SourceAiActorLifecycleServices &services) {
  shape(actors,256,0x888);
  shape(services.squad_records,335,0x98);
  shape(services.joint_states,20,0xbc);
  shape(services.props,256,0xc4);
  shape(services.prop_references,1024,0x3c);
  shape(services.tracking,100,0x124);
  shape(services.clumps,20,0x50);
  shape(services.locations,15,0x484);
  shape(services.perceptions,32,0x34);
  shape(services.commands,40,0xd4);
  shape(services.joint_commands,10,0x8c);
  shape(services.script_threads,320,0x418);
  if (services.ai_globals.size() < 0x36c ||
      services.flag3_statistics.size() < 0x24 || services.slot_handlers.size() != 0x83)
    throw std::invalid_argument("Actor lifecycle has incomplete source globals/handlers");
  auto actor = actors.lookup(id);
  if (actor.empty())
    throw std::invalid_argument("Actor lifecycle requires a live full actor datum");
  auto globals = services.ai_globals;
  const auto object = [&](std::uint32_t datum, unsigned minimum) {
    return services.units.object(datum,minimum);
  };
  for (unsigned index = 0;
       std::int32_t(index) <= std::int16_t(read(actor,0x190,2)); ++index) {
    const auto &callback = handler(services.slot_handlers,std::int16_t(read(actor,0x90+index*0x40,2)));
    if (callback.stop)
      required(services.stop_slot,"Actor lifecycle needs the real slot-stop owner")(
          callback.stop,id,slot_data(actor,index),actors);
  }
  write(actor,0x190,none,2);
  stop_commands(actors,id,services);
  for (unsigned index = 0; index < 4; ++index) {
    const auto offset = 0x194+index*12;
    if (std::int16_t(read(actor,offset,2)) == -1)
      continue;
    const auto member = std::int16_t(read(actor,offset+2,2));
    if (member < 0)
      throw std::invalid_argument("Actor lifecycle joint-state member is invalid");
    auto joint = row(services.joint_states,read(actor,offset+4));
    write(joint,unsigned(member)*12+4,none);
    write(joint,unsigned(member)*12+8,1,2);
    write(actor,offset,none,2);
  }
  if (read(actor,0xa,1))
    SourceAiActorMembership::remove_from_global(actors,id,globals);
  else
    SourceAiActorMembership::remove_from_squad(actors,id,keep_squad_count,services.squad_records,globals);
  if (read(actor,0x7c) != none)
    remove_clump(actors,id,services);
  auto next = read(actor,0x58);
  unsigned visited = 0;
  while (next != none) {
    if (++visited > 1024)
      throw std::invalid_argument("Actor lifecycle prop-reference chain is cyclic");
    const auto current = next;
    next = read(row(services.prop_references,current),0x2c);
    forget_observation(actors,id,current,services,object);
    const auto tracking = read(row(services.prop_references,current),0x14);
    if (tracking != none)
      services.tracking.release_unchecked(tracking);
    services.prop_references.release_unchecked(current);
  }
  write(actor,0x58,none);
  for (unsigned index = 0; index < unsigned(services.props.high_water()); ++index) {
    if (!(services.props.bitmap()[index/32]&(1u << (index%32))))
      continue;
    auto prop = row(services.props,index);
    if (read(prop,0x1c) == id)
      write(prop,0x1c,none);
  }
  //1E46C0 rereads live occupancy/active state after every callback.
  for (unsigned index = 0;
       read(globals,1,1) && index < unsigned(actors.high_water()); ++index) {
    if (!(actors.bitmap()[index/32]&(1u << (index%32))))
      continue;
    auto other = row(actors,index);
    if (other.data() == actor.data())
      continue;
    const auto other_id = (read(other,0,2) << 16)|index;
    if (read(other,0x3cc) == id)
      write(other,0x3cc,none);
    for (unsigned slot = 0;
         std::int32_t(slot) <= std::int16_t(read(other,0x190,2)); ++slot) {
      const auto &callback = handler(services.slot_handlers,std::int16_t(read(other,0x90+slot*0x40,2)));
      if (callback.release)
        required(services.release_slot_reference,
                 "Actor lifecycle needs the real other-actor slot owner")(
            callback.release,other_id,slot_data(other,slot),id,actors);
    }
  }
  // Complete2628F0's empty-reference branch: the registry owns actual removal.
  clear_follow_reference(actor,source_ai_actor_location_reference_services(services));
  if (read(actor,0x3f4) != none)
    release_location(actors,id,services);
  if (read(actor,7,1)) {
    if (const auto perception = read(actor,0x1c); perception != none) {
      write(actor,0x1c,none);
      write(row(services.perceptions,perception),4,none);
      visit_perception_objects(perception,services,object,
          [](std::span<std::uint8_t> data) { write(data,4,none); });
    }
  } else if (const auto unit_id = read(actor,0x18); unit_id != none) {
    const auto unit = services.units.object(unit_id,0xab);
    if (read(unit,0xaa,1) == 0)
      write(unit,0x3d8,read(actor,0x54));
    // Clear both links before the control owner can observe the retirement.
    write(unit,0x12c,none);
    write(actor,0x18,none);
    services.units.set_active(unit_id,false);
  }
  if (read(actor,0x54) != none) {
    const auto general = required(services.character_general,
        "Actor lifecycle needs canonical inherited character properties")(read(actor,0x54));
    if (general && (general->general_flags & 8)) {
      auto statistics = services.flag3_statistics;
      write(statistics,6,std::uint16_t(read(statistics,6,2)-1),2);
      if (keep_squad_count) {
        write(statistics,8,std::uint16_t(read(statistics,8,2)+1),2);
        write(statistics,0xc,std::uint32_t(required(services.game_tick,
            "Actor lifecycle needs the live game clock")()));
      }
      for (unsigned index = 0; index < 3; ++index)
        if (read(statistics,0x18+index*4) == id) {
          write(statistics,0x18+index*4,none);
          write(statistics,0x12+index*2,0,2);
        }
    }
  }
  if (read(actor,9,1)) {
    write(actor,9,0,1);
    write(actor,0x10,std::uint32_t(required(services.game_tick,
        "Actor lifecycle needs the live game clock")()));
    write(globals,0x36a,std::uint16_t(read(globals,0x36a,2)-1),2);
  }
  actors.release_unchecked(id);
}
} // namespace halo2
