#pragma once
// The shared boundary adapter of the item light's two entries, 1D9080 and
// 1D9100 (native_tu15_item_light_systems.hpp); each entry's header
// (native_tu15_item_light_place.hpp, native_tu15_item_light_aim.hpp) holds
// its branches and switches. Its callers and the children keep the original
// ABI, which the adapter owns: the item and the vector in registers (ECX
// and EAX placing, EAX and ECX aiming), the record on the stack (RET 4); EBP
// the frame (ESP aligned down to 16, then 0x1C below), ESI saved; 315C90
// gets ECX the light and the settled byte's address pushed (RET 4), 315CB0
// ECX the light, the light object's function ECX the object and the
// vector's address pushed (it pops it), every register each returns coming
// back. Each access publishes its original instruction.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_item_light_systems.hpp"

namespace halo2::native_item_light_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_item_light;
using Access=logic::item_light::Access;
// An entry's instructions, by Access, and its calls and exit.
struct Sites {std::uint32_t at[18];std::uint32_t esi_push,prepare,settled_call,refresh_call,hand_push,hand_call,exit,entry;};
template<class State,class Settled,class Refresh,class Route> struct Adapter : Port<State> {
    const Sites &sites;
    const logic::item_light::Variant &variant;
    Settled &settled_child;
    Refresh &refresh_child;
    Route &route;
    std::uint32_t extra{};      // bytes pushed past the saved ESI
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,const Sites &k,const logic::item_light::Variant &v,Settled &st,Refresh &rf,Route &r)
        :Port<State>(s,c,t,{}),sites(k),variant(v),settled_child(st),refresh_child(rf),route(r){}
    std::uint32_t argument()const{return this->gp[ebp]+8u;}
    std::uint32_t local(std::uint32_t k)const{return this->gp[esp]+extra+0x10u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        if(a==light) {
            if(variant.aim) {
                // MOV EAX,..*3; SHL EAX,5; LEA EAX,[EAX+EDX+40].
                gp[eax]=fl.shift(3u*gp[eax],5u,4)+gp[edx]+logic::light_slot;
            } else {
                // LEA ECX,[ECX+ECX*2]; SHL ECX,5; PUSH ESI.
                gp[ecx]=fl.shift(3u*gp[ecx],5u,4);this->push(sites.esi_push,gp[esi]);
            }
        }
        return sites.at[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &xmm=this->s.xmm;
        using enum Access;
        switch(a) {
        case record:if(variant.aim)gp[esi]=v;else gp[edx]=v;break;
        case flags:gp[edx]=v>>1;this->test8(gp[edx]&1u);break;                                       // SHR EDX,1; TEST DL,1
        case items:gp[edx]=v;break;
        case light:if(variant.aim)gp[edx]=v;else gp[esi]=v;break;
        case off:if(variant.aim)set_low_byte(gp[edx],v);else set_low_byte(gp[ecx],v);this->test8(v);break;
        case x:case y:case z:xmm[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(v)));break;
        case light_again:gp[esi]=v;break;
        case settled:this->flags.minus(v&0xffu,0u,1);break;                                         // CMP byte [EAX],0
        case pending:gp[eax]=v;this->test32(v);break;
        case object:gp[ecx]=v;break;
        case table:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::w_store)this->s.xmm[0]=_mm_setzero_ps();}
    // LEA EAX,[ESP+F]; PUSH EAX; MOV ECX,ESI.
    void prepare(const logic::item_light::Variant &) {
        auto &gp=this->gp;
        gp[eax]=gp[esp]+0xfu;this->push(sites.prepare,gp[eax]);extra=4u;gp[ecx]=gp[esi];
    }
    std::uint32_t settled(std::uint32_t &light_address) {
        this->call(sites.settled_call,sites.settled_call+5u,0x315c90u,settled_child);extra=0u;
        light_address=this->gp[esi];
        return this->gp[eax];
    }
    // MOV ECX,ESI; CALL 315CB0.
    std::uint32_t refresh(std::uint32_t light_address) {
        this->gp[ecx]=light_address;
        this->call(sites.refresh_call,sites.refresh_call+5u,0x315cb0u,refresh_child);
        return this->gp[esi];
    }
    // LEA EAX,[ESP+10]; PUSH EAX.
    void handing(std::uint32_t){auto &gp=this->gp;gp[eax]=gp[esp]+0x10u;this->push(sites.hand_push,gp[eax]);extra=4u;}
    void hand(std::uint32_t,std::uint32_t function,std::uint32_t) {
        this->call_routed(route,sites.hand_call,sites.hand_call+3u,function);extra=0u;
    }
    unsigned run_entry() {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto item=variant.aim?gp[eax]:gp[ecx],vector=variant.aim?gp[ecx]:gp[eax];
        this->push(sites.entry,gp[ebp]);gp[ebp]=gp[esp];                                           // PUSH EBP; MOV EBP,ESP
        gp[esp]=fl.logic(gp[esp]&0xfffffff0u,4);gp[esp]=fl.minus(gp[esp],0x1cu,4);                     // AND ESP,-10; SUB ESP,1C
        if(variant.aim)this->push(sites.esi_push,gp[esi]);                                           // PUSH ESI
        Memory<Adapter> memory{*this};
        const auto outcome=logic::item_light::deliver(memory,*this,variant,item,vector);
        gp[esi]=this->pop(sites.exit);gp[esp]=gp[ebp];gp[ebp]=this->pop(sites.exit+3u);
        this->ret(sites.exit+4u,4u);this->publish(sites.exit+4u);
        return unsigned(outcome);
    }
};
} // namespace halo2::native_item_light_detail
