#pragma once
#include "halo2/native_weapon_effect_birth_color_observer.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include <bit>
#include <cstdlib>

namespace halo2::native_effect_slot_scan_detail {
inline bool reference_mode() {
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_EFFECT_SLOT_SCAN");return p&&*p;}();return enabled;
}
// HALO2_CENSUS_NATIVE_BRIDGES: full census callbacks without a cursor. The
// natives behind this protocol (the 16EA60 slot scan, the 16E6A0 datum
// allocator and the 145560 transform product) hold no live service PC and
// write the cursor only when there is one, so they run natively; only their
// own per-PC census counts disappear.
inline bool census_callbacks(const SourcePhysicsWorldRuntimeServices&services) {
    return native_census_detail::enabled()&&services.instruction&&!services.instruction_cursor.position;
}
inline bool admitted_protocol(const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe) {
    if(!services.instruction)return !observe||NativeWeaponEffectBirthColorCollector::qualified(observe);
    if(census_callbacks(services))return true;
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
struct Resume {
    std::uint32_t pc;
};

inline void run(SourcePhysicsContactMemory&memory,std::array<std::uint32_t,8>&gp,
    std::uint32_t&flags,const SourcePhysicsContactRecordObserver&observe,
    const SourcePhysicsWorldRuntimeServices*services=nullptr) {
    const auto*cursor=services?&services->instruction_cursor:nullptr;
    auto*position=services&&services->instruction?cursor->position:nullptr;
    const bool sealed=position&&services->immutable_instruction_cursor.identifies(*services);
    const auto*census_type=!position&&services&&services->instruction&&census_callbacks(*services)
        ?&services->instruction.target_type():nullptr;
    const auto at=[&](std::uint32_t pc){if(position)*position=pc;};
    const auto recheck=[&](std::uint32_t next) {
        if(!services)return;
        if(sealed) {
            if((cursor->first_trace&&*cursor->first_trace)||(cursor->second_trace&&*cursor->second_trace))
                throw Resume{next};
        } else if(position) {
            if(!services->instruction||cursor->position!=position||!admitted_protocol(*services,observe))
                throw Resume{next};
        } else if(services->instruction) {
            if(!census_type||services->instruction.target_type()!=*census_type||cursor->position)
                throw Resume{next};
        } else if(observe&&!NativeWeaponEffectBirthColorCollector::qualified(observe)) {
            throw Resume{next};
        }
    };
    const auto push=[&](std::uint32_t site,std::uint32_t next,std::uint32_t value) {
        at(site);
        const auto address=gp[4]-4u;
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
        if(observe){event.before.resize(4);memory.read(address,event.before);}
        for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
        memory.write(address,event.written);
        gp[4]=address;
        if(observe)observe(event);
        recheck(next);
    };
    const auto load=[&](std::uint32_t site,std::uint32_t address) {
        at(site);
        return word(memory,address);
    };
    const auto pop=[&](std::uint32_t site) {
        const auto value=load(site,gp[4]);
        gp[4]+=4u;
        return value;
    };
    const auto below_limit=[&] {
        return std::bit_cast<std::int32_t>(gp[2])<std::bit_cast<std::int32_t>(gp[7]);
    };

    gp[0]=0xffffffffu;
    logic(flags,gp[2]);
    if(std::bit_cast<std::int32_t>(gp[2])<0) {
        pop(0x16ea98u);
        return;
    }
    push(0x16ea67u,0x16ea68u,gp[7]);
    gp[7]=load(0x16ea68u,gp[1]+0x38u);
    recheck(0x16ea6bu);
    compare(flags,gp[2],gp[7]);
    if(below_limit()) {
        push(0x16ea6fu,0x16ea70u,gp[6]);
        gp[6]=load(0x16ea70u,gp[1]+0x48u);
        recheck(0x16ea73u);
        push(0x16ea73u,0x16ea74u,gp[3]);
        do {
            // Every candidate reads its word with the committed index, mask
            // and SAR flags in place. TEST commits only after the read returns.
            gp[3]=1u<<(gp[2]&31u);
            gp[1]=std::uint32_t(std::bit_cast<std::int32_t>(gp[2])>>5);
            logic(flags,gp[1]);
            flags|=(gp[2]>>4u)&1u;
            const auto selected=load(0x16ea85u,gp[6]+gp[1]*4u)&gp[3];
            logic(flags,selected);
            recheck(0x16ea88u);
            if(selected) {
                gp[3]=pop(0x16ea93u);
                recheck(0x16ea94u);
                gp[0]=gp[2];
                gp[6]=pop(0x16ea96u);
                recheck(0x16ea97u);
                break;
            }
            ++gp[2];
            compare(flags,gp[2],gp[7]);
            if(!below_limit()) {
                gp[3]=pop(0x16ea8fu);
                recheck(0x16ea90u);
                gp[6]=pop(0x16ea90u);
                recheck(0x16ea91u);
                gp[7]=pop(0x16ea91u);
                recheck(0x16ea92u);
                pop(0x16ea92u);
                return;
            }
        } while(true);
    }
    gp[7]=pop(0x16ea97u);
    recheck(0x16ea98u);
    pop(0x16ea98u);
}
inline bool try_native(SourcePhysicsContactMemory&memory,std::array<std::uint32_t,8>&gp,
    std::uint32_t&flags,const SourcePhysicsWorldRuntimeServices&services,const SourcePhysicsContactRecordObserver&observe) {
    if(!admitted(services,observe))return false;
    run(memory,gp,flags,observe,&services);
    return true;
}
} // namespace halo2::native_effect_slot_scan_detail
