#pragma once
#include "halo2/native_tu15_bitmap_scan_kernel.hpp"
#include "halo2/native_weapon_effect_birth_color_observer.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <cstdlib>

namespace halo2::native_effect_slot_scan_detail {
inline bool reference_mode() {
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_EFFECT_SLOT_SCAN");return p&&*p;}();return enabled;
}
inline bool admitted_protocol(const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe) {
    if(!services.instruction)return !observe||NativeWeaponEffectBirthColorCollector::qualified(observe);
    const auto&cursor=services.instruction_cursor;
    return cursor.position&&cursor.callback_type&&services.instruction.target_type()==*cursor.callback_type&&
        cursor.executor==std::this_thread::get_id()&&cursor.literal_service_sites&&
        cursor.service_sites.size()==3&&cursor.service_sites[0]==0x38846eu&&
        cursor.service_sites[1]==0x21f590u&&cursor.service_sites[2]==0x33b595u&&
        !(cursor.first_trace&&*cursor.first_trace)&&!(cursor.second_trace&&*cursor.second_trace)&&
        (!observe||cursor.observation_only_stores||NativeWeaponEffectBirthColorCollector::qualified(observe));
}
inline bool admitted(const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe) {
    return !reference_mode()&&admitted_protocol(services,observe);
}
inline std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t at) {
    std::array<std::byte,4>bytes{};memory.read(at,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
}
inline void logic(std::uint32_t&flags,std::uint32_t value) {
    flags=(flags&~0x8d5u)|((std::popcount(value&255u)%2u==0u)?4u:0u)|
        (value==0?64u:0u)|(value&0x80000000u?128u:0u);
}
inline void compare(std::uint32_t&flags,std::uint32_t left,std::uint32_t right) {
    const auto result=left-right;logic(flags,result);
    flags|=unsigned(left<right)|(((left^right^result)&16u)?16u:0u)|
        (((left^right)&(left^result)&0x80000000u)?2048u:0u);
}
// Integer ABI transport only. The typed bitmap loop owns no virtual register
// or flag bookkeeping. Arbitrary memory-exception partial ABI remains separate.
inline std::uint32_t run(SourcePhysicsContactMemory&memory,std::array<std::uint32_t,8>&gp,
    std::uint32_t&flags,const SourcePhysicsContactRecordObserver&observe,std::uint32_t*position=nullptr) {
    const auto at=[&](std::uint32_t pc){if(position)*position=pc;};
    const auto push=[&](std::uint32_t site,std::uint32_t value) {
        at(site);
        const auto at=gp[4]-4u;SourcePhysicsContactRecordStore event{site,at,{},std::vector<std::byte>(4)};
        if(observe){event.before.resize(4);memory.read(at,event.before);}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
        memory.write(at,event.written);gp[4]=at;if(observe)observe(event);
    };
    const auto pop=[&](std::uint32_t site){at(site);const auto value=word(memory,gp[4]);gp[4]+=4u;return value;};
    const auto first=gp[2];gp[0]=0xffffffffu;
    logic(flags,first);
    if(std::bit_cast<std::int32_t>(first)<0){pop(0x16ea98u);return 0x16ea98u;}
    push(0x16ea67u,gp[7]);at(0x16ea68u);const auto limit=word(memory,gp[1]+0x38u);
    if(std::bit_cast<std::int32_t>(first)>=std::bit_cast<std::int32_t>(limit)) {
        compare(flags,first,limit);gp[7]=pop(0x16ea97u);pop(0x16ea98u);return 0x16ea98u;
    }
    push(0x16ea6fu,gp[6]);at(0x16ea70u);const auto bitmap=word(memory,gp[1]+0x48u);
    push(0x16ea73u,gp[3]);
    struct BitmapView {
        const SourcePhysicsContactMemory&memory;std::uint32_t*position;
        void read(std::uint32_t address,std::span<std::byte>out)const {
            if(position)*position=0x16ea85u;memory.read(address,out);
        }
    };
    const auto result=native_tu15_scan_bitmap_slots(BitmapView{memory,position},bitmap,first,limit);
    gp[1]=result.word_index;gp[2]=result.index;
    if(result.found){gp[0]=result.index;logic(flags,result.selected_bit);}
    else compare(flags,result.index,limit);
    gp[3]=pop(result.found?0x16ea93u:0x16ea8fu);
    gp[6]=pop(result.found?0x16ea96u:0x16ea90u);
    gp[7]=pop(result.found?0x16ea97u:0x16ea91u);pop(result.found?0x16ea98u:0x16ea92u);
    return result.found?0x16ea98u:0x16ea92u;
}
inline bool try_native(SourcePhysicsContactMemory&memory,std::array<std::uint32_t,8>&gp,
    std::uint32_t&flags,const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe) {
    if(!admitted(services,observe))return false;
    const auto final_pc=run(memory,gp,flags,observe,services.instruction?services.instruction_cursor.position:nullptr);
    if(services.instruction)*services.instruction_cursor.position=final_pc;
    return true;
}
} // namespace halo2::native_effect_slot_scan_detail
