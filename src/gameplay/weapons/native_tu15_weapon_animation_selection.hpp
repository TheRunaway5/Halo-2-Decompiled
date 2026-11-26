#pragma once
#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "physics/runtime/native_tu15_two_bone_solve.hpp"

namespace halo2::native_weapon_animation_selection {
using namespace native_object_tick_detail::boundary;
constexpr std::uint32_t entry=0x1e3420u,last=0x1e3f26u;
constexpr std::uint32_t absent=0xffffffffu, default_symbol=0x030000d9u;
constexpr std::uint32_t graph_reference=0x34u, graph_records=0xb0u;
constexpr std::uint32_t selection_count=0x24u, selection_indices=0x28u, record_enabled=0x10u;
constexpr std::uint32_t override_count=0x4cu, override_records=0x50u, override_stride=0x20u;

enum class Root {first_graph,saved_reference,overlay_with_graph,saved_overlay,default_overlay};
struct StackOperand {
    unsigned offset{};
    Register destination{};
};
struct Selection {
    Root root;
    StackOperand root_operand;
    bool default_root;
    StackOperand second_symbol,third_symbol;
    bool compare_with_ebp;
    Register index_table;
    bool alignment_before_loop,alignment_after_loop;
    // Low words of the original sites, solely for memory faults and tracing.
    // Each row shares the same lookup, rejection and enumeration algorithm.
    std::array<std::uint16_t,45> sites;
};

#include "gameplay/weapons/native_tu15_weapon_animation_selection_sites.inc"

template<class State,bool Observed> struct Frame {
    State &s;
    std::uint32_t *cursor;
    decltype(State::gp) &g;
    decltype(State::flags) &flags;
    bool resumed_trace{};

    Frame(State &state,std::uint32_t *position):s(state),cursor(position),g(s.gp),flags(s.flags){}
    void pc(std::uint32_t site) {
        if constexpr(Observed)s.instruction_literal(site);
        else if(resumed_trace)s.instruction_literal(site);
    }
    void after_access() {
        if constexpr(!Observed) {
            if(!cursor)return;
            const auto *policy=s.immutable_cursor.cursor;
            if((policy->first_trace&&*policy->first_trace)||(policy->second_trace&&*policy->second_trace)) {
                cursor=nullptr;
                resumed_trace=true;
            }
        }
    }
    void access(std::uint32_t site) {
        pc(site);
        if(cursor)*cursor=site;
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
    void test(std::uint32_t value,unsigned size=4){flags.logic(value,size);}
    bool signed_less()const{return flags.sf!=flags.of;}

    // Construct the query and preserve the absent-id path's smaller frame.
    bool open() {
        pc(entry);g[esp]=flags.minus(g[esp],0x2cu,4);
        g[edx]=read(0x1e3423u,stack(0x3cu));
        push(0x1e3427u,g[ebp]);
        g[ebp]=read(0x1e3428u,stack(0x48u));
        pc(0x1e342cu);g[ecx]=flags.logic(absent,4);
        pc(0x1e342fu);flags.minus(g[edx],g[ecx],4);
        push(0x1e3431u,g[edi]);
        pc(0x1e3432u);g[edi]=g[eax];
        g[eax]=read(0x1e3434u,stack(0x3cu));
        write(0x1e3438u,stack(0x14u),g[ecx],2);
        write(0x1e343du,stack(0x16u),g[ecx],2);
        pc(0x1e3442u);
        if(flags.zf)return false;
        g[ecx]=read(0x1e3448u,stack(0x40u));
        pc(0x1e344cu);g[eax]=flags.logic(absent,4);
        write(0x1e344fu,stack(8u),g[eax]);
        write(0x1e3453u,stack(0xcu),g[eax]);
        write(0x1e3457u,stack(0x10u),g[eax]);
        g[eax]=read(0x1e345bu,stack(0x38u));
        write(0x1e345fu,stack(0x18u),g[eax]);
        pc(0x1e3463u);g[eax]=flags.logic(0,4);
        write(0x1e3465u,stack(0x28u),g[eax]);
        write(0x1e3469u,stack(0x2cu),g[eax]);
        write(0x1e346du,stack(0x30u),g[eax]);
        g[eax]=read(0x1e3471u,stack(0x48u));
        push(0x1e3475u,g[ebx]);
        push(0x1e3476u,g[esi]);
        write(0x1e3477u,stack(0x24u),g[ecx]);
        write(0x1e347bu,stack(0x28u),g[edx]);
        write(0x1e347fu,stack(0x2cu),g[eax]);
        return true;
    }

    template<class Identify,class Query>
    void find_animation(Identify &identify,Query &query) {
        for(;;) {
            pc(0x1e3483u);g[ecx]=stack(0x14u);
            pc(0x1e3487u);g[edx]=stack(0x10u);
            push(0x1e348bu,g[ecx]);
            pc(0x1e348cu);g[esi]=stack(0x24u);
            pc(0x1e3490u);g[ebx]=stack(0x1cu);
            push(0x1e3494u,g[edx]);
            call(0x1e3495u,identify);
            pc(0x1e349au);g[esi]=flags.logic(absent,4);
            pc(0x1e349du);test(g[eax]);
            pc(0x1e349fu);
            if(flags.zf) {
                g[ecx]=read(0x1e34d6u,stack(0x1cu));
                return;
            }
            push(0x1e34a1u,8);
            access(0x1e34a3u);
            const auto symbol=source_weapon_animation_scoped_query_symbol_tu15(s.memory,g[edi]);
            native_two_bone_solve::push_word(s,0x1e34a3u,symbol);
            after_access();
            pc(0x1e34a4u);g[eax]=flags.plus(g[eax],4,4);
            push(0x1e34a7u,g[eax]);
            write(0x1e34a8u,stack(0x60u),g[esi],2);
            write(0x1e34adu,stack(0x62u),g[esi],2);
            call(0x1e34b2u,query);
            pc(0x1e34b7u);test(g[eax]);
            pc(0x1e34b9u);
            if(!flags.zf) {
                g[eax]=read(0x1e34bbu,g[eax]+4u);
                write(0x1e34beu,stack(0x54u),g[eax]);
            }
            g[ecx]=read(0x1e34c2u,stack(0x54u));
            pc(0x1e34c6u);g[eax]=g[ecx];
            pc(0x1e34c8u);g[eax]=flags.right(g[eax],16,4,false);
            pc(0x1e34cbu);flags.minus(s.reg(eax,2),s.reg(esi,2),2);
            write(0x1e34ceu,stack(0x1cu),g[ecx]);
            pc(0x1e34d2u);
            if(!flags.zf)return;
            pc(0x1e34d4u);
        }
    }

    bool publish_selection_inputs() {
        pc(0x1e34dau);test(g[ebp]);
        pc(0x1e34dcu);
        if(!flags.zf) {
            g[eax]=read(0x1e34deu,stack(0x10u));
            write(0x1e34e2u,g[ebp],g[eax]);
        }
        g[eax]=read(0x1e34e5u,stack(0x58u));
        pc(0x1e34e9u);test(g[eax]);
        pc(0x1e34ebu);
        if(!flags.zf) {
            g[edx]=read(0x1e34edu,stack(0x14u));
            write(0x1e34f1u,g[eax],g[edx]);
        }
        g[eax]=read(0x1e34f3u,stack(0x5cu));
        pc(0x1e34f7u);test(g[eax]);
        pc(0x1e34f9u);
        if(!flags.zf) {
            g[edx]=read(0x1e34fbu,stack(0x18u));
            write(0x1e34ffu,g[eax],g[edx]);
        }
        access(0x1e3501u);
        const auto tag=physics_step_detail::r(s.memory,stack(0x1eu),2);
        flags.minus(tag,s.reg(esi,2),2);
        after_access();
        pc(0x1e3506u);
        return !flags.zf;
    }

    // Move through a selection's source positions in the order dictated by
    // the native lookups. No source position is needed on a skipped branch.
    struct Sites {
        const std::uint16_t *position;
        std::uint32_t next(){return 0x1e0000u+*position++;}
    };
    void load_stack(Sites &sites,StackOperand operand) {
        g[operand.destination]=read(sites.next(),stack(operand.offset));
    }
    void compare_lookup(Sites &sites,bool compare_with_ebp,Register reg) {
        pc(sites.next());
        if(compare_with_ebp)flags.minus(g[reg],g[ebp],4);
        else test(g[reg]);
        pc(sites.next());
    }
    template<class Query> bool query_root(Sites &sites,const Selection &selection,Query &query) {
        const auto operand=selection.root_operand;
        if(operand.offset)load_stack(sites,operand);
        push(sites.next(),0x14u);
        switch(selection.root) {
            case Root::first_graph:
                pc(sites.next());g[eax]=g[ebx]+graph_reference;
                push(sites.next(),g[ebp]);
                push(sites.next(),g[eax]);
                write(sites.next(),stack(0x60u),g[eax]);
                break;
            case Root::saved_reference:
                push(sites.next(),selection.default_root?default_symbol:g[ebp]);
                push(sites.next(),g[operand.destination]);
                break;
            case Root::overlay_with_graph:
                push(sites.next(),g[operand.destination]);
                pc(sites.next());g[ebp]=g[ebx]+graph_reference;
                push(sites.next(),g[ebp]);
                break;
            case Root::saved_overlay:
                push(sites.next(),g[operand.destination]);
                push(sites.next(),g[ebp]);
                break;
            case Root::default_overlay:
                push(sites.next(),default_symbol);
                push(sites.next(),g[ebp]);
                break;
        }
        call(sites.next(),query);
        if(selection.compare_with_ebp) {
            pc(sites.next());g[ebp]=flags.logic(0,4);
        }
        compare_lookup(sites,selection.compare_with_ebp,eax);
        return !flags.zf;
    }
    template<class Query>
    void query_child(Sites &sites,StackOperand symbol,unsigned stride,Query &query) {
        if(symbol.offset)load_stack(sites,symbol);
        push(sites.next(),stride);
        push(sites.next(),symbol.offset?g[symbol.destination]:default_symbol);
        pc(sites.next());g[eax]=flags.plus(g[eax],4,4);
        push(sites.next(),g[eax]);
        call(sites.next(),query);
    }

    void load_animation(Sites &sites,Register table) {
        g[table]=read(sites.next(),g[esi]+selection_indices);
        g[eax]=read(sites.next(),g[table]+g[edi]*4u);
        if(table==eax) {
            g[edx]=read(sites.next(),g[ebx]+graph_records);
            pc(sites.next());g[ecx]=g[eax]*5u;
            pc(sites.next());g[eax]=g[edx]+g[ecx]*4u;
        } else if(table==edx) {
            g[ecx]=read(sites.next(),g[ebx]+graph_records);
            pc(sites.next());g[eax]*=5u;
            pc(sites.next());g[eax]=g[ecx]+g[eax]*4u;
        } else {
            pc(sites.next());g[edx]=g[eax]*5u;
            g[eax]=read(sites.next(),g[ebx]+graph_records);
        }
        const auto flags_address=g[eax]+record_enabled+(table==ecx?g[edx]*4u:0u);
        s.set(ecx,read(sites.next(),flags_address,1),1);
        pc(sites.next());test(s.reg(ecx,1),1);
        if(table==ecx) {
            pc(sites.next());g[eax]+=g[edx]*4u;
        }
        pc(sites.next());
    }
    template<class Query,class RegisterAnimation>
    void enumerate(const Selection &selection,Query &query,RegisterAnimation &register_animation) {
        Sites sites{selection.sites.data()};
        if(!query_root(sites,selection,query))return;
        query_child(sites,selection.second_symbol,0x14u,query);
        compare_lookup(sites,selection.compare_with_ebp,eax);
        if(flags.zf)return;
        query_child(sites,selection.third_symbol,0x34u,query);
        pc(sites.next());g[esi]=g[eax];
        compare_lookup(sites,selection.compare_with_ebp,esi);
        if(flags.zf)return;

        g[eax]=read(sites.next(),g[esi]+selection_count);
        pc(sites.next());g[edi]=flags.logic(0,4);
        compare_lookup(sites,selection.compare_with_ebp,eax);
        if(flags.zf||signed_less())return;
        if(selection.alignment_before_loop)pc(sites.next());
        const auto first=sites;
        do {
            sites=first;
            load_animation(sites,selection.index_table);
            const auto set_flag=sites.next();
            const auto call_site=sites.next();
            if(!flags.zf) {
                pc(set_flag);s.set(ecx,1,1);
                call(call_site,register_animation);
            }
            g[eax]=read(sites.next(),g[esi]+selection_count);
            pc(sites.next());g[edi]=flags.increment(g[edi],4,false);
            pc(sites.next());flags.minus(g[edi],g[eax],4);
            pc(sites.next());
        } while(signed_less());
        if(selection.alignment_after_loop)pc(sites.next());
    }

    // The two override passes use different frame slots and share the same
    // datum-index and graph reload. Retain each original observable access.
    struct Overrides {
        unsigned index_slot,displacement_slot;
        std::array<std::uint16_t,19> open_sites;
        std::array<std::uint16_t,10> advance_sites;
    };
    static constexpr Overrides symbolic_overrides{
        0x58,0x5c,
        {0x3760,0x3763,0x3765,0x3769,0x376f,0x3773,0x3777,0x3779,0x377b,0x377d,0x3781,0x3784,0x3788,0x378b,
         0x378e,0x3790,0x3796,0x379b,0x379e},
        {0x39e0,0x39e4,0x39e8,0x39ec,0x39ef,0x39f0,0x39f3,0x39f5,0x39f9,0x39fd}
    };
    static constexpr Overrides selected_overrides{
        0x4c,0x48,
        {0x3c50,0x3c53,0x3c55,0x3c59,0x3c5f,0x3c63,0x3c67,0x3c69,0x3c6b,0x3c6d,0x3c71,0x3c74,0x3c78,0x3c7b,
         0x3c7e,0x3c80,0x3c86,0x3c8b,0x3c8e},
        {0x3ed0,0x3ed4,0x3ed8,0x3edc,0x3edf,0x3ee0,0x3ee3,0x3ee5,0x3ee9,0x3eed}
    };
    bool open_overrides(Sites &sites,const Overrides &overrides) {
        g[eax]=read(sites.next(),g[ebx]+override_count);
        pc(sites.next());flags.minus(g[eax],g[ebp],4);
        write(sites.next(),stack(overrides.index_slot),g[ebp]);
        pc(sites.next());
        if(flags.zf||signed_less())return false;
        write(sites.next(),stack(overrides.displacement_slot),g[ebp]);
        return true;
    }
    void load_override(Sites sites,const Overrides &overrides) {
        g[ecx]=read(sites.next(),stack(overrides.index_slot));
        pc(sites.next());g[ebx]=flags.logic(0,4);
        pc(sites.next());flags.minus(g[ecx],g[eax],4);
        pc(sites.next());
        if(!signed_less())return;
        g[ecx]=read(sites.next(),stack(0x40u));
        g[eax]=read(sites.next(),g[ecx]+override_records);
        access(sites.next());
        const auto displacement=physics_step_detail::r(s.memory,stack(overrides.displacement_slot),4);
        g[eax]=flags.plus(g[eax],displacement,4);
        after_access();
        g[eax]=read(sites.next(),g[eax]+4u);
        pc(sites.next());flags.minus(g[eax],absent,4);
        pc(sites.next());
        if(flags.zf)return;
        g[edx]=read(sites.next(),0x4f0414u);
        pc(sites.next());g[eax]=flags.logic(g[eax]&0xffffu,4);
        pc(sites.next());g[eax]=flags.shift(g[eax],4,4);
        g[ebx]=read(sites.next(),g[eax]+g[edx]+8u);
    }
    bool advance_override(const Overrides &overrides) {
        Sites sites{overrides.advance_sites.data()};
        g[ecx]=read(sites.next(),stack(overrides.index_slot));
        g[esi]=read(sites.next(),stack(overrides.displacement_slot));
        g[eax]=read(sites.next(),stack(0x40u));
        g[eax]=read(sites.next(),g[eax]+override_count);
        pc(sites.next());g[ecx]=flags.increment(g[ecx],4,false);
        pc(sites.next());g[esi]=flags.plus(g[esi],override_stride,4);
        pc(sites.next());flags.minus(g[ecx],g[eax],4);
        write(sites.next(),stack(overrides.index_slot),g[ecx]);
        write(sites.next(),stack(overrides.displacement_slot),g[esi]);
        pc(sites.next());
        return signed_less();
    }
    template<class Query,class RegisterAnimation>
    bool enumerate_overrides(const Overrides &overrides,std::size_t first,Query &query,RegisterAnimation &register_animation) {
        Sites sites{overrides.open_sites.data()};
        if(!open_overrides(sites,overrides))return false;
        do {
            load_override(sites,overrides);
            for(std::size_t n=first;n<first+5u;++n)enumerate(selections[n],query,register_animation);
        } while(advance_override(overrides));
        return true;
    }

    void close_normal() {
        g[eax]=read(0x1e3ef7u,stack(0x44u));
        g[esi]=pop(0x1e3efbu);
        g[ebx]=pop(0x1e3efcu);
        g[edi]=pop(0x1e3efdu);
        write(0x1e3efeu,g[eax],g[ecx]);
        g[ebp]=pop(0x1e3f00u);
        pc(0x1e3f01u);g[esp]=flags.plus(g[esp],0x2cu,4);
        (void)pop(0x1e3f04u);
        g[esp]+=32u;
    }
    void close_without_overrides() {
        g[eax]=read(0x1e3f07u,stack(0x44u));
        g[edx]=read(0x1e3f0bu,stack(0x1cu));
        g[esi]=pop(0x1e3f0fu);
        g[ebx]=pop(0x1e3f10u);
        g[edi]=pop(0x1e3f11u);
        write(0x1e3f12u,g[eax],g[edx]);
        g[ebp]=pop(0x1e3f14u);
        pc(0x1e3f15u);g[esp]=flags.plus(g[esp],0x2cu,4);
        (void)pop(0x1e3f18u);
        g[esp]+=32u;
    }
    void close_absent() {
        g[ecx]=read(0x1e3f1bu,stack(0x14u));
        g[edi]=pop(0x1e3f1fu);
        write(0x1e3f20u,g[eax],g[ecx]);
        g[ebp]=pop(0x1e3f22u);
        pc(0x1e3f23u);g[esp]=flags.plus(g[esp],0x2cu,4);
        (void)pop(last);
        g[esp]+=32u;
    }

    template<class Identify,class Query,class LoadGraph,class RegisterAnimation>
    void select(Identify &identify,Query &query,LoadGraph &load_graph,RegisterAnimation &register_animation) {
        if(!open()) {
            close_absent();
            return;
        }
        find_animation(identify,query);
        if(publish_selection_inputs()) {
            g[ebx]=read(0x1e350cu,stack(0x40u));
            push(0x1e3510u,g[ecx]);
            pc(0x1e3511u);g[edx]=g[ebx];
            call(0x1e3513u,load_graph);
            for(std::size_t n=0;n<5u;++n)enumerate(selections[n],query,register_animation);
            if(enumerate_overrides(symbolic_overrides,5u,query,register_animation))
                g[ebx]=read(0x1e3a03u,stack(0x40u));
            g[ebp]=read(0x1e3a07u,stack(0x10u));
            for(std::size_t n=10u;n<15u;++n)enumerate(selections[n],query,register_animation);
            if(!enumerate_overrides(selected_overrides,15u,query,register_animation)) {
                close_without_overrides();
                return;
            }
            g[ecx]=read(0x1e3ef3u,stack(0x1cu));
        }
        close_normal();
    }
};

template<class State,class Identify,class Query,class LoadGraph,class RegisterAnimation>
bool bridge(State &s,Identify identify,Query query,LoadGraph load_graph,RegisterAnimation register_animation) {
    static const bool reference=native_small_queries_detail::flag("HALO2_REFERENCE_WEAPON_ANIMATION_SELECTION");
    if(reference||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    std::uint32_t *cursor;
    if(native_object_tick_detail::admitted(s,entry,last,cursor,false) &&
       (s.immutable_cursor.cursor||native_census_detail::uncursored(s,entry,last))) {
        Frame<State,false>{s,cursor}.select(identify,query,load_graph,register_animation);
    } else {
        Frame<State,true>{s,nullptr}.select(identify,query,load_graph,register_animation);
    }
    return true;
}
}
