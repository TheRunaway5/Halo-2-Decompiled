#pragma once
// Native TU1.5 item points, entered at 15D890 (multiplayer engines unit):
// each tick, a share of the multiplayer item points updates its object's
// respawn timer.
//
//   item_points::update   15D890(records); RET 4
//
// Nothing updates in game mode 4 (byte +0xC of [4F3218]). Each point (0x90
// bytes at the scenario's [4ECC20] +0x124, +0x120 of them) updates on the
// ticks when its index and the game time (+8 of [51DF54]) agree modulo the
// ticks per second (+2), and when 15CAB0 (EDX the current engine's answer to
// its first method, NONE without an engine; EDI the point's +4; 4 pushed)
// answers. Its record (8 bytes in the argument: the object, the timer and
// a count) tracks a vehicle (type 1) away from the point: while 15BA10 does
// not answer, beyond the distance [4500B8] and unoccupied (161AE0: EDI the
// object's position, XMM3 the respawn definition's +0x28 radius) the count
// grows, otherwise it restarts; past the point's limit (+0x10) the timer
// sets. An idle timer restarts (15D670, EAX the index, scaled by the game's
// respawn setting: +0x211 for the point's kind 0, +0x208 for kinds 1 to 7;
// 1 never, 2 half, 3 double) for a point without an object, a mode-1
// point whose object has the flag bit 2 (+0x10A), or a mode-0 point whose
// object (unless an item, types 2 to 4, with either low bit of +0x12C) is
// beyond 1.0 of it. A running timer counts down; reaching zero, the object
// is deleted (D9110) unless the point keeps it (flag bit 1 of +0): a weapon
// (type 2) stays, a vehicle stays while 15BA10 answers or 161AE0 finds it
// occupied (then the timer is 1 again); and 15D4F0 (the records and the
// index pushed) respawns the point.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_item_points.hpp) maps the role to the original instruction,
// owns the ABI of the children and leaves through the original's epilogue
// for each exit. The values the original keeps in callee-saved registers
// across the children are a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_item_points {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,tags=0x4ecc1cu,definitions=0x4f0414u,game_time=0x51df54u,
    scenario=0x4ecc20u,session=0x4f63b8u,engines=0x56dfa0u,objects=0x4ecbd0u,far_distance=0x4500b8u,near_distance=0x46a0d0u;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,none=0xffffffffu,spawn_kind=4u;
namespace game {inline constexpr std::uint32_t mode=0xcu,team_setting=0x208u,default_setting=0x211u;
    inline constexpr std::uint32_t pointless_mode=4u,team_mode=1u;}
namespace tags {inline constexpr std::uint32_t globals=0x16cu;}
namespace definition_entry {inline constexpr std::uint32_t size=16u,address=8u;}
namespace globals_definition {inline constexpr std::uint32_t block=0xcu,respawn=0x534u;}
namespace respawn {inline constexpr std::uint32_t radius=0x28u;}
namespace time {inline constexpr std::uint32_t rate=2u,now=8u;}
namespace session {inline constexpr std::uint32_t engine=0xc14u;}
namespace scenario {inline constexpr std::uint32_t point_count=0x120u,points=0x124u;}
namespace point {inline constexpr std::uint32_t size=0x90u,flags=0u,spawn=4u,limit=0x10u,mode=0x12u,kind=0x14u,x=0x40u,y=0x44u,z=0x48u;
    inline constexpr std::uint32_t kept_bit=2u;}
namespace record {inline constexpr std::uint32_t size=8u,object=0u,timer=4u,count=6u;}
namespace entry {inline constexpr std::uint32_t size=12u,address=8u;}
namespace object {inline constexpr std::uint32_t x=0x30u,y=0x34u,z=0x38u,type=0xaau,flags=0x10au,state=0x12cu;
    inline constexpr std::uint32_t moved_bit=4u,item_types=0x1cu;}
namespace type {inline constexpr std::uint32_t vehicle=1u,weapon=2u;}
// The respawn setting: never, half, double.
namespace setting {inline constexpr std::int32_t never=1,half=2,twice=3;inline constexpr std::uint32_t last_team_kind=7u;}
// The frame (offsets from the ESP after ESI, EBX, EBP and EDI are pushed):
// the object, the respawn definition, the index, the ticks per second, the
// timer's address, the scenario and the records.
namespace local {inline constexpr std::uint32_t object=0x10u,definition=0x14u,index=0x18u,rate=0x1cu,timer=0x20u,scenario=0x24u,
    records=0x2cu;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}
inline constexpr std::uint32_t signed_word(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(v))));}

namespace item_points {
enum class Access {
    game_load,game_mode,tags_load,globals_tag,definitions_load,globals_definition,globals_block,respawn_definition,time_load,
    tick_rate,definition_store,scenario_load,point_count,scenario_store,rate_store,index_store,rate_load,points_load,time_again,
    time_now,session_load,engine_index,engine_load,vtable_load,method_load,records_argument,record_handle,object_clear,objects_load,
    objects_data,object_load,object_type,object_store,point_limit,game_again,game_team_mode,object_reload,point_z,object_z,point_y,
    object_y,point_x,object_x,far_limit,definition_load,definition_radius,records_count,count_load,count_store,records_reset,
    count_reset,count_load_again,point_limit_again,object_after,records_flag,flag_store,object_skip,records_timer,timer_test,
    timer_pointer_store,handle_test,point_mode,object_flags,type_load,object_state,object_z_near,point_z_near,object_y_near,
    point_y_near,object_x_near,point_x_near,near_limit,point_scale,game_scale,game_default_scale,game_scale_again,game_team_scale,
    object_after_scale,timer_store,timer_load,timer_decrement_store,records_expired,expired_handle,point_flags,expired_type,
    expired_object,expired_definition,expired_radius,timer_pointer_load,timer_set_store,records_delete,delete_handle,
    records_respawn,index_load,index_next_store,scenario_next,point_count_next};
// The original's exits: game mode 4 (before ESI is saved), no points (ESI
// alone saved) and the loop's end.
enum class Exit {modeless,pointless,done};
// The original's EBX (each record's object), EBP (the point), ESI (the
// index) and EDI (the point's spawn block, later the timer's address).
struct Kept {std::uint32_t handle,point_address,index,timer;};

// The distance squared from b to a, in the original's order: (z, then y, then x).
struct Distance {std::uint32_t dx,dx2,dy2,d2;};
inline Distance distance(std::uint32_t az,std::uint32_t bz,std::uint32_t ay,std::uint32_t by,std::uint32_t ax,std::uint32_t bx) {
    const auto dz=sse::subtract(az,bz),dy=sse::subtract(ay,by),dx=sse::subtract(ax,bx);
    const auto dz2=sse::multiply(dz,dz),dy2=sse::multiply(dy,dy),dx2=sse::multiply(dx,dx);
    return {dx,dx2,dy2,sse::add(sse::add(dz2,dy2),dx2)};
}

// Frame: see native_tu15_item_points.hpp.
template<class Memory,class Frame> Exit update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // Nothing in game mode 4.
    const auto game_address=memory.load32(game_load,globals::game);
    const auto mode=memory.load8(game_mode,at(game_address,game::mode));
    frame.framing();
    if(frame.mode_is(mode,game::pointless_mode))return Exit::modeless;

    // The respawn definition, the ticks per second and the points.
    const auto tags_address=memory.load32(tags_load,globals::tags);
    const auto tag=memory.load32(globals_tag,at(tags_address,tags::globals));
    const auto definitions=memory.load32(definitions_load,globals::definitions);
    const auto tag_index=frame.tag_index(tag);
    const auto definition_address=memory.load32(globals_definition,definitions+definition_entry::size*tag_index+definition_entry::address);
    const auto block=memory.load32(globals_block,at(definition_address,globals_definition::block));
    const auto definition=memory.load32(respawn_definition,at(block,globals_definition::respawn));
    const auto time_address=memory.load32(time_load,globals::game_time);
    auto rate=signed_word(memory.load16(tick_rate,at(time_address,time::rate)));
    memory.store32(definition_store,frame.local(local::definition),definition);
    auto scenario_address=memory.load32(scenario_load,globals::scenario);
    const auto total=memory.load32(point_count,at(scenario_address,scenario::point_count));
    frame.saving(1u);
    kept.index=frame.first_point();
    const bool any=frame.above(total,kept.index);
    memory.store32(scenario_store,frame.local(local::scenario),scenario_address);
    memory.store32(rate_store,frame.local(local::rate),rate);
    memory.store32(index_store,frame.local(local::index),kept.index);
    if(!any)return Exit::pointless;
    frame.saving(2u);
    frame.saving(3u);
    frame.saving(4u);

    for(bool first=true;;first=false) {
        if(!first)rate=memory.load32(rate_load,frame.local(local::rate));
        const auto points=memory.load32(points_load,at(scenario_address,scenario::points));
        kept.point_address=frame.point_address(kept.index,points);
        const auto time_now_address=memory.load32(time_again,globals::game_time);
        const auto now=memory.load32(time_now,at(time_now_address,time::now));
        if(frame.this_tick(now,kept.index,rate)) {
            // The engine's answer, and 15CAB0's.
            const auto session_address=memory.load32(session_load,globals::session);
            const auto engine_slot=memory.load32(engine_index,at(session_address,session::engine));
            const auto engine=memory.load32(engine_load,globals::engines+4u*engine_slot);
            auto answer=frame.unanswered();
            if(frame.present(engine)) {
                const auto vtable=memory.load32(vtable_load,engine);
                answer=frame.engine_answer(kept,memory.load32(method_load,vtable));
            }
            if(frame.named(frame.spawn_answer(kept,answer))) {
                // A vehicle away from its point counts up; past the point's limit the timer sets.
                const auto records=memory.load32(records_argument,frame.local(local::records));
                kept.handle=memory.load32(record_handle,records+record::size*kept.index+record::object);
                auto object_address=frame.no_object();
                memory.store32(object_clear,frame.local(local::object),object_address);
                if(frame.named(kept.handle)) {
                    const auto object_slot=frame.object_index(kept.handle);
                    const auto objects_header=memory.load32(objects_load,globals::objects);
                    const auto objects=memory.load32(objects_data,at(objects_header,datum_data));
                    const auto found=memory.load32(object_load,objects+entry::size*object_slot+entry::address);
                    const bool vehicle=frame.byte_is(memory.load8(object_type,at(found,object::type)),type::vehicle);
                    memory.store32(object_store,frame.local(local::object),found);
                    bool counting=vehicle && frame.word_nonzero(memory.load16(point_limit,at(kept.point_address,point::limit)));
                    if(counting) {
                        const auto game_now=memory.load32(game_again,globals::game);
                        counting=!frame.byte_is(memory.load8(game_team_mode,at(game_now,game::team_setting)),game::team_mode);
                    }
                    if(counting) {
                        bool away=false;
                        if(!frame.idle(kept,false)) {
                            const auto o=memory.load32(object_reload,frame.local(local::object));
                            const auto pz=memory.load32(point_z,at(kept.point_address,point::z));
                            const auto oz=memory.load32(object_z,at(o,object::z));
                            const auto py=memory.load32(point_y,at(kept.point_address,point::y));
                            const auto oy=memory.load32(object_y,at(o,object::y));
                            const auto px=memory.load32(point_x,at(kept.point_address,point::x));
                            const auto ox=memory.load32(object_x,at(o,object::x));
                            frame.position(o);
                            const auto d=distance(pz,oz,py,oy,px,ox);
                            if(frame.beyond(d,memory.load32(far_limit,globals::far_distance),false)) {
                                const auto respawn_address=memory.load32(definition_load,frame.local(local::definition));
                                away=!frame.occupied(kept,memory.load32(definition_radius,at(respawn_address,respawn::radius)),false);
                            }
                        }
                        std::uint32_t count_address;
                        if(away) {
                            const auto records_now=memory.load32(records_count,frame.local(local::records));
                            count_address=records_now+record::size*kept.index+record::count;
                            memory.store16(count_store,count_address,frame.counted(memory.load16(count_load,count_address)));
                            frame.count_address(count_address);
                        } else {
                            const auto records_now=memory.load32(records_reset,frame.local(local::records));
                            count_address=frame.count_address(records_now+record::size*kept.index+record::count);
                            memory.store16(count_reset,count_address,0u);
                        }
                        const auto count=memory.load16(count_load_again,count_address);
                        const bool past=frame.word_above(count,memory.load16(point_limit_again,at(kept.point_address,point::limit)));
                        object_address=memory.load32(object_after,frame.local(local::object));
                        if(past) {
                            const auto records_now=memory.load32(records_flag,frame.local(local::records));
                            memory.store16(flag_store,records_now+record::size*kept.index+record::timer,1u);
                        }
                    } else object_address=memory.load32(object_skip,frame.local(local::object));
                }

                // An idle timer restarts when the point needs its object again.
                const auto records_now=memory.load32(records_timer,frame.local(local::records));
                const auto timer_value=memory.load16(timer_test,records_now+record::size*kept.index+record::timer);
                kept.timer=frame.timer_address(records_now+record::size*kept.index+record::timer);
                memory.store32(timer_pointer_store,frame.local(local::timer),kept.timer);
                if(!frame.word_nonzero(timer_value)) {
                    bool restart=!frame.named(memory.load32(handle_test,records_now+record::size*kept.index+record::object));
                    if(!restart) {
                        const auto point_mode=signed_word(memory.load16(Access::point_mode,at(kept.point_address,point::mode)));
                        const auto mode_kind=frame.mode_kind(point_mode);
                        if(mode_kind==1) restart=frame.moved(memory.load8(object_flags,at(object_address,object::flags)));
                        else if(mode_kind==0) {
                            // An item picked up or held restarts at once; any other object once it is away.
                            bool taken=false;
                            if(frame.item(memory.load8(type_load,at(object_address,object::type))))
                                taken=frame.item_taken(memory.load8(object_state,at(object_address,object::state)));
                            if(taken)restart=true;
                            else {
                                const auto oz=memory.load32(object_z_near,at(object_address,object::z));
                                const auto pz=memory.load32(point_z_near,at(kept.point_address,point::z));
                                const auto oy=memory.load32(object_y_near,at(object_address,object::y));
                                const auto py=memory.load32(point_y_near,at(kept.point_address,point::y));
                                const auto ox=memory.load32(object_x_near,at(object_address,object::x));
                                const auto px=memory.load32(point_x_near,at(kept.point_address,point::x));
                                const auto d=distance(oz,pz,oy,py,ox,px);
                                restart=frame.beyond(d,memory.load32(near_limit,globals::near_distance),true);
                            }
                        }
                    }
                    if(restart) {
                        auto time=frame.respawn_time(kept);
                        const auto kind=memory.load8(point_scale,at(kept.point_address,point::kind));
                        std::uint32_t chosen=0u;bool scaled=true;
                        if(frame.kind_default(kind)) {
                            const auto g=memory.load32(game_scale,globals::game);
                            chosen=signed_byte(memory.load8(game_default_scale,at(g,game::default_setting)));
                        } else if(frame.kind_team(kind)) {
                            const auto g=memory.load32(game_scale_again,globals::game);
                            chosen=signed_byte(memory.load8(game_team_scale,at(g,game::team_setting)));
                        } else scaled=false;
                        if(scaled)time=frame.scaled(time,chosen);
                        object_address=memory.load32(object_after_scale,frame.local(local::object));
                        memory.store16(timer_store,kept.timer,time);
                    }
                }

                // A running timer counts down; at zero the object goes and the point respawns.
                frame.timer_cleared();
                const auto running=memory.load16(timer_load,kept.timer);
                if(frame.word_positive(running)) {
                    const auto left=frame.counted_down(running);
                    const bool ended=!frame.word_tested(left);
                    memory.store16(timer_decrement_store,kept.timer,left);
                    if(ended) {
                        const auto records_end=memory.load32(records_expired,frame.local(local::records));
                        kept.handle=memory.load32(expired_handle,records_end+record::size*kept.index+record::object);
                        bool respawn=true,remove=false;
                        if(frame.named(kept.handle) && frame.kept(memory.load8(point_flags,at(kept.point_address,point::flags)))) {
                            const auto type_value=memory.load8(expired_type,at(object_address,object::type));
                            if(!frame.byte_is(type_value,type::weapon)) {
                                remove=true;
                                if(frame.byte_is(type_value,type::vehicle)) {
                                    bool held=frame.idle(kept,true);
                                    if(!held) {
                                        memory.load32(expired_object,frame.local(local::object));
                                        const auto respawn_address=memory.load32(expired_definition,frame.local(local::definition));
                                        held=frame.occupied(kept,memory.load32(expired_radius,at(respawn_address,respawn::radius)),true);
                                        if(held)kept.timer=memory.load32(timer_pointer_load,frame.local(local::timer));
                                    }
                                    if(held) {
                                        memory.store16(timer_set_store,kept.timer,1u);
                                        respawn=false;remove=false;
                                    }
                                }
                            }
                        }
                        if(remove) {
                            const auto records_delete_address=memory.load32(records_delete,frame.local(local::records));
                            frame.deleted(kept,memory.load32(delete_handle,records_delete_address+record::size*kept.index+record::object));
                        }
                        if(respawn)frame.respawned(kept,memory.load32(records_respawn,frame.local(local::records)));
                    }
                }
            }
        }
        // The next point.
        const auto index_value=memory.load32(index_load,frame.local(local::index));
        const auto next=frame.next_point(index_value);
        kept.index=frame.point_index(next);
        memory.store32(index_next_store,frame.local(local::index),next);
        scenario_address=memory.load32(scenario_next,frame.local(local::scenario));
        if(!frame.below(kept.index,memory.load32(point_count_next,at(scenario_address,scenario::point_count))))return Exit::done;
    }
}
} // namespace item_points
} // namespace halo2::native_item_points
