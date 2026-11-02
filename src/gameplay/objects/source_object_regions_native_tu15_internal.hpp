#pragma once
#include "halo2/native_tu15_object_regions.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2::physics_world_runtime_detail {
// Legacy-call boundary only: the semantic storage algorithm owns signed
// division, output decisions and pointer arithmetic. This adapter retains
// the existing source ABI, source stack/observer stores and fault prefixes.
template<class State,class Site>
NativeTu15ObjectRegionBank object_regions_native_tu15(State& s,Site site) {
    struct Storage final:NativeTu15ObjectRegionStorage {
        State& s;Site& site;std::int16_t extent{};
        Storage(State& state,Site& observe):s(state),site(observe){}
        std::uint32_t at(std::uint32_t pc){return site(pc);}
        std::uint32_t object_body(std::uint32_t datum)override {
            at(0xbc6c0u);s.gp[1]=r(s.memory,0x4ecbd0u,4);
            at(0xbc6c6u);s.gp[2]=r(s.memory,s.gp[1]+0x44u,4);
            at(0xbc6c9u);s.gp[0]=s.flags.logic(datum&0xffffu,4);
            at(0xbc6ceu);s.gp[0]*=3u;
            at(0xbc6d1u);return s.gp[0]=r(s.memory,s.gp[2]+s.gp[0]*4u+8u,4);
        }
        std::int16_t descriptor_extent(std::uint32_t body)override {
            at(0xbc6d5u);extent=std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,body+0x118u,2)));
            s.gp[2]=std::uint32_t(std::int32_t(extent));return extent;
        }
        void publish_extent(std::int32_t value)override {
            w(s.memory,s.observe,at(0xbc6dcu),s.gp[6],4,std::uint32_t(value));
        }
        std::uint32_t active_base(std::uint32_t body)override {
            at(0xbc6deu);const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,body+0x11au,2)));
            s.gp[1]=std::uint32_t(std::int32_t(offset));
            at(0xbc6e5u);return s.gp[1]=s.flags.plus(s.gp[1],body,4);
        }
        std::uint32_t states_output(std::int32_t count)override {
            // Only the compatibility register/flag postimage uses these
            // multiply intermediates; the native kernel computes the count.
            at(0xbc6e7u);s.gp[0]=0x66666667u;
            at(0xbc6ecu);const auto product=std::int64_t(0x66666667)*std::int32_t(extent);
            s.gp[0]=std::uint32_t(product);s.gp[2]=std::uint32_t(std::uint64_t(product)>>32u);
            s.flags.cf=s.flags.of=product!=std::int64_t(std::bit_cast<std::int32_t>(s.gp[0]));
            at(0xbc6eeu);s.gp[2]=s.flags.right(s.gp[2],2u,4,true);
            at(0xbc6f1u);s.gp[0]=s.gp[2];
            at(0xbc6f3u);s.gp[0]=s.flags.right(s.gp[0],31u,4,false);
            at(0xbc6f6u);(void)s.flags.plus(s.gp[0],s.gp[2],4);s.gp[0]=std::uint32_t(count);
            at(0xbc6f8u);s.flags.logic(s.gp[7],4);
            s.push(at(0xbc6fau),s.gp[5]);
            at(0xbc6fbu);return s.gp[5]=r(s.memory,s.gp[4]+8u,4);
        }
        void publish_count(std::int32_t value)override {
            w(s.memory,s.observe,at(0xbc6ffu),s.gp[6],4,std::uint32_t(value));
        }
        bool active_requested()override {at(0xbc701u);return s.gp[7]!=0;}
        void publish_active(std::uint32_t base)override {
            w(s.memory,s.observe,at(0xbc703u),s.gp[7],4,base);
        }
        bool cached_requested()override {
            at(0xbc705u);s.flags.logic(s.gp[3],4);at(0xbc707u);return s.gp[3]!=0;
        }
        std::uint32_t reread_count(bool states)override {
            at(states?0xbc713u:0xbc709u);const auto value=r(s.memory,s.gp[6],4);
            s.gp[states?0:2]=value;return value;
        }
        void publish_cached(std::uint32_t value)override {
            at(0xbc70bu);s.flags.plus(s.gp[2],s.gp[1],4);s.gp[2]=value;
            w(s.memory,s.observe,at(0xbc70du),s.gp[3],4,value);
        }
        bool states_requested(std::uint32_t output)override {
            at(0xbc70fu);s.flags.logic(output,4);at(0xbc711u);return output!=0;
        }
        void publish_states(std::uint32_t output,std::uint32_t value)override {
            at(0xbc715u);s.gp[1]=value;
            w(s.memory,s.observe,at(0xbc718u),output,4,value);
        }
    } storage(s,site);
    const auto bank=native_tu15_query_object_regions(storage,s.gp[0]);
    site(0xbc71bu);s.gp[5]=s.pop();
    site(0xbc71cu);s.pop();s.gp[4]+=4u;
    return bank;
}
}
