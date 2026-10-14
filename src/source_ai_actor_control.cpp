#include "halo2/source_ai_actor_control.hpp"
#include "halo2/source_unit_control.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset,
                   unsigned width = 4) {
  if (offset > bytes.size() || width > bytes.size()-offset)
    throw std::invalid_argument("Actor control requires its complete actual record");
  std::uint32_t value = 0;
  for (unsigned i = 0; i < width; ++i)
    value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned width = 4) {
  if (offset > bytes.size() || width > bytes.size()-offset)
    throw std::invalid_argument("Actor control requires its complete actual record");
  for (unsigned i = 0; i < width; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
void shape(const DatumPool &pool, unsigned capacity, unsigned stride) {
  if (pool.capacity() != int(capacity) || pool.stride() != int(stride))
    throw std::invalid_argument("Actor control received a different source bank");
}
std::span<std::uint8_t> row(DatumPool &pool, std::uint32_t datum) {
  const auto index = datum&0xffffu;
  if (index >= unsigned(pool.capacity()))
    throw std::invalid_argument("Actor control index exceeds its actual bank");
  return pool.element_storage().subspan(index*pool.stride(),pool.stride());
}
std::span<const std::uint8_t> actor_record(const DatumPool &actors, std::uint32_t id) {
  shape(actors,256,0x888);
  const auto actor = actors.lookup(id);
  if (actor.empty())
    throw std::invalid_argument("Actor control requires a live full actor datum");
  return actor;
}
template<class Objects>
std::uint32_t root(std::uint32_t id, const Objects &object) {
  std::uint32_t result = none;
  unsigned visited = 0;
  while (id != none) {
    if (++visited > 2048)
      throw std::invalid_argument("Actor control object parent chain is cyclic");
    result = id;
    id = read(object(id,0x18),0x14);
  }
  return result;
}
std::uint32_t ai_next(std::span<const std::uint8_t> body) {
  if (read(body,0x134))
    return none;
  const auto offset = std::int16_t(read(body,0x13a,2));
  if (offset < 0 || unsigned(offset) > body.size() || body.size()-offset < 0x10)
    throw std::invalid_argument("Actor control requires the actual object AI block");
  return read(body,unsigned(offset)+0xc);
}
}

bool SourceAiActorBank::can_activate(
    std::uint32_t id, const SourceAiActorControlServices &services) const {
  return SourceAiActorControl::can_activate(actors_,id,services);
}
bool SourceAiActorBank::set_active(
    std::uint32_t id, bool requested, const SourceAiActorControlServices &services) {
  return SourceAiActorControl::set_active(actors_,id,requested,services);
}
void SourceAiActorBank::set_team(
    std::uint32_t id, std::int16_t team, const SourceAiActorControlServices &services) {
  SourceAiActorControl::set_team(actors_,id,team,services);
}
bool SourceAiActorControl::can_activate(
    const DatumPool &actors, std::uint32_t id,
    const SourceAiActorControlServices &services) {
  const auto actor = actor_record(actors,id);
  shape(services.perceptions,32,0x34);
  const auto object = [&](std::uint32_t datum, unsigned minimum) {
    return services.objects.object(datum,minimum);
  };
  const auto allowed_root = [&](std::uint32_t datum) {
    const auto flags = read(object(root(datum,object),8),4);
    return !(flags & ((1u << 18)|(1u << 7)));
  };
  if (const auto unit = read(actor,0x18); unit != none)
    return allowed_root(unit);
  const auto perception = read(actor,0x1c);
  if (perception == none)
    return false;
  auto next = read(row(services.perceptions,perception),0x18);
  unsigned visited = 0;
  while (next != none) {
    if (++visited > 2048)
      throw std::invalid_argument("Actor control swarm chain is cyclic");
    const auto current = next;
    //1E13F0 still tests this object's root when its AI block is absent;
    // the missing block ends only subsequent traversal.
    next = ai_next(object(current,0x13c));
    if (allowed_root(current))
      return true;
  }
  return false;
}
bool SourceAiActorControl::set_active(
    DatumPool &actors, std::uint32_t id, bool requested,
    const SourceAiActorControlServices &services) {
  actor_record(actors,id);
  const auto actor = actors.lookup(id);
  shape(services.clumps,20,0x50);
  if (services.ai_globals.size() < 0x36c)
    throw std::invalid_argument("Actor control requires the actual AI globals");
  if (read(actor,9,1) == unsigned(requested))
    return true;
  if (requested) {
    if (!can_activate(actors,id,services))
      return false;
    if (const auto clump_id = read(actor,0x7c); clump_id != none) {
      const auto clump = row(services.clumps,clump_id);
      if (!read(clump,0x1c,1)) {
        write(clump,0x1c,1,1);
        write(clump,0x24,1,2);
      }
    }
    write(actor,9,1,1);
    write(services.ai_globals,0x36a,std::uint16_t(read(services.ai_globals,0x36a,2)+1),2);
  } else {
    if (!services.game_tick)
      throw std::invalid_argument("Actor deactivation requires the live game clock");
    write(actor,9,0,1);
    write(actor,0x10,std::uint32_t(services.game_tick()));
    write(services.ai_globals,0x36a,std::uint16_t(read(services.ai_globals,0x36a,2)-1),2);
  }
  return true;
}
void SourceAiActorControl::set_team(
    DatumPool &actors, std::uint32_t id, std::int16_t team,
    const SourceAiActorControlServices &services) {
  actor_record(actors,id);
  const auto actor = actors.lookup(id);
  write(actor,0x24,std::uint16_t(team),2);
  if (!read(actor,7,1)) {
    if (const auto unit = read(actor,0x18); unit != none)
      write(services.objects.object(unit,0x13a),0x138,std::uint16_t(team),2);
    return;
  }
  shape(services.perceptions,32,0x34);
  auto next = read(row(services.perceptions,read(actor,0x1c)),0x18);
  unsigned visited = 0;
  while (next != none) {
    if (++visited > 2048)
      throw std::invalid_argument("Actor control swarm team chain is cyclic");
    const auto body = services.objects.object(next,0x13c);
    if (read(body,0x134))
      break;
    write(body,0x12e,std::uint16_t(team),2);
    next = ai_next(body);
  }
}
} // namespace halo2
