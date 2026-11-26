#pragma once

#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

namespace halo2::native_death_cycle_motion_records {
using namespace native_death_cycle_opening;

enum class Entry { primary, alternate, contextual };
enum class Continuation { reference, expiration, apply_motion };

template<class State,class Route>
struct RecordFrame : Frame<State> {
    using Base = Frame<State>;
    using Base::s, Base::g, Base::flags, Base::site, Base::access;
    using Base::read, Base::store, Base::push, Base::field, Base::constant;

    Route &route;
    RecordFrame(State &state,Route &child) : Base{state},route(child) {}

    auto stack(std::uint32_t offset) { return field(esp,offset); }
    std::uint32_t status_address() const { return g[ebp]+g[ebx]+0x210u; }
    auto status() { return [this] {return status_address();}; }
    void call(std::uint32_t pc,std::uint32_t resume,std::uint32_t target) {
        push(pc,constant(resume));
        route(s,pc,target);
    }
    bool zero(std::uint32_t pc) {site(pc);return flags.zf;}
    bool requested(std::uint32_t load,std::uint32_t test,std::uint32_t branch) {
        set_low_byte(g[eax],read(load,stack(0x14u),1));
        site(test);
        flags.logic(low_byte(g[eax]),1);
        return !zero(branch);
    }
    bool recorded(std::uint32_t test,std::uint32_t branch) {
        flags.logic(read(test,status(),1)&1u,1);
        return !zero(branch);
    }
    bool child_accepted(std::uint32_t test,std::uint32_t branch) {
        site(test);
        flags.logic(low_byte(g[eax]),1);
        return !zero(branch);
    }
    bool select_index(std::uint32_t clear,std::uint32_t load,std::uint32_t compare,
                      std::uint32_t branch,std::uint32_t offset) {
        site(clear);
        g[eax]=flags.logic(0u,4);
        s.set(eax,read(load,field(esi,offset),2),2,0);
        site(compare);
        flags.minus(g[eax]&0xffffu,0xffffu,2);
        return !zero(branch);
    }
    std::uint32_t signed_index(std::uint32_t pc,std::uint32_t offset) {
        const auto raw=std::uint16_t(read(pc,field(esi,offset),2));
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(raw)));
    }

    Continuation queue_motion() {
        g[eax]=read(0x103a5au,status());
        g[edi]=read(0x103a61u,stack(0x4cu));
        site(0x103a65u);
        g[eax]=flags.logic(g[eax]|1u,4);
        site(0x103a68u);
        return Continuation::apply_motion;
    }
    Continuation mark_motion() {
        access(0x103a49u);
        native_death_cycle_motion::update_field(s,0x103a49u,g[eax]+4u,1,0xffu,1u);
        access(0x103a4du);
        native_death_cycle_motion::update_field(s,0x103a4du,status_address(),4,0xfffffffeu,0u);
        site(0x103a55u);
        return Continuation::expiration;
    }

    Continuation primary_call() {
        call(0x1038aau,0x1038afu,0x107610u);
        if (!child_accepted(0x1038afu,0x1038b1u)) return Continuation::expiration;
        g[eax]=signed_index(0x1038b7u,8u);
        g[ecx]=read(0x1038bbu,constant(0x4ecbd0u));
        site(0x1038c1u);
        g[eax]=flags.product(g[eax],0x34u);
        g[edx]=read(0x1038c4u,field(ecx,0x44u));
        g[ecx]=read(0x1038c7u,stack(0x24u));
        g[edx]=read(0x1038cbu,[&] {return g[edx]+g[ecx]+8u;});
        site(0x1038cfu);
        g[eax]=g[edx]+g[eax]+0x1a4u;
        site(0x1038d6u);
        return mark_motion();
    }
    Continuation primary() {
        if (!requested(0x10387cu,0x103880u,0x103882u)) {
            access(0x1038dbu);
            native_death_cycle_motion::update_field(s,0x1038dbu,status_address(),4,0xffffffffu,1u);
            s.set(ecx,read(0x1038e3u,field(esi,8u),2),2,0);
            g[eax]=read(0x1038e7u,stack(0x4cu));
            call(0x1038ebu,0x1038f0u,0x108040u);
            site(0x1038f0u);
            return Continuation::expiration;
        }
        if (!recorded(0x103884u,0x10388cu) || !select_index(0x103892u,0x103894u,0x103898u,0x10389cu,8u))
            return Continuation::expiration;
        push(0x1038a2u,constant(0u));
        push(0x1038a4u,field(eax));
        g[eax]=read(0x1038a5u,stack(0x54u));
        push(0x1038a9u,field(eax));
        return primary_call();
    }

    bool alternate_call(bool second) {
        if (!second) {
            if (!select_index(0x103920u,0x103922u,0x103926u,0x10392au,8u)) return false;
            g[ecx]=read(0x103930u,stack(0x4cu));
            push(0x103934u,constant(0u));
            push(0x103936u,field(eax));
            push(0x103937u,field(ecx));
            call(0x103938u,0x10393du,0x107610u);
            if (!child_accepted(0x10393du,0x10393fu)) return false;
            g[ecx]=signed_index(0x103945u,8u);
        } else {
            if (!select_index(0x103969u,0x10396bu,0x10396fu,0x103973u,0xau)) return false;
            g[ecx]=read(0x103979u,stack(0x4cu));
            push(0x10397du,constant(0u));
            push(0x10397fu,field(eax));
            push(0x103980u,field(ecx));
            call(0x103981u,0x103986u,0x107610u);
            if (!child_accepted(0x103986u,0x103988u)) return false;
            g[ecx]=signed_index(0x10398eu,0xau);
            site(0x103992u);
        }
        return true;
    }
    Continuation alternate() {
        if (!requested(0x1038f5u,0x1038f9u,0x1038fbu)) return queue_motion();
        if (!recorded(0x103901u,0x103909u)) return Continuation::expiration;
        set_low_byte(g[eax],read(0x10390fu,field(ebp,0x16eu),1));
        site(0x103915u);
        set_low_byte(g[eax],flags.right(low_byte(g[eax]),6u,1,false));
        site(0x103918u);
        set_low_byte(g[eax],flags.logic(low_byte(g[eax])&1u,1));
        store(0x10391au,stack(0x1bu),[&] {return low_byte(g[eax]);},1);
        if (!alternate_call(!zero(0x10391eu))) return Continuation::expiration;
        g[edx]=read(0x103949u,constant(0x4ecbd0u));
        site(0x10394fu);
        g[ecx]=flags.product(g[ecx],0x34u);
        g[eax]=read(0x103952u,field(edx,0x44u));
        g[edx]=read(0x103955u,stack(0x24u));
        g[eax]=read(0x103959u,[&] {return g[eax]+g[edx]+8u;});
        site(0x10395du);
        g[eax]+=g[ecx]+0x1a4u;
        site(0x103964u);
        return mark_motion();
    }

    Continuation contextual_index() {
        if (!select_index(0x103a0au,0x103a0cu,0x103a10u,0x103a14u,0xau)) return Continuation::expiration;
        push(0x103a1au,constant(0u));
        push(0x103a1cu,field(eax));
        push(0x103a1du,field(edi));
        call(0x103a1eu,0x103a23u,0x107610u);
        if (!child_accepted(0x103a23u,0x103a25u)) return Continuation::expiration;
        g[edx]=signed_index(0x103a2bu,0xau);
        g[eax]=read(0x103a2fu,constant(0x4ecbd0u));
        site(0x103a34u);
        g[edx]=flags.product(g[edx],0x34u);
        g[ecx]=read(0x103a37u,field(eax,0x44u));
        g[eax]=read(0x103a3au,stack(0x24u));
        g[ecx]=read(0x103a3eu,[&] {return g[ecx]+g[eax]+8u;});
        site(0x103a42u);
        g[eax]=g[ecx]+g[edx]+0x1a4u;
        return mark_motion();
    }
    Continuation contextual() {
        if (!requested(0x103994u,0x103998u,0x10399au)) return queue_motion();
        if (!recorded(0x1039a0u,0x1039a8u)) return Continuation::expiration;
        g[edi]=read(0x1039aeu,stack(0x4cu));
        push(0x1039b2u,constant(0u));
        push(0x1039b4u,constant(0u));
        push(0x1039b6u,field(edi));
        call(0x1039b7u,0x1039bcu,0x107610u);
        if (!child_accepted(0x1039bcu,0x1039beu)) return Continuation::expiration;
        flags.minus(read(0x1039c4u,stack(0x30u)),0xffffffffu,4);
        if (zero(0x1039c9u)) return contextual_index();
        site(0x1039cbu);
        g[ecx]=g[esp]+0x1bu;
        push(0x1039cfu,field(ecx));
        site(0x1039d0u);
        g[edx]=g[esp]+0x48u;
        push(0x1039d4u,field(edx));
        site(0x1039d5u);
        g[eax]=g[edi];
        call(0x1039d7u,0x1039dcu,0x10b330u);
        if (child_accepted(0x1039dcu,0x1039deu)) {
            g[eax]=read(0x1039e0u,stack(0x28u));
            push(0x1039e4u,field(eax));
            site(0x1039e5u);
            g[eax]=g[edi];
            call(0x1039e7u,0x1039ecu,0x108d10u);
            site(0x1039ecu);
            return Continuation::expiration;
        }
        if (!select_index(0x1039f1u,0x1039f3u,0x1039f7u,0x1039fbu,8u)) return Continuation::expiration;
        push(0x103a01u,constant(0u));
        push(0x103a03u,field(eax));
        push(0x103a04u,field(edi));
        site(0x103a05u);
        return primary_call();
    }
};

template<class State,class Route>
Continuation run(State &state,Route route,Entry entry) {
    static const bool reference=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_DEATH_CYCLE_MOTION_RECORDS_10387C");
        return value && *value;
    }();
    if (!selected(state) || reference) return Continuation::reference;
    RecordFrame<State,Route> frame{state,route};
    switch (entry) {
    case Entry::primary: return frame.primary();
    case Entry::alternate: return frame.alternate();
    case Entry::contextual: return frame.contextual();
    }
    return Continuation::reference;
}
} // namespace halo2::native_death_cycle_motion_records
