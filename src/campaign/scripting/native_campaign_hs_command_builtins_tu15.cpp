#include "halo2/native_campaign_hs_command_builtins_tu15.hpp"
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include "halo2/native_object_factory_unitactions_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2 {
namespace {
using U=std::uint32_t;
struct Builtins {
    SourcePhysicsContactMemory &memory;
    SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignHsCommandChildrenTu15 &children;
    const SourcePhysicsContactRecordObserver &observe;
    U read(U address,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));U value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<U>(bytes[n])<<(n*8u);
        return value;
    }
    void write(U pc,U address,U value,unsigned width=4) {
        const bool observed=native_tu15_observes_retained_store(memory,observe,pc,address,width);
        SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(observed){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observed)observe(event);
    }
    void push(U pc,U value){abi.gp[4]-=4u;write(pc,abi.gp[4],value);}
    U pop(){const auto result=read(abi.gp[4]);abi.gp[4]+=4u;return result;}
    void call(U pc,U target,unsigned arguments) {
        const auto expected=abi.gp[4]+arguments;
        push(pc,pc+5u);
        if(!children.call)throw NativeCampaignAiPerformRequiredOwnerTu15(pc,target);
        children.call(pc,target,abi);
        if(abi.gp[4]!=expected)throw std::runtime_error("Native HS child returned another source frame");
    }
    static U signed16(U value){return U(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}
    void at(U pc){if(children.instruction)children.instruction(pc);}
    void parse_arguments_saving_esi(U entry) {
        auto &g=abi.gp;
        at(entry);g[0]=signed16(read(g[4]+4u,2));
        at(entry+0x5u);g[0]=read(0x480c00u+g[0]*4u);
        at(entry+0xcu);g[1]=read(g[4]+12u);
        at(entry+0x10u);push(entry+0x10u,g[6]);
        at(entry+0x11u);g[6]=read(g[4]+12u);
        at(entry+0x15u);push(entry+0x15u,g[1]);
        at(entry+0x16u);g[2]=g[0]+14u;
        at(entry+0x19u);g[0]=signed16(read(g[0]+12u,2));
        at(entry+0x1du);push(entry+0x1du,g[2]);
        at(entry+0x1eu);push(entry+0x1eu,g[0]);
        at(entry+0x1fu);push(entry+0x1fu,g[6]);
        at(entry+0x20u);call(entry+0x20u,0x212010u,16u);
    }
    void play_line() {
        parse_arguments_saving_esi(0x2b0140u);
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        flags.load(abi.eflags);
        at(0x2b0165u);flags.logic(g[0],4);
        abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();
        at(0x2b0167u);
        if(g[0]) {
            at(0x2b0169u);g[1]=read(g[0]);
            at(0x2b016bu);push(0x2b016bu,g[7]);
            at(0x2b016cu);g[7]=read(g[0]+4u);
            at(0x2b016fu);call(0x2b016fu,0x27fe70u,0u);
            at(0x2b0174u);g[1]=signed16(g[0]);
            at(0x2b0177u);abi.xmm[0][0]=std::bit_cast<U>(static_cast<float>(std::bit_cast<std::int32_t>(g[1])));
            at(0x2b017bu);write(0x2b017bu,g[4]+12u,abi.xmm[0][0]);
            at(0x2b0181u);g[1]=g[6];
            at(0x2b0183u);g[0]=read(g[4]+12u);
            at(0x2b0187u);call(0x2b0187u,0x211da0u,0u);
            at(0x2b018cu);g[7]=pop();
        }
        at(0x2b018du);g[6]=pop();
        at(0x2b018eu);pop();g[4]+=12u;
    }
    void play_line_at_player() {
        parse_arguments_saving_esi(0x2b01a0u);
        auto &g=abi.gp;
        at(0x2b01c5u);g[1]=g[0];
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        flags.load(abi.eflags);
        at(0x2b01c7u);flags.logic(g[1],4);
        abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();
        at(0x2b01c9u);
        if(g[1]) {
            at(0x2b01cbu);g[0]=read(g[1]+4u);
            at(0x2b01ceu);g[1]=read(g[1]);
            at(0x2b01d0u);call(0x2b01d0u,0x27fee0u,0u);
            at(0x2b01d5u);g[1]=signed16(g[0]);
            // Every signed 16-bit result is represented exactly in binary32.
            // CVTSI2SS retains the other three lanes and does not round it.
            at(0x2b01d8u);abi.xmm[0][0]=std::bit_cast<U>(static_cast<float>(std::bit_cast<std::int32_t>(g[1])));
            at(0x2b01dcu);write(0x2b01dcu,g[4]+8u,abi.xmm[0][0]);
            at(0x2b01e2u);g[1]=g[6];
            at(0x2b01e4u);g[0]=read(g[4]+8u);
            at(0x2b01e8u);call(0x2b01e8u,0x211da0u,0u);
        }
        at(0x2b01edu);g[6]=pop();
        at(0x2b01eeu);pop();g[4]+=12u;
    }
    void shoot_at_actor() {
        parse_arguments_saving_esi(0x2b14b0u);
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2b14d5u);flags.logic(g[0],4);publish();
        at(0x2b14d7u);
        if(g[0]) {
            at(0x2b14d9u);g[2]=read(g[0]+4u);
            at(0x2b14dcu);g[1]=(g[1]&0xffffff00u)|read(g[0],1);
            at(0x2b14deu);call(0x2b14deu,0x280ee0u,0u);
            at(0x2b14e3u);g[0]=flags.logic(0u,4);publish();
            at(0x2b14e5u);g[1]=g[6];
            at(0x2b14e7u);call(0x2b14e7u,0x211da0u,0u);
        }
        at(0x2b14ecu);g[6]=pop();
        at(0x2b14edu);pop();g[4]+=12u;
    }
    // 280EE0 publishes the authored shot request on the active command row.
    void set_command_shot_target() {
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U none=0xffffffffu,arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x280ee0u);g[0]=read(0x50eee8u);
        at(0x280ee5u);flags.minus(g[0],none,4);publish();
        at(0x280ee8u);
        if(g[0]!=none) {
            at(0x280eeau);g[0]=flags.logic(g[0]&0xffffu,4);publish();
            at(0x280eefu);g[0]=flags.product(g[0],0xd4u);publish();
            at(0x280ef5u);push(0x280ef5u,g[6]);
            at(0x280ef6u);g[6]=read(0x50eee0u);
            at(0x280efcu);g[0]=flags.plus(g[0],read(g[6]+0x44u),4);publish();
            const U enabled=g[1]&0xffu;
            at(0x280effu);flags.logic(enabled,1);publish();
            at(0x280f01u);write(0x280f01u,g[0]+0x45u,enabled,1);
            at(0x280f04u);g[6]=pop();
            at(0x280f05u);
            if(enabled) {
                at(0x280f07u);flags.minus(g[2],none,4);publish();
                at(0x280f0au);
                if(g[2]!=none) {
                    at(0x280f0cu);g[1]=1u;
                    at(0x280f11u);write(0x280f11u,g[0]+0x52u,g[1],1);
                    at(0x280f14u);write(0x280f14u,g[0]+0x54u,g[1],2);
                    at(0x280f18u);write(0x280f18u,g[0]+0x58u,g[2]);
                    at(0x280f1bu);write(0x280f1bu,g[0]+0x46u,0u,1);
                }
            }
        }
        at(0x280f1fu);pop();
    }
    void set_forward_velocity() {
        auto &g=abi.gp;
        at(0x2aba90u);g[0]=signed16(read(g[4]+4u,2));
        at(0x2aba95u);g[0]=read(0x480c00u+g[0]*4u);
        at(0x2aba9cu);g[1]=read(g[4]+12u);
        at(0x2abaa0u);push(0x2abaa0u,g[7]);
        at(0x2abaa1u);g[7]=read(g[4]+12u);
        at(0x2abaa5u);push(0x2abaa5u,g[1]);
        at(0x2abaa6u);g[2]=g[0]+14u;
        at(0x2abaa9u);g[0]=signed16(read(g[0]+12u,2));
        at(0x2abaadu);push(0x2abaadu,g[2]);
        at(0x2abaaeu);push(0x2abaaeu,g[0]);
        at(0x2abaafu);push(0x2abaafu,g[7]);
        at(0x2abab0u);call(0x2abab0u,0x212010u,16u);
        constexpr U arithmetic_flags=0x8d5u;
        const auto logic_flags=[&](U value) {
            physics_contact_runtime_detail::Flags flags;
            flags.load(abi.eflags);flags.logic(value,4);
            abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();
        };
        at(0x2abab5u);logic_flags(g[0]);
        at(0x2abab7u);
        if(g[0]) {
            at(0x2abab9u);abi.xmm[6]={read(g[0]+4u),0u,0u,0u};
            at(0x2ababeu);push(0x2ababeu,g[6]);
            at(0x2ababfu);g[6]=read(g[0]);
            at(0x2abac1u);push(0x2abac1u,0u);
            at(0x2abac3u);push(0x2abac3u,0u);
            at(0x2abac5u);call(0x2abac5u,0x1100c0u,8u);
            at(0x2abacau);g[0]=0u;logic_flags(0u);
            at(0x2abaccu);g[1]=g[7];
            at(0x2abaceu);call(0x2abaceu,0x211da0u,0u);
            at(0x2abad3u);g[6]=pop();
        }
        at(0x2abad4u);g[7]=pop();
        at(0x2abad5u);pop();g[4]+=12u;
    }
    bool actor_vehicle_parent(physics_contact_runtime_detail::Flags &flags) {
        auto &g=abi.gp;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        at(0x27fb4eu);g[6]=read(0x5020c8u);
        at(0x27fb54u);g[1]=read(g[6]+0x38u);
        at(0x27fb57u);g[2]=g[0];
        at(0x27fb59u);g[2]=flags.logic(g[2]&0xffffu,4);publish();
        at(0x27fb5fu);flags.minus(g[2],g[1],4);publish();
        at(0x27fb61u);
        if(flags.sf==flags.of)return false;
        at(0x27fb63u);g[1]=read(g[6]+0x24u);
        at(0x27fb66u);g[1]=flags.product(g[1],g[2]);publish();
        at(0x27fb69u);g[1]=flags.plus(g[1],read(g[6]+0x44u),4);publish();
        at(0x27fb6cu);g[2]=(g[2]&0xffff0000u)|read(g[1],2);
        at(0x27fb6fu);flags.logic(g[2],2);publish();
        at(0x27fb72u);
        if(flags.zf)return false;
        at(0x27fb74u);g[2]=signed16(g[2]);
        at(0x27fb77u);g[0]=flags.right(g[0],16u,4,true);publish();
        at(0x27fb7au);flags.minus(g[2],g[0],4);publish();
        at(0x27fb7cu);
        if(!flags.zf)return false;

        at(0x27fb7eu);g[0]=read(0x4ecbd0u);
        at(0x27fb83u);g[2]=read(g[0]+0x44u);
        at(0x27fb86u);g[0]=read(g[1]+0x18u);
        at(0x27fb89u);g[0]=flags.logic(g[0]&0xffffu,4);publish();
        at(0x27fb8eu);g[1]=g[0]*3u;
        at(0x27fb91u);g[0]=read(g[2]+g[1]*4u+8u);
        at(0x27fb95u);g[0]=read(g[0]+0x14u);
        at(0x27fb98u);flags.minus(g[0],0xffffffffu,4);publish();
        at(0x27fb9bu);
        if(flags.zf)return false;
        at(0x27fb9du);g[1]=g[0];
        at(0x27fb9fu);g[1]=flags.logic(g[1]&0xffffu,4);publish();
        at(0x27fba5u);g[1]*=3u;
        at(0x27fba8u);g[2]=read(g[2]+g[1]*4u+8u);
        at(0x27fbacu);flags.minus(read(g[2]+0xaau,1),1u,1);publish();
        at(0x27fbb3u);
        return flags.zf;
    }
    void lookup_actor_vehicle() {
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x27fb30u);g[0]=g[1];
        at(0x27fb32u);push(0x27fb32u,g[7]);
        at(0x27fb33u);g[0]=flags.right(g[0],30u,4,false);publish();
        at(0x27fb36u);g[7]=flags.logic(0xffffffffu,4);publish();
        at(0x27fb39u);flags.minus(g[0],2u,4);publish();
        at(0x27fb3cu);
        if(!flags.zf) {
            at(0x27fb3eu);flags.minus(g[0],3u,4);publish();
            at(0x27fb41u);
        }
        if(flags.zf) {
            at(0x27fb43u);call(0x27fb43u,0x27c880u,0u);
            flags.load(abi.eflags);
            at(0x27fb48u);flags.minus(g[0],0xffffffffu,4);publish();
            at(0x27fb4bu);
            if(!flags.zf) {
                at(0x27fb4du);push(0x27fb4du,g[6]);
                if(!actor_vehicle_parent(flags)) {
                    at(0x27fbb5u);g[0]=g[7];
                }
                at(0x27fbb7u);g[6]=pop();
                at(0x27fbb8u);g[7]=pop();
                at(0x27fbb9u);pop();return;
            }
        }
        at(0x27fbbau);g[0]=g[7];
        at(0x27fbbcu);g[7]=pop();
        at(0x27fbbdu);pop();
    }
    void ai_vehicle_get() {
        auto &g=abi.gp;
        parse_arguments_saving_esi(0x2afad0u);
        physics_contact_runtime_detail::Flags flags;
        flags.load(abi.eflags);
        at(0x2afaf5u);flags.logic(g[0],4);
        abi.eflags=(abi.eflags&~0x8d5u)|flags.bits();
        at(0x2afaf7u);
        if(g[0]) {
            at(0x2afaf9u);g[1]=read(g[0]);
            const auto stack=g[4];
            at(0x2afafbu);push(0x2afafbu,0x2afb00u);
            lookup_actor_vehicle();
            if(g[4]!=stack)throw std::runtime_error("Native AI vehicle lookup returned another source frame");
            at(0x2afb00u);g[1]=g[6];
            at(0x2afb02u);call(0x2afb02u,0x211da0u,0u);
        }
        at(0x2afb07u);g[6]=pop();
        at(0x2afb08u);pop();g[4]+=12u;
    }
    void set_vehicle_speed() {
        auto &g=abi.gp;
        parse_arguments_saving_esi(0x2b1530u);
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2b1555u);flags.logic(g[0],4);publish();
        at(0x2b1557u);
        if(g[0]) {
            at(0x2b1559u);abi.xmm[0]={read(g[0]),0u,0u,0u};
            at(0x2b155du);g[0]=read(0x50eee8u);
            at(0x2b1562u);flags.minus(g[0],0xffffffffu,4);publish();
            at(0x2b1565u);
            bool set_speed=false;
            if(!flags.zf) {
                at(0x2b1567u);g[1]=read(0x50eee0u);
                at(0x2b156du);g[2]=read(g[1]+0x44u);
                at(0x2b1570u);g[0]=flags.logic(g[0]&0xffffu,4);publish();
                at(0x2b1575u);g[0]=flags.product(g[0],0xd4u);publish();
                at(0x2b157bu);g[0]=flags.plus(g[0],g[2],4);publish();
                at(0x2b157du);
                if(!abi.floating_point)throw std::invalid_argument("Native command speed requires its source FP state");
                const auto scalar=[](U bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));};
                {
                    physics_world_runtime_detail::FpScope environment(*abi.floating_point);
                    flags.scalar_compare(scalar(abi.xmm[0][0]),scalar(read(0x46a0e8u)),false);
                    abi.floating_point->mxcsr=_mm_getcsr();
                }
                publish();
                at(0x2b1584u);set_speed=!flags.cf&&!flags.zf;
                if(set_speed) {
                    at(0x2b1586u);write(0x2b1586u,g[0]+0x64u,1,1);
                    at(0x2b158au);write(0x2b158au,g[0]+0x68u,abi.xmm[0][0]);
                } else {
                    at(0x2b159cu);write(0x2b159cu,g[0]+0x64u,0,1);
                }
            }
            at(set_speed?0x2b158fu:0x2b15a0u);g[0]=flags.logic(0,4);publish();
            at(set_speed?0x2b1591u:0x2b15a2u);g[1]=g[6];
            call(set_speed?0x2b1593u:0x2b15a4u,0x211da0u,0u);
            if(set_speed) {
                at(0x2b1598u);g[6]=pop();at(0x2b1599u);pop();g[4]+=12u;return;
            }
        }
        at(0x2b15a9u);g[6]=pop();
        at(0x2b15aau);pop();g[4]+=12u;
    }
    void ai_vehicle_enter_immediate() {
        auto &g=abi.gp;
        parse_arguments_saving_esi(0x2afcc0u);
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2afce5u);g[1]=g[0];
        at(0x2afce7u);flags.logic(g[1],4);publish();
        at(0x2afce9u);
        if(g[1]) {
            at(0x2afcebu);g[2]=read(g[1]+8u);
            at(0x2afceeu);g[0]=read(g[1]+4u);
            at(0x2afcf1u);g[1]=read(g[1]);
            at(0x2afcf3u);push(0x2afcf3u,1u);
            at(0x2afcf5u);push(0x2afcf5u,g[2]);
            call(0x2afcf6u,0x27e760u,8u);
            at(0x2afcfbu);g[0]=flags.logic(0u,4);publish();
            at(0x2afcfdu);g[1]=g[6];
            call(0x2afcffu,0x211da0u,0u);
        }
        at(0x2afd04u);g[6]=pop();
        at(0x2afd05u);pop();g[4]+=12u;
    }
    void lookup_starting_location_vehicle() {
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x27c9a0u);g[1]=g[2];
        at(0x27c9a2u);g[1]=flags.logic(g[1]&0xc0000000u,4);publish();
        at(0x27c9a8u);g[0]=flags.logic(0xffffffffu,4);publish();
        at(0x27c9abu);flags.minus(g[1],0xc0000000u,4);publish();
        at(0x27c9b1u);
        if(flags.zf) {
            at(0x27c9b7u);g[1]=g[2];
            at(0x27c9b9u);g[1]=flags.right(g[1],16u,4,true);publish();
            at(0x27c9bcu);g[1]=flags.logic(g[1]&0x3fffu,4);publish();
            at(0x27c9c2u);
            at(0x27c9c8u);push(0x27c9c8u,g[3]);
            at(0x27c9c9u);push(0x27c9c9u,g[6]);
            at(0x27c9cau);push(0x27c9cau,g[7]);
            at(0x27c9cbu);g[7]=read(0x4ecc20u);
            at(0x27c9d1u);g[3]=read(g[7]+0x160u);
            at(0x27c9d7u);g[6]=signed16(g[1]);
            at(0x27c9dau);flags.minus(g[6],g[3],4);publish();
            at(0x27c9dcu);
            const bool exhausted=flags.sf!=flags.of && !lookup_starting_location_vehicle_in_squad(flags);
            if(exhausted) {
                at(0x27ca52u);g[7]=pop();
                at(0x27ca53u);g[6]=pop();
                at(0x27ca54u);g[3]=pop();
                at(0x27ca55u);pop();return;
            }
            at(0x27ca58u);g[7]=pop();
            at(0x27ca59u);g[6]=pop();
            at(0x27ca5au);g[3]=pop();
        }
        at(0x27ca5bu);pop();
    }
    bool lookup_starting_location_vehicle_in_squad(physics_contact_runtime_detail::Flags &flags) {
        auto &g=abi.gp;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        at(0x27c9deu);g[3]=read(g[7]+0x164u);
        at(0x27c9e4u);g[6]=g[1]&0xffffu;
        at(0x27c9e7u);g[1]=g[6];
        at(0x27c9e9u);g[1]=flags.product(g[1],0x74u);publish();
        at(0x27c9ecu);g[1]=flags.plus(g[1],g[3],4);publish();
        at(0x27c9eeu);flags.logic(g[2],2);publish();
        at(0x27c9f1u);
        if(flags.sf)return true;
        at(0x27c9f3u);g[7]=read(g[1]+0x48u);
        at(0x27c9f6u);g[2]=signed16(g[2]);
        at(0x27c9f9u);flags.minus(g[2],g[7],4);publish();
        at(0x27c9fbu);
        if(flags.sf==flags.of)return true;
        at(0x27c9fdu);g[7]=read(g[1]+0x4cu);
        at(0x27ca00u);g[6]=flags.product(g[6],0x98u);publish();
        at(0x27ca06u);g[2]=flags.product(g[2],0x64u);publish();
        at(0x27ca09u);g[1]=read(0x52be7cu);
        at(0x27ca0fu);g[1]=read(g[1]+0x44u);
        at(0x27ca12u);g[6]=read(g[6]+g[1]+0x70u);
        at(0x27ca16u);g[2]=flags.plus(g[2],g[7],4);publish();
        at(0x27ca18u);flags.minus(g[6],0xffffffffu,4);publish();
        at(0x27ca1bu);
        if(flags.zf)return true;
        at(0x27ca1du);g[1]=read(0x4ecbd0u);
        at(0x27ca23u);g[7]=read(g[1]+0x44u);
        at(0x27ca26u);g[2]=read(g[2]);
        at(0x27ca28u);
        for(;;) {
            at(0x27ca30u);g[1]=g[6];
            at(0x27ca32u);g[1]=flags.logic(g[1]&0xffffu,4);publish();
            at(0x27ca38u);g[1]*=3u;
            at(0x27ca3bu);g[1]=read(g[7]+g[1]*4u+8u);
            at(0x27ca3fu);flags.minus(read(g[1]+0x3a8u),g[2],4);publish();
            at(0x27ca45u);
            if(flags.zf) {
                at(0x27ca56u);g[0]=g[6];return true;
            }
            at(0x27ca47u);g[6]=read(g[1]+0x3a4u);
            at(0x27ca4du);flags.minus(g[6],0xffffffffu,4);publish();
            at(0x27ca50u);
            if(flags.zf)return false;
        }
    }
    void starting_location_vehicle_get() {
        auto &g=abi.gp;
        parse_arguments_saving_esi(0x2afb10u);
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2afb35u);flags.logic(g[0],4);publish();
        at(0x2afb37u);
        if(g[0]) {
            at(0x2afb39u);g[2]=read(g[0]);
            at(0x2afb3bu);g[1]=g[2];
            at(0x2afb3du);g[1]=flags.logic(g[1]&0xc0000000u,4);publish();
            at(0x2afb43u);g[0]=flags.logic(0xffffffffu,4);publish();
            at(0x2afb46u);flags.minus(g[1],0xc0000000u,4);publish();
            at(0x2afb4cu);
            if(flags.zf) {
                const auto stack=g[4];
                at(0x2afb4eu);push(0x2afb4eu,0x2afb53u);
                lookup_starting_location_vehicle();
                if(g[4]!=stack)throw std::runtime_error("Native starting-location lookup returned another source frame");
            }
            at(0x2afb53u);g[1]=g[6];
            call(0x2afb55u,0x211da0u,0u);
        }
        at(0x2afb5au);g[6]=pop();
        at(0x2afb5bu);pop();g[4]+=12u;
    }
    // 27E760 from the public immediate command. The mapped seat and the
    // actor list belong to this source frame. Action dispatch owns transfer.
    void board_vehicle_immediately() {
        using namespace physics_world_runtime_detail;
        using halo2::native_object_factory_tu15::unit_actions::mapped_seats;
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic=0x8d5u,none=0xffffffffu;
        const auto set_flags=[&]{abi.eflags=(abi.eflags&~arithmetic)|flags.bits();};
        const auto actor_reference=g[1],vehicle=g[0],mapping=read(g[4]+4u);
        if(read(g[4]+8u,1)!=1u)
            throw std::invalid_argument("Immediate campaign boarding requires the retained script flag");
        flags.load(abi.eflags);
        at(0x27e760u);g[4]=flags.minus(g[4],0x6acu,4);set_flags();
        at(0x27e766u);push(0x27e766u,g[6]);
        at(0x27e767u);push(0x27e767u,g[7]);
        at(0x27e768u);g[7]=actor_reference;
        at(0x27e76au);flags.minus(actor_reference,none,4);set_flags();
        at(0x27e76du);g[6]=vehicle;
        at(0x27e76fu);
        if(actor_reference!=none) {
            at(0x27e775u);push(0x27e775u,3u);
            call(0x27e777u,0xbcdf0u,4u);
            flags.load(abi.eflags);
            at(0x27e77cu);flags.logic(g[0],4);set_flags();
            at(0x27e77eu);
            if(g[0]) {
                at(0x27e784u);push(0x27e784u,g[3]);
                at(0x27e785u);push(0x27e785u,g[5]);
                at(0x27e786u);g[2]=vehicle;
                at(0x27e788u);g[5]=flags.logic(0u,4);set_flags();
                call(0x27e78au,0x1c1860u,0u);
                const auto frame=g[4],root_vehicle=g[0];
                at(0x27e78fu);g[6]=frame+0x20u;
                at(0x27e793u);g[3]=root_vehicle;
                call(0x27e795u,0xbbe00u,0u);
                const auto seats=mapped_seats(memory,mapping,root_vehicle,4u,2u,64);
                const auto seat_rows=frame+0x3bcu;
                for(U index=0;index<seats.size();++index) {
                    const auto &seat=seats[index];
                    const auto row=seat_rows+12u*index;
                    write(0x219ab3u,row,seat.object);
                    write(0x219ab8u,row+4u,U(std::uint16_t(seat.seat)),2);
                    write(0x219ac1u,row+8u,seat.definition);
                }
                g[0]=U(seats.size());
                at(0x27e7b7u);flags.logic(g[0],2);set_flags();
                write(0x27e7bau,frame+0x18u,g[0]);
                at(0x27e7beu);
                if(!seats.empty())select_boarding_actors(frame,actor_reference,root_vehicle,U(seats.size()));
                at(0x27eaa1u);g[5]=pop();
                at(0x27eaa2u);g[3]=pop();
            }
        }
        at(0x27eaa3u);g[7]=pop();
        at(0x27eaa4u);g[6]=pop();
        flags.load(abi.eflags);
        at(0x27eaa5u);g[4]=flags.plus(g[4],0x6acu,4);set_flags();
        at(0x27eaabu);pop();g[4]+=8u;
    }
    U boarding_distance(U center,U actor) {
        using namespace physics_world_runtime_detail;
        if(!abi.floating_point)throw std::invalid_argument("Native boarding requires its source FP state");
        const auto scalar=[](U bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));};
        FpScope fp(*abi.floating_point);
        const auto dx=_mm_sub_ss(scalar(read(center)),scalar(read(actor+0x238u)));
        const auto dy=_mm_sub_ss(scalar(read(center+4u)),scalar(read(actor+0x23cu)));
        const auto dz=_mm_sub_ss(scalar(read(center+8u)),scalar(read(actor+0x240u)));
        const auto dzy=_mm_add_ss(_mm_mul_ss(dz,dz),_mm_mul_ss(dy,dy));
        const auto result=_mm_add_ss(dzy,_mm_mul_ss(dx,dx));
        abi.floating_point->mxcsr=_mm_getcsr();
        return std::bit_cast<U>(_mm_cvtsi128_si32(_mm_castps_si128(result)));
    }
    void select_boarding_actors(U frame,U reference,U root_vehicle,U seat_count) {
        auto &g=abi.gp;
        const auto iterator=frame+0x8cu,actors=frame+0xbcu;
        at(0x27e7c4u);g[6]=iterator;
        at(0x27e7cbu);g[1]=reference;
        call(0x27e7cdu,0x27caa0u,0u);
        call(0x27e7d2u,0x27cb30u,0u);
        U count=0;
        while(g[0]) {
            const auto actor=g[0];
            if(read(actor+0x26cu)!=root_vehicle && count<64u) {
                const auto row=actors+count*12u;
                const auto datum=read(iterator+8u);
                write(0x27e80fu,row,datum);
                write(0x27e859u,row+4u,boarding_distance(frame+0x20u,actor));
                at(0x27e854u);g[7]=0x4cu;
                g[1]=datum;
                call(0x27e862u,0x1afbd0u,0u);
                write(0x27e86eu,row+8u,(g[0]&0xffffu)!=0xffffu,1);
                ++count;
            }
            at(0x27e876u);g[6]=iterator;
            call(0x27e87du,0x27cb30u,0u);
        }
        at(0x27e88au);push(0x27e88au,0x27e720u);
        g[0]=count;
        at(0x27e892u);push(0x27e892u,12u);
        at(0x27e894u);push(0x27e894u,count);
        at(0x27e895u);g[1]=actors;
        at(0x27e89cu);push(0x27e89cu,actors);
        call(0x27e89du,0x32b560u,0u);
        at(0x27e8a2u);g[4]+=16u;
        if(count) {
            write(0x27e8b8u,frame+0x10u,actors);
            write(0x27e8bcu,frame+0x1cu,count);
        }
        for(U index=0;index<count;++index) {
            const auto row=actors+index*12u;
            const auto actor=read(read(0x5020c8u)+0x44u)+(read(row)&0xffffu)*0x888u;
            const auto unit=read(actor+0x18u);
            const auto seat=choose_boarding_seat(frame,seat_count,unit,actor);
            if(seat)board_selected_seat(frame,seat,unit,actor);
            write(0x27ea93u,frame+0x10u,row+12u);
            write(0x27ea97u,frame+0x1cu,count-index-1u);
        }
    }
    U choose_boarding_seat(U frame,U count,U unit,U actor) {
        using namespace physics_world_runtime_detail;
        auto &g=abi.gp;
        U selected=0,best=0;
        write(0x27e8eau,frame+0x14u,best);
        for(U index=0;index<count;++index) {
            const auto row=frame+0x3bcu+index*12u;
            if(read(row)==0xffffffffu || read(row+4u,2)==0xffffu)continue;
            g[0]=read(row);
            g[1]=unit;
            g[2]=read(row+4u,2);
            g[5]=index;
            g[6]=actor;
            g[7]=row+8u;
            call(0x27e917u,0xca6e0u,0u);
            if(!(g[0]&0xffu))continue;
            const auto bits=read(read(row+8u));
            const auto priority=read(bits&4u?0x46a0e4u:bits&8u?0x46a11cu:bits&0x800u?0x46a178u:0x46a0d0u);
            if(!abi.floating_point)throw std::invalid_argument("Native boarding requires its source FP state");
            physics_contact_runtime_detail::Flags flags;
            const auto scalar=[](U bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));};
            {
                FpScope fp(*abi.floating_point);
                at(0x27e963u);flags.scalar_compare(scalar(priority),scalar(best),false);
                abi.floating_point->mxcsr=_mm_getcsr();
            }
            if(flags.cf || flags.zf)continue;
            best=priority;
            selected=row;
            write(0x27e96au,frame+0x14u,best);
        }
        return selected;
    }
    void board_selected_seat(U frame,U seat,U unit,U actor) {
        auto &g=abi.gp;
        constexpr U none=0xffffffffu;
        const auto object_bank=read(read(0x4ecbd0u)+0x44u);
        const auto body=read(object_bank+(unit&0xffffu)*12u+8u);
        const auto old_parent=read(body+0x14u);
        g[5]=seat;g[6]=actor;
        if(old_parent!=none && read(body+0x1fcu,2)!=0xffffu && old_parent!=read(seat)) {
            at(0x27e9d3u);push(0x27e9d3u,unit);
            at(0x27e9d7u);g[1]=0x1eu;
            call(0x27e9d9u,0xe8e10u,4u);
        }
        const auto request=frame+0x2cu;
        g[0]=unit;g[1]=read(seat);g[2]=(g[2]&0xffff0000u)|read(seat+4u,2);g[7]=request;
        write(0x27e9f7u,request,0x1cu);
        write(0x27e9ffu,request+4u,g[1]);
        write(0x27ea03u,request+8u,g[2],2);
        write(0x27ea08u,request+0xau,1u,1);
        write(0x27ea0du,request+0xbu,0u,1);
        call(0x27ea12u,0xe8e50u,0u);
        if(g[0]&0xffu) {
            write(0x27ea7au,seat,none);
            write(0x27ea81u,seat+4u,0xffffu,2);
        }
    }
    void compare_boarding_actor_rows() {
        using namespace physics_world_runtime_detail;
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic)|flags.bits();};
        flags.load(abi.eflags);
        at(0x27e720u);g[0]=read(g[4]+4u);
        at(0x27e724u);g[2]=read(g[4]+8u);
        const auto a=g[0],b=g[2];
        at(0x27e728u);g[1]=(g[1]&0xffffff00u)|read(a+8u,1);
        at(0x27e72bu);flags.minus(g[1],read(b+8u,1),1);publish();
        at(0x27e72eu);
        if(!flags.zf) {
            at(0x27e730u);g[0]=flags.logic(0u,4);publish();
            at(0x27e732u);flags.logic(g[1],1);publish();
            at(0x27e734u);g[0]=!flags.zf;
            at(0x27e737u);g[0]=g[0]*2u-1u;
            at(0x27e73bu);pop();return;
        }
        if(!abi.floating_point)throw std::invalid_argument("Native boarding actor order requires source FP state");
        const auto scalar=[](U bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));};
        const auto compare=[&](U pc,U load_pc,U lhs,U rhs) {
            at(load_pc);abi.xmm[0]={read(lhs+4u),0u,0u,0u};
            FpScope fp(*abi.floating_point);
            at(pc);flags.scalar_compare(scalar(abi.xmm[0][0]),scalar(read(rhs+4u)),false);
            abi.floating_point->mxcsr=_mm_getcsr();
            publish();
            return !flags.cf&&!flags.zf;
        };
        if(compare(0x27e741u,0x27e73cu,b,a)) {
            at(0x27e745u);
            at(0x27e747u);g[0]=flags.logic(0xffffffffu,4);publish();
            at(0x27e74au);pop();return;
        }
        at(0x27e745u);
        if(compare(0x27e750u,0x27e74bu,a,b)) {
            at(0x27e754u);
            at(0x27e756u);g[0]=1u;
            at(0x27e75bu);pop();return;
        }
        at(0x27e754u);
        at(0x27e75cu);g[0]=flags.logic(0u,4);publish();
        at(0x27e75eu);pop();
    }
    void object_hide() {
        auto &g=abi.gp;
        parse_arguments_saving_esi(0x2ab320u);
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2ab345u);flags.logic(g[0],4);publish();
        at(0x2ab347u);
        if(g[0]) {
            at(0x2ab349u);g[1]=read(g[0]);
            at(0x2ab34bu);flags.minus(g[1],0xffffffffu,4);publish();
            at(0x2ab34eu);
            if(!flags.zf) {
                at(0x2ab350u);g[2]=flags.logic(0u,4);publish();
                at(0x2ab352u);g[2]=read(g[0]+4u,1);
                at(0x2ab355u);g[0]=g[1];
                at(0x2ab357u);push(0x2ab357u,g[2]);
                call(0x2ab358u,0xbbc90u,4u);
            }
            at(0x2ab35du);g[0]=flags.logic(0u,4);publish();
            at(0x2ab35fu);g[1]=g[6];
            call(0x2ab361u,0x211da0u,0u);
        }
        at(0x2ab366u);g[6]=pop();
        at(0x2ab367u);pop();g[4]+=12u;
    }
    void transition_hidden_state() {
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic)|flags.bits();};
        flags.load(abi.eflags);
        at(0xbbc90u);g[1]=read(0x4ecbd0u);
        at(0xbbc96u);g[2]=read(g[1]+0x44u);
        at(0xbbc99u);push(0xbbc99u,g[6]);
        at(0xbbc9au);g[6]=g[0];
        at(0xbbc9cu);g[0]=flags.logic(g[0]&0xffffu,4);publish();
        at(0xbbca1u);g[0]*=3u;
        at(0xbbca4u);push(0xbbca4u,g[7]);
        at(0xbbca5u);g[7]=read(g[2]+g[0]*4u+8u);
        at(0xbbca9u);g[0]=(g[0]&0xffffff00u)|read(g[4]+12u,1);
        at(0xbbcadu);flags.logic(g[0],1);publish();
        const auto hiding=!flags.zf;
        at(0xbbcafu);g[0]=(g[0]&0xffffff00u)|read(g[7]+4u,1);
        at(0xbbcb2u);
        if(hiding) {
            at(0xbbcb4u);flags.logic(g[0]&1u,1);publish();
            at(0xbbcb6u);
            if(!flags.zf)goto unchanged;
            at(0xbbcb8u);g[0]=g[6];
            call(0xbbcbau,0xbbd50u,0u);
            at(0xbbcbfu);flags.logic(g[0],1);publish();
            at(0xbbcc1u);
            if(!flags.zf) {
                at(0xbbcc3u);push(0xbbcc3u,0u);
                at(0xbbcc5u);push(0xbbcc5u,0u);
                at(0xbbcc7u);push(0xbbcc7u,0u);
                at(0xbbcc9u);push(0xbbcc9u,1u);
                at(0xbbccbu);push(0xbbccbu,g[6]);
                call(0xbbcccu,0xc0f60u,20u);
            }
            at(0xbbcd1u);write(0xbbcd1u,g[7]+4u,flags.logic(read(g[7]+4u)|1u,4));publish();
            call(0xbbcd5u,0xbaba0u,0u);
            at(0xbbcdau);g[7]=pop();
            at(0xbbcdbu);g[6]=pop();
            at(0xbbcdcu);pop();g[4]+=4u;return;
        }
        at(0xbbcdfu);flags.logic(g[0]&1u,1);publish();
        at(0xbbce1u);
        if(!flags.zf) {
            at(0xbbce3u);g[2]=read(g[7]+4u);
            at(0xbbce6u);g[2]=flags.logic(g[2]&~1u,4);publish();
            at(0xbbce9u);g[0]=g[6];
            at(0xbbcebu);write(0xbbcebu,g[7]+4u,g[2]);
            call(0xbbceeu,0xbbd50u,0u);
            at(0xbbcf3u);flags.logic(g[0],1);publish();
            at(0xbbcf5u);
            if(!flags.zf) {
                at(0xbbcf7u);push(0xbbcf7u,0u);
                at(0xbbcf9u);push(0xbbcf9u,0u);
                at(0xbbcfbu);push(0xbbcfbu,1u);
                at(0xbbcfdu);push(0xbbcfdu,0u);
                at(0xbbcffu);push(0xbbcffu,g[6]);
                call(0xbbd00u,0xc0f60u,20u);
            }
            call(0xbbd05u,0xbaba0u,0u);
        }
    unchanged:
        at(0xbbd0au);g[7]=pop();
        at(0xbbd0bu);g[6]=pop();
        at(0xbbd0cu);pop();g[4]+=4u;
    }
    void ai_get_object() {
        auto &g=abi.gp;
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic=0x8d5u;
        const auto publish=[&]{abi.eflags=(abi.eflags&~arithmetic)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2aeb30u);g[0]=signed16(read(g[4]+4u,2));
        at(0x2aeb35u);g[0]=read(0x480c00u+g[0]*4u);
        at(0x2aeb3cu);g[1]=read(g[4]+12u);
        at(0x2aeb40u);g[4]=flags.minus(g[4],0x30u,4);publish();
        at(0x2aeb43u);push(0x2aeb43u,g[3]);
        at(0x2aeb44u);g[3]=read(g[4]+0x3cu);
        at(0x2aeb48u);push(0x2aeb48u,g[1]);
        at(0x2aeb49u);g[2]=g[0]+14u;
        at(0x2aeb4cu);g[0]=signed16(read(g[0]+12u,2));
        at(0x2aeb50u);push(0x2aeb50u,g[2]);
        at(0x2aeb51u);push(0x2aeb51u,g[0]);
        at(0x2aeb52u);push(0x2aeb52u,g[3]);
        call(0x2aeb53u,0x212010u,16u);
        flags.load(abi.eflags);
        at(0x2aeb58u);flags.logic(g[0],4);publish();
        at(0x2aeb5au);
        if(g[0]) {
            at(0x2aeb5cu);g[0]=read(g[0]);
            at(0x2aeb5eu);push(0x2aeb5eu,g[7]);
            at(0x2aeb5fu);g[7]=flags.logic(0xffffffffu,4);publish();
            at(0x2aeb62u);flags.minus(g[0],0xffffffffu,4);publish();
            at(0x2aeb65u);
            if(!flags.zf) {
                at(0x2aeb67u);push(0x2aeb67u,g[6]);
                at(0x2aeb68u);g[6]=g[4]+12u;
                at(0x2aeb6cu);g[1]=g[0];
                call(0x2aeb6eu,0x27caa0u,0u);
                call(0x2aeb73u,0x27cb30u,0u);
                at(0x2aeb78u);flags.logic(g[0],4);publish();
                at(0x2aeb7au);g[6]=pop();
                at(0x2aeb7bu);
                if(!flags.zf) {
                    at(0x2aeb7du);g[7]=read(g[0]+0x18u);
                }
            }
            at(0x2aeb80u);g[0]=g[7];
            at(0x2aeb82u);g[1]=g[3];
            call(0x2aeb84u,0x211da0u,0u);
            at(0x2aeb89u);g[7]=pop();
        }
        at(0x2aeb8au);g[3]=pop();
        flags.load(abi.eflags);
        at(0x2aeb8bu);g[4]=flags.plus(g[4],0x30u,4);publish();
        at(0x2aeb8eu);pop();g[4]+=12u;
    }
    void abort_on_combat_status() {
        auto &g=abi.gp;
        parse_arguments_saving_esi(0x2b2340u);
        physics_contact_runtime_detail::Flags flags;
        constexpr U arithmetic_flags=0x8d5u;
        const auto publish_flags=[&]{abi.eflags=(abi.eflags&~arithmetic_flags)|flags.bits();};
        flags.load(abi.eflags);
        at(0x2b2365u);flags.logic(g[0],4);publish_flags();
        at(0x2b2367u);
        if(g[0]) {
            at(0x2b2369u);g[1]=read(0x50eee8u);
            at(0x2b236fu);flags.minus(g[1],0xffffffffu,4);publish_flags();
            at(0x2b2372u);
            if(g[1]!=0xffffffffu) {
                at(0x2b2374u);g[2]=read(0x50eee0u);
                at(0x2b237au);g[2]=read(g[2]+0x44u);
                at(0x2b237du);g[0]=(g[0]&0xffff0000u)|read(g[0],2);
                at(0x2b2380u);g[1]=flags.logic(g[1]&0xffffu,4);publish_flags();
                at(0x2b2386u);g[1]=flags.product(g[1],0xd4u);publish_flags();
                at(0x2b238cu);write(0x2b238cu,g[1]+g[2]+0x7cu,g[0],2);
            }
            at(0x2b2391u);g[0]=flags.logic(0,4);publish_flags();
            at(0x2b2393u);g[1]=g[6];
            at(0x2b2395u);call(0x2b2395u,0x211da0u,0);
        }
        at(0x2b239au);g[6]=pop();
        at(0x2b239bu);pop();g[4]+=12u;
    }
    void flag(U entry) {
        auto &g=abi.gp;const bool moving=entry==0x2b2470u,looking=entry==0x2b2410u;
        parse_arguments_saving_esi(entry);
        if(g[0]) {
            g[1]=read(0x50eee8u);
            if(g[1]!=0xffffffffu) {
                g[2]=read(0x50eee0u);g[2]=read(g[2]+0x44u);
                g[0]=(g[0]&0xffffff00u)|read(g[0],1);
                g[1]=(g[1]&0xffffu)*0xd4u;
                write(entry+0x4bu,g[1]+g[2]+(looking?0x80u:(moving?0x81u:0x76u)),g[0],1);
            }
            g[0]=0;g[1]=g[6];call(entry+(moving||looking?0x56u:0x53u),0x211da0u,0);
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void face(U entry) {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(entry+0x10u,g[3]);g[3]=read(g[4]+12u);
        push(entry+0x15u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(entry+0x1du,g[2]);push(entry+0x1eu,g[0]);push(entry+0x1fu,g[3]);
        call(entry+0x20u,0x212010u,16u);
        if(g[0]) {
            g[1]=read(0x50eee8u);g[2]=(g[2]&0xffffff00u)|read(g[0],1);
            if(g[1]!=0xffffffffu) {
                g[1]=(g[1]&0xffffu)*0xd4u;push(entry+0x42u,g[6]);g[6]=read(0x50eee0u);
                push(entry+0x49u,g[7]);g[1]+=read(g[6]+0x44u);g[7]=read(g[0]+4u);g[6]=g[1];
                call(entry+0x52u,entry==0x2b1200u?0x280c90u:0x280d70u,0);
                if(g[2]&0xffu) {
                    g[0]=(g[0]&0xffffff00u)|1u;write(entry+0x5du,g[6]+0x50u,g[0],1);
                    write(entry+0x60u,g[6]+0x51u,g[0],1);
                }
                g[7]=pop();g[6]=pop();
            }
            g[0]=0;g[1]=g[3];call(entry+0x69u,0x211da0u,0);
        }
        g[3]=pop();pop();g[4]+=12u;
    }
    void look(U entry) {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(entry+0x10u,g[6]);g[6]=read(g[4]+12u);
        push(entry+0x15u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(entry+0x1du,g[2]);push(entry+0x1eu,g[0]);push(entry+0x1fu,g[6]);
        call(entry+0x20u,0x212010u,16u);
        if(g[0]) {
            g[2]=read(g[0]+4u);g[1]=(g[1]&0xffffff00u)|read(g[0],1);g[0]=read(0x50eee8u);
            if(g[0]!=0xffffffffu) {
                g[0]=(g[0]&0xffffu)*0xd4u;push(entry+0x43u,g[7]);g[7]=read(0x50eee0u);
                g[0]+=read(g[7]+0x44u);g[7]=pop();write(entry+0x50u,g[0]+0x46u,g[1],1);
                if(g[1]&0xffu) {
                    write(entry+0x55u,g[0]+0x48u,entry==0x2b1000u?2u:1u,2);
                    write(entry+0x5bu,g[0]+0x4cu,g[2]);
                }
            }
            g[0]=0;g[1]=g[6];call(entry+0x62u,0x211da0u,0);
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void aim_object() {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(0x2b11c0u,g[6]);g[6]=read(g[4]+12u);
        push(0x2b11c5u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(0x2b11cdu,g[2]);push(0x2b11ceu,g[0]);push(0x2b11cfu,g[6]);
        call(0x2b11d0u,0x212010u,16u);
        if(g[0]) {
            g[2]=(g[2]&0xffffff00u)|read(g[0],1);push(0x2b11dbu,g[7]);g[7]=read(g[0]+4u);
            call(0x2b11dfu,0x280d70u,0);g[0]=0;g[1]=g[6];call(0x2b11e8u,0x211da0u,0);g[7]=pop();
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void go_to_and_face() {
        auto &g=abi.gp;
        g[0]=signed16(read(g[4]+4u,2));g[0]=read(0x480c00u+g[0]*4u);
        g[1]=read(g[4]+12u);push(0x2b0e40u,g[6]);g[6]=read(g[4]+12u);
        push(0x2b0e45u,g[1]);g[2]=g[0]+14u;g[0]=signed16(read(g[0]+12u,2));
        push(0x2b0e4du,g[2]);push(0x2b0e4eu,g[0]);push(0x2b0e4fu,g[6]);
        call(0x2b0e50u,0x212010u,16u);
        if(g[0]) {
            g[1]=read(g[0]+4u);g[2]=read(g[0]);g[0]=read(0x50eee8u);
            if(g[0]!=0xffffffffu) {
                g[0]=(g[0]&0xffffu)*0xd4u;push(0x2b0e73u,g[7]);g[7]=read(0x50eee0u);
                g[0]+=read(g[7]+0x44u);g[7]=pop();write(0x2b0e7eu,g[0]+4u,3,2);
                write(0x2b0e84u,g[0]+0x28u,g[2]);write(0x2b0e87u,g[0]+0x2cu,g[1]);
            }
            g[0]=0;g[1]=g[6];call(0x2b0e8eu,0x211da0u,0);
            g[1]=read(0x505e5cu);g[2]=read(g[1]+0x44u);g[6]=(g[6]&0xffffu)*0x418u;
            write(0x2b0ea8u,g[6]+g[2]+8u,0xfffffffdu);
        }
        g[6]=pop();pop();g[4]+=12u;
    }
    void sleep_forever() {
        auto &g=abi.gp;
        g[0]=read(0x505e6cu);g[1]=read(g[0]+0x44u);
        g[0]=read(0x505e5cu);g[0]=read(g[0]+0x44u);
        push(0x210a20u,g[6]);g[6]=read(g[4]+12u);g[2]=(g[6]&0xffffu)*0x418u;
        g[2]=read(g[2]+g[0]+0x10u);g[0]=read(g[2]+4u)&0xffffu;
        g[0]*=5u;g[0]=read(g[1]+g[0]*4u+0x10u)&0xffffu;
        g[2]=g[0]*5u;g[1]=read(g[1]+g[2]*4u+8u);g[0]=g[6];
        if(g[1]!=0xffffffffu) {
            push(0x210a59u,g[3]);push(0x210a5au,g[1]);g[3]=g[4]+0x14u;g[2]=g[6];
            call(0x210a61u,0x211cb0u,4u);g[3]=read(g[4]+0x10u);
            call(0x210a6au,0x212130u,0);g[3]=pop();
        }
        if(g[0]!=0xffffffffu) {
            g[1]=read(0x505e5cu);g[2]=read(g[1]+0x44u);g[0]=(g[0]&0xffffu)*0x418u;
            write(0x210a89u,g[0]+g[2]+8u,0xfffffffeu);
        }
        g[0]=0;g[1]=g[6];call(0x210a95u,0x211da0u,0);
        g[6]=pop();pop();g[4]+=12u;
    }
};
}
void native_campaign_hs_command_builtins_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignHsCommandChildrenTu15 &children,const SourcePhysicsContactRecordObserver &observe) {
    Builtins body{memory,abi,children,observe};
    switch(entry) {
    case 0x2b0140u:body.play_line();return;
    case 0x2b01a0u:body.play_line_at_player();return;
    case 0x2b14b0u:body.shoot_at_actor();return;
    case 0x280ee0u:body.set_command_shot_target();return;
    case 0x2aba90u:body.set_forward_velocity();return;
    case 0x2afad0u:body.ai_vehicle_get();return;
    case 0x2b1530u:body.set_vehicle_speed();return;
    case 0x2afb10u:body.starting_location_vehicle_get();return;
    case 0x2afcc0u:body.ai_vehicle_enter_immediate();return;
    case 0x27e760u:body.board_vehicle_immediately();return;
    case 0x27e720u:body.compare_boarding_actor_rows();return;
    case 0x2ab320u:body.object_hide();return;
    case 0xbbc90u:body.transition_hidden_state();return;
    case 0x2aeb30u:body.ai_get_object();return;
    case 0x2b2340u:body.abort_on_combat_status();return;
    case 0x2b2410u:case 0x2b2470u:case 0x2b2670u:body.flag(entry);return;
    case 0x210a10u:body.sleep_forever();return;
    case 0x2b1200u:case 0x2b1300u:body.face(entry);return;
    case 0x2b1000u:case 0x2b10b0u:body.look(entry);return;
    case 0x2b11b0u:body.aim_object();return;
    case 0x2b0e30u:body.go_to_and_face();return;
    default:throw std::invalid_argument("Unknown native HS command builtin");
    }
}
// The retained motion call takes an object in ESI, forward in XMM6, and
// left/up on its stack. Its vector calculation and scheduling stay native,
// while the existing world owners move and wake the actual object.
void native_campaign_hs_object_velocity_call_tu15(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignHsCommandChildrenTu15 &children,const SourcePhysicsContactRecordObserver &observe) {
    using namespace physics_world_runtime_detail;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=children.instruction;
    if(!abi.floating_point)throw std::invalid_argument("Native object velocity requires its source FP state");
    FpScope environment(*abi.floating_point);
    WorldState state(memory,observe,services,SourcePhysicsContactImage::TitleUpdate15);
    state.load(abi);
    auto &g=state.gp;
    auto &x=state.xmm;
    const auto read_word=[&](auto address){return r(memory,address,4);};
    const auto scalar=[&](auto address){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read_word(address))));};
    const auto store_scalar=[&](std::uint32_t pc,std::uint32_t address,__m128 value) {
        const auto bits=std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));
        state.instruction(pc);w(memory,observe,pc,address,4,bits);
    };
    const auto call=[&](std::uint32_t pc,std::uint32_t owner,unsigned arguments) {
        state.instruction(pc);
        const auto expected=g[4]+arguments;
        state.push(pc,pc+5u);
        if(!children.call)throw std::runtime_error("Native object velocity requires its world motion child");
        auto child=state.save();
        try {children.call(pc,owner,child);}
        catch(...) {state.load(child);throw;}
        state.load(child);
        if(g[4]!=expected)throw std::runtime_error("Native motion owner returned another source frame");
    };
    try {
        state.instruction(0x1100c0u);g[4]=state.flags.minus(g[4],12u,4);
        state.instruction(0x1100c3u);state.flags.minus(g[6],0xffffffffu,4);
        state.instruction(0x1100c6u);
        if(!state.flags.zf) {
            state.instruction(0x1100ccu);g[1]=read_word(0x4ecbd0u);
            state.instruction(0x1100d2u);g[2]=read_word(g[1]+0x44u);
            state.instruction(0x1100d5u);g[0]=g[6];
            state.instruction(0x1100d7u);g[0]=state.flags.logic(g[0]&0xffffu,4);
            state.instruction(0x1100dcu);g[0]*=3u;
            state.instruction(0x1100dfu);g[0]=read_word(g[2]+g[0]*4u+8u);
            const auto body=g[0];
            const auto forward_x=[&] {return scalar(body+0x70u);};
            const auto forward_y=[&] {return scalar(body+0x74u);};
            const auto forward_z=[&] {return scalar(body+0x78u);};
            const auto up_x=[&] {return scalar(body+0x7cu);};
            const auto up_y=[&] {return scalar(body+0x80u);};
            const auto up_z=[&] {return scalar(body+0x84u);};
            // The cross product lanes have different operation order in the
            // source. Keep it through the spill and original wake threshold.
            state.instruction(0x1100e3u);x[0]=forward_z();
            state.instruction(0x1100e8u);x[0]=_mm_mul_ss(x[0],up_y());
            state.instruction(0x1100f0u);x[1]=up_z();
            state.instruction(0x1100f8u);x[1]=_mm_mul_ss(x[1],forward_y());
            state.instruction(0x1100fdu);x[2]=up_x();
            state.instruction(0x110102u);x[2]=_mm_mul_ss(x[2],forward_z());
            state.instruction(0x110107u);x[3]=forward_x();
            state.instruction(0x11010cu);x[3]=_mm_mul_ss(x[3],up_y());
            state.instruction(0x110114u);x[4]=forward_y();
            state.instruction(0x110119u);x[5]=forward_z();
            state.instruction(0x11011eu);x[7]=up_y();
            state.instruction(0x110126u);x[0]=_mm_sub_ss(x[0],x[1]);
            state.instruction(0x11012au);x[1]=up_z();
            state.instruction(0x110132u);x[1]=_mm_mul_ss(x[1],forward_x());
            state.instruction(0x110137u);x[1]=_mm_sub_ss(x[1],x[2]);
            state.instruction(0x11013bu);x[2]=up_x();
            state.instruction(0x110140u);x[2]=_mm_mul_ss(x[2],forward_y());
            state.instruction(0x110145u);x[2]=_mm_sub_ss(x[2],x[3]);
            state.instruction(0x110149u);x[3]=forward_x();
            state.instruction(0x11014eu);x[3]=_mm_mul_ss(x[3],x[6]);
            state.instruction(0x110152u);x[4]=_mm_mul_ss(x[4],x[6]);
            state.instruction(0x110156u);x[5]=_mm_mul_ss(x[5],x[6]);
            state.instruction(0x11015au);x[6]=scalar(g[4]+16u);
            state.instruction(0x110160u);x[1]=_mm_mul_ss(x[1],x[6]);
            state.instruction(0x110164u);x[2]=_mm_mul_ss(x[2],x[6]);
            store_scalar(0x110168u,g[4]+4u,x[1]);
            state.instruction(0x11016eu);x[1]=up_z();
            state.instruction(0x110176u);x[4]=_mm_add_ss(x[4],scalar(g[4]+4u));
            store_scalar(0x11017cu,g[4]+8u,x[2]);
            state.instruction(0x110182u);x[2]=scalar(g[4]+20u);
            state.instruction(0x110188u);x[5]=_mm_add_ss(x[5],scalar(g[4]+8u));
            state.instruction(0x11018eu);state.push(0x11018eu,g[3]);
            state.instruction(0x11018fu);x[0]=_mm_mul_ss(x[0],x[6]);
            state.instruction(0x110193u);state.push(0x110193u,g[7]);
            state.instruction(0x110194u);x[6]=x[2];
            state.instruction(0x110197u);x[6]=_mm_mul_ss(x[6],up_x());
            state.instruction(0x11019cu);x[7]=_mm_mul_ss(x[7],x[2]);
            state.instruction(0x1101a0u);x[1]=_mm_mul_ss(x[1],x[2]);
            state.instruction(0x1101a4u);x[3]=_mm_add_ss(x[3],x[0]);
            state.instruction(0x1101a8u);x[3]=_mm_add_ss(x[3],x[6]);
            state.instruction(0x1101acu);x[4]=_mm_add_ss(x[4],x[7]);
            state.instruction(0x1101b0u);x[5]=_mm_add_ss(x[5],x[1]);
            state.instruction(0x1101b4u);state.push(0x1101b4u,0u);
            state.instruction(0x1101b6u);g[7]=state.flags.logic(0u,4);
            state.instruction(0x1101b8u);g[1]=g[4]+12u;
            state.instruction(0x1101bcu);g[0]=g[6];
            store_scalar(0x1101beu,g[4]+12u,x[3]);
            store_scalar(0x1101c4u,g[4]+16u,x[4]);
            store_scalar(0x1101cau,g[4]+20u,x[5]);
            call(0x1101d0u,0xb9770u,4u);
            state.instruction(0x1101d5u);state.push(0x1101d5u,1u);
            state.instruction(0x1101d7u);state.push(0x1101d7u,g[7]);
            state.instruction(0x1101d8u);g[3]=g[4]+16u;
            state.instruction(0x1101dcu);g[0]=g[6];
            call(0x1101deu,0x1cd6a0u,8u);
            state.instruction(0x1101e3u);x[0]=scalar(g[4]+16u);
            x[1]=_mm_mul_ss(x[0],x[0]);
            state.instruction(0x1101f0u);x[0]=scalar(g[4]+12u);
            x[2]=_mm_mul_ss(x[0],x[0]);
            state.instruction(0x1101fdu);x[0]=scalar(g[4]+8u);
            x[1]=_mm_add_ss(x[1],x[2]);
            x[2]=_mm_mul_ss(x[0],x[0]);
            x[1]=_mm_add_ss(x[1],x[2]);
            state.instruction(0x110212u);state.flags.scalar_compare(x[1],scalar(0x46a0ecu),false);
            state.instruction(0x110219u);g[7]=state.pop();
            state.instruction(0x11021au);g[3]=state.pop();
            state.instruction(0x11021bu);
            if(!state.flags.cf && !state.flags.zf) {
                state.instruction(0x11021du);state.push(0x11021du,g[6]);
                state.instruction(0x11021eu);state.set(2,state.flags.logic(0u,1),1,0);
                call(0x110220u,0xbbbc0u,4u);
                state.instruction(0x110225u);g[0]=g[6];
                call(0x110227u,0xb9390u,0u);
                state.instruction(0x11022cu);g[0]=g[6];
                call(0x11022eu,0xbda50u,0u);
            }
        }
        state.instruction(0x110233u);g[4]=state.flags.plus(g[4],12u,4);
        state.instruction(0x110236u);state.pop();g[4]+=8u;
    } catch(...) {
        abi=state.save();
        throw;
    }
    abi=state.save();
}
}
