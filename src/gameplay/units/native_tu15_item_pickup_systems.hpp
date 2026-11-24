#pragma once
// Native TU1.5 item pickup, entered at 1538B0 (multiplayer engines unit): a
// player's unit touching an item.
//
//   item_pickup::touch   1538B0(player handle, object handle); RET 8
//
// A player (0x21C bytes in [4F54F4]) without a unit (+0x2C) does nothing.
// The item (an object of [4ECBD0], 12-byte entries, the address at +8) is
// refused when it rides a parent (+0x14) other than a machine or a crate
// (the parent's type, byte +3 of its entry, 7 or 0xB), when the unit owns it
// (+0x14C), or unless the unit's radius (+0x3C) plus a margin ([456918])
// reaches it: the squared radius above the squared distance between the two
// positions (+0x30). In a game of mode 2 (+8 of [4F3218]) C6920 (ESI the
// item) must also accept it.
//
// Then each of the unit's four weapons (+0x218), from the current one (the
// signed byte +0x212, none counted as 0) and around, is offered the item's
// ammunition: 104D30 (the weapon, the item, the unit, the player's signed word
// +0x28 and a count pushed) takes it, and with a positive count 197E9D (ECX
// the weapon, EAX its definition; the count and the player's word pushed)
// reports it. An item BCDF0 finds to be equipment (the mask 8) acts by its
// definition's ([4F0414], 16-byte entries) powerup kind (+0x12C): a grenade
// (6) goes to CF080 (ESI the unit, EDI the item pushed), and 197E11 (EAX the
// equipment's definition, the player's word pushed) reports it; any other
// powerup, when CF060 (EAX the unit) answers NONE and 13B360 does not
// answer, to 1549D0 (the player and the item pushed). A weapon (type 2) that
// CFC90 (the unit, the item and a byte pushed) offers, with the byte set and
// 153850 (EAX the player) agreeing, is handed to the unit by B3140 (EAX the
// unit; the item and 3 pushed) when 13B360 answers, otherwise asked for by
// E8E50 (EAX the unit, EDI a request: 0x14, the item and 3). After the
// ammunition, every touch not refused ends with 161320 (the player pushed).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_item_pickup.hpp) maps the role to the original instruction,
// owns the ABI of the children and leaves through the original's epilogue
// for each exit. The values the original keeps in callee-saved registers
// across the children are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_item_pickup {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t players=0x4f54f4u,objects=0x4ecbd0u,definitions=0x4f0414u,game=0x4f3218u,
    pickup_margin=0x456918u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,none=0xffffffffu,player_size=0x21cu,weapon_slots=4u,
    equipment_mask=8u,weapon_type_bit=4u,checked_mode=2u;
// An object's entry and a definition's.
namespace entry {inline constexpr std::uint32_t size=12u,type=3u,address=8u;}
namespace definition_entry {inline constexpr std::uint32_t size=16u,address=8u;}
// A parent the item may rest on: a machine or a crate.
namespace object_type {inline constexpr std::uint32_t machine=7u,crate=0xbu;}
namespace player {inline constexpr std::uint32_t index=0x28u,unit=0x2cu;}
namespace object {inline constexpr std::uint32_t definition=0u,parent=0x14u,x=0x30u,y=0x34u,z=0x38u,radius=0x3cu,owner=0x14cu,
    current_weapon=0x212u,weapons=0x218u;}
namespace game {inline constexpr std::uint32_t mode=8u;}
namespace equipment {inline constexpr std::uint32_t powerup=0x12cu;inline constexpr std::uint32_t no_powerup=0u,grenade=6u;}
// The request E8E50 is given: its kind, the item and its mode.
namespace request {inline constexpr std::uint32_t kind=0u,item=4u,mode=8u,size=0x20u;
    inline constexpr std::uint32_t pickup_kind=0x14u,pickup_mode=3u;}
inline constexpr std::uint32_t handed_mode=3u;
// The frame (offsets from the ESP after EBX, EDI, EBP and ESI are pushed):
// the ammunition's count, the item's entry offset (later the byte CFC90
// sets), the request, and the two arguments.
namespace local {inline constexpr std::uint32_t count=0x10u,offset=0x14u,taken=0x14u,request=0x18u,player=0x3cu,item=0x40u;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace item_pickup {
enum class Access {players_load,players_data,player_argument,unit_load,objects_load,objects_data,unit_address,item_argument,
    item_address,offset_store,parent_load,parent_objects,parent_type,owner_test,item_z,unit_z,item_x,unit_x,item_y,unit_y,
    unit_radius,pickup_margin,game_load,game_mode,mode_item,current_weapon,weapon_load,player_index,unit_handle,weapon_item,
    count_load,ammo_objects,ammo_objects_data,ammo_player_index,weapon_address,weapon_definition,equipment_item,
    equipment_definition,definitions_load,definition_address,powerup_kind,grenade_item,grenade_unit,grenade_player_index,
    grenade_definition,powerup_unit,powerup_item,powerup_player,item_reload,type_objects,type_objects_data,offset_load,item_type,
    weapon_unit,taken_load,taken_player,handing_unit,handed_player,request_kind_clear,request_item_clear,request_mode_clear,
    request_clear_c,request_clear_10,request_clear_14,request_clear_18,request_item_store,request_clear_1c,request_kind_store,
    request_mode_store,request_item_reload,notified_player};
// The original's exits: no unit (before EBP and ESI are saved), refused,
// and the two after 161320, each with its own epilogue.
enum class Exit {unitless,refused,notified,handed};
// The original's EBX (the player), EBP (the unit, later the equipment), ESI
// (the item, later each weapon) and EDI (the unit's handle, later the slot
// counter and then the item's handle).
struct Kept {std::uint32_t player_address,subject,item,counter;};

// Frame: see native_tu15_item_pickup.hpp.
template<class Memory,class Frame> Exit touch(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The player and its unit.
    const auto players_address=memory.load32(players_load,globals::players);
    const auto players=memory.load32(players_data,at(players_address,datum_data));
    frame.framing();
    frame.saving(1u);
    kept.player_address=frame.player_address(memory.load32(player_argument,frame.local(local::player)),players);
    frame.saving(2u);
    kept.counter=memory.load32(unit_load,at(kept.player_address,player::unit));
    if(!frame.named(kept.counter))return Exit::unitless;
    const auto objects_header=memory.load32(objects_load,globals::objects);
    const auto objects=memory.load32(objects_data,at(objects_header,datum_data));
    const auto unit_index=frame.unit_index(kept.counter);
    frame.saving(3u);
    kept.subject=memory.load32(unit_address,objects+entry::size*unit_index+entry::address);
    const auto item_offset=frame.item_offset(memory.load32(item_argument,frame.local(local::item)));
    frame.saving(4u);
    kept.item=memory.load32(item_address,objects+item_offset+entry::address);
    memory.store32(offset_store,frame.local(local::offset),item_offset);

    // Refused: on a parent other than a machine or a crate, owned by the unit, or out of reach.
    const auto parent=memory.load32(parent_load,at(kept.item,object::parent));
    frame.unrested();
    if(frame.named(parent)) {
        const auto parent_data=memory.load32(parent_objects,at(objects_header,datum_data));
        const auto type=memory.load8(parent_type,parent_data+entry::size*frame.parent_index(parent)+entry::type);
        if(!frame.resting(type,parent))return Exit::refused;
    }
    if(!frame.different(memory.load32(owner_test,at(kept.item,object::owner)),kept.counter))return Exit::refused;
    const auto item_z_value=memory.load32(item_z,at(kept.item,object::z));
    const auto dz=sse::subtract(item_z_value,memory.load32(unit_z,at(kept.subject,object::z)));
    const auto item_x_value=memory.load32(item_x,at(kept.item,object::x));
    const auto dx=sse::subtract(item_x_value,memory.load32(unit_x,at(kept.subject,object::x)));
    const auto item_y_value=memory.load32(item_y,at(kept.item,object::y));
    const auto dy=sse::subtract(item_y_value,memory.load32(unit_y,at(kept.subject,object::y)));
    const auto unit_radius_value=memory.load32(unit_radius,at(kept.subject,object::radius));
    const auto reach=sse::add(unit_radius_value,memory.load32(pickup_margin,globals::pickup_margin));
    const auto dz2=sse::multiply(dz,dz),dx2=sse::multiply(dx,dx),dy2=sse::multiply(dy,dy);
    const auto distance2=sse::add(sse::add(dz2,dx2),dy2);
    const auto reach2=sse::multiply(reach,reach);
    if(!frame.within(dy,dx2,reach,distance2,reach2))return Exit::refused;
    const auto game_address=memory.load32(game_load,globals::game);
    if(frame.checked(memory.load32(game_mode,at(game_address,game::mode)))) {
        kept.item=memory.load32(mode_item,frame.local(local::item));
        if(!frame.mode_accepts(kept))return Exit::refused;
    }

    // The ammunition, offered to each weapon from the current one.
    kept.counter=frame.first_slot();
    for(;;) {
        const auto current=signed_byte(memory.load8(current_weapon,at(kept.subject,object::current_weapon)));
        const auto slot=frame.slot(current,kept.counter);
        kept.item=memory.load32(weapon_load,at(kept.subject,object::weapons)+4u*slot);
        if(frame.named(kept.item)) {
            const auto index=signed_word(memory.load16(player_index,at(kept.player_address,player::index)));
            const auto unit=memory.load32(unit_handle,at(kept.player_address,player::unit));
            frame.count_pushed();
            const auto item=memory.load32(weapon_item,frame.local(local::item));
            if(frame.ammunition_taken(kept,index,unit,item)) {
                const auto count=memory.load32(count_load,frame.local(local::count));
                if(frame.positive_count(count)) {
                    const auto objects_now=memory.load32(ammo_objects,globals::objects);
                    frame.count_reported(count);
                    const auto data=memory.load32(ammo_objects_data,at(objects_now,datum_data));
                    const auto index_now=signed_word(memory.load16(ammo_player_index,at(kept.player_address,player::index)));
                    kept.item=frame.weapon_index(kept.item);
                    const auto weapon=memory.load32(weapon_address,data+entry::size*kept.item+entry::address);
                    const auto tag=memory.load32(weapon_definition,at(weapon,object::definition));
                    frame.ammunition_reported(kept,index_now,weapon,tag);
                }
                break;
            }
        }
        kept.counter=frame.next_slot(kept.counter);
        if(!frame.slots_left(kept.counter))break;
    }

    // Equipment: a grenade, or any other powerup.
    bool item_known=false;
    kept.subject=frame.equipment(kept,memory.load32(equipment_item,frame.local(local::item)));
    if(frame.found(kept.subject)) {
        const auto tag=memory.load32(equipment_definition,at(kept.subject,object::definition));
        const auto definitions=memory.load32(definitions_load,globals::definitions);
        const auto definition=memory.load32(definition_address,
            definitions+definition_entry::size*frame.definition_index(tag)+definition_entry::address);
        const auto kind=memory.load16(powerup_kind,at(definition,equipment::powerup));
        if(frame.grenade(kind)) {
            kept.counter=memory.load32(grenade_item,frame.local(local::item));
            kept.item=memory.load32(grenade_unit,at(kept.player_address,player::unit));
            if(frame.grenade_taken(kept)) {
                const auto index=signed_word(memory.load16(grenade_player_index,at(kept.player_address,player::index)));
                frame.grenade_reported(kept,index,memory.load32(grenade_definition,at(kept.subject,object::definition)));
            }
            item_known=true;
        } else if(frame.powerup(kind)) {
            if(!frame.named(frame.unit_powerup(kept,memory.load32(powerup_unit,at(kept.player_address,player::unit))))
               && !frame.deferred(kept,false)) {
                const auto item=memory.load32(powerup_item,frame.local(local::item));
                frame.powerup_taken(kept,item,memory.load32(powerup_player,frame.local(local::player)));
            }
        }
    }
    if(!item_known)kept.counter=memory.load32(item_reload,frame.local(local::item));

    // A weapon the unit may take.
    const auto objects_now=memory.load32(type_objects,globals::objects);
    const auto data=memory.load32(type_objects_data,at(objects_now,datum_data));
    const auto offset=memory.load32(offset_load,frame.local(local::offset));
    if(frame.weapon(memory.load8(item_type,data+offset+entry::type))) {
        const auto unit=memory.load32(weapon_unit,at(kept.player_address,player::unit));
        if(frame.weapon_offered(kept,unit) && frame.set(memory.load8(taken_load,frame.local(local::taken))) &&
           frame.player_agrees(kept,memory.load32(taken_player,frame.local(local::player)))) {
            const bool handing=frame.deferred(kept,true);
            memory.load32(handing_unit,at(kept.player_address,player::unit));
            if(handing) {
                frame.handed(kept);
                frame.notify(kept,memory.load32(handed_player,frame.local(local::player)),true);
                return Exit::handed;
            }
            // The request: cleared, then its kind, the item and its mode.
            const auto zero=frame.cleared();
            memory.store32(request_kind_clear,frame.local(local::request+request::kind),zero);
            memory.store32(request_item_clear,frame.local(local::request+request::item),zero);
            memory.store32(request_mode_clear,frame.local(local::request+request::mode),zero);
            memory.store32(request_clear_c,frame.local(local::request+0xcu),zero);
            memory.store32(request_clear_10,frame.local(local::request+0x10u),zero);
            memory.store32(request_clear_14,frame.local(local::request+0x14u),zero);
            memory.store32(request_clear_18,frame.local(local::request+0x18u),zero);
            memory.store32(request_item_store,frame.local(local::request+request::item),kept.counter);
            frame.request_address();
            memory.store32(request_clear_1c,frame.local(local::request+0x1cu),zero);
            memory.store32(request_kind_store,frame.local(local::request+request::kind),request::pickup_kind);
            memory.store16(request_mode_store,frame.local(local::request+request::mode),request::pickup_mode);
            frame.requested(kept);
            kept.counter=memory.load32(request_item_reload,frame.local(local::item));
        }
    }
    frame.notify(kept,memory.load32(notified_player,frame.local(local::player)),false);
    return Exit::notified;
}
} // namespace item_pickup
} // namespace halo2::native_item_pickup
