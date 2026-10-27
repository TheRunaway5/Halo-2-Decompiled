// Native kernels: sound property routines (TU1.5 sound manager).
//
// A sound tag's definition names its custom playback (+0xD, signed byte, -1
// for none): a 0x34-byte row of the gestalt's custom playbacks (+0x34). A
// custom playback keeps a block of filters at +0x2C (count) and +0x30 (rows);
// a filter row's +4 is a tag datum. Sound effect collections keep 0x38-byte
// rows at +4 whose custom playback starts at row +4.
//
// None of these routines touches x87 or SSE state. Every register they leave
// is reproduced, so the kernels owe (and keep) the full default contract.
#include "audio/native_tu15_sound_kernel.hpp"

namespace halo2::native_tu15_sound {
namespace {
// Retail globals read by the property routines.
constexpr std::uint32_t global_scenario=0x4ecc20u;      // scenario tag data; +0x324 sound effect collection datum
constexpr std::uint32_t property_override=0x51df90u;   // runtime record; +6 a 16-bit value, 0xFFFF for none
constexpr std::uint32_t sound_group=0x736e6421u;       // 'snd!'
constexpr std::uint32_t none=0xffffffffu;

// MOVSX r32,r16 of a datum's index, then SHL 4: the tag-instance row offset
// the group check uses (indices from 0x8000 address rows below the table).
std::uint32_t signed_row(std::uint32_t datum){return std::uint32_t(std::int32_t(std::int16_t(datum)))<<4;}
// AND 0FFFFh, SHL 4: the row offset the data-pointer read uses.
std::uint32_t unsigned_row(std::uint32_t datum){return (datum&0xffffu)<<4;}

// 21FEB0 body (also the tail of 18FF60 and 18FE80): the custom playback row
// of a sound definition. EAX ends as the row, or 0 for index -1; on a row
// ECX holds the gestalt and EDX its custom playback rows.
void definition_custom_playback(const M &m,Abi &a,std::uint32_t definition) {
    const auto index=u8(m,definition+0xdu);
    if(index==0xffu){a.gp[0]=0;return;}
    a.gp[1]=u32(m,sound_gestalt);
    a.gp[2]=u32(m,a.gp[1]+0x34u);
    a.gp[0]=std::uint32_t(std::int32_t(std::int8_t(index)))*0x34u+a.gp[2];
}

// The 'snd!' group check and definition read of a sound tag datum. EDX takes
// the tag-instance table; false when the datum is not a sound.
bool sound_definition(const M &m,Abi &a,std::uint32_t datum,std::uint32_t &definition) {
    const auto tags=u32(m,tag_instances);
    a.gp[2]=tags;
    if(u32(m,signed_row(datum)+tags)!=sound_group)return false;
    definition=u32(m,unsigned_row(datum)+tags+8u);
    return true;
}

// 18FF60 body: the custom playback of a sound tag (ECX, -1 for none). EAX is
// the row or 0. ESI is pushed and popped around the group check.
void sound_custom_playback(const M &m,Abi &a) {
    a.gp[0]=0;
    const auto datum=a.gp[1];
    if(datum==none)return;
    std::uint32_t definition;
    if(!sound_definition(m,a,datum,definition))return;
    a.gp[1]=definition;
    definition_custom_playback(m,a,definition);
}

// The custom playback in row (effect & 3FFFFFFFh) of a sound effect
// collection tag (-1 for none): the row's +4.
void collection_custom_playback(const M &m,Abi &a,std::uint32_t collection,std::uint32_t effect) {
    a.gp[1]=collection;
    if(collection==none)return;
    const auto tags=u32(m,tag_instances);
    a.gp[1]=u32(m,unsigned_row(collection)+tags+8u);
    const auto rows=u32(m,a.gp[1]+4u);
    a.gp[2]=(effect&0x3fffffffu)*0x38u;
    a.gp[0]=rows+a.gp[2]+4u;
}

// 18FE80 body: the custom playback a sound (ECX) plays with an effect
// reference (EDX). -1 is the sound's own; otherwise bits 30-31 select the
// source: 0 the sound's own (without the -1 sound check), 1 a row of the
// globals' sound effect collection (first sound globals row +0xC), 2 a row of
// the scenario's (+0x324), 3 none. EAX is the custom playback or 0; ESI is
// pushed at entry and restored on every path.
void effect_custom_playback(const M &m,Abi &a) {
    a.gp[0]=0;
    const auto datum=a.gp[1],effect=a.gp[2];
    if(effect==none) {
        if(datum==none)return;
        std::uint32_t definition;
        if(!sound_definition(m,a,datum,definition))return;
        a.gp[1]=definition;
        definition_custom_playback(m,a,definition);
        return;
    }
    switch(effect>>30) {
    case 0: {
        std::uint32_t definition;
        if(!sound_definition(m,a,datum,definition))return;
        a.gp[1]=unsigned_row(datum);
        definition_custom_playback(m,a,definition);   // pop esi; jmp 21FEB0
        return;
    }
    case 1: {
        const auto globals=u32(m,sound_globals);
        a.gp[1]=globals;
        if(!u32(m,globals+0xc0u))return;
        const auto rows=u32(m,globals+0xc4u);
        a.gp[1]=rows;
        if(!rows)return;
        collection_custom_playback(m,a,u32(m,rows+0xcu),effect);
        return;
    }
    case 2:
        collection_custom_playback(m,a,u32(m,u32(m,global_scenario)+0x324u),effect);
        return;
    default:
        return;
    }
}

// 1974B0 body: the property index a filter name (string id, ECX) sets, or
// -1 in EAX. Names above 1600013Dh are compared by subtraction, which leaves
// ECX as the difference.
void filter_property(Abi &a) {
    const auto name=a.gp[1];
    a.gp[0]=none;
    if(std::int32_t(name)>std::int32_t(0x1600013du)) {
        a.gp[1]=name-0x1600013eu;
        if(!a.gp[1]){a.gp[0]=0x1du;return;}
        --a.gp[1];
        if(!a.gp[1])a.gp[0]=0x1eu;
        return;
    }
    switch(name) {
    case 0x1600013du:a.gp[0]=0x1cu;break;
    case 0x1300012du:a.gp[0]=0xdu;break;
    case 0x14000135u:a.gp[0]=0xeu;break;
    case 0x1600013cu:a.gp[0]=0x1bu;break;
    default:break;
    }
}

// 18FE80: EDX effect reference, ECX sound tag datum; EAX custom playback or 0.
void effect_custom_playback_entry(M &m,Abi &a,FP,const O &,const Services &) {
    effect_custom_playback(m,a);
    pop_return(m,a);
}
// 18FF60: ECX sound tag datum; EAX custom playback or 0.
void sound_custom_playback_entry(M &m,Abi &a,FP,const O &,const Services &) {
    sound_custom_playback(m,a);
    pop_return(m,a);
}
// 1974B0: ECX filter name; EAX property index or -1.
void filter_property_entry(M &m,Abi &a,FP,const O &,const Services &) {
    filter_property(a);
    pop_return(m,a);
}
// 21FEB0: EAX sound definition; EAX custom playback or 0.
void definition_custom_playback_entry(M &m,Abi &a,FP,const O &,const Services &) {
    definition_custom_playback(m,a,a.gp[0]);
    pop_return(m,a);
}

// 2290C0: sound properties of a playing sound. EBX: the record (+0 flags,
// bit 8 an effect reference at +0x98); EDI: its sound tag datum. The effect's
// custom playback (18FE80) wins over the sound's own (18FF60) when it has
// filters. Its first filter's tag (+0x14 count, +0x18 rows) names a property
// at row +8: name 1300012Dh copies the override record's +6 (unless 0xFFFF)
// to +0x8E and sets flag 0x40; a known name (1974B0) stores its index at
// +0x90 and sets flag 0x200. ESI is pushed and popped; EAX, ECX and EDX are
// left as the original leaves them.
void sound_properties(M &m,Abi &a,FP,const O &o,const Services &) {
    const auto record=a.gp[3],sound=a.gp[7];
    a.gp[0]=u32(m,record);
    a.gp[2]=(a.gp[0]&0x100u)?u32(m,record+0x98u):none;
    a.gp[1]=sound;
    effect_custom_playback(m,a);
    const auto effect=a.gp[0];
    a.gp[1]=sound;
    sound_custom_playback(m,a);
    const auto own=a.gp[0];
    std::uint32_t chosen=0;
    if(own) {
        a.gp[2]=u32(m,own+0x2cu);
        if(std::int32_t(a.gp[2])>0)chosen=own;
    }
    if(effect) {
        a.gp[0]=u32(m,effect+0x2cu);
        if(std::int32_t(a.gp[0])>0)chosen=effect;
    }
    a.gp[1]=chosen;
    if(chosen) {
        // The original re-reads the chosen filter count; it is positive here,
        // so its zero-filter fallback (filter rows at address 0) never runs.
        a.gp[0]=u32(m,chosen+0x2cu);
        a.gp[1]=std::int32_t(a.gp[0])>0?u32(m,chosen+0x30u):0u;
        a.gp[0]=u32(m,a.gp[1]+4u);
        if(a.gp[0]!=none) {
            const auto tags=u32(m,tag_instances);
            a.gp[1]=tags;
            a.gp[0]=u32(m,unsigned_row(a.gp[0])+tags+8u);
            a.gp[1]=u32(m,a.gp[0]+0x14u);
            if(std::int32_t(a.gp[1])>0) {
                a.gp[0]=u32(m,a.gp[0]+0x18u);
                const auto filter=a.gp[0];
                if(u32(m,filter+8u)==0x1300012du) {
                    a.gp[2]=u32(m,property_override);
                    const auto value=u16(m,a.gp[2]+6u);
                    a.gp[1]=(a.gp[1]&0xffff0000u)|value;
                    if(value!=0xffffu) {
                        store16(m,o,0x22914fu,record+0x8eu,value);
                        store(m,o,0x229156u,record,u32(m,record)|0x40u);
                    }
                }
                a.gp[1]=u32(m,filter+8u);
                filter_property(a);
                if((a.gp[0]&0xffffu)!=0xffffu) {
                    store8(m,o,0x229167u,record+0x90u,a.gp[0]);
                    store(m,o,0x22916du,record,u32(m,record)|0x200u);
                }
            }
        }
    }
    pop_return(m,a);
}

constexpr Kernel properties_table[]{
    {0x18fe80u,effect_custom_playback_entry},
    {0x18ff60u,sound_custom_playback_entry},
    {0x1974b0u,filter_property_entry},
    {0x21feb0u,definition_custom_playback_entry},
    {0x2290c0u,sound_properties},
};
} // namespace
std::span<const Kernel> properties_kernels(){return properties_table;}
} // namespace halo2::native_tu15_sound
