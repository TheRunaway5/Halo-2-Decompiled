#pragma once
// Native TU1.5 players facing, entered at 164750 (multiplayer engines
// unit): whether any player stands near a point and faces it.
//
//   players_facing::any   164750(the object's position from BBE00) -> AL
//
// BBE00 (through the dispatch) answers the point. Each player (1A8F10,
// through the dispatch, iterates the players' data array [4F54F4]) has its
// unit (+0x2C; the object headers [4ECBD0]) aim along +0x88 and its position
// from BBE00. A unit whose horizontal distance to the point (each component
// the point's less the unit's, y squared plus x squared) is inside
// [4500B8], whose horizontal aim is longer than [46A19C], and whose angle to
// the point (11F7E0, through the dispatch: the offset and the aim, both
// flat) is below [46A1EC] answers 1; otherwise the answer is BL as the
// children leave it (cleared on entry).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_players_facing.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_players_facing {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u,object_headers=0x4ecbd0u,range=0x4500b8u,shortest=0x46a19cu,
    facing=0x46a1ecu;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,handle_mask=0xffffu,none=0xffffffffu;
namespace player {inline constexpr std::uint32_t unit=0x2cu;}
namespace object {inline constexpr std::uint32_t aim=0x88u;}
namespace iterator {inline constexpr std::uint32_t datum=0u,array=4u,index=8u,absolute=0xcu;}
// The frame below EBX and ESI: the aim, the offset, the point, the unit's position, the iterator.
namespace local {inline constexpr std::uint32_t aim=8u,offset=0x14u,point=0x20u,position=0x2cu,iterator=0x38u,size=0x40u;}
inline constexpr std::uint32_t x=0u,y=4u,z=8u;

namespace players_facing {
enum class Access {players_load,iterator_array,iterator_absolute,iterator_index,headers_load,offset_flat,datum_load,unit_load,
    datums_load,unit_address,aim_x,aim_x_store,aim_y,aim_z,aim_y_store,aim_z_store,point_y,position_y,point_x,position_x,aim_flat,
    offset_y_store,offset_x_store,range_load,aim_y_again,aim_x_again,shortest_load,facing_load};
// The original's EBX (the answer when none faces) and EDI (the object headers) across the children.
struct Kept {std::uint32_t answer,headers;};
// Frame: framing(); local(offset) below EBX and ESI (above EDI once saved); point_found(kept) [BBE00]; iterated(); next(kept)
// [1A8F10]; looping(); indexed(handle); aimed(unit); positioned(kept) [BBE00]; distance(...); aim_length(...); angle(kept)
// [11F7E0]; facing_compared(facing, angle); restoring(n).
template<class Memory,class Frame> bool any(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The point, and the players' iterator.
    frame.framing();
    frame.point_found(kept);
    const auto players=memory.load32(players_load,globals::players);
    memory.store32(iterator_array,frame.local(local::iterator+iterator::array),players);
    frame.iterated();
    memory.store32(iterator_absolute,frame.local(local::iterator+iterator::absolute),none);
    memory.store32(iterator_index,frame.local(local::iterator+iterator::index),none);
    if(!frame.next(kept)){frame.restoring(0u);return false;}
    frame.looping();
    kept.headers=memory.load32(headers_load,globals::object_headers);
    memory.store32(offset_flat,frame.local(local::offset+z),0u);
    for(;;) {
        // The unit's aim (flat) and position.
        const auto datum=memory.load32(datum_load,frame.local(local::iterator+iterator::datum));
        const auto unit_handle=memory.load32(unit_load,at(datum,player::unit));
        const auto datums=memory.load32(datums_load,at(kept.headers,datum_data));
        const auto unit_address_value=memory.load32(unit_address,datums+header_size*frame.indexed(unit_handle)+header_address);
        const auto aim_address=frame.aimed(unit_address_value);
        const auto ax=memory.load32(aim_x,aim_address+x);
        memory.store32(aim_x_store,frame.local(local::aim+x),ax);
        const auto ay=memory.load32(aim_y,aim_address+y);
        const auto az=memory.load32(aim_z,aim_address+z);
        frame.position_address();
        memory.store32(aim_y_store,frame.local(local::aim+y),ay);
        memory.store32(aim_z_store,frame.local(local::aim+z),az);
        frame.positioned(kept);
        // The horizontal offset to the point, and its length against the range.
        const auto point_y_value=memory.load32(point_y,frame.local(local::point+y));
        const auto dy=sse::subtract(point_y_value,memory.load32(position_y,frame.local(local::position+y)));
        const auto point_x_value=memory.load32(point_x,frame.local(local::point+x));
        const auto dx=sse::subtract(point_x_value,memory.load32(position_x,frame.local(local::position+x)));
        memory.store32(aim_flat,frame.local(local::aim+z),0u);
        const auto dy2=sse::multiply(dy,dy);
        memory.store32(offset_y_store,frame.local(local::offset+y),dy);
        const auto dx2=sse::multiply(dx,dx);
        memory.store32(offset_x_store,frame.local(local::offset+x),dx);
        const auto range=memory.load32(range_load,globals::range);
        const auto distance=sse::add(dy2,dx2);
        frame.distance(dy2,dx2,range,distance);
        if(!sse::not_above(range,distance)) {
            // The aim's horizontal length, then the angle to the point.
            const auto aim_y_value=memory.load32(aim_y_again,frame.local(local::aim+y));
            const auto ay2=sse::multiply(aim_y_value,aim_y_value);
            const auto aim_x_value=memory.load32(aim_x_again,frame.local(local::aim+x));
            const auto ax2=sse::multiply(aim_x_value,aim_x_value);
            const auto aim_length=sse::add(ay2,ax2);
            const auto shortest=memory.load32(shortest_load,globals::shortest);
            frame.aim_length(aim_x_value,ax2,aim_length,shortest);
            if(!sse::not_above(aim_length,shortest)) {
                const auto angle=frame.angle(kept);
                const auto facing=memory.load32(facing_load,globals::facing);
                frame.facing_compared(facing,angle);
                if(!sse::not_above(facing,angle)){frame.restoring(2u);return true;}
            }
        }
        if(!frame.next(kept))break;
    }
    frame.restoring(1u);
    return false;
}
} // namespace players_facing
} // namespace halo2::native_players_facing
