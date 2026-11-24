#pragma once
// Native TU1.5 weapon targeting search, entered at 1AE090 (combat action
// unit): the targets an aim ray can reach.
//
//   aim_search::find_targets   1AE090
//
//   1. The search's widest angle and farthest range: the larger of pair 0's
//      and of the larger of pairs 3 and 1 (pair 2 is not read here), as
//      COMISS/JBE picks them (an unordered comparison picks the second, read
//      again). Unless both are positive there are no targets.
//   2. The cone: the x87 sine and cosine of the angle (FLD m32, FSIN/FCOS,
//      FSTP m32 on the live control word) are the cone's width and spread.
//   3. The aim cluster flood (14D200) lists the clusters the cone reaches
//      (up to 512), and the cluster object query (BD460) lists the
//      collideable objects they hold (up to 2048), marking each with the
//      query's stamp.
//   4. Every player's unit is added as its outermost parent object, unless
//      that object is already marked; the list has no bound here.
//   5. Each listed object's chain is walked by the aim target chain (1AE2B0)
//      until the target count reaches 64.
// The result is the target count.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_aim_search.hpp) maps the role to the original instruction
// and owns the ABI of the five children (the stack probe 32A660, 14D200,
// BD460, the data array iterator 16EA60 and 1AE2B0). The arguments and the
// 0x2410-byte frame (the cluster and object lists, the angle, range, sine
// and cosine, the object count) are read and written as memory through
// Frame::local, in the original's order; the original reads its locals and
// arguments again for each child call.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses in the original order and width, including the
//     search fields read again by the comparisons;
//   - COMISS for every comparison (a quiet NaN raises invalid) on the live
//     MXCSR, and the x87 functions and single-precision stores on the live
//     control word, with the logical x87 status the original leaves;
//   - 32-bit address arithmetic: a player record is data + index * size
//     (zero ends the walk), an object list slot is the list plus count * 4;
//   - the values the original keeps in callee-saved registers across the
//     children are a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_cluster_objects_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace halo2::native_aim_search {
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;

// ------------------------------------------------------------- x87 functions
namespace x87 {
// FLD m32; FSIN or FCOS; FSTP m32 on the live control word: the single
// result, and the status words right after the function and after the store
// (the original keeps C1/C2 of the first and C1 of the second).
struct Rounded {std::uint32_t value;std::uint16_t function_status,store_status;};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
inline Rounded sine(std::uint32_t angle) {
    Rounded r{};
    asm volatile("flds %3\n\tfsin\n\tfnstsw %1\n\tfstps %0\n\tfnstsw %2"
        :"=m"(r.value),"=m"(r.function_status),"=m"(r.store_status):"m"(angle):"st");
    return r;
}
inline Rounded cosine(std::uint32_t angle) {
    Rounded r{};
    asm volatile("flds %3\n\tfcos\n\tfnstsw %1\n\tfstps %0\n\tfnstsw %2"
        :"=m"(r.value),"=m"(r.function_status),"=m"(r.store_status):"m"(angle):"st");
    return r;
}
#else
inline constexpr bool available=false;
inline Rounded sine(std::uint32_t){throw std::runtime_error("Required TU1.5 FSIN backend unavailable");}
inline Rounded cosine(std::uint32_t){throw std::runtime_error("Required TU1.5 FCOS backend unavailable");}
#endif
} // namespace x87

// ------------------------------------------------------------- game layouts
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u;          // DataArray* of player records
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of object headers
using native_cluster_objects::globals::query_active;      // byte: 1 while a query runs
using native_cluster_objects::globals::query_stamp;       // the current query's stamp
using native_cluster_objects::globals::query_marks;       // one stamp per object index
}
// The search's angle/range pairs; the game meaning of each pair is not
// established.
struct AngleRange {float angle,range;};
struct AimSearch {AngleRange pairs[4];};
static_assert(sizeof(AimSearch)==0x20 && offsetof(AimSearch,pairs[3].range)==0x1c);
// The DataArray fields read here.
inline constexpr std::uint32_t datum_size=0x24u,datum_data=0x44u;
inline constexpr std::uint32_t player_unit=0x2cu;           // a player record's unit (object handle)
inline constexpr std::uint32_t header_data=8u,header_size=12u;   // an object header's data, the header's size
inline constexpr std::uint32_t object_parent=0x14u;         // an object's parent (object handle)
inline constexpr std::uint32_t cluster_capacity=0x200u,object_capacity=0x800u;
inline constexpr std::uint32_t target_limit=64u;

namespace aim_search {
enum class Local {
    search,            // AimSearch*; also passed to 1AE2B0
    start_cluster,     // the aim cluster flood's start (DX)
    point,             // Vector3*: where the aim ray starts
    direction,         // Vector3*: the aim ray's direction
    test_first,        // passed to 1AE2B0; the game meaning is not established
    test_second,       // passed to 1AE2B0; the game meaning is not established
    add_context,       // passed to 1AE2B0; the game meaning is not established
    angle,             // the widest angle; then the object count
    range,             // the farthest range
    cosine,            // the cone's spread
    sine,              // the cone's width
    clusters,          // int16[512]: the flooded clusters
    objects            // uint32[2048]: the objects to walk
};
enum class Access {
    search_argument,
    // the farthest range: pair 3, pair 1, then pair 0
    range_last,range_middle,range_middle_again,range_first,range_last_again,range_middle_compare,range_middle_third,
    // the widest angle
    angle_last,angle_middle,range_store,angle_middle_again,angle_first,angle_last_again,angle_middle_compare,
    angle_middle_third,angle_store,
    // the cone and the flood
    angle_for_sine,range_for_flood,direction_for_flood,sine_store,angle_for_cosine,sine_for_flood,point_for_flood,
    start_for_flood,cosine_store,spread_for_flood,
    // the players' units
    players_table,count_store,active_set,player_size,player_data,unit,headers,header_rows,object_data,parent,
    stamp,root_mark,root_mark_store,count_read,object_append,count_update,
    // the chains
    count_check,active_clear,spread_for_chains,context_for_chain,second_for_chain,first_for_chain,width_for_chain,
    range_for_chain,direction_for_chain,point_for_chain,object_for_chain,count_bound
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t search;     // EBP
    std::uint32_t targets;    // EDI: the target count
    std::uint32_t players;    // ESI during the player walk: the player table
    std::uint32_t position;   // ESI during the chain walk: the object list position
};

// The larger of a pair's field and the larger of two other pairs' fields,
// as the original picks it (see 1). between() runs right after the first
// comparison, before its branch.
struct WidestRoles {Access last,middle,middle_again,first,last_again,middle_compare,middle_third;};
template<class Memory,class Between> std::uint32_t widest(Memory &memory,std::uint32_t search,std::uint32_t field,
                                                          const WidestRoles &roles,Between between) {
    const auto pair_field=[&](unsigned pair){return at(search,offsetof(AimSearch,pairs)+pair*sizeof(AngleRange)+field);};
    auto larger=memory.load32(roles.last,pair_field(3));
    const bool last_above=!sse::not_above(larger,memory.load32(roles.middle,pair_field(1)));
    between();
    if(!last_above)larger=memory.load32(roles.middle_again,pair_field(1));
    const auto first=memory.load32(roles.first,pair_field(0));
    if(!sse::not_above(first,larger))return first;
    const auto last=memory.load32(roles.last_again,pair_field(3));
    if(!sse::not_above(last,memory.load32(roles.middle_compare,pair_field(1))))return last;
    return memory.load32(roles.middle_third,pair_field(1));
}

// Frame:
//   local(Local)                 an argument's or local's address, as the
//                                original addresses it now
//   limits(range, angle)         the positive checks ran (XMM0-XMM2, flags)
//   rounded(angle, Rounded)      an x87 function of the loaded angle and its
//                                store ran
//   flood(), query()             14D200 and BD460 with the loaded values;
//                                each returns AX
//   next_player(kept)            16EA60 from the previous player; returns
//                                the next index or NONE
//   chain(kept)                  1AE2B0 with the loaded values; returns the
//                                target count
// The child hooks refresh kept from the registers the child returned.
enum class Exit {closed,no_objects,walked};
struct Result {Exit exit;std::uint32_t targets;};
template<class Memory,class Frame> Result find_targets(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Kept kept{};
    kept.search=memory.load32(search_argument,local(Local::search));
    // 1. The farthest range and the widest angle.
    const auto range=widest(memory,kept.search,offsetof(AngleRange,range),
        {range_last,range_middle,range_middle_again,range_first,range_last_again,range_middle_compare,range_middle_third},
        []{});
    const auto angle=widest(memory,kept.search,offsetof(AngleRange,angle),
        {angle_last,angle_middle,angle_middle_again,angle_first,angle_last_again,angle_middle_compare,angle_middle_third},
        [&]{memory.store32(range_store,local(Local::range),range);});
    const bool reaches=!sse::not_above(range,0u);
    memory.store32(angle_store,local(Local::angle),angle);
    const bool opens=reaches && !sse::not_above(angle,0u);
    frame.limits(range,angle,reaches);
    if(!opens)return {Exit::closed,0};
    // 2. The cone, and 3. the flood and the query.
    const auto sine_angle=memory.load32(angle_for_sine,local(Local::angle));
    const auto sine=x87::sine(sine_angle);
    (void)memory.load32(range_for_flood,local(Local::range));
    (void)memory.load32(direction_for_flood,local(Local::direction));
    frame.rounded(sine_angle,sine);
    memory.store32(sine_store,local(Local::sine),sine.value);
    const auto cosine_angle=memory.load32(angle_for_cosine,local(Local::angle));
    const auto cosine=x87::cosine(cosine_angle);
    (void)memory.load32(sine_for_flood,local(Local::sine));
    (void)memory.load32(point_for_flood,local(Local::point));
    (void)memory.load32(start_for_flood,local(Local::start_cluster));
    frame.rounded(cosine_angle,cosine);
    memory.store32(cosine_store,local(Local::cosine),cosine.value);
    (void)memory.load32(spread_for_flood,local(Local::cosine));
    const auto reached=frame.flood(kept);
    const auto found=frame.query(kept,widen16(reached));
    // 4. The players' units, as their outermost parents.
    kept.players=memory.load32(players_table,globals::players);
    memory.store32(count_store,local(Local::angle),widen16(found));
    memory.store8(active_set,globals::query_active,1);
    for(;;) {
        const auto player=frame.next_player(kept);
        if(player==none)break;
        const auto size=memory.load32(player_size,at(kept.players,datum_size));
        const auto record=size*player+memory.load32(player_data,at(kept.players,datum_data));
        if(record==0)break;
        auto object=memory.load32(unit,at(record,player_unit));
        if(object==none)continue;
        const auto rows=memory.load32(header_rows,at(memory.load32(headers,globals::object_headers),datum_data));
        std::uint32_t root;
        do {
            root=object;
            object=memory.load32(parent,at(memory.load32(object_data,at(rows+(root&0xffffu)*header_size,header_data)),
                                           object_parent));
        } while(object!=none);
        const auto stamp_now=memory.load32(stamp,globals::query_stamp);
        const auto mark=element(globals::query_marks,root&0xffffu,4);
        if(memory.load32(root_mark,mark)==stamp_now)continue;
        memory.store32(root_mark_store,mark,stamp_now);
        const auto count=memory.load32(count_read,local(Local::angle));
        memory.store32(object_append,element(local(Local::objects),count,4),root);
        memory.store32(count_update,local(Local::angle),count+1u);
    }
    const auto count=memory.load32(count_check,local(Local::angle));
    memory.store8(active_clear,globals::query_active,0);
    if(signed_value(count)<=0)return {Exit::no_objects,kept.targets};
    // 5. Each object's chain, until 64 targets.
    (void)memory.load32(spread_for_chains,local(Local::cosine));
    kept.position=0;
    for(;;) {
        (void)memory.load32(context_for_chain,local(Local::add_context));
        (void)memory.load32(second_for_chain,local(Local::test_second));
        (void)memory.load32(first_for_chain,local(Local::test_first));
        (void)memory.load32(width_for_chain,local(Local::sine));
        (void)memory.load32(range_for_chain,local(Local::range));
        (void)memory.load32(direction_for_chain,local(Local::direction));
        (void)memory.load32(point_for_chain,local(Local::point));
        (void)memory.load32(object_for_chain,element(local(Local::objects),kept.position,4));
        kept.targets=frame.chain(kept);
        if(signed_value(kept.targets)>=signed_value(target_limit))break;
        const auto bound=memory.load32(count_bound,local(Local::angle));
        ++kept.position;
        if(!(signed_value(kept.position)<signed_value(bound)))break;
    }
    return {Exit::walked,kept.targets};
}
} // namespace aim_search
} // namespace halo2::native_aim_search
