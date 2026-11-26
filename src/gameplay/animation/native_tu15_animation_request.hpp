#pragma once

#include "physics/runtime/native_tu15_two_bone_solve.hpp"

namespace halo2::native_animation_request {
using namespace native_object_tick_detail::boundary;

constexpr std::uint32_t entry=0x113d30u, exit=0x11473eu;
constexpr std::uint32_t none=0xffffffffu, default_activity=0x07000101u;
constexpr std::uint32_t unit_table=0x4ecbd0u, definition_table=0x4f0414u;
constexpr std::uint32_t channel_state=0x12au, pose_state=0x33eu;
constexpr std::uint32_t current_activity=0x70u, current_overlay=0x7cu;
constexpr unsigned frame_size=0x80u, unit_argument=0x94u, request_argument=0x98u;
enum class ActivityRoute : std::uint32_t {
    missing_channel=0x11435au, check_channel=0x114349u,
    special_activity=0x11401fu, high_activity=0x11420eu,
    high_variant=0x114183u, standard_activity=0x11418fu,
    low_variant=0x11412eu, apply_activity=0x11429du
};

// The original modifies these small flags in memory. Publish the resulting
// integer flags at the committed store, including when its observer aborts.
template<class State>
void update_field(State &state,std::uint32_t pc,std::uint32_t address,unsigned size,
                  std::uint32_t mask,std::uint32_t bits) {
    auto flags=state.flags;
    const auto value=physics_step_detail::r(state.memory,address,size);
    const auto updated=flags.logic((value&mask)|bits,size);
    if(state.observe) {
        const SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &event) {
            state.flags=flags;
            state.observe(event);
        };
        physics_step_detail::w(state.memory,observe,pc,address,size,updated);
    } else {
        physics_step_detail::w(state.memory,{},pc,address,size,updated);
        state.flags=flags;
    }
}

template<class State,bool Observed> struct Frame {
    State &s;
    std::uint32_t *cursor;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;
    bool reopened{};

    Frame(State &state,std::uint32_t *position):s(state),cursor(position),g(s.gp),flags(s.flags){}
    void pc(std::uint32_t site) {
        if constexpr(Observed)s.instruction_literal(site);
        else if(reopened)s.instruction_literal(site);
    }
    void access(std::uint32_t site) {
        pc(site);
        if(cursor)*cursor=site;
    }
    void after_access() {
        if constexpr(!Observed) {
            if(!cursor)return;
            const auto *policy=s.immutable_cursor.cursor;
            if((policy->first_trace&&*policy->first_trace)||(policy->second_trace&&*policy->second_trace)) {
                cursor=nullptr;
                reopened=true;
            }
        }
    }
    std::uint32_t stack(unsigned offset)const{return g[esp]+offset;}
    std::uint32_t read(std::uint32_t site,std::uint32_t address,unsigned size=4) {
        access(site);
        const auto value=physics_step_detail::r(s.memory,address,size);
        after_access();
        return value;
    }
    void write(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned size=4) {
        access(site);
        physics_step_detail::w(s.memory,s.observe,site,address,size,value);
        after_access();
    }
    void push(std::uint32_t site,std::uint32_t value) {
        access(site);
        native_two_bone_solve::push_word(s,site,value);
        after_access();
    }
    std::uint32_t pop(std::uint32_t site) {
        const auto value=read(site,g[esp]);
        g[esp]+=4u;
        return value;
    }
    template<class Child> void call(std::uint32_t site,Child &child) {
        push(site,site+5u);
        child(s);
        after_access();
    }
    template<class Route> void call(std::uint32_t site,std::uint32_t target,Route &route) {
        push(site,site+5u);
        route(s,target,site);
        after_access();
    }
    void test_byte(std::uint32_t site,Register reg,std::uint32_t mask=0xffu) {
        pc(site);flags.logic(s.reg(reg,1)&mask,1);
    }
    void test_field(std::uint32_t site,std::uint32_t address,std::uint32_t mask) {
        flags.logic(read(site,address,1)&mask,1);
    }
    void enable_field(std::uint32_t site,std::uint32_t address,unsigned size,std::uint32_t bits) {
        access(site);
        update_field(s,site,address,size,none,bits);
        after_access();
    }
    static std::uint32_t extend_short(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }
    bool signed_less()const{return flags.sf!=flags.of;}
    void compare(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned size=4) {
        flags.minus(read(site,address,size),value,size);
    }

    void open() {
        pc(entry);g[esp]=flags.minus(g[esp],frame_size,4);
        g[ecx]=read(0x113d36u,stack(0x84u));
        push(0x113d3du,g[ebx]);
        push(0x113d3eu,g[ebp]);
        push(0x113d3fu,g[esi]);
        pc(0x113d40u);g[eax]=g[ecx];
        pc(0x113d42u);g[eax]=flags.logic(g[eax]&0xffffu,4);
        push(0x113d47u,g[edi]);
        pc(0x113d48u);g[edi]=g[eax]*3u;
        g[eax]=read(0x113d4bu,unit_table);
        g[edx]=read(0x113d50u,g[eax]+0x44u);
        pc(0x113d53u);g[edi]=flags.shift(g[edi],2,4);
        g[ebx]=read(0x113d56u,g[edi]+g[edx]+8u);
        g[eax]=read(0x113d5au,g[ebx]);
        g[edx]=read(0x113d5cu,definition_table);
        g[esi]=extend_short(read(0x113d62u,g[ebx]+pose_state,2));
        g[ebp]=extend_short(read(0x113d69u,g[ebx]+channel_state,2));
        pc(0x113d70u);g[eax]=flags.logic(g[eax]&0xffffu,4);
        pc(0x113d75u);g[eax]=flags.shift(g[eax],4,4);
        g[eax]=read(0x113d78u,g[eax]+g[edx]+8u);
        write(0x113d7cu,stack(0x38u),g[eax]);
        pc(0x113d80u);g[eax]=default_activity;
        write(0x113d85u,stack(0x14u),g[eax]);
        write(0x113d89u,stack(0x20u),g[eax]);
        pc(0x113d8du);g[esi]=flags.plus(g[esi],g[ebx],4);
        pc(0x113d8fu);g[eax]=g[ecx];
        write(0x113d91u,stack(0x1cu),g[ebx]);
        write(0x113d95u,stack(0x24u),g[esi]);
        pc(0x113d99u);g[ebp]=flags.plus(g[ebp],g[ebx],4);
        write(0x113d9bu,stack(0x11u),0,1);
        write(0x113da0u,stack(0x13u),0,1);
        write(0x113da5u,stack(0x48u),0,1);
        write(0x113daau,stack(0x3cu),0);
    }

    void read_requested_activities() {
        g[ecx]=read(0x113db7u,stack(request_argument));
        write(0x113dbeu,stack(0x12u),s.reg(eax,1),1);
        g[eax]=read(0x113dc2u,g[ecx]);
        pc(0x113dc4u);flags.minus(g[eax],none,4);
        pc(0x113dc7u);
        if(!flags.zf) {
            pc(0x113dc9u);flags.logic(g[eax],4);
            pc(0x113dcbu);
            if(!flags.zf)write(0x113dcdu,stack(0x14u),g[eax]);
        }
        g[ecx]=read(0x113dd1u,stack(request_argument));
        g[eax]=read(0x113dd8u,g[ecx]+4u);
        pc(0x113ddbu);flags.minus(g[eax],none,4);
        pc(0x113ddeu);
        if(!flags.zf) {
            pc(0x113de0u);flags.logic(g[eax],4);
            pc(0x113de2u);
            if(!flags.zf) {
                s.set(edx,read(0x113de4u,g[ecx]+8u,1),1);
                write(0x113de7u,stack(0x20u),g[eax]);
                write(0x113debu,stack(0x48u),s.reg(edx,1),1);
            }
        }
    }
    template<class Resolve> void resolve_local_activity(Resolve &resolve) {
        flags.minus(read(0x113defu,g[ebx]+0x14u),none,4);
        pc(0x113df3u);
        if(!flags.zf)return;
        g[ecx]=read(0x113df9u,g[ebp]+current_activity);
        pc(0x113dfcu);g[eax]=0x020001b3u;
        pc(0x113e01u);flags.minus(g[ecx],g[eax],4);
        pc(0x113e03u);
        if(flags.zf) {
            write(0x113e05u,stack(0x14u),g[eax]);
            pc(0x113e09u);
            return;
        }
        s.set(eax,read(0x113e0eu,g[ebx]+0x10au,1),1);
        pc(0x113e14u);s.set(eax,flags.right(s.reg(eax,1),2,1,false),1);
        test_byte(0x113e17u,eax,1);
        pc(0x113e19u);
        if(!flags.zf) {
            write(0x113e1bu,stack(0x14u),0x06000086u);
            pc(0x113e23u);
            return;
        }
        g[ecx]=read(0x113e25u,unit_table);
        g[edx]=read(0x113e2bu,g[ecx]+0x44u);
        g[eax]=read(0x113e2eu,g[edi]+g[edx]+8u);
        g[ecx]=extend_short(read(0x113e32u,g[eax]+0x346u,2));
        flags.minus(read(0x113e39u,g[ecx]+g[eax]+0x36u,2),0,2);
        pc(0x113e3fu);
        if(!flags.zf) {
            write(0x113e41u,stack(0x20u),default_activity);
            pc(0x113e49u);
            return;
        }
        flags.minus(read(0x113e4bu,stack(0x14u)),default_activity,4);
        pc(0x113e53u);
        if(flags.zf) {
            g[edx]=read(0x113e55u,stack(request_argument));
            s.set(edx,read(0x113e5cu,g[edx]+9u,1),1);
            g[eax]=read(0x113e5fu,stack(unit_argument));
            push(0x113e66u,0);
            call(0x113e68u,resolve);
            write(0x113e6du,stack(0x14u),g[eax]);
        }
        s.set(eax,read(0x113e71u,g[ebx]+0x1f4u,1),1);
        test_byte(0x113e77u,eax);
        pc(0x113e79u);
        if(!flags.cf&&!flags.zf)write(0x113e7bu,stack(0x14u),0x07000039u);
        g[esi]=read(0x113e83u,g[esi]+0x2cu);
        pc(0x113e86u);flags.minus(g[esi],none,4);
        pc(0x113e89u);
        if(!flags.zf)write(0x113e8bu,stack(0x14u),g[esi]);
    }

    template<class Query,class Resolve> void prepare(Query &query,Resolve &resolve) {
        open();
        call(0x113db2u,query);
        read_requested_activities();
        resolve_local_activity(resolve);
    }

    template<class PrepareGraph> void prepare_channel_graph(PrepareGraph &prepare_graph) {
        g[eax]=read(0x113ea4u,unit_table);
        g[ecx]=read(0x113ea9u,g[eax]+0x44u);
        g[eax]=read(0x113eacu,g[ecx]+g[edi]+8u);
        g[edx]=extend_short(read(0x113eb0u,g[eax]+0x112u,2));
        g[ecx]=extend_short(read(0x113eb7u,g[eax]+0x10eu,2));
        pc(0x113ebeu);g[edx]=flags.plus(g[edx],g[eax],4);
        push(0x113ec0u,g[edx]);
        g[edx]=extend_short(read(0x113ec1u,g[eax]+0x110u,2));
        pc(0x113ec8u);g[ecx]=flags.plus(g[ecx],g[eax],4);
        g[eax]=read(0x113ecau,stack(request_argument));
        push(0x113ed1u,g[ecx]);
        pc(0x113ed2u);g[edx]=flags.right(g[edx],5,4,false);
        push(0x113ed5u,g[edx]);
        push(0x113ed6u,g[eax]);
        push(0x113ed7u,0xc1630u);
        pc(0x113edcu);g[esi]=g[ebp];
        call(0x113edeu,prepare_graph);
    }
    template<class Clock> void update_channel_clock(Clock &clock) {
        g[ecx]=read(0x113ee3u,g[ebp]+current_activity);
        g[eax]=read(0x113ee6u,g[ebp]+current_overlay);
        write(0x113ee9u,stack(0x4cu),g[ecx]);
        s.set(ecx,read(0x113eedu,g[ebp]+0x14u,2),2);
        pc(0x113ef1u);flags.logic(s.reg(ecx,2),2);
        write(0x113ef4u,stack(0x34u),g[eax]);
        pc(0x113ef8u);
        if(flags.zf)return;
        g[edx]=read(0x113efau,g[ebp]+0x16u,2);
        g[esi]=read(0x113efeu,stack(unit_argument));
        pc(0x113f05u);g[ecx]=s.reg(ecx,2);
        push(0x113f08u,g[edx]);
        push(0x113f09u,g[ecx]);
        call(0x113f0au,clock);
    }
    template<class ChannelInfo> bool allow_channel_selection(ChannelInfo &channel_info) {
        test_field(0x113f0fu,g[ebp]+0x6cu,1);
        pc(0x113f13u);
        if(!flags.zf)return false;
        test_field(0x113f19u,g[ebp]+0x12u,1);
        pc(0x113f1du);
        if(flags.zf)return false;
        test_field(0x113f23u,g[ebp]+0x11u,9);
        pc(0x113f27u);
        if(!flags.zf)return false;
        pc(0x113f2du);g[ecx]=g[ebp];
        call(0x113f2fu,channel_info);
        pc(0x113f34u);g[edx]=1u;
        flags.minus(read(0x113f39u,g[eax]+0x14u,2),s.reg(edx,2),2);
        pc(0x113f3du);
        if(flags.zf||signed_less())return false;
        test_field(0x113f43u,g[ebp]+0x11u,0xau);
        write(0x113f47u,stack(0x12u),s.reg(edx,1),1);
        pc(0x113f4bu);
        if(flags.zf)return false;
        g[eax]=read(0x113f51u,g[ebp]+0x12u,2);
        pc(0x113f55u);g[ecx]=g[eax];
        pc(0x113f57u);g[ecx]=flags.logic(g[ecx]&0x4000u,4);
        pc(0x113f5du);
        if(!flags.zf) {
            test_byte(0x113f5fu,eax,2);
            pc(0x113f61u);
            if(!flags.zf) {
                write(0x113f63u,stack(0x11u),0,1);
                pc(0x113f68u);
                return false;
            }
        }
        pc(0x113f6du);flags.logic(g[ecx],4);
        pc(0x113f6fu);
        if(flags.zf)return true;
        g[eax]=read(0x113f71u,stack(0x24u));
        test_field(0x113f75u,g[eax]+1u,1);
        pc(0x113f79u);
        if(!flags.zf)return true;
        enable_field(0x113f7bu,g[ebp]+0x6cu,2,s.reg(edx,2));
        write(0x113f7fu,stack(0x11u),0,1);
        pc(0x113f84u);
        return false;
    }
    template<class Select> void choose_activities(Select &select) {
        g[esi]=read(0x113f89u,stack(0x34u));
        g[ebx]=read(0x113f8du,stack(unit_argument));
        pc(0x113f94u);g[ecx]=stack(0x40u);
        push(0x113f98u,g[ecx]);
        pc(0x113f99u);g[edx]=stack(0x48u);
        pc(0x113f9du);g[eax]=default_activity;
        push(0x113fa2u,g[edx]);
        push(0x113fa3u,g[esi]);
        write(0x113fa4u,stack(0x50u),g[eax]);
        write(0x113fa8u,stack(0x4cu),g[eax]);
        call(0x113facu,select);
        g[eax]=read(0x113fb1u,stack(0x44u));
        pc(0x113fb5u);flags.minus(g[eax],default_activity,4);
        g[ecx]=read(0x113fbau,stack(0x40u));
        pc(0x113fbeu);
        if(flags.zf) {
            pc(0x113fc0u);flags.minus(g[ecx],default_activity,4);
            pc(0x113fc6u);
            if(flags.zf)return;
        }
        write(0x113fc8u,stack(0x14u),g[eax]);
        write(0x113fccu,stack(0x20u),g[ecx]);
        write(0x113fd0u,stack(0x13u),1,1);
    }
    ActivityRoute route_activity() {
        pc(0x113fd5u);flags.minus(g[esi],0x0a00001du,4);
        pc(0x113fdbu);
        if(!flags.zf&&!signed_less())return ActivityRoute::high_activity;
        pc(0x113fe1u);
        if(flags.zf)return ActivityRoute::special_activity;
        pc(0x113fe3u);flags.minus(g[esi],0x09000020u,4);
        pc(0x113fe9u);
        if(!flags.zf&&!signed_less())return ActivityRoute::high_variant;
        pc(0x113fefu);flags.minus(g[esi],0x0900001fu,4);
        pc(0x113ff5u);
        if(!signed_less())return ActivityRoute::standard_activity;
        pc(0x113ffbu);flags.minus(g[esi],0x0700005cu,4);
        pc(0x114001u);
        if(signed_less())return ActivityRoute::apply_activity;
        pc(0x114007u);flags.minus(g[esi],0x0700005du,4);
        pc(0x11400du);
        if(flags.zf||signed_less())return ActivityRoute::low_variant;
        pc(0x114013u);flags.minus(g[esi],0x0800001cu,4);
        pc(0x114019u);
        return flags.zf?ActivityRoute::special_activity:ActivityRoute::apply_activity;
    }
    template<class Graph,class Clock,class ChannelInfo,class Select>
    ActivityRoute select_activity(Graph &graph,Clock &clock,ChannelInfo &channel_info,Select &select) {
        flags.minus(read(0x113e8fu,g[ebp]),none,4);
        pc(0x113e93u);
        if(flags.zf)return ActivityRoute::missing_channel;
        flags.minus(read(0x113e99u,g[ebp]+6u,2),none,2);
        pc(0x113e9eu);
        if(flags.zf)return ActivityRoute::check_channel;
        prepare_channel_graph(graph);
        update_channel_clock(clock);
        if(!allow_channel_selection(channel_info))return ActivityRoute::check_channel;
        choose_activities(select);
        return route_activity();
    }

    void use_default_offset() {
        g[edx]=read(0x1141b7u,0x474d04u);
        g[eax]=read(0x1141bdu,g[edx]);
        g[ecx]=read(0x1141bfu,g[edx]+4u);
        g[edx]=read(0x1141c2u,g[edx]+8u);
        g[edi]=read(0x1141c5u,stack(unit_argument));
        write(0x1141ccu,stack(0x28u),g[eax]);
        write(0x1141d0u,stack(0x2cu),g[ecx]);
        write(0x1141d4u,stack(0x30u),g[edx]);
    }
    __m128 scalar(std::uint32_t site,std::uint32_t address) {
        return _mm_set_ss(native_tu15_leaves::as_float(read(site,address)));
    }
    void spill_offset(std::uint32_t site,unsigned offset) {
        write(site,stack(offset),native_tu15_leaves::low_bits(s.xmm[0]));
    }
    template<class Route> void locate_activity_offset(Route &route) {
        pc(0x1140a6u);g[edx]=g[ebx]+1u;
        push(0x1140a9u,g[edx]);
        pc(0x1140aau);g[edx]=stack(0x2cu);
        call(0x1140aeu,0x111550u,route);
        push(0x1140b3u,g[eax]);
        call(0x1140b4u,0x140f00u,route);
        g[edi]=read(0x1140b9u,stack(unit_argument));
        pc(0x1140c0u);g[esi]=g[eax];
        pc(0x1140c2u);g[eax]=stack(0x5cu);
        push(0x1140c6u,g[eax]);
        pc(0x1140c7u);g[eax]=g[edi];
        pc(0x1140c9u);g[esi]=flags.increment(g[esi],4,true);
        call(0x1140cau,0xbc190u,route);
        push(0x1140cfu,g[esi]);
        push(0x1140d0u,0);
        pc(0x1140d2u);g[esi]=stack(0x58u);
        pc(0x1140d6u);g[eax]=g[ebp];
        call(0x1140d8u,0x1cf650u,route);
        pc(0x1140ddu);g[ecx]=stack(0x5cu);
        push(0x1140e1u,g[ecx]);
        pc(0x1140e2u);g[edx]=g[esi];
        pc(0x1140e4u);g[eax]=g[esi];
        call(0x1140e6u,0x145070u,route);
        push(0x1140ebu,g[ebx]);
        pc(0x1140ecu);g[eax]=g[edi];
        call(0x1140eeu,0xbac00u,route);
        s.xmm[0]=scalar(0x1140f3u,g[eax]+0x28u);
        s.xmm[0]=_mm_sub_ss(s.xmm[0],scalar(0x1140f8u,stack(0x50u)));
        pc(0x1140feu);g[eax]=flags.plus(g[eax],0x28u,4);
        spill_offset(0x114101u,0x28u);
        s.xmm[0]=scalar(0x114107u,g[eax]+4u);
        s.xmm[0]=_mm_sub_ss(s.xmm[0],scalar(0x11410cu,stack(0x54u)));
        spill_offset(0x114112u,0x2cu);
        s.xmm[0]=scalar(0x114118u,g[eax]+8u);
        s.xmm[0]=_mm_sub_ss(s.xmm[0],scalar(0x11411du,stack(0x58u)));
        spill_offset(0x114123u,0x30u);
        pc(0x114129u);
    }
    template<class Route> void publish_activity_offset(Route &route) {
        g[ebx]=read(0x1141d8u,stack(0x20u));
        const auto overlay=read(0x1141dcu,stack(0x34u));
        flags.minus(g[ebx],overlay,4);
        pc(0x1141e0u);
        bool changed=!flags.zf;
        if(!changed) {
            g[eax]=read(0x1141e2u,stack(0x4cu));
            compare(0x1141e6u,stack(0x14u),g[eax]);
            pc(0x1141eau);
            changed=!flags.zf;
        }
        if(changed) {
            pc(0x1141f0u);g[eax]=flags.logic(0,4);
            pc(0x1141f2u);s.set(eax,1,1);
        } else {
            pc(0x1141ecu);s.set(eax,flags.logic(0,1),1);
            pc(0x1141eeu);
        }
        push(0x1141f4u,g[eax]);
        pc(0x1141f5u);g[ecx]=stack(0x2cu);
        pc(0x1141f9u);g[esi]=g[edi];
        call(0x1141fbu,0xe7d90u,route);
        const auto prior_overlay=read(0x114200u,stack(0x34u));
        flags.minus(g[ebx],prior_overlay,4);
        write(0x114204u,stack(0x11u),!flags.zf,1);
        pc(0x114209u);
    }
    template<class Route> void use_special_activity(Route &route) {
        g[ecx]=read(0x11401fu,stack(0x1cu));
        s.set(eax,read(0x114023u,g[ecx]+0xaau,1),1);
        test_byte(0x114029u,eax);
        pc(0x11402bu);
        if(!flags.zf)return;
        g[ecx]=read(0x114031u,stack(unit_argument));
        call(0x114038u,0xc1cc0u,route);
        test_byte(0x11403du,eax);
        pc(0x11403fu);
        if(!flags.zf)return;
        g[edx]=read(0x114045u,unit_table);
        g[eax]=read(0x11404bu,g[edx]+0x44u);
        g[ecx]=read(0x11404eu,g[eax]+g[edi]+8u);
        g[edx]=read(0x114052u,g[ecx]);
        g[eax]=read(0x114054u,definition_table);
        pc(0x114059u);g[edx]=flags.logic(g[edx]&0xffffu,4);
        pc(0x11405fu);g[edx]=flags.shift(g[edx],4,4);
        g[ecx]=read(0x114062u,g[edx]+g[eax]+8u);
        g[ebx]=extend_short(read(0x114066u,g[ecx]+0x254u,2));
        pc(0x11406du);flags.minus(g[ebx],none,4);
        pc(0x114070u);
        if(flags.zf)return;
        pc(0x114076u);g[eax]=flags.logic(0,4);
        write(0x114078u,stack(0x28u),g[eax]);
        write(0x11407cu,stack(0x2cu),g[eax]);
        write(0x114080u,stack(0x30u),s.reg(eax,1),1);
        write(0x114084u,stack(0x31u),s.reg(eax,1),1);
        pc(0x114088u);g[edi]=stack(0x28u);
        pc(0x11408cu);g[eax]=g[ebp];
        call(0x11408eu,0x1cf630u,route);
        pc(0x114093u);g[ecx]=g[ebx];
        pc(0x114095u);g[edx]=stack(0x28u);
        call(0x114099u,0x111580u,route);
        test_byte(0x11409eu,eax);
        pc(0x1140a0u);
        if(flags.zf)use_default_offset();
        else locate_activity_offset(route);
        publish_activity_offset(route);
    }

    struct DeferredChannel {
        std::uint32_t identity,no_identity,frame,no_frame,status,inactive,mark;
    };
    bool defer_channel(unsigned offset,const DeferredChannel &sites,bool identity_loaded=false) {
        if(identity_loaded) {
            pc(sites.identity);flags.minus(g[ecx],g[eax],4);
        } else {
            compare(sites.identity,g[ebp]+offset,g[eax]);
        }
        pc(sites.no_identity);
        if(flags.zf)return false;
        compare(sites.frame,g[ebp]+offset+6u,s.reg(eax,2),2);
        pc(sites.no_frame);
        if(flags.zf)return false;
        test_field(sites.status,g[ebp]+offset+0x12u,1);
        pc(sites.inactive);
        if(flags.zf)return false;
        enable_field(sites.mark,g[ebp]+offset+0x11u,1,1);
        return true;
    }
    void use_low_variant() {
        g[ecx]=read(0x11412eu,g[ebp]+0x40u);
        pc(0x114131u);g[eax]=flags.logic(none,4);
        defer_channel(0x40u,{0x114134u,0x114136u,0x114138u,0x11413cu,0x11413eu,0x114142u,0x114144u},true);
        defer_channel(0,{0x114148u,0x11414bu,0x11414du,0x114151u,0x114153u,0x114157u,0x114159u});
        if(defer_channel(0x20u,{0x11415du,0x114160u,0x114166u,0x11416au,0x114170u,0x114174u,0x11417au}))
            pc(0x11417eu);
    }
    template<class Route> void use_standard_activity(Route &route) {
        g[eax]=read(0x11418fu,stack(0x1cu));
        s.set(ecx,read(0x114193u,g[eax]+0xaau,1),1);
        test_byte(0x114199u,ecx);
        pc(0x11419bu);
        if(!flags.zf)return;
        g[eax]=read(0x1141a1u,stack(unit_argument));
        call(0x1141a8u,0x0e6850u,route);
        write(0x1141adu,stack(0x11u),1,1);
        pc(0x1141b2u);
    }
    template<class Route> void check_high_activity_slot(Route &route) {
        g[edx]=read(0x114245u,stack(0x38u));
        g[eax]=read(0x114249u,g[edx]+0xbcu);
        g[esi]=read(0x11424fu,stack(unit_argument));
        pc(0x114256u);g[eax]=flags.right(g[eax],1,4,false);
        pc(0x114258u);s.set(ebx,flags.logic(0,1),1);
        test_byte(0x11425au,eax,1);
        pc(0x11425cu);
        if(!flags.zf) {
            pc(0x11425eu);g[eax]=g[esi];
            call(0x114260u,0x111cd0u,route);
            pc(0x114265u);flags.minus(g[eax],0x0d000042u,4);
            pc(0x11426au);
            bool active=!flags.zf;
            if(!active) {
                g[ecx]=read(0x11426cu,stack(0x1cu));
                test_field(0x114270u,g[ecx]+0xc1u,1);
                pc(0x114277u);
                active=!flags.zf;
            }
            if(active){pc(0x114279u);s.set(ebx,1,1);}
        }
        pc(0x11427bu);g[eax]=g[esi];
        call(0x11427du,0xb1870u,route);
        test_byte(0x114282u,eax);
        pc(0x114284u);
        if(flags.zf) {
            test_byte(0x114286u,ebx);
            pc(0x114288u);
            if(!flags.zf) {
                push(0x11428au,g[esi]);
                call(0x11428bu,0xd9110u,route);
                pc(0x114290u);
                return;
            }
        }
        g[eax]=read(0x114292u,stack(0x24u));
        enable_field(0x114296u,g[eax],1,4);
        enable_field(0x114299u,g[ebp]+0x6cu,1,1);
    }
    template<class Route> void use_high_activity(Route &route) {
        pc(0x11420eu);flags.minus(g[esi],0x0c000043u,4);
        pc(0x114214u);
        if(flags.zf) {
            check_high_activity_slot(route);
            return;
        }
        pc(0x114216u);flags.minus(g[esi],0x0d000042u,4);
        pc(0x11421cu);
        if(flags.zf) {
            write(0x11423eu,stack(0x11u),0,1);
            pc(0x114243u);
            return;
        }
        pc(0x11421eu);flags.minus(g[esi],0x0e0000c2u,4);
        pc(0x114224u);
        if(flags.zf) {
            g[ecx]=read(0x114226u,stack(0x14u));
            g[eax]=read(0x11422au,stack(unit_argument));
            push(0x114231u,g[ecx]);
            call(0x114232u,0x11ddf0u,route);
            write(0x114237u,stack(0x13u),1,1);
            pc(0x11423cu);
        }
    }

    template<class Route> void adjust_pose_activity(Route &route) {
        g[ebx]=read(0x1142c6u,stack(0x1cu));
        g[edi]=read(0x1142cau,g[ebx]+0x14u);
        pc(0x1142cdu);flags.minus(g[edi],none,4);
        pc(0x1142d0u);
        if(flags.zf)return;
        pc(0x1142d2u);g[ecx]=g[esi];
        call(0x1142d4u,0xc9550u,route);
        test_byte(0x1142d9u,eax);
        pc(0x1142dbu);
        if(flags.zf)return;
        g[eax]=read(0x1142ddu,unit_table);
        g[ecx]=read(0x1142e2u,g[eax]+0x44u);
        pc(0x1142e5u);g[edi]=flags.logic(g[edi]&0xffffu,4);
        pc(0x1142ebu);g[edx]=g[edi]*3u;
        g[edx]=read(0x1142eeu,g[ecx]+g[edx]*4u+8u);
        g[eax]=read(0x1142f2u,g[edx]);
        g[ecx]=read(0x1142f4u,definition_table);
        pc(0x1142fau);g[eax]=flags.logic(g[eax]&0xffffu,4);
        pc(0x1142ffu);g[eax]=flags.shift(g[eax],4,4);
        g[edx]=read(0x114302u,g[eax]+g[ecx]+8u);
        g[eax]=extend_short(read(0x114306u,g[ebx]+0x1fcu,2));
        g[edi]=read(0x11430du,g[edx]+0x1ccu);
        pc(0x114313u);g[eax]=flags.product(g[eax],0xb0u);
        g[ecx]=read(0x114319u,g[eax]+g[edi]);
        pc(0x11431cu);g[eax]=flags.plus(g[eax],g[edi],4);
        pc(0x11431eu);g[ecx]=flags.right(g[ecx],11,4,false);
        test_byte(0x114321u,ecx,1);
        pc(0x114324u);
        if(!flags.zf) {
            compare(0x114326u,g[eax]+0x3eu,none,2);
            pc(0x11432bu);
            if(!flags.zf) {
                write(0x11432du,stack(0x3cu),4);
                write(0x114335u,stack(0x14u),default_activity);
                write(0x11433du,stack(0x20u),0x0400000cu);
            }
        }
    }
    template<class Route> void apply_activity(Route &route) {
        g[esi]=read(0x11429du,stack(unit_argument));
        pc(0x1142a4u);g[ecx]=g[esi];
        call(0x1142a6u,0x1136f0u,route);
        test_byte(0x1142abu,eax);
        pc(0x1142adu);
        if(flags.zf) {
            compare(0x1142afu,stack(0x20u),default_activity);
            write(0x1142b7u,stack(0x11u),1,1);
            pc(0x1142bcu);
            if(flags.zf)write(0x1142beu,stack(0x20u),0x0400000cu);
        }
        adjust_pose_activity(route);
        g[ebx]=read(0x114345u,stack(0x1cu));
    }
    void check_channel(bool present) {
        if(present) {
            compare(0x114349u,g[ebp],none);
            pc(0x11434du);
            if(!flags.zf) {
                compare(0x11434fu,g[ebp]+6u,none,2);
                pc(0x114354u);
                if(!flags.zf) {
                    pc(0x114356u);s.set(ecx,1,1);
                    pc(0x114358u);
                    return;
                }
            }
        }
        pc(0x11435au);s.set(ecx,flags.logic(0,1),1);
    }
    template<class Route> bool needs_transition(Route &route) {
        g[esi]=read(0x11435cu,stack(0x14u));
        g[edx]=read(0x114360u,g[ebp]+current_activity);
        pc(0x114363u);s.set(eax,flags.logic(0,1),1);
        pc(0x114365u);flags.minus(g[esi],g[edx],4);
        write(0x114367u,stack(0x1bu),s.reg(eax,1),1);
        pc(0x11436bu);
        if(!flags.zf) {
            pc(0x11436du);flags.minus(g[esi],default_activity,4);
            pc(0x114373u);
            if(!flags.zf){pc(0x114375u);s.set(eax,1,1);}
        }
        g[edi]=read(0x114377u,stack(0x20u));
        const auto overlay=read(0x11437bu,g[ebp]+current_overlay);
        flags.minus(g[edi],overlay,4);
        pc(0x11437eu);
        if(!flags.zf) {
            pc(0x114380u);flags.minus(g[edi],default_activity,4);
            pc(0x114386u);
            if(!flags.zf){pc(0x114388u);s.set(eax,1,1);}
        }
        s.set(edx,read(0x11438au,stack(0x11u),1),1);
        test_byte(0x11438eu,edx);
        pc(0x114390u);
        bool signal=!flags.zf;
        if(!signal) {
            test_byte(0x114392u,ecx);
            pc(0x114394u);
            signal=flags.zf;
        }
        if(signal)write(0x114396u,stack(0x1bu),1,1);
        test_byte(0x11439bu,eax);
        pc(0x11439du);
        if(!flags.zf) {
            s.set(eax,read(0x11439fu,stack(0x13u),1),1);
            test_byte(0x1143a3u,eax);
            pc(0x1143a5u);
            if(!flags.zf)return true;
            g[edx]=read(0x1143a7u,stack(0x48u));
            g[eax]=read(0x1143abu,stack(unit_argument));
            push(0x1143b2u,g[edx]);
            push(0x1143b3u,g[edi]);
            push(0x1143b4u,g[eax]);
            call(0x1143b5u,0x112a80u,route);
            test_byte(0x1143bau,eax);
            pc(0x1143bcu);
            if(!flags.zf)return true;
        }
        s.set(eax,read(0x1143beu,stack(0x1bu),1),1);
        test_byte(0x1143c2u,eax);
        pc(0x1143c4u);
        return !flags.zf;
    }

    template<class Route> bool should_reset_channel(Route &route) {
        g[eax]=read(0x1143cau,stack(0x3cu));
        g[ebx]=read(0x1143ceu,stack(unit_argument));
        push(0x1143d5u,0);
        push(0x1143d7u,0);
        push(0x1143d9u,g[edi]);
        push(0x1143dau,default_activity);
        push(0x1143dfu,default_activity);
        push(0x1143e4u,g[esi]);
        pc(0x1143e5u);g[eax]=flags.logic(g[eax]|0x401u,4);
        push(0x1143eau,g[ebx]);
        call(0x1143ebu,0x111b10u,route);
        test_byte(0x1143f0u,eax);
        pc(0x1143f2u);
        if(!flags.zf)return false;
        pc(0x1143f4u);flags.minus(g[edi],default_activity,4);
        pc(0x1143fau);
        if(flags.zf) {
            compare(0x1143fcu,g[ebp]+current_overlay,0x0400000cu);
            pc(0x114403u);
            if(flags.zf)return true;
            pc(0x114405u);
            return false;
        }
        pc(0x114407u);flags.minus(g[edi],0x0400000cu,4);
        pc(0x11440du);
        return flags.zf;
    }
    template<class Route> void submit_transition(Route &route) {
        if(should_reset_channel(route)) {
            test_field(0x11440fu,g[ebp]+0x6eu,1);
            pc(0x114413u);
            if(flags.zf) {
                pc(0x114415u);g[eax]=g[ebp];
                call(0x114417u,0x1d4e50u,route);
            }
        }
        g[eax]=read(0x11441cu,g[ebp]+0x68u);
        pc(0x11441fu);g[esi]=flags.logic(none,4);
        pc(0x114422u);flags.minus(g[eax],g[esi],4);
        pc(0x114424u);
        if(flags.zf)return;
        compare(0x11442au,g[ebp],g[esi]);
        pc(0x11442du);
        if(flags.zf)return;
        compare(0x114433u,g[ebp]+6u,s.reg(esi,2),2);
        pc(0x114437u);
        if(flags.zf)return;
        pc(0x11443du);g[ecx]=g[ebp];
        call(0x11443fu,0x111500u,route);
        test_byte(0x114444u,eax);
        pc(0x114446u);
        if(!flags.zf)return;
        write(0x114448u,stack(0x12u),1,1);
        pc(0x11444du);
    }
    template<class Route> void refresh_attachment_animations(Route &route) {
        compare(0x11446au,g[ebp]+0x68u,g[esi]);
        pc(0x11446du);
        if(!flags.zf) {
            test_field(0x11446fu,g[ebp]+0x6cu,1);
            pc(0x114473u);
            if(flags.zf) {
                g[ecx]=read(0x114475u,g[ebp]+current_activity);
                g[edx]=read(0x114478u,g[ebp]+0x78u);
                push(0x11447bu,1);
                push(0x11447du,1);
                push(0x11447fu,g[ecx]);
                g[ecx]=read(0x114480u,g[ebp]+0x74u);
                pc(0x114483u);g[edi]=g[ebp];
                call(0x114485u,0x1d6430u,route);
                g[ebx]=read(0x11448au,stack(0x1cu));
                pc(0x11448eu);g[esi]=flags.logic(none,4);
            }
        }
        pc(0x114491u);g[ebx]=flags.plus(g[ebx],0x218u,4);
        pc(0x114497u);g[edi]=4u;
        pc(0x11449cu);
        do {
            g[eax]=read(0x1144a0u,g[ebx]);
            pc(0x1144a2u);flags.minus(g[eax],g[esi],4);
            pc(0x1144a4u);
            if(!flags.zf) {
                g[ecx]=read(0x1144a6u,stack(unit_argument));
                call(0x1144adu,0x10b160u,route);
            }
            pc(0x1144b2u);g[ebx]=flags.plus(g[ebx],4,4);
            pc(0x1144b5u);g[edi]=flags.increment(g[edi],4,true);
            pc(0x1144b6u);
        } while(!flags.zf);
    }
    template<class Route> void idle_transition(Route &route) {
        constexpr std::uint32_t tick_counter=0x56e1dcu;
        g[eax]=read(0x11444fu,tick_counter);
        g[edx]=read(0x114454u,stack(unit_argument));
        pc(0x11445bu);g[eax]=flags.increment(g[eax],4,false);
        write(0x11445cu,tick_counter,g[eax]);
        pc(0x114461u);g[eax]=flags.logic(g[eax]^g[edx],4);
        pc(0x114463u);g[esi]=flags.logic(none,4);
        test_byte(0x114466u,eax,0xfu);
        pc(0x114468u);
        if(flags.zf)refresh_attachment_animations(route);
        g[ebx]=read(0x1144b8u,stack(unit_argument));
    }

    struct ActivityCheck {
        std::uint32_t identity,no_identity,frame,no_frame,status,inactive,flags,blocked,mark;
    };
    void mark_activity(const ActivityCheck &check,std::uint32_t absent_id) {
        compare(check.identity,g[edi],absent_id);
        pc(check.no_identity);
        if(flags.zf)return;
        compare(check.frame,g[edi]+6u,absent_id,2);
        pc(check.no_frame);
        if(flags.zf)return;
        test_field(check.status,g[edi]+0x12u,1);
        pc(check.inactive);
        if(flags.zf)return;
        test_field(check.flags,g[edi]+0x11u,9);
        pc(check.blocked);
        if(flags.zf)write(check.mark,stack(0x12u),1,1);
    }
    template<class Route> void refresh_timed_slot(Route &route) {
        compare(0x1144e3u,g[ebp]+0x68u,g[esi]);
        pc(0x1144e6u);
        if(!flags.zf) {
            test_field(0x1144e8u,g[ebp]+0x6cu,1);
            pc(0x1144ecu);
            if(flags.zf) {
                push(0x1144eeu,g[ebx]);
                push(0x1144efu,0xc1630u);
                push(0x1144f4u,g[ebp]);
                pc(0x1144f5u);g[eax]=g[edi];
                call(0x1144f7u,0x1ceb70u,route);
            }
        }
        test_field(0x1144fcu,g[edi]+0x11u,0xau);
        pc(0x114500u);
        if(!flags.zf) {
            g[edx]=read(0x114502u,stack(0x24u));
            s.set(eax,read(0x114506u,g[edx],1),1);
            pc(0x114508u);s.set(eax,flags.right(s.reg(eax,1),7,1,false),1);
            test_byte(0x11450bu,eax,1);
            pc(0x11450du);
            if(!flags.zf) {
                pc(0x11450fu);g[eax]=g[edi];
                call(0x114511u,0x1ce6d0u,route);
                g[eax]=read(0x114516u,stack(0x24u));
                access(0x11451au);update_field(s,0x11451au,g[eax],1,0x7fu,0);after_access();
                pc(0x11451du);g[eax]=g[ebx];
                call(0x11451fu,0x116a10u,route);
            }
        }
        mark_activity({0x114524u,0x114526u,0x114528u,0x11452cu,0x11452eu,0x114532u,
                       0x114534u,0x114538u,0x11453au},g[esi]);
    }
    template<class Route> void update_timed_slot(Route &route) {
        g[edi]=read(0x1144c9u,stack(0x24u));
        g[eax]=read(0x1144cdu,g[edi]+0xdcu);
        pc(0x1144d3u);g[edi]=flags.plus(g[edi],0xdcu,4);
        pc(0x1144d9u);flags.minus(g[eax],g[esi],4);
        pc(0x1144dbu);
        if(flags.zf)return;
        compare(0x1144ddu,g[edi]+6u,s.reg(esi,2),2);
        pc(0x1144e1u);
        if(!flags.zf)refresh_timed_slot(route);
    }

    template<class Route> void reset_primary_slot(Route &route) {
        g[esi]=read(0x1145c3u,stack(unit_argument));
        pc(0x1145cau);g[ecx]=flags.right(g[ecx],14,4,false);
        test_byte(0x1145cdu,ecx,1);
        pc(0x1145d0u);
        if(!flags.zf) {
            g[eax]=read(0x1145d2u,g[ebp]+current_activity);
            push(0x1145d5u,g[eax]);
            pc(0x1145d6u);g[eax]=g[esi];
            call(0x1145d8u,0x11ddf0u,route);
        }
        push(0x1145ddu,0);
        push(0x1145dfu,0);
        push(0x1145e1u,g[ebx]);
        pc(0x1145e2u);g[ebx]=g[esi];
        call(0x1145e4u,0xe9050u,route);
        g[ecx]=read(0x1145e9u,stack(0x38u));
        compare(0x1145edu,g[edi]+8u,g[ecx]);
        pc(0x1145f0u);
        if(!flags.zf)return;
        pc(0x1145f2u);g[eax]=g[esi];
        call(0x1145f4u,0x1136a0u,route);
        test_byte(0x1145f9u,eax);
        pc(0x1145fbu);
        if(!flags.zf)return;
        pc(0x1145fdu);g[eax]=g[edi];
        call(0x1145ffu,0x1ce6d0u,route);
        g[edx]=read(0x114604u,stack(0x1cu));
        compare(0x114608u,g[edx]+0xaau,1,1);
        pc(0x11460fu);
        if(flags.zf)return;
        g[esi]=read(0x114611u,stack(unit_argument));
        push(0x114618u,0x3e88b439u);
        call(0x11461du,0xbc380u,route);
    }
    template<class Route> void refresh_primary_slot(Route &route) {
        compare(0x11456bu,g[ebp]+0x68u,g[esi]);
        pc(0x11456eu);
        if(!flags.zf) {
            test_field(0x114570u,g[ebp]+0x6cu,1);
            pc(0x114574u);
            if(flags.zf) {
                push(0x114576u,g[ebx]);
                push(0x114577u,0xc1630u);
                push(0x11457cu,g[ebp]);
                pc(0x11457du);g[eax]=g[edi];
                call(0x11457fu,0x1ceb70u,route);
            }
        }
        s.set(eax,read(0x114584u,g[edi]+0x14u,2),2);
        pc(0x114588u);flags.logic(s.reg(eax,2),2);
        g[ebx]=read(0x11458bu,g[edi]+8u);
        write(0x11458eu,stack(0x38u),g[ebx]);
        pc(0x114592u);
        if(!flags.zf) {
            g[ecx]=read(0x114594u,g[edi]+0x16u,2);
            g[esi]=read(0x114598u,stack(unit_argument));
            pc(0x11459fu);g[edx]=s.reg(eax,2);
            push(0x1145a2u,g[ecx]);
            push(0x1145a3u,g[edx]);
            pc(0x1145a4u);g[eax]=g[ebx];
            call(0x1145a6u,0xe8f70u,route);
        }
        test_field(0x1145abu,g[edi]+0x11u,0xeu);
        pc(0x1145afu);
        if(!flags.zf) {
            pc(0x1145b1u);g[ecx]=flags.logic(0,4);
            s.set(ecx,read(0x1145b3u,g[edi]+0x12u,2),2);
            pc(0x1145b7u);g[eax]=s.reg(ecx,2);
            pc(0x1145bau);flags.logic(s.reg(eax,1,8)&0x40u,1);
            pc(0x1145bdu);
            bool reset=flags.zf;
            if(!reset) {
                test_byte(0x1145bfu,eax,2);
                pc(0x1145c1u);
                reset=flags.zf;
            }
            if(reset)reset_primary_slot(route);
        }
        mark_activity({0x114622u,0x114625u,0x114627u,0x11462cu,0x11462eu,0x114632u,
                       0x114634u,0x114638u,0x11463au},none);
    }
    template<class Route> void update_primary_slot(Route &route) {
        g[edi]=read(0x114549u,stack(0x24u));
        g[eax]=read(0x11454du,g[edi]+0x9cu);
        pc(0x114553u);g[edi]=flags.plus(g[edi],0x9cu,4);
        pc(0x114559u);flags.minus(g[eax],g[esi],4);
        pc(0x11455bu);
        if(flags.zf)return;
        compare(0x114561u,g[edi]+6u,s.reg(esi,2),2);
        pc(0x114565u);
        if(!flags.zf)refresh_primary_slot(route);
    }

    template<class Route> void reset_secondary_slot(Route &route) {
        g[ebx]=read(0x1146b9u,stack(unit_argument));
        push(0x1146c0u,0);
        push(0x1146c2u,0);
        push(0x1146c4u,g[ebp]);
        call(0x1146c5u,0xe9050u,route);
        compare(0x1146cau,g[edi]+8u,g[ebp]);
        pc(0x1146cdu);
        if(!flags.zf)return;
        pc(0x1146cfu);g[eax]=g[ebx];
        call(0x1146d1u,0x1136a0u,route);
        test_byte(0x1146d6u,eax);
        pc(0x1146d8u);
        if(!flags.zf)return;
        pc(0x1146dau);g[eax]=g[edi];
        call(0x1146dcu,0x1ce6d0u,route);
        g[eax]=read(0x1146e1u,stack(0x1cu));
        compare(0x1146e5u,g[eax]+0xaau,1,1);
        pc(0x1146ecu);
        if(flags.zf)return;
        push(0x1146eeu,0x3e08b439u);
        pc(0x1146f3u);g[esi]=g[ebx];
        call(0x1146f5u,0xbc380u,route);
    }
    template<class Route> void refresh_secondary_slot(Route &route) {
        compare(0x114670u,g[ebp]+0x68u,g[eax]);
        pc(0x114673u);
        if(!flags.zf) {
            test_field(0x114675u,g[ebp]+0x6cu,1);
            pc(0x114679u);
            if(flags.zf) {
                g[eax]=read(0x11467bu,stack(unit_argument));
                push(0x114682u,g[eax]);
                push(0x114683u,0xc1630u);
                push(0x114688u,g[ebp]);
                pc(0x114689u);g[eax]=g[edi];
                call(0x11468bu,0x1ceb70u,route);
            }
        }
        s.set(eax,read(0x114690u,g[edi]+0x14u,2),2);
        pc(0x114694u);flags.logic(s.reg(eax,2),2);
        g[ebp]=read(0x114697u,g[edi]+8u);
        pc(0x11469au);
        if(!flags.zf) {
            g[ecx]=read(0x11469cu,g[edi]+0x16u,2);
            g[esi]=read(0x1146a0u,stack(unit_argument));
            pc(0x1146a7u);g[edx]=s.reg(eax,2);
            push(0x1146aau,g[ecx]);
            push(0x1146abu,g[edx]);
            pc(0x1146acu);g[eax]=g[ebp];
            call(0x1146aeu,0xe8f70u,route);
        }
        test_field(0x1146b3u,g[edi]+0x11u,0xeu);
        pc(0x1146b7u);
        if(!flags.zf)reset_secondary_slot(route);
        mark_activity({0x1146fau,0x1146fdu,0x1146ffu,0x114704u,0x114706u,0x11470au,
                       0x11470cu,0x114710u,0x114712u},none);
    }
    template<class Route> void update_secondary_slot(Route &route) {
        g[edi]=read(0x114649u,stack(0x24u));
        g[ecx]=read(0x11464du,g[edi]+0xbcu);
        pc(0x114653u);g[edi]=flags.plus(g[edi],0xbcu,4);
        pc(0x114659u);g[eax]=none;
        pc(0x11465eu);flags.minus(g[ecx],g[eax],4);
        pc(0x114660u);
        if(flags.zf)return;
        compare(0x114666u,g[edi]+6u,s.reg(eax,2),2);
        pc(0x11466au);
        if(!flags.zf)refresh_secondary_slot(route);
    }
    template<class Route> void update_pose_slots(Route &route) {
        test_field(0x1144bfu,g[ebp]+0x6cu,1);
        pc(0x1144c3u);
        if(!flags.zf)return;
        update_timed_slot(route);
        test_field(0x11453fu,g[ebp]+0x6cu,1);
        pc(0x114543u);
        if(!flags.zf)return;
        update_primary_slot(route);
        test_field(0x11463fu,g[ebp]+0x6cu,1);
        pc(0x114643u);
        if(flags.zf)update_secondary_slot(route);
    }
    template<class Route> void close(Route &route) {
        g[esi]=read(0x114717u,stack(unit_argument));
        pc(0x11471eu);g[eax]=g[esi];
        call(0x114720u,0x110350u,route);
        s.set(ebx,read(0x114725u,stack(0x12u),1),1);
        pc(0x114729u);s.set(ebx,flags.logic(s.reg(ebx,1)|s.reg(eax,1),1),1);
        pc(0x11472bu);g[eax]=g[esi];
        call(0x11472du,0x1111d0u,route);
        g[edi]=pop(0x114732u);
        g[esi]=pop(0x114733u);
        g[ebp]=pop(0x114734u);
        pc(0x114735u);s.set(eax,flags.logic(s.reg(eax,1)|s.reg(ebx,1),1),1);
        g[ebx]=pop(0x114737u);
        pc(0x114738u);g[esp]=flags.plus(g[esp],frame_size,4);
        (void)pop(exit);
        g[esp]+=8u;
    }

    template<class Route> void run(Route &route) {
        auto child=[&](std::uint32_t target,std::uint32_t site) {
            return [&,target,site](State &state){route(state,target,site);};
        };
        auto query=child(0x115ff0u,0x113db2u);
        auto resolve=child(0x1115d0u,0x113e68u);
        auto graph=child(0x1d39c0u,0x113edeu);
        auto clock=child(0xe8f70u,0x113f0au);
        auto info=child(0xb8780u,0x113f2fu);
        auto select=child(0xe9050u,0x113facu);
        prepare(query,resolve);
        const auto activity=select_activity(graph,clock,info,select);
        if(activity!=ActivityRoute::missing_channel&&activity!=ActivityRoute::check_channel) {
            switch(activity) {
                case ActivityRoute::special_activity: use_special_activity(route);break;
                case ActivityRoute::low_variant: use_low_variant();break;
                case ActivityRoute::high_activity: use_high_activity(route);break;
                case ActivityRoute::high_variant:
                    pc(0x114183u);flags.minus(g[esi],0x090006b2u,4);
                    pc(0x114189u);
                    if(!flags.zf)break;
                    [[fallthrough]];
                case ActivityRoute::standard_activity: use_standard_activity(route);break;
                case ActivityRoute::apply_activity: break;
                case ActivityRoute::check_channel:
                case ActivityRoute::missing_channel: break;
            }
            apply_activity(route);
        }
        check_channel(activity!=ActivityRoute::missing_channel);
        if(needs_transition(route))submit_transition(route);
        else idle_transition(route);
        update_pose_slots(route);
        close(route);
    }
};

inline bool enabled() {
    static const bool retained=native_small_queries_detail::flag("HALO2_REFERENCE_ANIMATION_REQUEST");
    return !retained;
}

template<class State,class Action>
auto execute(State &state,Action action) {
    std::uint32_t *position;
    if(native_object_tick_detail::admitted(state,entry,exit,position,false) &&
       (state.immutable_cursor.cursor||native_census_detail::uncursored(state,entry,exit))) {
        Frame<State,false> frame{state,position};
        return action(frame);
    }
    Frame<State,true> frame{state,nullptr};
    return action(frame);
}

template<class State,class Route>
bool bridge(State &state,Route route) {
    if(!enabled()||state.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    execute(state,[&](auto &frame){frame.run(route);});
    return true;
}
}
