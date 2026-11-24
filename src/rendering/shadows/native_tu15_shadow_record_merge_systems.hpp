#pragma once
// Native TU1.5 shadow record merge, entered at 4A7B0 (shadow frame unit): the
// frame's shadow casters merged into the cache of shadow records.
//
//   shadow_record_merge::merge   4A7B0(EAX cache; casters, count); RET 8
//
// The cache holds up to 32 records (0x24 bytes; the count at +0x480). A
// record whose key (+4) a caster (0x14 bytes) has takes the caster's five
// words and the frame stamp ([4C6854], at +0x1C), and the caster is marked
// (EDX, bit by caster). A record the frame did not stamp is replaced by the
// last record. Then each unmarked caster, while the cache has room, is
// appended: its five words, the stamp twice (+0x1C, +0x20), its +0xC as
// +0x18, a zero weight (+0x14, XMM0 zeroed); the original lists four at a
// time while four remain, then the rest.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_shadow_record_merge.hpp) maps the role to the original
// instruction and leaves through the original's epilogue.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_shadow_record_merge {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t stamp=0x4c6854u;}
inline constexpr std::uint32_t capacity=0x20u,record_words=9u,caster_words=5u,unrolled=4u;
namespace cache {inline constexpr std::uint32_t count=0x480u;}
namespace record {inline constexpr std::uint32_t size=0x24u,key=4u,source=0xcu,weight=0x14u,copied=0x18u,stamp=0x1cu,first_stamp=0x20u;}
namespace caster {inline constexpr std::uint32_t size=0x14u,key=4u;}
// The frame (offsets from the ESP after EBX, EBP, ESI and EDI are pushed):
// the pass, the bit (later the record index), the casters and their count.
namespace local {inline constexpr std::uint32_t pass=0x10u,index=0x14u,bit=0x14u,casters=0x1cu,count=0x20u;}

namespace shadow_record_merge {
enum class Access {
    count_load,index_clear,list_count_load,list_load,record_key,entry_key,list_count_again,refresh_load_0,refresh_store_0,
    refresh_load_1,refresh_store_1,refresh_load_2,refresh_store_2,refresh_load_3,refresh_store_3,refresh_load_4,refresh_store_4,
    refresh_stamp,refresh_stamp_store,index_load,count_again,index_store,count_remove,count_check,record_stamp_test,stamp_test,
    stale_copy_load,stale_copy_store,count_drop,count_drop_store,count_next,list_count_add,pass_clear,list_add,bit_init,full_0,
    append_0_load_0,append_0_store_0,append_0_load_1,append_0_store_1,append_0_load_2,append_0_store_2,append_0_load_3,
    append_0_store_3,append_0_load_4,append_0_store_4,count_a_0,stamp_a_0,stamp_store_a_0,count_b_0,stamp_b_0,pass_reload_0,
    stamp_store_b_0,count_c_0,key_load_0,key_store_0,count_d_0,weight_store_0,count_inc_load_0,count_inc_store_0,full_1,bit_1,
    append_1_load_0,append_1_store_0,append_1_load_1,append_1_store_1,append_1_load_2,append_1_store_2,append_1_load_3,
    append_1_store_3,append_1_load_4,append_1_store_4,count_a_1,stamp_a_1,stamp_store_a_1,count_b_1,stamp_b_1,pass_reload_1,
    stamp_store_b_1,count_c_1,key_load_1,key_store_1,count_d_1,weight_store_1,count_inc_load_1,count_inc_store_1,full_2,bit_2,
    append_2_load_0,append_2_store_0,append_2_load_1,append_2_store_1,append_2_load_2,append_2_store_2,append_2_load_3,
    append_2_store_3,append_2_load_4,append_2_store_4,count_a_2,stamp_a_2,stamp_store_a_2,count_b_2,stamp_b_2,pass_reload_2,
    stamp_store_b_2,count_c_2,key_load_2,key_store_2,count_d_2,weight_store_2,count_inc_load_2,count_inc_store_2,full_3,bit_3,
    append_3_load_0,append_3_store_0,append_3_load_1,append_3_store_1,append_3_load_2,append_3_store_2,append_3_load_3,
    append_3_store_3,append_3_load_4,append_3_store_4,count_a_3,stamp_a_3,stamp_store_a_3,count_b_3,stamp_b_3,pass_reload_3,
    stamp_store_b_3,count_c_3,key_load_3,key_store_3,count_d_3,weight_store_3,count_inc_load_3,count_inc_store_3,pass_bit_load,
    pass_list_count,pass_store,pass_bit_store,list_count_rest,list_rest,cursor_init,full_rest,cursor_rest,append_rest_load_0,
    append_rest_store_0,append_rest_load_1,append_rest_store_1,append_rest_load_2,append_rest_store_2,append_rest_load_3,
    append_rest_store_3,append_rest_load_4,append_rest_store_4,count_a_rest,stamp_a_rest,stamp_store_a_rest,count_b_rest,
    stamp_b_rest,stamp_store_b_rest,count_c_rest,key_load_rest,key_store_rest,count_d_rest,weight_store_rest,count_inc_load_rest,
    count_inc_store_rest,cursor_load,list_count_tail,cursor_store};
// The roles of one append: the room test, the bit (or the cursor), the
// five words, and the record's stamps, copy, weight and the count.
struct Append {Access full,bit;Access load[caster_words],store[caster_words];
    Access count_a,stamp_a,stamp_store_a,count_b,stamp_b,reload,stamp_store_b,count_c,key_load,key_store,count_d,weight_store,
    count_inc_load,count_inc_store;};
inline constexpr Append appends[unrolled+1u]{
    {Access::full_0,Access::full_0,{Access::append_0_load_0,Access::append_0_load_1,Access::append_0_load_2,Access::append_0_load_3,Access::append_0_load_4},{Access::append_0_store_0,Access::append_0_store_1,Access::append_0_store_2,Access::append_0_store_3,Access::append_0_store_4},
        Access::count_a_0,Access::stamp_a_0,Access::stamp_store_a_0,Access::count_b_0,Access::stamp_b_0,Access::pass_reload_0,Access::stamp_store_b_0,
        Access::count_c_0,Access::key_load_0,Access::key_store_0,Access::count_d_0,Access::weight_store_0,Access::count_inc_load_0,Access::count_inc_store_0},
    {Access::full_1,Access::bit_1,{Access::append_1_load_0,Access::append_1_load_1,Access::append_1_load_2,Access::append_1_load_3,Access::append_1_load_4},{Access::append_1_store_0,Access::append_1_store_1,Access::append_1_store_2,Access::append_1_store_3,Access::append_1_store_4},
        Access::count_a_1,Access::stamp_a_1,Access::stamp_store_a_1,Access::count_b_1,Access::stamp_b_1,Access::pass_reload_1,Access::stamp_store_b_1,
        Access::count_c_1,Access::key_load_1,Access::key_store_1,Access::count_d_1,Access::weight_store_1,Access::count_inc_load_1,Access::count_inc_store_1},
    {Access::full_2,Access::bit_2,{Access::append_2_load_0,Access::append_2_load_1,Access::append_2_load_2,Access::append_2_load_3,Access::append_2_load_4},{Access::append_2_store_0,Access::append_2_store_1,Access::append_2_store_2,Access::append_2_store_3,Access::append_2_store_4},
        Access::count_a_2,Access::stamp_a_2,Access::stamp_store_a_2,Access::count_b_2,Access::stamp_b_2,Access::pass_reload_2,Access::stamp_store_b_2,
        Access::count_c_2,Access::key_load_2,Access::key_store_2,Access::count_d_2,Access::weight_store_2,Access::count_inc_load_2,Access::count_inc_store_2},
    {Access::full_3,Access::bit_3,{Access::append_3_load_0,Access::append_3_load_1,Access::append_3_load_2,Access::append_3_load_3,Access::append_3_load_4},{Access::append_3_store_0,Access::append_3_store_1,Access::append_3_store_2,Access::append_3_store_3,Access::append_3_store_4},
        Access::count_a_3,Access::stamp_a_3,Access::stamp_store_a_3,Access::count_b_3,Access::stamp_b_3,Access::pass_reload_3,Access::stamp_store_b_3,
        Access::count_c_3,Access::key_load_3,Access::key_store_3,Access::count_d_3,Access::weight_store_3,Access::count_inc_load_3,Access::count_inc_store_3},
    {Access::full_rest,Access::cursor_rest,{Access::append_rest_load_0,Access::append_rest_load_1,Access::append_rest_load_2,Access::append_rest_load_3,Access::append_rest_load_4},{Access::append_rest_store_0,Access::append_rest_store_1,Access::append_rest_store_2,Access::append_rest_store_3,Access::append_rest_store_4},
        Access::count_a_rest,Access::stamp_a_rest,Access::stamp_store_a_rest,Access::count_b_rest,Access::stamp_b_rest,Access::count_b_rest,Access::stamp_store_b_rest,
        Access::count_c_rest,Access::key_load_rest,Access::key_store_rest,Access::count_d_rest,Access::weight_store_rest,Access::count_inc_load_rest,Access::count_inc_store_rest}
};

// One caster appended (its bit set in the mask).
template<class Memory,class Frame> void append(Memory &memory,Frame &frame,std::uint32_t cache_address,std::uint32_t total,
                                              std::uint32_t source,const Append &r,bool reloads) {
    const auto destination=cache_address+record::size*total;
    for(std::uint32_t w=0;w<caster_words;++w)memory.store32(r.store[w],destination+4u*w,memory.load32(r.load[w],source+4u*w));
    auto n=memory.load32(r.count_a,at(cache_address,cache::count));
    memory.store32(r.stamp_store_a,cache_address+record::size*n+record::stamp,memory.load32(r.stamp_a,globals::stamp));
    n=memory.load32(r.count_b,at(cache_address,cache::count));
    const auto stamp=memory.load32(r.stamp_b,globals::stamp);
    if(reloads)memory.load32(r.reload,frame.local(local::pass));
    memory.store32(r.stamp_store_b,cache_address+record::size*n+record::first_stamp,stamp);
    n=memory.load32(r.count_c,at(cache_address,cache::count));
    memory.store32(r.key_store,cache_address+record::size*n+record::copied,memory.load32(r.key_load,cache_address+record::size*n+record::source));
    n=memory.load32(r.count_d,at(cache_address,cache::count));
    memory.store32(r.weight_store,cache_address+record::size*n+record::weight,frame.weight());
    memory.store32(r.count_inc_store,at(cache_address,cache::count),frame.counted(memory.load32(r.count_inc_load,at(cache_address,cache::count))));
}

// Frame: see native_tu15_shadow_record_merge.hpp.
template<class Memory,class Frame> void merge(Memory &memory,Frame &frame,std::uint32_t cache_address) {
    using enum Access;
    // The records the casters refresh.
    frame.framing();
    const auto records=memory.load32(count_load,at(cache_address,cache::count));
    frame.saving(1u);
    frame.saving(2u);
    auto mask=frame.unmarked();
    const bool any=frame.above(records,mask);
    frame.saving(3u);
    frame.saving(4u);
    memory.store32(index_clear,frame.local(local::index),mask);
    if(any) {
        auto record_address=cache_address;
        for(;;) {
            const auto casters_total=memory.load32(list_count_load,frame.local(local::count));
            if(frame.positive(casters_total)) {
                const auto casters=memory.load32(list_load,frame.local(local::casters));
                const auto key=memory.load32(record_key,at(record_address,record::key));
                for(std::uint32_t j=0;;) {
                    if(frame.same(key,memory.load32(entry_key,casters+caster::size*j+caster::key))) {
                        const auto source=casters+caster::size*j;
                        for(std::uint32_t w=0;w<caster_words;++w) {
                            static constexpr Access loads[]{refresh_load_0,refresh_load_1,refresh_load_2,refresh_load_3,refresh_load_4};
                            static constexpr Access stores[]{refresh_store_0,refresh_store_1,refresh_store_2,refresh_store_3,refresh_store_4};
                            memory.store32(stores[w],record_address+4u*w,memory.load32(loads[w],source+4u*w));
                        }
                        memory.store32(refresh_stamp_store,at(record_address,record::stamp),memory.load32(refresh_stamp,globals::stamp));
                        mask=frame.marked(mask,j);
                        break;
                    }
                    j=frame.next_caster(j);
                    if(!frame.below(j,memory.load32(list_count_again,frame.local(local::count))))break;
                }
            }
            const auto index=frame.next_record(memory.load32(index_load,frame.local(local::index)));
            const bool more=frame.below(index,memory.load32(count_again,at(cache_address,cache::count)));
            record_address+=record::size;
            memory.store32(index_store,frame.local(local::index),index);
            if(!more)break;
        }
    }

    // The records the frame did not stamp give way to the last.
    if(frame.positive(memory.load32(count_remove,at(cache_address,cache::count)))) {
        auto record_address=cache_address;
        std::uint32_t i=0;
        for(;;) {
            const auto n=memory.load32(count_check,at(cache_address,cache::count));
            bool stale=false;
            if(frame.positive(n)) {
                const auto record_stamp=memory.load32(record_stamp_test,at(record_address,record::stamp));
                stale=!frame.same(record_stamp,memory.load32(stamp_test,globals::stamp));
            }
            if(stale) {
                auto from=frame.copying(cache_address+record::size*n-record::size,record_address);
                auto to=record_address;
                const auto step=frame.step();
                for(std::uint32_t w=0;w<record_words;++w,from+=step,to+=step)memory.store32(stale_copy_store,to,memory.load32(stale_copy_load,from));
                memory.store32(count_drop_store,at(cache_address,cache::count),frame.dropped(memory.load32(count_drop,at(cache_address,cache::count))));
                i-=1u;
                record_address-=record::size;
            }
            const auto n_now=memory.load32(count_next,at(cache_address,cache::count));
            i+=1u;
            record_address+=record::size;
            if(!frame.below(i,n_now))break;
        }
    }

    // The unmarked casters appended while there is room: four at a time while four remain, then the rest.
    const auto casters_total=memory.load32(list_count_add,frame.local(local::count));
    frame.weight_cleared();
    std::uint32_t pass=0u;
    const bool four=frame.at_least_four(casters_total);
    memory.store32(pass_clear,frame.local(local::pass),pass);
    if(four) {
        auto casters=memory.load32(list_add,frame.local(local::casters));
        memory.store32(bit_init,frame.local(local::bit),2u);
        for(;;) {
            for(std::uint32_t k=0;k<unrolled;++k) {
                const auto &r=appends[k];
                const auto total=memory.load32(r.full,at(cache_address,cache::count));
                if(!frame.room(total))break;
                const auto bit=k==0u?pass:frame.unrolled_bit(memory.load32(r.bit,frame.local(local::bit)),k);
                if(frame.marked_already(mask,bit))continue;
                append(memory,frame,cache_address,total,casters+caster::size*k,r,true);
                mask=frame.mark(mask);
            }
            const auto bit_now=memory.load32(pass_bit_load,frame.local(local::bit));
            const auto limit=frame.pass_limit(memory.load32(pass_list_count,frame.local(local::count)));
            pass=frame.next_pass(pass);
            casters+=unrolled*caster::size;
            const bool more=frame.below(pass,limit);
            memory.store32(pass_store,frame.local(local::pass),pass);
            memory.store32(pass_bit_store,frame.local(local::bit),bit_now+unrolled);
            if(!more)break;
        }
    }
    if(frame.below(pass,memory.load32(list_count_rest,frame.local(local::count)))) {
        memory.store32(cursor_init,frame.local(local::casters),memory.load32(list_rest,frame.local(local::casters))+caster::size*pass);
        const auto &r=appends[unrolled];
        for(;;) {
            const auto total=memory.load32(r.full,at(cache_address,cache::count));
            if(frame.room(total) && !frame.marked_already(mask,pass)) {
                append(memory,frame,cache_address,total,memory.load32(r.bit,frame.local(local::casters)),r,false);
                mask=frame.mark(mask);
            }
            const auto cursor=memory.load32(cursor_load,frame.local(local::casters));
            const auto count=memory.load32(list_count_tail,frame.local(local::count));
            pass=frame.next_pass_rest(pass);
            const bool more=frame.below(pass,count);
            memory.store32(cursor_store,frame.local(local::casters),cursor+caster::size);
            if(!more)break;
        }
    }
}
} // namespace shadow_record_merge
} // namespace halo2::native_shadow_record_merge
