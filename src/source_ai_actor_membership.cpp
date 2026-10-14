#include "halo2/source_ai_actor_membership.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
std::uint32_t read(std::span<const std::uint8_t> bytes, unsigned offset,
                   unsigned width = 4) {
  if (offset > bytes.size() || width > bytes.size()-offset)
    throw std::invalid_argument("Actor membership requires the complete actual record");
  std::uint32_t value = 0;
  for (unsigned i = 0; i < width; ++i) value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned width = 4) {
  if (offset > bytes.size() || width > bytes.size()-offset)
    throw std::invalid_argument("Actor membership requires the complete actual record");
  for (unsigned i = 0; i < width; ++i) bytes[offset+i] = std::uint8_t(value >> (8*i));
}
void shape(const DatumPool &pool, int capacity, int stride) {
  if (pool.capacity() != capacity || pool.stride() != stride)
    throw std::invalid_argument("Actor membership received a different source bank");
}
std::span<std::uint8_t> row(DatumPool &pool, std::uint32_t datum) {
  const auto index = datum&0xffffu;
  if (index >= unsigned(pool.capacity()))
    throw std::invalid_argument("Actor membership index exceeds its actual bank");
  return pool.element_storage().subspan(index*pool.stride(),pool.stride());
}
std::span<std::uint8_t> actor_record(DatumPool &actors, std::uint32_t id) {
  shape(actors,256,0x888);
  const auto actor = actors.lookup(id);
  if (actor.empty())
    throw std::invalid_argument("Actor membership requires a live full actor datum");
  return actor;
}
void unlink(DatumPool &actors, std::span<std::uint8_t> first,
            unsigned first_offset, std::uint32_t id) {
  auto link = first;
  auto offset = first_offset;
  unsigned visited = 0;
  while (read(link,offset) != id) {
    const auto current = read(link,offset);
    if (current == none || ++visited > 256)
      throw std::invalid_argument("Actor membership chain is invalid");
    link = row(actors,current);
    offset = 0x20;
  }
  write(link,offset,read(row(actors,id),0x20));
}
std::uint32_t time(const SourceAiActorMembershipServices &services) {
  if (!services.control.game_tick)
    throw std::invalid_argument("Actor membership requires the live game clock");
  return std::uint32_t(services.control.game_tick());
}
}

void SourceAiActorBank::join_squad(std::uint32_t id, std::int16_t squad, bool keep_team,
                                  const SourceAiActorMembershipServices &services) {
  SourceAiActorMembership::join_squad(actors_,id,squad,keep_team,services);
}
bool SourceAiActorBank::activate_squad(std::int16_t squad,
                                      const SourceAiActorMembershipServices &services) {
  return SourceAiActorMembership::activate_squad(actors_,squad,services);
}
void SourceAiActorBank::add_to_global(std::uint32_t id,
                                     const SourceAiActorMembershipServices &services) {
  SourceAiActorMembership::add_to_global(actors_,id,services);
}
void SourceAiActorBank::remove_from_squad(std::uint32_t id, bool keep_count,
                                         const SourceAiActorMembershipServices &services) {
  SourceAiActorMembership::remove_from_squad(actors_,id,keep_count,services.squads,
                                            services.control.ai_globals);
}
void SourceAiActorBank::remove_from_global(std::uint32_t id,
                                          const SourceAiActorMembershipServices &services) {
  SourceAiActorMembership::remove_from_global(actors_,id,services.control.ai_globals);
}
void SourceAiActorMembership::remove_from_squad(
    DatumPool &actors, std::uint32_t id, bool keep_count, DatumPool &squads,
    std::span<std::uint8_t> globals) {
  if (!read(globals,1,1)) return;
  const auto actor = actor_record(actors,id);
  shape(squads,335,0x98);
  if (const auto squad_id = read(actor,0x30); squad_id != none) {
    const auto squad = row(squads,squad_id);
    unlink(actors,squad,0x68,id);
    if (!keep_count) write(squad,8,std::uint16_t(read(squad,8,2)-1),2);
    write(actor,0x20,none);
    write(actor,0x30,none);
    write(squad,3,read(squad,3,1)|1,1);
  }
}
void SourceAiActorMembership::remove_from_global(
    DatumPool &actors, std::uint32_t id, std::span<std::uint8_t> globals) {
  if (!read(globals,1,1)) return;
  const auto actor = actor_record(actors,id);
  unlink(actors,globals,0x14,id);
  write(actor,0xa,0,1);
  write(actor,0x20,none);
  write(actor,0xb,0,1);
}
void SourceAiActorMembership::add_to_global(
    DatumPool &actors, std::uint32_t id, const SourceAiActorMembershipServices &services) {
  const auto globals = services.control.ai_globals;
  if (!read(globals,1,1)) return;
  const auto actor = actor_record(actors,id);
  write(actor,0x20,read(globals,0x14));
  write(globals,0x14,id);
  write(actor,0xa,1,1);
  write(actor,0x14,time(services));
  SourceAiActorLifecycle::reset_location_references(actors,id,false,services.references);
}
bool SourceAiActorMembership::activate_squad(
    DatumPool &actors, std::int16_t squad_id, const SourceAiActorMembershipServices &services) {
  shape(services.squads,335,0x98);
  const auto squad = row(services.squads,std::uint16_t(squad_id));
  const auto globals = services.control.ai_globals;
  if (std::int16_t(read(squad,0x7e,2)) == services.bsp_index) {
    if (!(read(squad,2,1)&0x80)) {
      auto next = read(globals,1,1) ? read(squad,0x68) : none;
      unsigned visited = 0;
      while (read(globals,1,1) && next != none) {
        if (++visited > 256) throw std::invalid_argument("Squad activation actor chain is cyclic");
        const auto current = next;
        const auto actor = row(actors,current);
        next = read(actor,0x20);
        if (!SourceAiActorControl::set_active(actors,current,true,services.control)) {
          write(actor,0x34,std::uint32_t(std::int32_t(squad_id)));
          remove_from_squad(actors,current,false,services.squads,globals);
          add_to_global(actors,current,services);
        }
      }
    }
    //203BF0 sets squad state/time even when inactive AI skipped iteration.
    write(squad,2,read(squad,2,1)|0x80,1);
    write(squad,0x78,time(services));
  }
  return (read(squad,2,1)&0x80) != 0;
}
void SourceAiActorMembership::join_squad(
    DatumPool &actors, std::uint32_t id, std::int16_t squad_id, bool keep_team,
    const SourceAiActorMembershipServices &services) {
  const auto globals = services.control.ai_globals;
  if (!read(globals,1,1)) return;
  const auto actor = actor_record(actors,id);
  shape(services.squads,335,0x98);
  const auto squad = row(services.squads,std::uint16_t(squad_id));
  if (SourceAiActorControl::can_activate(actors,id,services.control)) {
    if (std::int16_t(read(squad,0x7e,2)) != services.bsp_index) {
      auto next = read(squad,0x68);
      unsigned visited = 0;
      while (read(globals,1,1) && next != none) {
        if (++visited > 256) throw std::invalid_argument("Squad BSP migration actor chain is cyclic");
        const auto current = next;
        const auto previous = row(actors,current);
        next = read(previous,0x20);
        write(previous,0x34,std::uint32_t(std::int32_t(squad_id)));
        remove_from_squad(actors,current,false,services.squads,globals);
        add_to_global(actors,current,services);
      }
      write(squad,0x7e,std::uint16_t(services.bsp_index),2);
    }
    if (read(actor,9,1) && !(read(squad,2,1)&0x80))
      activate_squad(actors,squad_id,services);
  }
  write(actor,0x34,none);
  write(actor,0x20,read(squad,0x68));
  write(squad,0x68,id);
  write(actor,0x30,std::uint32_t(std::int32_t(squad_id)));
  SourceAiActorControl::set_active(actors,id,(read(squad,2,1)&0x80) != 0,services.control);
  const auto team = std::int16_t(std::int8_t(read(squad,0x76,1)));
  if (std::int16_t(read(actor,0x24,2)) != team && !keep_team)
    SourceAiActorControl::set_team(actors,id,team,services.control);
  write(squad,8,std::uint16_t(read(squad,8,2)+1),2);
  write(squad,3,read(squad,3,1)|1,1);
}
} // namespace halo2
