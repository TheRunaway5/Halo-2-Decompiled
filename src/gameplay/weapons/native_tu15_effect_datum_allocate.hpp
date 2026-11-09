#pragma once
#include "halo2/native_tu15_datum_allocate_kernel.hpp"
#include "native_tu15_effect_slot_scan.hpp"

namespace halo2::native_effect_datum_allocate_detail {
using Phase=NativeTu15DatumAllocatePhase;
using Data=NativeTu15DatumAllocation;
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE");return p&&*p;}();return enabled;}
inline std::uint32_t logic(std::uint32_t value,unsigned bytes=4) {
    const auto mask=bytes==4?0xffffffffu:0xffffu,sign=bytes==4?0x80000000u:0x8000u;value&=mask;
    return ((std::popcount(value&255u)%2u==0u)?4u:0u)|(value==0?64u:0u)|(value&sign?128u:0u);
}
inline std::uint32_t compare(std::uint32_t left,std::uint32_t right,unsigned bytes=4) {
    const auto mask=bytes==4?0xffffffffu:0xffffu,sign=bytes==4?0x80000000u:0x8000u;
    left&=mask;right&=mask;const auto result=(left-right)&mask;
    return logic(result,bytes)|unsigned(left<right)|(((left^right^result)&16u)?16u:0u)|
        (((left^right)&(left^result)&sign)?2048u:0u);
}
inline std::uint32_t increment_half(std::uint32_t value) {
    value&=0xffffu;const auto result=(value+1u)&0xffffu;
    return logic(result,2)|(((value^1u^result)&16u)?16u:0u)|(value==0x7fffu?2048u:0u);
}
// These source shifts/multiply leave some arithmetic bits undefined. Exact
// incoming contexts plus real instructions retain their physical host result;
// no PUSHF writes below the native ABI stack/red zone are introduced.
inline std::uint32_t scan_shift_flags(std::uint32_t index) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags,count=index,mask=1,word=index;
    asm volatile("andl $31,%%ecx; shll %%cl,%2; sarl $5,%3; seto %%cl; lahf; movzbl %%ah,%%eax"
        : "=&a"(flags),"+&c"(count),"+&r"(mask),"+&r"(word)::"cc");
    return (flags&0xd5u)|((count&1u)<<11u);
#else
    throw std::runtime_error("Exact datum scan flag backend unavailable");
#endif
}
inline std::uint32_t product_flags(std::uint32_t stride,std::uint32_t index) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags,overflow;
    asm volatile("cmpl $-1,%3; imull %3,%2; seto %%cl; lahf; movzbl %%ah,%%eax"
        : "=&a"(flags),"=c"(overflow),"+&r"(stride):"r"(index):"cc");
    return (flags&0xd5u)|((overflow&1u)<<11u);
#else
    throw std::runtime_error("Exact datum product flag backend unavailable");
#endif
}
inline std::uint32_t occupancy_shift_flags(std::uint32_t product,std::uint32_t rows,std::uint32_t index) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t flags,overflow;
    asm volatile("addl %4,%2; sarl $5,%3; seto %%cl; lahf; movzbl %%ah,%%eax"
        : "=&a"(flags),"=c"(overflow),"+&r"(product),"+&r"(index):"r"(rows):"cc");
    return (flags&0xd5u)|((overflow&1u)<<11u);
#else
    throw std::runtime_error("Exact datum occupancy flag backend unavailable");
#endif
}
inline std::uint32_t pc(Phase phase) {
    switch(phase) {
    case Phase::scan_bitmap:return 0x16e6b1u;case Phase::scan_word:return 0x16e6c5u;
    case Phase::scan_extent:return 0x16e6cau;case Phase::capacity:return 0x16e6dbu;
    case Phase::stride:return 0x16e6e7u;case Phase::rows:return 0x16e6eau;
    case Phase::publish_bitmap:return 0x16e6f0u;case Phase::occupancy_word:return 0x16e6fau;
    case Phase::occupancy:return 0x16e70eu;case Phase::used:return 0x16e710u;
    case Phase::extent:return 0x16e713u;case Phase::used_count:return 0x16e71cu;
    case Phase::cursor:return 0x16e71fu;case Phase::grown_extent:return 0x16e724u;
    case Phase::clear_stride:return 0x16e727u;case Phase::clear_word:return 0x16e733u;
    case Phase::clear_byte:return 0x16e73au;case Phase::salt:return 0x16e73cu;
    case Phase::salt_record:return 0x16e740u;case Phase::salt_increment:return 0x16e743u;
    case Phase::salt_check:return 0x16e747u;case Phase::salt_reset:return 0x16e74eu;
    case Phase::record_salt:return 0x16e754u;case Phase::finished:return 0x16e75cu;
    }
    throw std::runtime_error("Unknown typed datum allocation phase");
}
struct Access {
    SourcePhysicsContactMemory&memory;
    std::array<std::uint32_t,8>&gp;
    std::uint32_t&flags;
    const SourcePhysicsContactRecordObserver&observe;
    std::array<std::uint32_t,8>entry;
    std::uint32_t entry_flags;
    std::uint32_t*position{};
    const Data*current{};
    Phase phase{Phase::finished};bool after{};
    SourcePhysicsContactRecordStore event{};
    void at(Phase next,const Data&data,bool completed=false){phase=next;current=&data;after=completed;if(position)*position=pc(next);}
    // Materialize the current observable boundary, on a completed observer
    // event or a stopped memory operation. The numerical loop stays typed.
    void publish() {
        if(!current)return;
        const auto&s=*current;using S=NativeTu15DatumSelection;
        gp=entry;gp[4]=entry[4]-16u;gp[0]=0xffffffffu;gp[6]=s.cursor;gp[7]=s.extent;
        const bool scanned=s.selection!=S::initial;
        if(scanned){gp[3]=s.bitmap;gp[5]=s.bit;gp[1]=s.selection==S::exhausted?s.scan_extent:s.word_index;}
        if(s.selection==S::invalid_hole)gp[3]=s.cursor;
        std::uint32_t arithmetic{};
        if(phase==Phase::scan_bitmap)arithmetic=compare(s.cursor,s.extent);
        else if(phase==Phase::scan_word)arithmetic=scan_shift_flags(s.cursor);
        else if(phase==Phase::scan_extent)arithmetic=logic(s.word&s.bit);
        else if(phase==Phase::capacity) {
            if(s.selection==S::initial)arithmetic=compare(s.cursor,s.extent);
            else if(s.selection==S::exhausted)arithmetic=compare(s.cursor,s.scan_extent);
            else arithmetic=compare(s.cursor,0xffffffffu);
        }else if(phase==Phase::finished&&!s.success) {
            if(s.failure==NativeTu15DatumFailure::sentinel)gp[3]=s.index;
            arithmetic=s.failure==NativeTu15DatumFailure::capacity?compare(s.extent,s.capacity):compare(s.extent,0xffffffffu);
        }else {
            gp[3]=s.index;
            if(phase==Phase::stride)arithmetic=compare(s.index,0xffffffffu);
            else if(phase==Phase::rows){gp[6]=s.stride;arithmetic=compare(s.index,0xffffffffu);}
            else if(phase==Phase::publish_bitmap){gp[6]=s.product;gp[0]=s.rows;arithmetic=product_flags(s.stride,s.index);}
            else if(phase==Phase::occupancy_word) {
                gp[0]=std::uint32_t(std::bit_cast<std::int32_t>(s.index)>>5u);gp[1]=s.publish_bitmap;gp[6]=s.record;
                arithmetic=occupancy_shift_flags(s.product,s.rows,s.index);
            }else {
                gp[0]=s.occupied;gp[1]=s.index&31u;gp[5]=s.new_word;gp[6]=s.record;gp[7]=1u<<(s.index&31u);
                arithmetic=logic(s.new_word);
                if(phase==Phase::extent)gp[7]=s.used;
                else if(phase!=Phase::occupancy&&phase!=Phase::used) {
                    gp[0]=s.next_index;gp[1]=s.current_extent;gp[7]=s.next_used;arithmetic=compare(s.current_extent,s.index);
                    if(phase!=Phase::used_count&&phase!=Phase::cursor&&phase!=Phase::grown_extent&&phase!=Phase::clear_stride) {
                        gp[0]=0;gp[1]=0;gp[5]=s.clear_bytes;gp[7]=s.clear_at;
                        arithmetic=logic(s.tail_initial);
                        if(phase==Phase::clear_word){gp[1]=s.word_count;arithmetic=logic(0);}
                        else if(phase==Phase::clear_byte)gp[1]=s.tail_count;
                        else if(phase!=Phase::salt) {
                            gp[0]=s.salt;
                            if(phase==Phase::salt_check||(phase==Phase::salt_increment&&after))arithmetic=increment_half(s.increment_old);
                            else if(phase==Phase::salt_reset||phase==Phase::record_salt)arithmetic=compare(s.salt_check,0xffffu,2);
                            else if(phase==Phase::finished){gp[0]=s.handle;arithmetic=logic(s.handle);}
                        }
                    }
                }
            }
        }
        flags=(entry_flags&~0x8d5u)|arithmetic;
    }
    std::uint32_t read(Phase next,const Data&s,std::uint32_t address,unsigned bytes) {
        at(next,s);publish();std::array<std::byte,4>storage{};memory.read(address,std::span(storage).first(bytes));std::uint32_t value{};
        for(unsigned n=0;n<bytes;++n)value|=std::to_integer<unsigned>(storage[n])<<(n*8u);return value;
    }
    void write(Phase next,const Data&s,std::uint32_t address,std::uint32_t value,unsigned bytes,std::uint32_t before,bool known) {
        at(next,s);publish();event={pc(next),address,{},std::vector<std::byte>(bytes)};
        if(observe){event.before.resize(bytes);if(known)for(unsigned n=0;n<bytes;++n)event.before[n]=std::byte(before>>(n*8u));else memory.read(address,event.before);}
        for(unsigned n=0;n<bytes;++n)event.written[n]=std::byte(value>>(n*8u));memory.write(address,event.written);
    }
    void complete(Phase next,const Data&s){at(next,s,true);if(observe){publish();observe(event);}}
};
inline bool try_native(SourcePhysicsContactMemory&memory,std::array<std::uint32_t,8>&gp,
    std::uint32_t&flags,const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe) {
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||!native_effect_slot_scan_detail::admitted_protocol(services,observe))return false;
    const auto entry=gp;const auto entry_flags=flags;std::uint32_t*position=services.instruction?services.instruction_cursor.position:nullptr;
    const auto at=[&](std::uint32_t value){if(position)*position=value;};
    const auto push=[&](std::uint32_t site,std::uint32_t value){
        at(site);const auto target=gp[4]-4u;SourcePhysicsContactRecordStore event{site,target,{},std::vector<std::byte>(4)};
        if(observe){event.before.resize(4);memory.read(target,event.before);}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));memory.write(target,event.written);gp[4]=target;if(observe)observe(event);
    };
    const auto pop=[&](std::uint32_t site){at(site);const auto value=native_effect_slot_scan_detail::word(memory,gp[4]);gp[4]+=4u;return value;};
    push(0x16e6a0u,gp[3]);push(0x16e6a1u,gp[5]);push(0x16e6a2u,gp[6]);
    at(0x16e6a3u);gp[6]=native_effect_slot_scan_detail::word(memory,gp[2]+0x34u);push(0x16e6a6u,gp[7]);
    at(0x16e6a7u);gp[7]=native_effect_slot_scan_detail::word(memory,gp[2]+0x38u);
    Data data;data.pool=gp[2];data.cursor=gp[6];data.extent=gp[7];
    Access access{memory,gp,flags,observe,entry,entry_flags,position};
    try{native_tu15_allocate_datum(access,data,(flags&0x400u)!=0);access.at(Phase::finished,data);access.publish();}
    catch(...){access.publish();throw;}
    gp[7]=pop(0x16e75cu);gp[6]=pop(0x16e75du);gp[5]=pop(0x16e75eu);gp[3]=pop(0x16e75fu);pop(0x16e760u);return true;
#endif
}
} // namespace halo2::native_effect_datum_allocate_detail
