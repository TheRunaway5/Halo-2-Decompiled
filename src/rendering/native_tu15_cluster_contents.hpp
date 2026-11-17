#pragma once
// Boundary adapter for the cluster contents system
// (native_tu15_cluster_contents_systems.hpp): 134D30, called by 134C00 with
// the view as its one stack argument (stdcall, RET 4). Its caller and most of
// its children are still translated TU1.5 code or separately proven natives,
// so this adapter owns that ABI and nothing else:
//   - the 0x24-byte frame with the four saved registers (EBX, EBP, ESI, EDI),
//     its slots (the list position, the entry offset, the descriptor, the sky
//     cluster word, the fog byte or listed count, the four listed lights), the
//     argument slot and every push, pop and return address;
//   - every register as the original leaves it at each access, child call and
//     exit: each access role's register effects, the list walks' address
//     arithmetic (IMUL/ADD), partial-register writes (AL, CL, AX, SI) and the
//     flags of each instruction, with hardware flags where an instruction's
//     undefined flags reach the next access (IMUL ECX,ECX,110 at 1350D4 and
//     SHL ESI,2 at 135149);
//   - the values the logic keeps across a call (Kept), loaded into their
//     callee-saved registers before it and read back after it;
//   - the diagnostic cursor (each access publishes its original instruction)
//     and the store stream (every store goes through the provider and the
//     store observer).
// The original executes no x87, MMX or SSE instruction, so there is no FP
// scope: the children open their own.
//
// Children stay genuine and are entered through the unit's own route (the
// Call supplied by the generated owner): 171070 and 135B60 (each with its own
// native bridge), and the external owners 135540, C5980, C5170 and C5140,
// which the unit's dispatcher hands to the State's required boundary. The
// queue append 166B60 is a pure leaf and runs inline (its frame, arguments
// and RET 10 included). PREFETCHT0 performs no architectural access and is
// not replayed.
//
// Admission: TU1.5 image, no MMX alias, the exact live instruction-cursor
// contract, and a certified required owner (the native parent skips the
// remaining instruction callbacks after a required owner returns, so that
// owner must preserve the caller's callback, cursor and trace policy).
// HALO2_REFERENCE_CLUSTER_CONTENTS=1 restores the translated body.
// HALO2_CENSUS_NATIVE_BRIDGES=1 also admits the uncursored census callbacks
// (the closure, 134D30 and 166B60, holds no service PC).
#include "rendering/native_tu15_cluster_contents_systems.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <cstdlib>
#include <optional>
#include <thread>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#endif

namespace halo2::native_cluster_contents_detail {
namespace logic=halo2::native_cluster_contents;
using logic::Access;
using logic::Walk;
using logic::LinkField;
using logic::Kept;

struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_CLUSTER_CONTENTS");return p&&*p;}();return value;}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;
    if(native_census_detail::uncursored(s,0x134d30u,0x166bafu)){
    }else if(s.services.instruction){
        const auto*c=s.instruction_cursor;
        if(!s.literal_service_sites||!c||c!=&s.services.instruction_cursor||!c->position||!c->callback_type||
           s.services.instruction.target_type()!=*c->callback_type||c->executor!=std::this_thread::get_id()||
           !c->literal_service_sites||c->service_sites.size()!=3||c->service_sites[0]!=0x38846eu||
           c->service_sites[1]!=0x21f590u||c->service_sites[2]!=0x33b595u||
           (s.observe&&!c->observation_only_stores)||(c->first_trace&&*c->first_trace)||
           (c->second_trace&&*c->second_trace))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;
    return true;
#else
    (void)s;return false;
#endif
}
template<class State>bool admitted(const State&s){return !reference()&&policy_admitted(s);}

// The genuine children, by entry, with the call site each is entered from.
inline constexpr std::uint32_t fog_membership_entry=0x171070u;
inline constexpr std::uint32_t object_admission_entry=0x135b60u;
inline constexpr std::uint32_t light_admission_entry=0x135540u;
inline constexpr std::uint32_t list_lights_entry=0xc5980u;
inline constexpr std::uint32_t light_active_entry=0xc5170u;
inline constexpr std::uint32_t mark_queued_entry=0xc5140u;

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// Flags seeded from the source (SAHF for SF/ZF/AF/PF/CF, the ADD for OF) in
// the same assembly block as the instruction, so the result carries what the
// CPU leaves in the flags the manual calls undefined.
struct Hardware {std::uint32_t value,flags;};
// IMUL ECX,ECX,110 (1350D4): a light datum's offset.
inline Hardware light_datum_offset(std::uint32_t index,std::uint32_t flags){
    auto status=((flags&0xd5u)<<8u)|((flags>>11u)&1u);unsigned overflow;
    asm volatile("addb $127,%%al;sahf;imull $0x110,%2,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(status),"=c"(overflow),"+r"(index)::"cc");
    return{index,(status&0xd5u)|((overflow&1u)<<11u)};
}
// SHL ESI,2 (135149): the sky cluster's head offset.
inline Hardware sky_head_offset(std::uint32_t cluster,std::uint32_t flags){
    auto status=((flags&0xd5u)<<8u)|((flags>>11u)&1u);unsigned overflow;
    asm volatile("addb $127,%%al;sahf;shll $2,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(status),"=c"(overflow),"+r"(cluster)::"cc");
    return{cluster,(status&0xd5u)|((overflow&1u)<<11u)};
}

// The ten list walks: the original instruction of each access, and the
// register that receives the owner. The two collideable walks of step 2 keep
// their pool in ESI (no pool read).
struct WalkSite {std::uint32_t pool,size,data,next,owner;unsigned owner_register;bool takes_eax_after;};
enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
inline constexpr WalkSite walk_sites[]{
    {0u,0x134da8u,0x134dabu,0x134dbbu,0x134dd3u,ecx,false},          // collideable_first
    {0u,0x134e3au,0x134e3du,0x134e4du,0x134e65u,ecx,false},          // collideable_next
    {0x134e94u,0x134e99u,0x134e9cu,0x134eacu,0x134ec4u,eax,false},   // other_first
    {0x134ee8u,0x134eefu,0x134ef2u,0x134f02u,0x134f1au,ebx,true},    // other_next
    {0x134f68u,0x134f6du,0x134f70u,0x134f80u,0x134f98u,ebx,false},   // light_first
    {0x134fbdu,0x134fc2u,0x134fc5u,0x134fd5u,0x134fedu,ebx,false},   // light_next
    {0x135154u,0x135159u,0x13515cu,0x13516cu,0x135184u,eax,false},   // sky_collideable_first
    {0x1351a4u,0x1351a9u,0x1351acu,0x1351bcu,0x1351d4u,ebx,true},    // sky_collideable_next
    {0x1351f0u,0x1351f5u,0x1351f8u,0x135208u,0x135220u,eax,false},   // sky_other_first
    {0x135243u,0x135248u,0x13524bu,0x13525bu,0x135273u,ebx,true},    // sky_other_next
};
static_assert(std::size(walk_sites)==unsigned(Walk::count));

template<class State,class Call,class Before>class Adapter:public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::push;using Base::pop;
    Call&call_;
    // The list walk whose end the original has not yet tested, and whether
    // its owner was read (otherwise OR reg,-1 ends it).
    std::optional<Walk> pending_;bool walked_{};
    std::uint32_t&r(unsigned n){return s_.gp[n];}
    std::uint32_t esp_(){return s_.gp[esp];}
    void byte(unsigned n,std::uint32_t value){s_.set(n,value,1,0);}
    void half(unsigned n,std::uint32_t value){s_.set(n,value,2,0);}
    static std::uint32_t widen(std::uint32_t value){return logic::widen16(value);}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        io([&]{s_.memory.write(address,std::span(bytes).first(width));});
        if(s_.observe){event.written.assign(bytes.begin(),bytes.begin()+width);io([&]{s_.observe(event);});}
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width){at(pc);return word(address,width);}
    void call(std::uint32_t site,std::uint32_t target){push(site,site+5u);call_(s_,target,site,before_);}

    // ---- list walks
    void begin_walk(Walk walk){pending_=walk;walked_=false;}
    // The end test after a walk step: OR reg,-1 when the list was empty,
    // CMP reg,-1, and for the "next" walks of steps 2 and 6 MOV EAX,EBX.
    void finish_walk(){
        if(!pending_)return;
        const auto&site=walk_sites[unsigned(*pending_)];
        if(!walked_)r(site.owner_register)=s_.flags.logic(r(site.owner_register)|0xffffffffu,4);
        s_.flags.minus(r(site.owner_register),0xffffffffu,4);
        if(site.takes_eax_after)r(eax)=r(ebx);
        pending_.reset();
    }
    std::uint32_t walk_pc(Access a){
        const auto n=unsigned(a)-unsigned(Access::walk_base);const auto&site=walk_sites[n/5u];
        switch(LinkField(n%5u)){
        case LinkField::pool:return site.pool;
        case LinkField::size:return site.size;
        case LinkField::data:return site.data;
        case LinkField::next:return site.next;
        case LinkField::owner:return site.owner;
        }
        return 0;
    }
    void walk_loaded(Access a,std::uint32_t v){
        const auto n=unsigned(a)-unsigned(Access::walk_base);const auto&site=walk_sites[n/5u];
        switch(LinkField(n%5u)){
        case LinkField::pool:r(eax)=v;break;
        case LinkField::size:r(ecx)=v;break;
        // AND EDI,FFFF; IMUL EDI,ECX; ADD EDI,EDX; MOV EBX,EDI (IMUL's flags
        // are replaced by the ADD's).
        case LinkField::data:r(edx)=v;r(edi)=s_.flags.plus((r(edi)&0xffffu)*r(ecx),r(edx),4);r(ebx)=r(edi);break;
        // CMP EDI,-1; and for a next entry MOV EAX,EDI; AND; IMUL; ADD (the
        // PREFETCHT0 operand).
        case LinkField::next:
            r(edi)=v;s_.flags.minus(v,0xffffffffu,4);
            if(v!=0xffffffffu)r(eax)=s_.flags.plus((v&0xffffu)*r(ecx),r(edx),4);
            break;
        case LinkField::owner:r(site.owner_register)=v;walked_=true;break;
        }
    }

    // ---- the original instruction of each role, with the register work
    // between the previous access and it
    std::uint32_t before(Access a){
        using enum Access;
        if(unsigned(a)>=unsigned(walk_base))return walk_pc(a);
        switch(a){
        case entry_view:
            r(esp)=s_.flags.minus(r(esp),0x24u,4);push(0x134d33u,r(ebx));push(0x134d34u,r(ebp));return 0x134d35u;
        case entry_descriptor:return 0x134d39u;
        case visible_count:return 0x134d3eu;
        case sky_none:push(0x134d45u,r(esi));push(0x134d46u,r(edi));return 0x134d47u;
        case first_position:return 0x134d4fu;
        case first_descriptor:return 0x134d53u;
        case first_offset:return 0x134d5du;
        case loop_descriptor:return 0x134d63u;
        case loop_offset:return 0x134d67u;
        case visible_cluster:return 0x134d6bu;
        case current_fog:return 0x134d73u;
        case collideable_pool:return 0x134d79u;
        case fog_view:return 0x134d86u;
        case inside_fog:return 0x134d8au;
        case object_skip:return 0x134d8eu;
        case collideable_heads:return 0x134d9bu;
        case collideable_head:return 0x134da0u;
        case inside_fog_word:finish_walk();return 0x134ddeu;
        case filter_view:finish_walk();return 0x134df0u;
        case filter_flags:return 0x134df4u;
        case object_headers:return 0x134dffu;
        case object_header_data:return 0x134e05u;
        case object_data:return 0x134e12u;
        case object_type:return 0x134e16u;
        case admit_position:return 0x134e1fu;
        case collideable_pool_again:return 0x134e2fu;
        case other_offset:finish_walk();return 0x134e76u;
        case other_descriptor:return 0x134e7au;
        case other_cluster:return 0x134e7eu;
        case other_heads:return 0x134e86u;
        case other_head:return 0x134e8cu;
        case other_view:finish_walk();return 0x134ed1u;
        case other_position:finish_walk();return 0x134ed5u;
        case light_view:finish_walk();return 0x134f29u;
        case light_kind:return 0x134f2du;
        case light_skip:r(ecx)=r(eax);return 0x134f3du;
        case light_offset:return 0x134f4au;
        case light_descriptor:return 0x134f4eu;
        case light_cluster:return 0x134f52u;
        case light_heads:return 0x134f5au;
        case light_head:return 0x134f60u;
        case light_walk_view:finish_walk();return 0x134fa5u;
        case light_position:return 0x134fa9u;
        case sky_known:finish_walk();r(eax)=s_.flags.logic(r(eax)|0xffffffffu,4);return 0x134ffdu;
        case sky_view:return 0x135004u;
        case sky_kind:return 0x135008u;
        case sky_visible:return 0x135012u;
        case sky_index:return 0x13501cu;
        case sky_selection:return 0x135026u;
        case structure_bsp:return 0x13502fu;
        case sky_owner_count:return 0x135034u;
        case sky_owners:return 0x13503eu;
        case sky_owner:return 0x135044u;
        case sky_store:return 0x135048u;
        case next_offset:return 0x13504du;
        case next_view:return 0x135051u;
        case next_descriptor:return 0x135055u;
        case next_position:return 0x135057u;
        case offset_store:return 0x13505eu;
        case next_count:return 0x135062u;
        case position_store:return 0x13506cu;
        case descriptor_store:return 0x135070u;
        case after_view:return 0x13507au;
        case direct_kind:return 0x13507eu;
        case direct_skip:return 0x13508cu;
        case direct_owner:return 0x135099u;
        // PUSH 4; LEA EAX,[ESP+28]; PUSH EAX (C5980's size and buffer).
        case direct_source:push(0x1350a4u,4u);r(eax)=r(esp)+0x28u;push(0x1350aau,r(eax));return 0x1350abu;
        case listed_count:return 0x1350b9u;
        case light_array:return 0x1350bfu;
        case listed_light:return 0x1350c5u;
        case light_records:return 0x1350c9u;
        case light_link:return 0x1350dau;
        case light_frame:return 0x1350f0u;
        case light_queued:return 0x1350f5u;
        case queue_view:return 0x1350fau;
        case light_queue:return 0x1350feu;
        case queue_count:return 0x166b60u;
        case queue_capacity:push(0x166b64u,r(esi));return 0x166b65u;
        case queue_handles:return 0x166b6eu;
        case append_handle:return 0x166b71u;
        case handle_store:return 0x166b75u;
        case queue_count_flags:return 0x166b78u;
        case queue_flags:return 0x166b7cu;
        case append_flags:return 0x166b7fu;
        case flags_store:return 0x166b84u;
        case queue_count_values:return 0x166b88u;
        case queue_values:return 0x166b8cu;
        case append_value:return 0x166b8fu;
        case value_store:return 0x166b93u;
        case queue_count_associations:return 0x166b96u;
        case queue_associations:return 0x166b9au;
        case append_association:return 0x166b9du;
        case association_store:return 0x166ba2u;
        case queue_grow:return 0x166ba6u;
        case listed_count_again:return 0x135118u;
        case sky_objects_view:return 0x135121u;
        case sky_cluster:return 0x135125u;
        case sky_skip:return 0x135133u;
        case sky_collideable_heads:return 0x135140u;
        case sky_collideable_head:return 0x13514cu;
        case sky_other_heads:finish_walk();return 0x1351e3u;
        case sky_other_head:return 0x1351e8u;
        case walk_base:break;
        }
        return 0;
    }
    // The access's own effect (the destination register, a compare's flags)
    // and the register work up to the next branch.
    void loaded(Access a,std::uint32_t v){
        using enum Access;
        if(unsigned(a)>=unsigned(walk_base))return walk_loaded(a,v);
        auto&f=s_.flags;
        switch(a){
        case entry_view:r(ebp)=v;break;
        case entry_descriptor:r(eax)=v;r(ecx)=f.logic(0u,4);break;                    // XOR ECX,ECX
        case visible_count:f.minus(v,r(ecx),2);break;                            // CMP [..],CX
        case loop_descriptor:r(eax)=v;break;
        case loop_offset:r(ecx)=v;break;
        case visible_cluster:r(ebx)=widen(v);break;
        case current_fog:r(edi)=v;break;
        case collideable_pool:r(esi)=v;r(ecx)=r(ebx);break;                     // MOV ECX,EBX
        case fog_view:r(edx)=v;break;
        case object_skip:f.logic(v&0x40u,1);break;
        case collideable_heads:r(eax)=v;break;
        case collideable_head:r(edi)=v;f.minus(v,0xffffffffu,4);begin_walk(Walk::collideable_first);break;
        case inside_fog_word:r(ebp)=v;break;
        case filter_view:r(edx)=v;break;
        case filter_flags:r(eax)=v;f.logic((v>>8u)&4u,1);break;                // TEST AH,4
        case object_headers:r(ebx)=v;break;
        // MOV EAX,ECX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        case object_header_data:r(ebx)=v;r(eax)=f.logic(r(ecx)&0xffffu,4)*3u;break;
        case object_data:r(eax)=v;break;
        // CMP BYTE [..],7; a non-machine goes to CMP EDI,-1.
        case object_type:
            f.minus(v,7u,1);
            if(v!=7u){f.minus(r(edi),0xffffffffu,4);begin_walk(Walk::collideable_next);}
            break;
        case admit_position:r(eax)=v;break;
        case collideable_pool_again:r(esi)=v;f.minus(r(edi),0xffffffffu,4);begin_walk(Walk::collideable_next);break;
        case other_offset:r(ecx)=v;break;
        case other_descriptor:r(edx)=v;break;
        case other_cluster:r(eax)=widen(v);break;
        case other_heads:r(ecx)=v;break;
        case other_head:r(edi)=v;f.minus(v,0xffffffffu,4);begin_walk(Walk::other_first);break;
        case other_view:r(esi)=v;break;
        case other_position:r(edx)=v;break;
        case light_view:r(eax)=v;break;
        case light_kind:r(ecx)=v;f.logic(v,4);break;                            // TEST ECX,ECX
        case light_skip:f.logic(v&0x10u,1);break;
        case light_offset:r(edx)=v;break;
        case light_descriptor:r(eax)=v;break;
        case light_cluster:r(ecx)=widen(v);break;
        case light_heads:r(edx)=v;break;
        case light_head:r(edi)=v;f.minus(v,0xffffffffu,4);begin_walk(Walk::light_first);break;
        case light_walk_view:r(esi)=v;break;
        case light_position:r(ebp)=v;break;
        case sky_known:f.minus(v,r(eax),2);break;                               // CMP [..],AX
        case sky_view:r(ecx)=v;break;
        case sky_kind:r(edx)=v;f.logic(v,4);break;
        case sky_visible:byte(ecx,v);f.logic(v,1);break;                        // MOV CL; TEST CL,CL
        case sky_index:r(ecx)=v;f.minus(v,r(eax),4);break;                      // CMP ECX,EAX
        case sky_selection:f.minus(v,r(eax),2);break;                           // CMP [..],AX
        case structure_bsp:r(eax)=v;break;
        case sky_owner_count:r(edx)=v;f.logic(v,4);break;
        case sky_owners:r(edx)=v;break;
        case sky_owner:half(eax,v);break;                                       // MOV AX
        case next_offset:r(edx)=v;break;
        case next_view:r(ecx)=v;break;
        case next_descriptor:r(ecx)=v;break;
        case next_position:r(eax)=v;r(edx)=f.plus(r(edx),0x1au,4);break;        // ADD EDX,1A
        // INC EAX; CMP EAX,EDX
        case next_count:r(edx)=widen(v);r(eax)=f.increment(r(eax),4,false);f.minus(r(eax),r(edx),4);break;
        case after_view:r(ebp)=v;break;
        case direct_kind:r(eax)=v;f.logic(v,4);break;
        case direct_skip:f.logic(v&0x10u,1);break;
        case direct_owner:f.minus(v,0xffffu,2);break;
        case direct_source:r(eax)=v;break;
        case light_array:r(ebp)=v;break;
        case listed_light:r(edi)=v;break;
        // MOV ECX,EDI; AND ECX,FFFF; IMUL ECX,ECX,110 (hardware flags).
        case light_records:{
            r(edx)=v;r(ecx)=f.logic(r(edi)&0xffffu,4);
            const auto product=light_datum_offset(r(ecx),f.bits());r(ecx)=product.value;f.load(product.flags);
            break;
        }
        case light_link:f.minus(v,0xffffu,2);r(ebx)=r(ecx)+r(edx);break;       // CMP; LEA EBX,[ECX+EDX]
        case light_frame:r(eax)=v;break;
        case light_queued:f.minus(v,r(eax),4);break;
        case queue_view:r(ecx)=v;break;
        case light_queue:r(ecx)=v;break;
        case queue_count:r(edx)=v;break;                                         // MOVZX
        // XOR AL,AL; DEC ESI; CMP EDX,ESI
        case queue_capacity:r(esi)=v;byte(eax,f.logic(0u,1));r(esi)=f.increment(r(esi),4,true);f.minus(r(edx),r(esi),4);break;
        case queue_handles:case queue_flags:case queue_values:case queue_associations:r(eax)=v;break;
        case append_handle:case append_value:r(esi)=v;break;
        case append_flags:case append_association:half(esi,v);break;            // MOV SI
        case queue_count_flags:case queue_count_values:case queue_count_associations:r(edx)=v;break;
        // INC ESI; CMP ESI,EAX
        case listed_count_again:r(eax)=v;r(esi)=f.increment(r(esi),4,false);f.minus(r(esi),r(eax),4);break;
        case sky_objects_view:r(ebp)=v;break;
        case sky_cluster:r(eax)=v;f.minus(v,0xffffu,2);break;                   // CMP AX,FFFF
        case sky_skip:f.logic(v&0x40u,1);break;
        // MOVSX ESI,AX; SHL ESI,2 (hardware flags).
        case sky_collideable_heads:{
            r(edx)=v;const auto offset=sky_head_offset(widen(r(eax)),f.bits());r(esi)=offset.value;f.load(offset.flags);
            break;
        }
        case sky_collideable_head:r(edi)=v;f.minus(v,0xffffffffu,4);begin_walk(Walk::sky_collideable_first);break;
        case sky_other_heads:r(eax)=v;break;
        case sky_other_head:r(edi)=v;f.minus(v,0xffffffffu,4);begin_walk(Walk::sky_other_first);break;
        default:break;
        }
    }
    // The ESP-relative slot of each argument, local and queue argument role.
    struct Slot {std::uint32_t offset;unsigned width;};
    static Slot slot(Access a){
        using enum Access;
        switch(a){
        case entry_view:return{0x30u,4};
        case fog_view:case filter_view:case other_view:case light_view:case light_walk_view:case sky_view:case next_view:
        case after_view:case queue_view:case sky_objects_view:return{0x38u,4};
        case first_position:case admit_position:case other_position:case light_position:case next_position:case position_store:return{0x10u,4};
        case first_offset:case loop_offset:case other_offset:case light_offset:case next_offset:case offset_store:return{0x14u,4};
        case first_descriptor:case loop_descriptor:case other_descriptor:case light_descriptor:case descriptor_store:return{0x18u,4};
        case sky_none:case sky_cluster:return{0x1cu,4};
        case sky_known:case sky_store:return{0x1cu,2};
        case inside_fog:return{0x20u,1};
        case inside_fog_word:case listed_count:case listed_count_again:return{0x20u,4};
        case listed_light:return{0x24u,4};
        case append_handle:return{8u,4};
        case append_flags:return{0xcu,2};
        case append_value:return{0x10u,4};
        case append_association:return{0x14u,2};
        default:return{0u,0};
        }
    }
    // The value a local store writes, from the register the original stores.
    std::uint32_t stored(Access a,std::uint32_t value){
        using enum Access;
        switch(a){
        case sky_none:return 0xffffffffu;
        case first_position:case first_offset:return r(ecx);
        case first_descriptor:return r(eax);
        case inside_fog:return r(eax)&0xffu;
        case sky_store:return r(eax)&0xffffu;
        case offset_store:return r(edx);
        case position_store:return r(eax);
        case descriptor_store:return r(ecx);
        case listed_count:return r(eax);
        default:return value;
        }
    }
    std::uint32_t load(Access a,std::uint32_t address,unsigned width){
        const auto pc=before(a);const auto v=read(pc,address,width);loaded(a,v);return v;
    }
    // A frame slot: its address is taken after the role's own pushes.
    std::uint32_t load_slot(Access a,std::uint32_t extra=0){
        const auto pc=before(a);const auto at_slot=slot(a);const auto v=read(pc,esp_()+at_slot.offset+extra,at_slot.width);loaded(a,v);return v;
    }
public:
    Adapter(State&s,Call&call,Before&before):Base{s,before},call_(call){}

    // ---- Memory
    std::uint32_t load32(Access a,std::uint32_t address){return load(a,address,4);}
    std::uint32_t load16(Access a,std::uint32_t address){return load(a,address,2);}
    std::uint32_t load8(Access a,std::uint32_t address){return load(a,address,1);}
    void store32(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value,4);}
    void store16(Access a,std::uint32_t address,std::uint32_t value){write(before(a),address,value&0xffffu,2);}
    // INC WORD [ECX+4]: the flags commit after the provider accepted the store.
    void increment16(Access a,std::uint32_t address){
        const auto pc=before(a);at(pc);const auto prior=word(address,2);auto flags=s_.flags;const auto value=flags.increment(prior,2,false);
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(2);io([&]{s_.memory.read(address,event.before);});}
        std::array<std::byte,2>bytes{std::byte(value),std::byte(value>>8u)};
        io([&]{s_.memory.write(address,bytes);});s_.flags=flags;
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
        byte(eax,1u);                                                             // MOV AL,1
    }

    // ---- Frame
    std::uint32_t argument(Access a){return load_slot(a);}
    std::uint32_t local(Access a){return load_slot(a);}
    void set_local(Access a,std::uint32_t value){
        const auto pc=before(a);const auto at_slot=slot(a);write(pc,esp_()+at_slot.offset,stored(a,value),at_slot.width);
    }
    std::uint32_t listed(Access a,std::uint32_t position){r(esi)=position;return load_slot(a,position*4u);}
    std::uint32_t append_argument(Access a){return local(a);}
    // 171070(ECX cluster, EDI fog) -> AL; EBX and ESI are kept.
    std::uint32_t fog_membership(Kept&k,std::uint32_t fog){
        r(ebx)=k.cluster;r(esi)=k.pool;r(edi)=fog;r(ecx)=k.cluster;
        call(0x134d81u,fog_membership_entry);
        k.cluster=r(ebx);k.pool=r(esi);return r(eax)&0xffu;
    }
    // 135B60(object, position, fog byte, sky byte), the view in EAX.
    void admit_object(Walk walk,const logic::ObjectAdmission&args,Kept&k){
        finish_walk();
        r(edi)=k.next;
        switch(walk){
        case Walk::collideable_next:
            r(ebp)=k.inside_fog;push(0x134e23u,args.sky);push(0x134e25u,args.inside_fog);push(0x134e26u,args.position);push(0x134e27u,args.object);
            r(eax)=k.view;call(0x134e2au,object_admission_entry);
            k.next=r(edi);k.inside_fog=r(ebp);
            break;
        case Walk::other_next:
            r(ebp)=k.inside_fog;r(esi)=k.view;
            push(0x134ed9u,args.sky);push(0x134edbu,args.inside_fog);push(0x134edcu,args.position);push(0x134eddu,args.object);
            r(eax)=k.view;call(0x134ee0u,object_admission_entry);
            k.next=r(edi);k.inside_fog=r(ebp);k.view=r(esi);
            s_.flags.minus(r(edi),0xffffffffu,4);begin_walk(Walk::other_next);
            break;
        default:{
            // The sky loops: PUSH 1; PUSH 0; PUSH -1; PUSH EAX; MOV EAX,EBP.
            const bool collideable=walk==Walk::sky_collideable_next;
            const std::uint32_t base=collideable?0x135191u:0x135230u;
            r(ebp)=k.view;r(esi)=k.sky_offset;
            push(base,args.sky);push(base+2u,args.inside_fog);push(base+4u,args.position);push(base+6u,args.object);
            r(eax)=k.view;call(base+9u,object_admission_entry);
            k.next=r(edi);k.view=r(ebp);k.sky_offset=r(esi);
            s_.flags.minus(r(edi),0xffffffffu,4);begin_walk(walk);
            break;
        }
        }
    }
    // 135540(position, 0), the light in EBX and the view in ESI.
    void admit_light(Kept&k){
        finish_walk();
        r(ebx)=k.entry;r(esi)=k.view;r(ebp)=k.position;r(edi)=k.next;
        push(0x134fb0u,0u);push(0x134fb2u,k.position);call(0x134fb3u,light_admission_entry);
        k.next=r(edi);k.view=r(esi);k.position=r(ebp);
        s_.flags.minus(r(edi),0xffffffffu,4);begin_walk(Walk::light_next);
    }
    // C5980(EAX source; buffer, 4) -> EAX count; XOR ESI,ESI; TEST EAX,EAX.
    // EBP (the view) is kept.
    std::uint32_t list_lights(Kept&k,std::uint32_t source){
        r(ebp)=k.view;r(eax)=source;call(0x1350b0u,list_lights_entry);
        k.view=r(ebp);r(esi)=s_.flags.logic(0u,4);s_.flags.logic(r(eax),4);return r(eax);
    }
    // C5170(EAX light) -> AL; TEST AL,AL.
    std::uint32_t light_active(Kept&k){
        r(esi)=k.position;r(edi)=k.listed;r(ebx)=k.light;r(ebp)=k.lights;r(eax)=k.listed;
        call(0x1350e7u,light_active_entry);
        k.position=r(esi);k.listed=r(edi);k.light=r(ebx);k.lights=r(ebp);
        s_.flags.logic(r(eax)&0xffu,1);return r(eax)&0xffu;
    }
    // C5140(EAX light).
    void mark_queued(Kept&k){
        r(esi)=k.position;r(edi)=k.listed;r(ebp)=k.lights;r(eax)=k.listed;
        call(0x135113u,mark_queued_entry);
        k.position=r(esi);k.listed=r(edi);k.lights=r(ebp);
    }
    // 166B60(handle, flags, value, association), the queue in ECX: the
    // caller's pushes and CALL, then (end) POP ESI; RET 10; TEST AL,AL.
    void begin_append(const logic::QueueAppend&args){
        push(0x135101u,args.association);push(0x135103u,args.value);push(0x135105u,args.flags);push(0x135107u,args.handle);
        push(0x135108u,0x13510du);
    }
    bool end_append(bool joined){
        r(esi)=pop(0x166bacu);pop(0x166badu);r(esp)+=0x10u;
        s_.flags.logic(r(eax)&0xffu,1);return joined;
    }
    // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,24; RET 4.
    void finish(){
        finish_walk();
        r(edi)=pop(0x135282u);r(esi)=pop(0x135283u);r(ebp)=pop(0x135284u);r(ebx)=pop(0x135285u);
        r(esp)=s_.flags.plus(r(esp),0x24u,4);pop(0x135289u);r(esp)+=4u;
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
    if(!admitted(s))return false;
    Adapter adapter{s,call,before};logic::cluster_contents::collect(adapter,adapter);return true;
}
#else
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_cluster_contents_detail
