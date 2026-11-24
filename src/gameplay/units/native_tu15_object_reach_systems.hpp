#pragma once
// Native TU1.5 object reach, entered at 154270 (combat action unit): a
// player's unit tries to act on an object within reach.
//
//   object_reach::reach_for   154270(EAX = object; player, result*)
//
// The player's unit (the players' datums, 0x21C bytes: +0x2C) reaches its
// radius (+0x3C) plus [46A178] when the object's definition (through the tag
// instances) has its word +0x290, plus [456918]. An object without its flag
// (+0x12C bit 0) and not parented to the unit (+0x14C) inside that reach
// (the squared distance: z, then x, then y) is offered to CFC90 (through the
// dispatch: the unit, the object, the hands record), whose answer names the
// hands (first byte 1, else third 2; second byte 4, else fourth 8). The
// action (type 7 when extended, else 1; the hands; the object; the result's
// two words) goes to 153E10 (EBX the action, the player pushed); in game
// kind 2, unless C6920 (EDI the unit) vetoes, the result takes the action's
// two words.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_object_reach.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_reach {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,players=0x4f54f4u,tag_instances=0x4f0414u,extension=0x46a178u,
    reach=0x456918u,game=0x4f3218u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,handle_mask=0xffffu;
namespace player {inline constexpr std::uint32_t size=0x21cu,unit=0x2cu;}
namespace object {inline constexpr std::uint32_t tag=0u,x=0x30u,y=0x34u,z=0x38u,radius=0x3cu,flags=0x12cu,parent=0x14cu;
    inline constexpr std::uint32_t unreachable=1u;}
namespace definition {inline constexpr std::uint32_t extended=0x290u;}
namespace game {inline constexpr std::uint32_t kind=8u;inline constexpr std::uint32_t acting_kind=2u;}
// The action on the stack (also CFC90's hands record): type, hands, object, the result's two words.
namespace action {inline constexpr std::uint32_t type=0u,hands=2u,object=4u,first=8u,second=0xcu;
    inline constexpr std::uint32_t plain=1u,extended=7u;}
namespace result {inline constexpr std::uint32_t first=0u,second=4u;}
namespace local {inline constexpr std::uint32_t action=0x10u,size=0x10u;}
namespace argument {inline constexpr unsigned player=0u,result=1u;}

namespace object_reach {
enum class Access {headers_load,datums_load,player_argument,players_load,player_datums,unit_handle_load,instances_load,extension_load,
    unit_load,object_load,tag_load,definition_load,extended_word,flags_load,radius_load,reach_load,parent_load,object_z,unit_z,object_x,
    unit_x,object_y,unit_y,hands_low,hands_high,result_argument,result_second,action_clear,object_clear,player_again,hands_store,
    result_first,type_store,object_store,first_store,second_store,game_load,kind_load,unit_again,result_again,action_first,
    action_second,result_first_store,result_second_store};
// The original's EBX (the extension flag, then the action), EBP (the player), ESI (the object) and EDI (the unit, then the result).
struct Kept {std::uint32_t extended_flag,player_address,object_handle,unit_address;};
// Frame: see native_tu15_object_reach.hpp.
template<class Memory,class Frame> void reach_for(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The player's unit and the object.
    const auto headers=memory.load32(headers_load,globals::object_headers);
    frame.framing();
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    frame.saving(0u);
    const auto player_handle=memory.load32(player_argument,frame.argument(argument::player));
    const auto player_offset=frame.player_offset(player_handle);
    kept.object_handle=frame.saving_object(kept);
    const auto players=memory.load32(players_load,globals::players);
    const auto player_datums_value=memory.load32(player_datums,at(players,datum_data));
    const auto unit_handle=memory.load32(unit_handle_load,player_offset+player_datums_value+player::unit);
    kept.player_address=frame.player_address(player_offset,player_datums_value);
    const auto instances=memory.load32(instances_load,globals::tag_instances);
    const auto extension=memory.load32(extension_load,globals::extension);
    const auto unit_index=frame.index(unit_handle,0u);
    frame.saving(2u);
    kept.unit_address=memory.load32(unit_load,datums+header_size*unit_index+header_address);
    const auto object_index=frame.index(kept.object_handle,1u);
    const auto object_address=memory.load32(object_load,datums+header_size*object_index+header_address);
    const auto tag=memory.load32(tag_load,at(object_address,object::tag));
    const auto definition_address=memory.load32(definition_load,frame.tag_offset(tag)+instances+tag_data);
    // The reach.
    const bool extended=frame.extended_tested(memory.load16(extended_word,at(definition_address,definition::extended)));
    const auto flags=memory.load8(flags_load,at(object_address,object::flags));
    frame.flags_tested(flags);
    const auto radius=memory.load32(radius_load,at(kept.unit_address,object::radius));
    const auto reach_value=memory.load32(reach_load,globals::reach);
    const auto reach=sse::add(sse::add(radius,extended?extension:0u),reach_value);
    frame.reached_out(reach);
    if(flags&object::unreachable){frame.restoring();return;}
    const auto parent=memory.load32(parent_load,at(object_address,object::parent));
    frame.parent_compared(parent,unit_handle);
    if(parent==unit_handle){frame.restoring();return;}
    // Inside the reach.
    const auto object_z_value=memory.load32(object_z,at(object_address,object::z));
    const auto dz=sse::subtract(object_z_value,memory.load32(unit_z,at(kept.unit_address,object::z)));
    const auto object_x_value=memory.load32(object_x,at(object_address,object::x));
    const auto dx=sse::subtract(object_x_value,memory.load32(unit_x,at(kept.unit_address,object::x)));
    const auto object_y_value=memory.load32(object_y,at(object_address,object::y));
    const auto dy=sse::subtract(object_y_value,memory.load32(unit_y,at(kept.unit_address,object::y)));
    const auto x2=sse::multiply(dx,dx);
    const auto distance=sse::add(sse::add(sse::multiply(dz,dz),x2),sse::multiply(dy,dy));
    const auto reach2=sse::multiply(reach,reach);
    frame.distance(dy,x2,distance,reach2);
    if(sse::not_above(reach2,distance)){frame.restoring();return;}
    // The hands, and the action.
    if(!frame.hands_found(unit_handle,kept)){frame.restoring();return;}
    const auto pair_low=memory.load32(hands_low,frame.local(local::action));
    const auto pair_high=memory.load16(hands_high,frame.local(local::action+2u));
    const auto hands=frame.hands(pair_low,pair_high);
    const auto result_address=memory.load32(result_argument,frame.argument(argument::result));
    const auto result_second_value=memory.load32(result_second,at(result_address,result::second));
    frame.negated();
    memory.store32(action_clear,frame.local(local::action),0u);
    memory.store32(object_clear,frame.local(local::action+action::object),0u);
    const auto player_now=memory.load32(player_again,frame.argument(argument::player));
    memory.store8(hands_store,frame.local(local::action+action::hands),hands);
    const auto result_first_value=memory.load32(result_first,at(result_address,result::first));
    frame.pass(player_now);
    memory.store16(type_store,frame.local(local::action+action::type),frame.action_type());
    frame.action_address();
    memory.store32(object_store,frame.local(local::action+action::object),kept.object_handle);
    memory.store32(first_store,frame.local(local::action+action::first),result_first_value);
    memory.store32(second_store,frame.local(local::action+action::second),result_second_value);
    if(!frame.acted(kept)){frame.restoring();return;}
    // In game kind 2, unless vetoed, the result.
    const auto game_address=memory.load32(game_load,globals::game);
    const auto kind=memory.load32(kind_load,at(game_address,game::kind));
    frame.kind_compared(kind);
    if(kind!=game::acting_kind){frame.restoring();return;}
    kept.unit_address=memory.load32(unit_again,at(kept.player_address,player::unit));
    if(frame.vetoed(kept)){frame.restoring();return;}
    const auto result_now=memory.load32(result_again,frame.argument(argument::result));
    const auto first=memory.load32(action_first,frame.local(local::action+action::first));
    const auto second=memory.load32(action_second,frame.local(local::action+action::second));
    memory.store32(result_first_store,at(result_now,result::first),first);
    memory.store32(result_second_store,at(result_now,result::second),second);
    frame.restoring();
}
} // namespace object_reach
} // namespace halo2::native_object_reach
