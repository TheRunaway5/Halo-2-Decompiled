#pragma once
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_model_record_systems.hpp"

namespace halo2::native_first_person_model_preparation {
using namespace native_object_tick_detail::boundary;
using native_object_tick_detail::Trace;
using native_small_queries_detail::VerifySummary;

enum Branch : unsigned {unavailable_view, no_weapon, no_representation, camera_override,
    arms_model, weapon_model, body_model, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> hits{};return hits;}
inline VerifySummary &summary(){static VerifySummary tally{"First-person model preparation"};return tally;}

inline constexpr std::uint32_t none=0xffffffffu, user_stride=0x20ccu, slot_stride=0x1010u, record_stride=0xd0cu;
inline constexpr std::uint32_t session=0x4f3218u, view_mask=0x5020bau, first_person_views=0x4f6498u,
    representations=0x4ecc1cu, objects=0x4ecbd0u, tags=0x4f0414u;
inline constexpr std::uint32_t camera_position=0x4c65c0u, camera_forward=0x4c65ccu, camera_up=0x4c65d8u;
enum Local : std::uint32_t {
    count=4u, slot_count=8u, body_tag=8u, arms_tag=12u, weapon_scan_count=12u, next_record=16u,
    unit_nodes=20u, unit_model_tag=24u, unit_matrices=28u, camera_root=80u, camera_adjustment=132u
};
enum Argument : std::uint32_t {view=8u, unit=12u, capacity=16u, records=20u};

template<class State,class Operation>
std::uint32_t commit_update(State &s,std::uint32_t pc,std::uint32_t address,std::uint32_t before,Operation op) {
    auto next=s.flags;
    const auto value=op(next,before);
    // The original memory-destination instruction retires its flag change
    // with the write. Deliver observation only after both have committed.
    if(s.observe) {
        const SourcePhysicsContactRecordObserver commit=[&](const SourcePhysicsContactRecordStore &event) {
            s.flags=next;
            s.observe(event);
        };
        physics_step_detail::w(s.memory,commit,pc,address,4,value);
    } else {
        physics_step_detail::w(s.memory,{},pc,address,4,value);
        s.flags=next;
    }
    return value;
}
template<class State,class Operation>
void modify(State &s,std::uint32_t pc,std::uint32_t address,Operation op) {
    const auto before=physics_step_detail::r(s.memory,address,4);
    (void)commit_update(s,pc,address,before,op);
}

template<class State,class Route,bool Observed> struct Frame : Port<State> {
    State &state;
    Route &route;
    decltype(State::gp) &reg;
    typename Port<State>::Flags &flags;

    Frame(State &s,std::uint32_t *cursor,Trace<State> *trace,Route &child):
        Port<State>(s,cursor,trace,{}),state(s),route(child),reg(s.gp),flags(s.flags){}

    void pc(std::uint32_t address) {
        if constexpr(Observed)state.instruction_literal(address);
    }
    std::uint32_t word(std::uint32_t at,std::uint32_t address) {
        pc(at);
        return this->read(at,address,4);
    }
    std::uint32_t byte(std::uint32_t at,std::uint32_t address) {
        pc(at);
        return this->read(at,address,1);
    }
    void set(std::uint32_t at,std::uint32_t address,std::uint32_t value) {
        pc(at);
        this->store(at,address,4,value);
    }
    std::uint32_t local(Local field) const {return reg[ebp]-field;}
    std::uint32_t argument(Argument field) const {return reg[ebp]+field;}
    std::uint32_t get(std::uint32_t at,Local field){return word(at,local(field));}
    std::uint32_t get(std::uint32_t at,Argument field){return word(at,argument(field));}
    void put(std::uint32_t at,Local field,std::uint32_t value){set(at,local(field),value);}
    void push_raw(std::uint32_t at,std::uint32_t value) {
        reg[esp]-=4u;
        this->store(at,reg[esp],4,value);
    }
    void push(std::uint32_t at,std::uint32_t value){pc(at);push_raw(at,value);}
    void push_word(std::uint32_t at,std::uint32_t address) {
        const auto value=word(at,address);
        push_raw(at,value);
    }
    std::uint32_t pop_raw(std::uint32_t at) {
        const auto value=this->read(at,reg[esp],4);
        reg[esp]+=4u;
        return value;
    }
    std::uint32_t pop(std::uint32_t at){pc(at);return pop_raw(at);}
    template<class Operation>
    void update(std::uint32_t at,Local field,Operation op) {
        const auto address=local(field);
        const auto before=word(at,address);
        const auto after=commit_update(state,at,address,before,op);
        if(this->trace)this->trace->events.push_back({1,at,address,4,after});
    }
    void decrement(std::uint32_t at,Local field) {
        update(at,field,[](auto &f,auto n){return f.increment(n,4,true);});
    }
    void increment(std::uint32_t at,Local field) {
        update(at,field,[](auto &f,auto n){return f.increment(n,4,false);});
    }
    void zero(std::uint32_t at,Local field) {
        // The original AND with zero still reads the stack slot before writing.
        update(at,field,[](auto &f,auto){return f.logic(0u,4);});
    }
    void child(std::uint32_t at,std::uint32_t next,std::uint32_t target) {
        push(at,next);
        if(this->trace) {
            Port<State>::gp=reg;
            Port<State>::flags=flags;
            this->enter(target,[&](State &s){route(s,target,at);});
        } else {
            route(state,target,at);
        }
    }

    bool view_enabled() {
        push(0x168dedu,reg[ebp]);
        pc(0x168deeu);reg[ebp]=reg[esp];
        pc(0x168df0u);reg[esp]=flags.minus(reg[esp],0x84u,4);
        reg[eax]=word(0x168df6u,session);
        zero(0x168dfbu,count);
        flags.minus(word(0x168dffu,reg[eax]+8u),1u,4);
        pc(0x168e03u);set_low_byte(reg[eax],!flags.zf);
        pc(0x168e06u);set_low_byte(reg[eax],flags.increment(reg[eax]&0xffu,1,true));
        reg[eax]=flags.logic(reg[eax]&word(0x168e08u,view_mask),4);
        pc(0x168e0eu);flags.logic(reg[eax]&0xffu,1);
        pc(0x168e10u);
        return flags.zf;
    }
    bool enter_view() {
        push(0x168e16u,reg[ebx]);
        reg[ebx]=get(0x168e17u,view);
        push(0x168e1au,reg[edi]);
        pc(0x168e1bu);reg[edi]=flags.logic(none,4);
        pc(0x168e1eu);flags.minus(reg[ebx],reg[edi],4);
        pc(0x168e20u);
        return !flags.zf;
    }
    bool slots_ready() {
        pc(0x168e26u);reg[ebx]=flags.product(reg[ebx],user_stride);
        reg[ebx]=flags.plus(reg[ebx],word(0x168e2cu,first_person_views),4);
        push(0x168e32u,reg[esi]);
        pc(0x168e33u);set_low_byte(reg[ecx],flags.logic(0u,1));
        pc(0x168e35u);reg[esi]=reg[ebx]+12u;
        put(0x168e38u,slot_count,2u);
        do {
            set_low_byte(reg[edx],byte(0x168e3fu,reg[esi]));
            pc(0x168e41u);set_low_byte(reg[edx],flags.logic(reg[edx]&1u,1));
            pc(0x168e44u);set_low_byte(reg[eax],flags.logic((reg[eax]|reg[edx])&0xffu,1));
            flags.minus(word(0x168e46u,reg[esi]+4u),reg[edi],4);
            pc(0x168e49u);set_low_byte(reg[edx],!flags.zf);
            pc(0x168e4cu);set_low_byte(reg[ecx],flags.logic((reg[ecx]|reg[edx])&0xffu,1));
            pc(0x168e4eu);reg[esi]=flags.plus(reg[esi],slot_stride,4);
            decrement(0x168e54u,slot_count);
            pc(0x168e57u);
        } while(!flags.zf);
        pc(0x168e59u);flags.logic(reg[eax]&0xffu,1);
        pc(0x168e5bu);
        if(flags.zf)return false;
        pc(0x168e61u);flags.logic(reg[ecx]&0xffu,1);
        pc(0x168e63u);
        return !flags.zf;
    }
    bool select_representation() {
        reg[eax]=word(0x168e69u,reg[ebx]+4u);
        reg[esi]=get(0x168e6cu,unit);
        pc(0x168e6fu);flags.minus(reg[eax],reg[esi],4);
        pc(0x168e71u);
        if(!flags.zf)return false;
        pc(0x168e77u);flags.minus(reg[eax],reg[edi],4);
        pc(0x168e79u);
        if(flags.zf)return false;
        reg[eax]=word(0x168e7fu,reg[ebx]+8u);
        pc(0x168e82u);flags.logic(reg[eax],4);
        reg[ecx]=word(0x168e84u,representations);
        pc(0x168e8au);
        if(flags.sf!=flags.of)return false;
        flags.minus(reg[eax],word(0x168e90u,reg[ecx]+0x138u),4);
        pc(0x168e96u);
        return flags.sf!=flags.of;
    }
    void select_body_matrices() {
        zero(0x168e9cu,unit_matrices);
        pc(0x168ea0u);reg[eax]=flags.product(reg[eax],0xbcu);
        reg[eax]=flags.plus(reg[eax],word(0x168ea6u,reg[ecx]+0x13cu),4);
        zero(0x168eacu,unit_nodes);
        pc(0x168eb0u);flags.minus(reg[esi],reg[edi],4);
        reg[ecx]=word(0x168eb2u,reg[eax]+4u);
        reg[eax]=word(0x168eb5u,reg[eax]+12u);
        put(0x168eb8u,arms_tag,reg[ecx]);
        put(0x168ebbu,body_tag,reg[eax]);
        put(0x168ebeu,unit_model_tag,reg[edi]);
        pc(0x168ec1u);
        if(flags.zf)return;

        reg[edx]=word(0x168ec3u,objects);
        reg[edx]=word(0x168ec9u,reg[edx]+0x44u);
        pc(0x168eccu);reg[ecx]=0xffffu;
        pc(0x168ed1u);reg[eax]=reg[esi];
        pc(0x168ed3u);reg[eax]=flags.logic(reg[eax]&reg[ecx],4);
        pc(0x168ed5u);reg[eax]*=3u;
        reg[eax]=word(0x168ed8u,reg[edx]+8u+reg[eax]*4u);
        reg[eax]=word(0x168edcu,reg[eax]);
        reg[edx]=word(0x168edeu,tags);
        pc(0x168ee4u);reg[eax]=flags.logic(reg[eax]&reg[ecx],4);
        pc(0x168ee6u);reg[eax]=flags.shift(reg[eax],4u,4);
        reg[eax]=word(0x168ee9u,reg[edx]+reg[eax]+8u);
        reg[eax]=word(0x168eedu,reg[eax]+0x38u);
        pc(0x168ef0u);flags.minus(reg[eax],reg[edi],4);
        pc(0x168ef2u);
        if(flags.zf)return;
        pc(0x168ef4u);reg[eax]=flags.logic(reg[eax]&reg[ecx],4);
        pc(0x168ef6u);reg[eax]=flags.shift(reg[eax],4u,4);
        reg[eax]=word(0x168ef9u,reg[edx]+reg[eax]+8u);
        reg[eax]=word(0x168efdu,reg[eax]+4u);
        pc(0x168f00u);flags.minus(reg[eax],reg[edi],4);
        pc(0x168f02u);
        if(flags.zf)return;
        put(0x168f04u,unit_model_tag,reg[eax]);
        pc(0x168f07u);reg[eax]=local(unit_matrices);
        push(0x168f0au,reg[eax]);
        pc(0x168f0bu);reg[eax]=reg[esi];
        child(0x168f0du,0x168f12u,0xbac30u);
        put(0x168f12u,unit_nodes,reg[eax]);
    }
    void build_camera() {
        pc(0x168f15u);reg[eax]=camera_up;
        pc(0x168f1au);reg[ecx]=camera_forward;
        pc(0x168f1fu);reg[esi]=camera_position;
        pc(0x168f24u);reg[edx]=local(camera_root);
        child(0x168f27u,0x168f2cu,0x144bf0u);
        flags.logic(byte(0x168f2cu,reg[ebx])&4u,1);
        pc(0x168f2fu);
        if(flags.zf)return;
        this->reached(camera_override);
        pc(0x168f31u);reg[ecx]=reg[ebx]+0x2098u;
        pc(0x168f37u);reg[eax]=local(camera_adjustment);
        child(0x168f3du,0x168f42u,0x144090u);
        pc(0x168f42u);reg[eax]=local(camera_root);
        push(0x168f45u,reg[eax]);
        pc(0x168f46u);reg[edx]=local(camera_adjustment);
        pc(0x168f4cu);reg[ecx]=reg[eax];
        child(0x168f4eu,0x168f53u,0x145560u);
    }
    void publish_camera() {
        push(0x168f53u,native_model_record::matrix_words);
        reg[ecx]=pop(0x168f55u);
        pc(0x168f56u);reg[edi]=reg[ebx]+0x2060u;
        pc(0x168f5cu);reg[esi]=local(camera_root);
        pc(0x168f5fu);
        while(reg[ecx]) {
            const auto value=this->read(0x168f5fu,reg[esi],4);
            this->store(0x168f5fu,reg[edi],4,value);
            const auto delta=(state.other_flags&0x400u)?0xfffffffcu:4u;
            reg[esi]+=delta;reg[edi]+=delta;
            --reg[ecx];
            pc(0x168f5fu);
        }
    }
    bool arms_visible() {
        reg[eax]=word(0x168f71u,reg[esi]-0x1fcu);
        pc(0x168f77u);flags.logic(reg[eax]&1u,1);
        pc(0x168f79u);
        if(flags.zf)return false;
        flags.minus(word(0x168f7bu,reg[esi]-0x1f8u),none,4);
        pc(0x168f82u);
        if(flags.zf)return false;
        flags.minus(get(0x168f84u,capacity),1u,4);
        pc(0x168f88u);
        if(flags.zf||flags.sf!=flags.of)return false;
        pc(0x168f8au);flags.logic(reg[eax]&4u,1);
        pc(0x168f8cu);
        if(flags.zf)return false;
        reg[eax]=get(0x168f8eu,arms_tag);
        pc(0x168f91u);flags.minus(reg[eax],none,4);
        pc(0x168f94u);
        return !flags.zf;
    }
    void submit_arms() {
        push(0x168f61u,2u);
        put(0x168f63u,count,1u);
        pc(0x168f6au);reg[esi]=reg[ebx]+0x208u;
        reg[edi]=pop(0x168f70u);
        do {
            if(arms_visible()) {
                this->reached(arms_model);
                push_word(0x168f96u,argument(records));
                pc(0x168f99u);reg[ecx]=local(camera_root);
                push(0x168f9cu,reg[esi]);
                push(0x168f9du,reg[ecx]);
                reg[ecx]=word(0x168f9eu,reg[esi]+0x108u);
                push(0x168fa4u,1u);
                push_word(0x168fa6u,argument(unit));
                pc(0x168fa9u);reg[edx]=reg[esi]+0x10cu;
                child(0x168fafu,0x168fb4u,0x168d76u);
            }
            pc(0x168fb4u);reg[esi]=flags.plus(reg[esi],slot_stride,4);
            pc(0x168fbau);reg[edi]=flags.increment(reg[edi],4,true);
            pc(0x168fbbu);
        } while(!flags.zf);
    }
    bool weapon_visible() {
        flags.logic(byte(0x168fd2u,reg[edi]-4u)&1u,1);
        pc(0x168fd6u);
        if(flags.zf)return false;
        reg[eax]=word(0x168fd8u,reg[edi]);
        pc(0x168fdau);flags.minus(reg[eax],none,4);
        pc(0x168fddu);
        if(flags.zf)return false;

        reg[edx]=word(0x168fdfu,objects);
        reg[edx]=word(0x168fe5u,reg[edx]+0x44u);
        pc(0x168fe8u);reg[ecx]=0xffffu;
        pc(0x168fedu);reg[eax]=flags.logic(reg[eax]&reg[ecx],4);
        pc(0x168fefu);reg[eax]*=3u;
        reg[eax]=word(0x168ff2u,reg[edx]+8u+reg[eax]*4u);
        reg[eax]=word(0x168ff6u,reg[eax]);
        reg[edx]=word(0x168ff8u,reg[ebx]+8u);
        pc(0x168ffbu);reg[eax]=flags.logic(reg[eax]&reg[ecx],4);
        reg[ecx]=word(0x168ffdu,tags);
        pc(0x169003u);reg[eax]=flags.shift(reg[eax],4u,4);
        reg[esi]=word(0x169006u,reg[ecx]+reg[eax]+8u);
        child(0x16900au,0x16900fu,0x11e060u);
        pc(0x16900fu);flags.logic(reg[eax],4);
        pc(0x169011u);
        if(flags.zf) {
            pc(0x169013u);reg[eax]=flags.logic(none,4);
            pc(0x169016u);
        } else {
            reg[eax]=word(0x169018u,reg[eax]+4u);
        }
        reg[ecx]=get(0x16901bu,count);
        flags.minus(reg[ecx],get(0x16901eu,capacity),4);
        pc(0x169021u);
        if(flags.sf==flags.of)return false;
        flags.logic(byte(0x169023u,reg[edi]-4u)&2u,1);
        pc(0x169027u);
        if(flags.zf)return false;
        pc(0x169029u);reg[ecx]=flags.logic(none,4);
        pc(0x16902cu);flags.minus(reg[eax],reg[ecx],4);
        pc(0x16902eu);
        return !flags.zf;
    }
    void submit_weapons() {
        reg[eax]=get(0x168fbdu,records);
        pc(0x168fc0u);reg[eax]=flags.plus(reg[eax],record_stride,4);
        put(0x168fc5u,next_record,reg[eax]);
        pc(0x168fc8u);reg[edi]=reg[ebx]+0x10u;
        put(0x168fcbu,weapon_scan_count,2u);
        do {
            if(weapon_visible()) {
                this->reached(weapon_model);
                push_word(0x169030u,local(next_record));
                pc(0x169033u);reg[edx]=reg[edi]+0x304u;
                pc(0x169039u);reg[esi]=reg[edx]-0x20cu;
                push(0x16903fu,reg[esi]);
                pc(0x169040u);reg[esi]=local(camera_root);
                push(0x169043u,reg[esi]);
                push(0x169044u,1u);
                push_word(0x169046u,reg[edi]);
                child(0x169048u,0x16904du,0x168d76u);
                increment(0x16904du,count);
                update(0x169050u,next_record,[](auto &f,auto n){return f.plus(n,record_stride,4);});
            }
            pc(0x169057u);reg[edi]=flags.plus(reg[edi],slot_stride,4);
            decrement(0x16905du,weapon_scan_count);
            pc(0x169060u);
        } while(!flags.zf);
    }
    bool body_visible() {
        reg[eax]=get(0x169066u,count);
        flags.minus(reg[eax],get(0x169069u,capacity),4);
        pc(0x16906cu);
        if(flags.sf==flags.of)return false;
        flags.minus(get(0x169072u,body_tag),none,4);
        pc(0x169076u);
        if(flags.zf)return false;
        reg[ecx]=get(0x169078u,unit_model_tag);
        pc(0x16907bu);flags.minus(reg[ecx],none,4);
        pc(0x16907eu);
        if(flags.zf)return false;
        pc(0x169080u);reg[esi]=flags.logic(0u,4);
        flags.minus(get(0x169082u,unit_nodes),reg[esi],4);
        pc(0x169085u);
        if(flags.zf)return false;

        reg[edi]=get(0x169087u,body_tag);
        reg[eax]=word(0x16908au,tags);
        pc(0x16908fu);reg[edx]=0xffffu;
        pc(0x169094u);reg[ecx]=flags.logic(reg[ecx]&reg[edx],4);
        pc(0x169096u);reg[edi]=flags.logic(reg[edi]&reg[edx],4);
        pc(0x169098u);reg[edi]=flags.shift(reg[edi],4u,4);
        reg[edx]=word(0x16909bu,reg[eax]+reg[edi]+8u);
        pc(0x16909fu);reg[ecx]=flags.shift(reg[ecx],4u,4);
        reg[ecx]=word(0x1690a2u,reg[eax]+reg[ecx]+8u);
        reg[eax]=word(0x1690a6u,reg[edx]+0x48u);
        flags.minus(reg[eax],word(0x1690a9u,reg[ecx]+0x48u),4);
        pc(0x1690acu);
        if(!flags.zf)return false;
        reg[eax]=word(0x1690aeu,reg[edx]+0x44u);
        flags.minus(reg[eax],word(0x1690b1u,reg[ecx]+0x44u),4);
        pc(0x1690b4u);
        if(!flags.zf)return false;

        using physics_step_detail::f;
        state.xmm[0]=_mm_set_ss(f(word(0x1690b6u,reg[edx]+0x70u)));
        flags.scalar_compare(state.xmm[0],_mm_set_ss(f(word(0x1690bbu,0x46a0e8u))),true);
        pc(0x1690c2u);state.set(eax,flags.bits()|2u,1,8);
        pc(0x1690c3u);flags.logic((reg[eax]>>8u)&0x44u,1);
        pc(0x1690c6u);
        if(!flags.pf)return true;
        flags.scalar_compare(state.xmm[0],_mm_set_ss(f(word(0x1690c8u,camera_forward+8u))),false);
        pc(0x1690cfu);
        return !flags.cf&&!flags.zf;
    }
    void submit_body() {
        if(!body_visible())return;
        this->reached(body_model);
        reg[eax]=get(0x1690d1u,count);
        reg[edx]=get(0x1690d4u,unit_nodes);
        pc(0x1690d7u);reg[eax]=flags.product(reg[eax],record_stride);
        reg[eax]=flags.plus(reg[eax],get(0x1690ddu,records),4);
        reg[ecx]=get(0x1690e0u,unit_matrices);
        push(0x1690e3u,reg[eax]);
        reg[eax]=get(0x1690e4u,body_tag);
        push(0x1690e7u,reg[esi]);
        push(0x1690e8u,reg[esi]);
        push(0x1690e9u,reg[esi]);
        push_word(0x1690eau,argument(unit));
        child(0x1690edu,0x1690f2u,0x168d76u);
        increment(0x1690f2u,count);
    }
    void collect() {
        if(!slots_ready()){this->reached(no_weapon);return;}
        if(!select_representation()){this->reached(no_representation);return;}
        select_body_matrices();
        build_camera();
        publish_camera();
        submit_arms();
        submit_weapons();
        submit_body();
        reg[ecx]=get(0x1690f5u,records);
        pc(0x1690f8u);reg[eax]=reg[ecx]+record_stride;
        push(0x1690feu,reg[eax]);
        reg[eax]=get(0x1690ffu,view);
        push(0x169102u,reg[ecx]);
        child(0x169103u,0x169108u,0x16b4e8u);
    }
    void run() {
        if(view_enabled()) {
            if(enter_view()) {
                collect();
                reg[esi]=pop(0x169108u);
            } else {
                this->reached(unavailable_view);
            }
            reg[edi]=pop(0x169109u);
            reg[ebx]=pop(0x16910au);
        } else {
            this->reached(unavailable_view);
        }
        reg[eax]=get(0x16910bu,count);
        pc(0x16910eu);reg[esp]=reg[ebp];reg[ebp]=pop_raw(0x16910eu);
        (void)pop(0x16910fu);
        reg[esp]+=16u;
    }
};

template<bool Observed,class State,class Reference,class Route>
void run(State &s,std::uint32_t *cursor,bool verify,Reference reference,Route &route) {
    auto &tally=summary();
    native_object_tick_detail::execute(s,cursor,verify,coverage(),tally.calls,tally,reference,
        [&](State &v,std::uint32_t *pc,Trace<State> *trace){return Frame<State,Route,Observed>{v,pc,trace,route};});
    if(verify)tally.hit=coverage().load(std::memory_order_relaxed);
}

template<class State,class Reference,class Route>
bool bridge(State &s,Reference reference,Route route) {
    using native_small_queries_detail::flag;
    static const bool forced=flag("HALO2_REFERENCE_FIRST_PERSON_MODEL_PREPARATION");
    static const bool verify=flag("HALO2_VERIFY_FIRST_PERSON_MODEL_PREPARATION");
    if(forced || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    std::uint32_t *cursor;
    const bool silent=native_object_tick_detail::admitted(s,0x168dedu,0x16910fu,cursor,false);
    const bool sealed=!s.services.instruction || s.immutable_cursor.cursor ||
        native_census_detail::uncursored(s,0x168dedu,0x16910fu);
    if(silent && sealed)
        run<false>(s,cursor,verify,reference,route);
    else
        run<true>(s,nullptr,silent && verify,reference,route);
    return true;
}
}
