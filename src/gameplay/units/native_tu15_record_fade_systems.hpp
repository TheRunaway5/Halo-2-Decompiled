#pragma once
// Native TU1.5 record fade, entered at 17FB60 (combat action unit): a
// record's age decides whether it expires or how faded it is.
//
//   record_fade::update   17FB60(EAX = record handle) -> AL
//
// The records are the 64-byte records of the data array at 4F7220. A
// record's age is (now - its start tick +0x18) * seconds per tick (the game
// time at 51DF54; CVTSI2SS, MULSS). Its fade byte (+0x27) becomes 0xFF;
// a record flagged 2 at +2 stays so (result 0). Otherwise, with a lifetime
// (+0x1C) other than zero (UCOMISS) that the age has reached (COMISS), the
// record expires: the "counted" bit (1 at +2) is cleared on it and on every
// record of its chain (+0x3C), each clear decrementing the counter at
// [4F721C]+0x3804, and 140310 runs (result 1). Else, with a positive
// lifetime and fade time (+0x20) longer than the remaining time, the fade
// byte becomes the remaining fraction times 46A26C, at least zero and at
// most 46A148, converted by 32CC0C (the CRT float-to-integer conversion of
// an x87 value) (result 0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_fade.hpp) maps the role to the original instruction
// and owns the ABI of the children (140310, 32CC0C).
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace halo2::native_record_fade {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;

namespace globals {
inline constexpr std::uint32_t records=0x4f7220u;     // DataArray* of 64-byte records
inline constexpr std::uint32_t game_time=0x51df54u;   // +4 seconds per tick, +8 the tick
inline constexpr std::uint32_t counters=0x4f721cu;    // +0x3804: the counted records
inline constexpr std::uint32_t expire_owner=0x515f28u;
inline constexpr std::uint32_t fade_scale=0x46a26cu,fade_limit=0x46a148u;
}
inline constexpr std::uint32_t datum_data=0x44u,record_shift=6u,counted_records=0x3804u;
namespace record {
inline constexpr std::uint32_t flags=2u,owner=4u,start=0x18u,lifetime=0x1cu,fade_time=0x20u,fade_word=0x24u,
    fade_byte=0x27u,next=0x3cu;
}
namespace flag {inline constexpr std::uint32_t counted=1u,held=2u;}

namespace scalar {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr bool available=true;
inline std::uint32_t from_integer(std::uint32_t x) {
    __m128 r=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(r):"r"(std::int32_t(x)));return sse::bits(r);
}
// UCOMISS a,b; LAHF; TEST AH,44; JNP: ordered and equal.
inline bool ordered_equal(std::uint32_t a,std::uint32_t b) {
    unsigned char zero,parity;
    asm volatile("ucomiss %3,%2\n\tsetz %0\n\tsetp %1":"=qm"(zero),"=qm"(parity):"x"(sse::scalar(a)),"x"(sse::scalar(b)):"cc");
    return zero && !parity;
}
#else
inline constexpr bool available=false;
inline std::uint32_t from_integer(std::uint32_t){throw std::runtime_error("Required TU1.5 CVTSI2SS backend unavailable");}
inline bool ordered_equal(std::uint32_t,std::uint32_t){throw std::runtime_error("Required TU1.5 UCOMISS backend unavailable");}
#endif
} // namespace scalar

namespace record_fade {
enum class Local {fade};
enum class Access {
    records_table,data,game_time,now,start,flags,seconds,fade_reset,lifetime,counters,flags_clear,count_read,count_store,
    chain_next,chain_data,chain_flags,chain_flags_clear,chain_count_read,chain_count_store,chain_after,owner,expire_owner,
    lifetime_again,fade_time,lifetime_remaining,fade_time_again,fade_scale,fade_limit,fade_store_zero,fade_store_limit,
    fade_store_value,fade_for_x87,fade_word,fade_word_store
};
// The age's and the fade's registers (XMM0-XMM2) and comparisons.
struct Age {std::uint32_t age,lifetime;bool equal_zero,reached;};
enum class Exit {held,expired,unchanged,faded};
// Frame:
//   local(Local)               the fade local's address
//   age_computed(age)          the age (XMM2, upper lanes kept)
//   aged(Age)                  the lifetime tests ran (XMM0, XMM1, flags, AH)
//   expire(owner, expiry)      140310 (EBX the owner, EDI the expiry owner)
//   remaining(value)           the remaining time (XMM0)
//   valued(value)              the fade value (XMM0) and its floor test
//   converted(record)          FLD m32; 32CC0C; returns EAX; record as ESI
//                              came back
template<class Memory,class Frame> Exit update(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto table=memory.load32(records_table,globals::records);
    const auto record_address=((handle&0xffffu)<<record_shift)+memory.load32(data,at(table,datum_data));
    const auto time=memory.load32(game_time,globals::game_time);
    const auto now_tick=memory.load32(now,at(time,8u));
    const auto ticks=now_tick-memory.load32(start,at(record_address,record::start));
    const auto converted_ticks=scalar::from_integer(ticks);
    const auto flag_bits=memory.load8(flags,at(record_address,record::flags));
    Age a{};
    a.age=sse::multiply(converted_ticks,memory.load32(seconds,at(time,4u)));
    frame.age_computed(a.age);
    memory.store8(fade_reset,at(record_address,record::fade_byte),0xffu);
    if(flag_bits&flag::held)return Exit::held;
    a.lifetime=memory.load32(lifetime,at(record_address,record::lifetime));
    a.equal_zero=scalar::ordered_equal(a.lifetime,0u);
    a.reached=!a.equal_zero && sse::not_above(a.lifetime,a.age);
    frame.aged(a);
    if(a.reached) {
        // Expired: the counted bit off along the chain.
        if(flag_bits&flag::counted) {
            const auto counter=memory.load32(counters,globals::counters);
            memory.store8(flags_clear,at(record_address,record::flags),flag_bits&0xfeu);
            memory.store32(count_store,at(counter,counted_records),memory.load32(count_read,at(counter,counted_records))-1u);
            auto next=memory.load32(chain_next,at(record_address,record::next));
            while(next!=none) {
                const auto element=((next&0xffffu)<<record_shift)+memory.load32(chain_data,at(table,datum_data));
                const auto element_flags=memory.load8(chain_flags,at(element,record::flags));
                if(element_flags&flag::counted) {
                    memory.store8(chain_flags_clear,at(element,record::flags),element_flags&0xfeu);
                    memory.store32(chain_count_store,at(counter,counted_records),
                                   memory.load32(chain_count_read,at(counter,counted_records))-1u);
                }
                next=memory.load32(chain_after,at(element,record::next));
            }
        }
        const auto owner_handle=memory.load32(owner,at(record_address,record::owner));
        frame.expire(owner_handle,memory.load32(expire_owner,globals::expire_owner));
        return Exit::expired;
    }
    // Fading: the remaining fraction of the fade time, as a byte.
    const auto lifetime_now=memory.load32(lifetime_again,at(record_address,record::lifetime));
    if(sse::not_above(lifetime_now,0u))return Exit::unchanged;
    if(sse::not_above(memory.load32(fade_time,at(record_address,record::fade_time)),0u))return Exit::unchanged;
    const auto remaining=sse::subtract(memory.load32(lifetime_remaining,at(record_address,record::lifetime)),a.age);
    frame.remaining(remaining);
    const auto fade=memory.load32(fade_time_again,at(record_address,record::fade_time));
    if(sse::not_above(fade,remaining))return Exit::unchanged;
    const auto fraction=sse::divide(remaining,fade);
    auto value=sse::multiply(fraction,memory.load32(fade_scale,globals::fade_scale));
    frame.valued(value);
    if(!sse::not_above(0u,value))memory.store32(fade_store_zero,local(Local::fade),0u);
    else {
        const auto limit=memory.load32(fade_limit,globals::fade_limit);
        memory.store32(fade_store_limit,local(Local::fade),limit);
        if(sse::not_above(value,limit))memory.store32(fade_store_value,local(Local::fade),value);
    }
    (void)memory.load32(fade_for_x87,local(Local::fade));
    // The original reads the record through the ESI 32CC0C returned.
    std::uint32_t record_now=record_address;
    const auto level=frame.converted(record_now);
    const auto word=memory.load32(fade_word,at(record_now,record::fade_word));
    memory.store32(fade_word_store,at(record_now,record::fade_word),(level<<24)|(word&0xffffffu));
    return Exit::faded;
}
} // namespace record_fade
} // namespace halo2::native_record_fade
