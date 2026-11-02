#pragma once
#include "halo2/native_tu15_region_transition.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2::physics_world_runtime_detail {
// Compatibility boundary for the whole BC720 parent. The native kernel owns
// its decisions, count and copy loops. Real original callees retain their own
// execution; this adapter preserves current argument reads and the source ABI.
template<class State,class Site,class Invoke>
void region_transition_native_tu15(State& s,Site site,Invoke invoke) {
    struct Storage final:NativeTu15RegionTransitionStorage {
        State& s;Site& site;Invoke& invoke;std::int16_t extent{};bool live{},refreshed{};
        Storage(State& state,Site& position,Invoke& call):s(state),site(position),invoke(call){}
        std::uint32_t at(std::uint32_t pc){return site(pc);}
        void call(std::uint32_t pc,std::uint32_t owner,std::uint32_t after){
            s.push(at(pc),after);invoke(pc,owner);
        }
        std::uint32_t object_definition()override {
            at(0xbc720u);s.gp[4]=s.flags.minus(s.gp[4],28u,4);
            at(0xbc723u);s.gp[0]=r(s.memory,s.gp[4]+32u,4);
            at(0xbc727u);s.gp[1]=r(s.memory,0x4ecbd0u,4);
            at(0xbc72du);s.gp[2]=r(s.memory,s.gp[1]+68u,4);
            at(0xbc730u);s.gp[1]=r(s.memory,0x4f0414u,4);
            at(0xbc736u);s.gp[0]=s.flags.logic(s.gp[0]&0xffffu,4);
            at(0xbc73bu);s.gp[0]*=3u;
            at(0xbc73eu);s.gp[0]=s.gp[2]+s.gp[0]*4u;
            w(s.memory,s.observe,at(0xbc741u),s.gp[4],4,s.gp[0]);
            s.push(at(0xbc744u),s.gp[3]);
            at(0xbc745u);s.gp[3]=r(s.memory,s.gp[0]+8u,4);
            at(0xbc748u);s.gp[0]=r(s.memory,s.gp[3],4);
            at(0xbc74au);s.gp[0]=s.flags.logic(s.gp[0]&0xffffu,4);
            at(0xbc74fu);s.gp[0]=s.flags.shift(s.gp[0],4u,4);
            at(0xbc752u);return s.gp[0]=r(s.memory,s.gp[1]+s.gp[0]+8u,4);
        }
        std::uint32_t model(std::uint32_t definition)override {
            at(0xbc756u);const auto model=r(s.memory,definition+56u,4);s.flags.minus(model,0xffffffffu,4);
            w(s.memory,s.observe,at(0xbc75au),s.gp[4]+12u,4,definition);
            at(0xbc75eu);return model;
        }
        std::int16_t region_extent()override {
            at(0xbc764u);extent=std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,s.gp[3]+280u,2)));
            s.gp[1]=std::uint32_t(std::int32_t(extent));return extent;
        }
        void prepare_snapshot(std::int32_t count)override {
            at(0xbc76bu);s.gp[0]=0x66666667u;
            at(0xbc770u);const auto product=std::int64_t(0x66666667)*std::int32_t(extent);
            s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(std::uint64_t(product)>>32u);
            s.flags.cf=s.flags.of=product!=std::int64_t(std::bit_cast<std::int32_t>(s.gp[0]));
            at(0xbc772u);s.gp[2]=s.flags.right(s.gp[2],2u,4,true);
            s.push(at(0xbc775u),s.gp[5]);
            at(0xbc776u);s.gp[5]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,s.gp[3]+282u,2)))));
            at(0xbc77du);s.gp[1]=s.gp[2];
            at(0xbc77fu);s.gp[1]=s.flags.right(s.gp[1],31u,4,false);
            at(0xbc782u);s.flags.plus(s.gp[1],s.gp[2],4);s.gp[1]=std::uint32_t(count);
            at(0xbc784u);s.gp[5]=s.flags.plus(s.gp[5],s.gp[3],4);
            at(0xbc786u);s.gp[2]=s.gp[5]+std::uint32_t(count)*2u;
            at(0xbc78au);s.gp[0]=std::uint32_t(count);
            s.push(at(0xbc78cu),s.gp[6]);
            at(0xbc78du);s.gp[1]=s.flags.right(s.gp[1],2u,4,false);
            s.push(at(0xbc790u),s.gp[7]);
            at(0xbc791u);s.gp[6]=s.gp[5];
            at(0xbc793u);s.gp[7]=s.gp[4]+28u;
            live=true;at(0xbc797u);
        }
        std::uint32_t snapshot_word()override {return r(s.memory,s.gp[6],4);}
        void snapshot_word(std::uint32_t value)override {w(s.memory,s.observe,0xbc797u,s.gp[7],4,value);}
        void advance_snapshot_word()override {
            const auto delta=(s.other_flags&1024u)?0xfffffffcu:4u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];at(0xbc797u);
        }
        void prepare_snapshot_bytes(std::uint32_t count)override {
            at(0xbc799u);s.gp[1]=s.gp[0];
            at(0xbc79bu);s.flags.logic(s.gp[1]&3u,4);s.gp[1]=count;at(0xbc79eu);
        }
        std::uint8_t snapshot_byte()override {return std::uint8_t(r(s.memory,s.gp[6],1));}
        void snapshot_byte(std::uint8_t value)override {w(s.memory,s.observe,0xbc79eu,s.gp[7],1,value);}
        void advance_snapshot_byte()override {
            const auto delta=(s.other_flags&1024u)?0xffffffffu:1u;s.gp[6]+=delta;s.gp[7]+=delta;--s.gp[1];at(0xbc79eu);
        }
        std::uint8_t object_type()override {
            at(0xbc7a0u);s.gp[1]=r(s.memory,s.gp[4]+16u,4);
            at(0xbc7a4u);const auto type=std::uint8_t(r(s.memory,s.gp[1]+3u,1));s.flags.minus(type,6u,1);
            w(s.memory,s.observe,at(0xbc7a8u),s.gp[4]+20u,4,s.gp[2]);at(0xbc7acu);return type;
        }
        std::uint16_t attached_index()override {
            at(0xbc7aeu);const auto value=std::uint16_t(r(s.memory,s.gp[3]+26u,2));s.flags.minus(value,0xffffu,2);
            at(0xbc7b3u);return value;
        }
        std::uint8_t flags()override {
            at(0xbc7b5u);const auto value=std::uint8_t(r(s.memory,s.gp[3]+300u,1));s.flags.logic(value&2u,1);
            at(0xbc7bcu);return value;
        }
        void refresh_link()override {
            at(0xbc7beu);s.gp[6]=r(s.memory,s.gp[4]+48u,4);
            at(0xbc7c2u);s.gp[0]=s.gp[6];call(0xbc7c4u,0x1ce340u,0xbc7c9u);at(0xbc7c9u);
            refreshed=true;
        }
        void apply_model_state()override {
            // The skipped-refresh path reads the object formal here; the
            // taken path retained its earlier input across the real callee.
            if(!refreshed){at(0xbc7cbu);s.gp[6]=r(s.memory,s.gp[4]+48u,4);}
            at(0xbc7cfu);s.gp[2]=r(s.memory,s.gp[4]+20u,4);
            at(0xbc7d3u);s.gp[0]=r(s.memory,s.gp[4]+60u,4);
            at(0xbc7d7u);s.gp[1]=r(s.memory,s.gp[4]+56u,4);
            s.push(at(0xbc7dbu),s.gp[2]);
            at(0xbc7dcu);s.gp[2]=r(s.memory,s.gp[4]+56u,4);
            s.push(at(0xbc7e0u),s.gp[5]);
            s.push(at(0xbc7e1u),0u);
            s.push(at(0xbc7e3u),0xffffffffu);
            s.push(at(0xbc7e5u),s.gp[0]);
            at(0xbc7e6u);s.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(r(s.memory,s.gp[3]+177u,1)))));
            s.push(at(0xbc7edu),s.gp[1]);
            at(0xbc7eeu);s.gp[1]=r(s.memory,s.gp[4]+48u,4);
            s.push(at(0xbc7f2u),s.gp[2]);
            at(0xbc7f3u);s.gp[2]=r(s.memory,s.gp[1]+56u,4);
            s.push(at(0xbc7f6u),s.gp[0]);
            s.push(at(0xbc7f7u),s.gp[2]);
            s.push(at(0xbc7f8u),s.gp[6]);
            call(0xbc7f9u,0x1704c0u,0xbc7feu);
        }
        void notify_model()override {
            at(0xbc7feu);s.gp[0]=s.gp[6];call(0xbc800u,0x17e060u,0xbc805u);
        }
        void rebind_snapshot()override {
            s.push(at(0xbc805u),s.gp[6]);
            at(0xbc806u);s.gp[0]=s.gp[4]+32u;call(0xbc80au,0x1ce0e0u,0xbc80fu);
        }
        void notify_object()override {
            at(0xbc80fu);s.gp[0]=s.gp[6];call(0xbc811u,0xb9390u,0xbc816u);
        }
    } storage(s,site,invoke);
    native_tu15_region_transition(storage);
    if(storage.live){
        site(0xbc816u);s.gp[7]=s.pop();
        site(0xbc817u);s.gp[6]=s.pop();
        site(0xbc818u);s.gp[5]=s.pop();
    }
    site(0xbc819u);s.gp[3]=s.pop();
    site(0xbc81au);s.gp[4]=s.flags.plus(s.gp[4],28u,4);
    site(0xbc81du);s.pop();s.gp[4]+=16u;
}
}
