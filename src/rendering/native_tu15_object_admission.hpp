#pragma once
#include "halo2/native_tu15_object_admission_kernel.hpp"
#include "physics/runtime/native_tu15_census_admission.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "pstencil/campaign_lighting_timing.hpp"
#include <cstdlib>
#include <thread>
#include <type_traits>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_bsp_surface_2d_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "rendering/native_tu15_bsp_admission.hpp"
#include "rendering/native_tu15_geometry_visibility.hpp"
#include "rendering/native_tu15_visibility_association.hpp"
#endif

namespace halo2::native_object_admission_detail {
// Retained ABI view for the shared typed 135B60 object admission policy.
enum class Child {geometry,association,geometry_frustum,geometry_auxiliary};
struct Noop {static constexpr bool silent_hook=true;template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_ADMISSION");return p&&*p;}();return value;}
inline bool reference_queue_scan(){
    static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_OBJECT_QUEUE_SCAN");return p&&*p;}();
    return value;
}
inline bool reference_geometry_scope(){
    static const bool value=[] {
        const auto* reference=std::getenv("HALO2_REFERENCE_OBJECT_GEOMETRY_SCOPE");
        const auto* literal=std::getenv("HALO2_REFERENCE_LITERAL_INSTRUCTIONS");
        return (reference && *reference) || (literal && *literal);
    }();
    return value;
}
inline bool reference_push_io(){
    static const bool value=[] {
        const auto* reference=std::getenv("HALO2_REFERENCE_OBJECT_ADMISSION_PUSH_IO");
        const auto* literal=std::getenv("HALO2_REFERENCE_LITERAL_INSTRUCTIONS");
        return (reference && *reference) || (literal && *literal);
    }();
    return value;
}
inline bool reference_stack_io(){
    static const bool value=[]{
        const auto* reference=std::getenv("HALO2_REFERENCE_PROJECTED_STACK_IO");
        return reference && *reference;
    }();
    return value;
}
inline bool reference_queue_store_io(){
    static const bool value=[] {
        const auto* reference=std::getenv("HALO2_REFERENCE_PROJECTED_QUEUE_STORE_IO");
        return reference && *reference;
    }();
    return value;
}
template<class State>bool policy_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    // The parent needs two x87 slots (135AE0); the genuine geometry child
    // needs five. Both run at relative depth zero.
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>3)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    // Census diagnostic: full callbacks without a cursor. The native closure
    // (135B60 with inlined 135AE0/135620/166B60/C0CA0) holds no service PC;
    // the 135970/136E10 children keep their own admission.
    if(native_census_detail::uncursored(s,0xc0ca0u,0x166bafu)){
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
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace N=native_bsp_surface_2d_detail;
template<class Scope>struct Io {Scope*scope;template<class Callback>void operator()(Callback&&callback){scope->io(std::forward<Callback>(callback));}};
template<class State,class Call,class Before>class View:public native_mopp_ray_caller_detail::Frame<State,Before,Io<native_raw_x87_detail::Scope<State>>> {
    using Scope=native_raw_x87_detail::Scope<State>;
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<Scope>>;
    using Base::s_;using Base::before_;using Base::at;using Base::word;using Base::io;using Base::pop;
    Scope&scope_;Call&call_;bool borrow_geometry_,callback_free_pushes_,callback_free_stack_,callback_free_outputs_;
    void push(std::uint32_t pc,std::uint32_t value){
        if(!callback_free_pushes_){Base::push(pc,value);return;}
        at(pc);
        const auto address=s_.gp[4]-4u;
        const auto bytes=std::bit_cast<std::array<std::byte,4>>(value);
        if(address>=0x60000000u && address<=0x600ffffcu &&
           s_.memory.try_callback_free_write(address,bytes)) {
            s_.gp[4]=address;
            return;
        }
        Base::store(pc,address,value,[&]{s_.gp[4]=address;},true);
    }
    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4){at(pc);return word(address,width);}
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){
        at(pc);
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        const auto written=std::span(bytes).first(width);
        if(callback_free_stack_ && !s_.observe &&
           (callback_free_outputs_ || (address>=0x60000000u &&
              std::uint64_t(address)+width<=0x60100000u)) &&
           s_.memory.try_callback_free_write(address,written))return;
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,written);});
        if(s_.observe){event.written.assign(written.begin(),written.end());io([&]{s_.observe(event);});}
    }
    // RMW flags commit only after the provider accepted the store.
    template<class Operation>void modify(std::uint32_t pc,std::uint32_t address,unsigned width,Operation operation){
        at(pc);const auto prior=word(address,width);auto flags=s_.flags;const auto value=operation(flags,prior);
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        const auto written=std::span(bytes).first(width);
        if(callback_free_outputs_ && !s_.observe && s_.memory.try_callback_free_write(address,written)) {
            s_.flags=flags;
            return;
        }
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(width);io([&]{s_.memory.read(address,event.before);});}
        io([&]{s_.memory.write(address,written);});s_.flags=flags;
        if(s_.observe){event.written.assign(written.begin(),written.end());io([&]{s_.observe(event);});}
    }
    void byte(unsigned reg,std::uint32_t value){s_.set(reg,value,1,0);}
    static std::uint32_t signed_half(std::uint32_t value){return std::uint32_t(std::int32_t(std::int16_t(value)));}
    bool less()const{return s_.flags.sf!=s_.flags.of;}
    bool less_equal()const{return s_.flags.zf||s_.flags.sf!=s_.flags.of;}
    static std::uint32_t bits(N::Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
    // Signed queue search shared by the three stamped-object loops. The
    // object handle stays in EBP; each probe sign-extends the 16-bit cursor.
    template<unsigned Index>bool probe(std::uint32_t pc,std::uint32_t base){
        auto&g=s_.gp;g[Index]=signed_half(g[0]);s_.flags.minus(read(pc,base+g[Index]*4u),g[5],4);return s_.flags.zf;
    }
    bool admitted_queue_scan(std::uint32_t count)const{
        return !reference_queue_scan() && s_.gp[0]==0 &&
            native_bsp_admission_detail::admits_ordinary_queue_scan<Before>(s_,count);
    }
    static void record_scan(std::uint32_t probes,bool bulk) noexcept {
        if(auto* timing=pstencil::projected_object_timing) {
            auto& work=timing->work;
            ++work.stamped_scans;
            if(bulk) {
                ++work.stamped_bulk_scans;
                work.stamped_bulk_probes+=probes;
            } else work.stamped_scalar_probes+=probes;
        }
    }
    bool try_find_stamped_row(){
        auto&g=s_.gp;const auto count=g[6];
        if(!admitted_queue_scan(count))return false;
        // In this first loop the original reloads the handle pointer before
        // every probe. Borrow it only if the pointer read is ordinary too.
        std::uint32_t handles;
        if(!native_raw_x87_detail::direct_read(s_.memory,g[1]+12u,std::as_writable_bytes(std::span(&handles,1))))
            return false;
        const auto index=native_bsp_admission_detail::scan_ordinary_queue(s_.memory,handles,count,g[5]);
        if(!index)return false;
        const bool found=*index<count;
        at(0x135ceau);
        g[3]=found?*index:count-1u;
        g[2]=g[3];byte(2,found);
        g[0]=g[3]+1u;s_.flags.minus(g[0],count,4);
        if(found&&g[0]<count)s_.flags.logic(1u,1);
        record_scan(g[0],true);
        return true;
    }
    template<unsigned Index>std::optional<bool> try_find_flag_row(std::uint32_t pc,
        std::uint32_t handles,std::uint32_t count){
        if(!admitted_queue_scan(count))return std::nullopt;
        auto&g=s_.gp;
        const auto index=native_bsp_admission_detail::scan_ordinary_queue(s_.memory,handles,count,g[5]);
        if(!index)return std::nullopt;
        // These two searches return directly from the matching comparison.
        // A miss advances EAX to the count but keeps the last probed index.
        at(pc);const bool found=*index<count;
        g[0]=*index;g[Index]=found?*index:count-1u;
        if(found)s_.flags.minus(g[5],g[5],4);
        else s_.flags.minus(g[0],count,4);
        record_scan(g[0]+std::uint32_t(found),true);
        return found;
    }
public:
    static constexpr std::uint32_t common_epilogue=0x135daau,unmatched_epilogue=0x135d92u,stamped_epilogue=0x135cc0u;
    View(State&s,Call&call,Before&before,Scope&scope,bool borrow_geometry,bool callback_free_pushes):
        Base{s,before,Io<Scope>{&scope}},scope_(scope),call_(call),borrow_geometry_(borrow_geometry),
        callback_free_pushes_(callback_free_pushes),callback_free_stack_(callback_free_pushes && !reference_stack_io()),
        callback_free_outputs_(callback_free_stack_ && !reference_queue_store_io()){}
    bool stamped(){
        auto&g=s_.gp;g[2]=read(0x135b60u,0x4eabccu);g[4]=s_.flags.minus(g[4],0x14u,4);
        push(0x135b69u,g[3]);push(0x135b6au,g[5]);g[5]=read(0x135b6bu,g[4]+0x20u);
        push(0x135b6fu,g[6]);push(0x135b70u,g[7]);g[7]=g[0];g[0]=s_.flags.logic(g[5]&0xffffu,4);
        g[1]=read(0x135b7au,0x4eabd0u+g[0]*4u);s_.flags.minus(g[1],g[2],4);return s_.flags.zf;
    }
    // Stamped: the first loop retains the last probed cursor in EBX.
    void find_row(){
        auto&g=s_.gp;g[1]=read(0x135ccau,g[7]+0x14u);g[6]=signed_half(read(0x135ccdu,g[1]+4u,2));
        g[3]=s_.flags.logic(0xffffffffu,4);byte(2,s_.flags.logic(0u,1));g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[6],4);
        if(less_equal())return;
        if(try_find_stamped_row())return;
        do{
            s_.flags.logic(g[2]&255u,1);if(g[2]&255u)break;
            g[3]=read(0x135ce4u,g[1]+12u);const bool found=probe<2>(0x135ceau,g[3]);g[3]=g[0];byte(2,found);
            g[0]=s_.flags.increment(g[0],4,false);s_.flags.minus(g[0],g[6],4);
        }while(less());
        record_scan(g[0],false);
    }
    bool row_merged(){
        auto&g=s_.gp;g[2]=read(0x135cf7u,g[1]+8u);
        const auto shifted=native_bsp_admission_detail::doubled(signed_half(g[3]),s_.flags.bits());g[0]=shifted.first;s_.flags.load(shifted.second);
        s_.flags.logic(read(0x135cffu,g[0]+g[2]+1u,1)&8u,1);return !s_.flags.zf;
    }
    bool row_associated(){
        auto&g=s_.gp;g[1]=read(0x135d06u,g[1]+0x14u);s_.set(0,read(0x135d09u,g[0]+g[1],2),2,0);
        s_.flags.minus(g[0]&0xffffu,0xffffu,2);return !s_.flags.zf;
    }
    void merge_association(){
        auto&g=s_.gp;g[1]=signed_half(read(0x135d13u,g[4]+0x2cu,2));g[2]=signed_half(g[0]);
        const auto location=native_bsp_admission_detail::association_bit(g[1]);
        g[2]=location.word+g[2]*4u+0x298u;g[0]=g[7]+g[2]*4u;g[1]=location.count;g[2]=location.mask;s_.flags.load(location.flags);
        modify(0x135d34u,g[0],4,[&](auto&flags,auto value){return flags.logic(value|g[2],4);});
    }
    bool primary(){const auto value=read(0x135d36u,s_.gp[4]+0x30u,1);byte(0,value);s_.flags.logic(value,1);return value!=0;}
    bool find_primary(){
        auto&g=s_.gp;g[1]=read(0x135d3eu,g[7]+0x14u);g[2]=signed_half(read(0x135d41u,g[1]+4u,2));
        g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[2],4);if(less_equal())return false;
        g[6]=read(0x135d4bu,g[1]+12u);
        if(const auto found=try_find_flag_row<3>(0x135d53u,g[6],g[2]);found)return *found;
        do{
            if(probe<3>(0x135d53u,g[6])){record_scan(g[0]+1u,false);return true;}
            g[0]=s_.flags.increment(g[0],4,false);s_.flags.minus(g[0],g[2],4);
        }while(less());
        record_scan(g[0],false);
        return false;
    }
    void mark_primary(){
        auto&g=s_.gp;g[1]=read(0x135d5fu,g[1]+8u);g[0]=signed_half(g[0]);
        modify(0x135d65u,g[1]+g[0]*2u+1u,1,[](auto&flags,auto value){return flags.logic(value|4u,1);});g[0]=g[1]+g[0]*2u;
    }
    bool secondary(){const auto value=read(0x135d6du,s_.gp[4]+0x34u,1);byte(0,value);s_.flags.logic(value,1);return value!=0;}
    bool secondary_rows(){
        auto&g=s_.gp;g[7]=read(0x135d75u,g[7]+0x14u);g[1]=signed_half(read(0x135d78u,g[7]+4u,2));
        g[0]=s_.flags.logic(0u,4);s_.flags.logic(g[1],4);return !less_equal();
    }
    bool find_secondary(){
        auto&g=s_.gp;g[2]=read(0x135d82u,g[7]+12u);
        if(const auto found=try_find_flag_row<6>(0x135d88u,g[2],g[1]);found)return *found;
        do{
            if(probe<6>(0x135d88u,g[2])){record_scan(g[0]+1u,false);return true;}
            g[0]=s_.flags.increment(g[0],4,false);s_.flags.minus(g[0],g[1],4);
        }while(less());
        record_scan(g[0],false);
        return false;
    }
    void mark_secondary(){
        auto&g=s_.gp;g[2]=signed_half(g[0]);g[0]=read(0x135d9fu,g[7]+8u);
        modify(0x135da2u,g[0]+g[2]*2u+1u,1,[](auto&flags,auto value){return flags.logic(value|2u,1);});g[0]+=g[2]*2u;
    }
    // Unstamped: the object's record and optional auxiliary owner.
    void object(){
        auto&g=s_.gp;g[2]=g[0]*3u;g[0]=read(0x135b8cu,0x4ecbd0u);g[1]=read(0x135b91u,g[0]+0x44u);
        g[0]=read(0x135b94u,g[7]+0x2ad0u);s_.flags.logic(g[0],4);g[1]=read(0x135b9cu,g[1]+g[2]*4u+8u);
        write(0x135ba0u,g[4]+0x1cu,1u,1);write(0x135ba5u,g[4]+0x14u,1u,1);write(0x135baau,g[4]+0x12u,1u,1);write(0x135bafu,g[4]+0x20u,g[1]);
        if(!s_.flags.zf)g[0]=read(0x135bb5u,g[0]+0x14u);
    }
    // The context's sphere selector picks the record's first or second sphere.
    void sphere(){
        auto&g=s_.gp;g[2]=read(0x135bb8u,g[7]+0x2a78u);s_.flags.logic(g[2],4);
        if(!g[2]){s_.xmm[0]=N::scalar(read(0x135bc2u,g[1]+0x3cu));g[3]=g[1]+0x30u;}
        else{s_.xmm[0]=N::scalar(read(0x135bccu,g[1]+0x5cu));g[3]=g[1]+0x50u;}
    }
    bool geometry(){
        auto&g=s_.gp;g[5]=read(0x135bd4u,g[4]+0x34u);g[2]=g[4]+0x14u;push(0x135bdcu,g[2]);g[1]=g[4]+0x20u;push(0x135be1u,g[1]);
        g[1]=read(0x135be2u,g[4]+0x34u);g[2]=g[4]+0x1au;push(0x135beau,g[2]);g[2]=read(0x135bebu,g[4]+0x34u);
        push(0x135befu,g[5]);push(0x135bf0u,g[1]);push(0x135bf1u,g[2]);write(0x135bf2u,g[4]+0x30u,bits(s_.xmm[0]));
        g[6]=read(0x135bf8u,g[4]+0x30u);push(0x135bfcu,g[0]);push(0x135bfdu,g[6]);g[1]=g[3];g[0]=g[7];
        push(0x135c02u,0x135c07u);
        const auto descendants=[&](auto& child,auto which,auto& before){
            if(which==native_geometry_visibility_detail::Child::frustum){
                if(native_cluster_frustum_abi_detail::bridge(child,before))return;
                call_(child,Child::geometry_frustum,before);
            }else{
                if(native_bsp_queue_find_detail::bridge(child,before))return;
                call_(child,Child::geometry_auxiliary,before);
            }
        };
        if(!borrow_geometry_ || !native_geometry_visibility_detail::borrow_object_child(
            s_,scope_,descendants,before_,callback_free_stack_))
            scope_.child([&]{call_(s_,Child::geometry,before_);});
        s_.flags.logic(g[0]&255u,1);return (g[0]&255u)!=0;
    }
    bool forced(){
        const auto value=read(0x135c0bu,s_.gp[7]+0x2a60u,1);byte(0,value);s_.flags.logic(value,1);
        if(!s_.flags.sf)byte(0,s_.flags.logic(0u,1));return s_.flags.sf;
    }
    void accept(){byte(0,1u);}
    void override_sides(){
        auto&g=s_.gp;g[1]=read(0x135c1bu,g[7]+0x2a78u);s_.flags.logic(g[1],4);if(!g[1])return;
        byte(1,read(0x135c25u,g[4]+0x14u,1));s_.flags.logic(g[1]&255u,1);if(!(g[1]&255u))return;
        g[1]=read(0x135c2du,g[4]+0x20u);byte(2,read(0x135c31u,g[1]+6u,1));byte(2,~g[2]);byte(2,s_.flags.logic(g[2]&1u,1));
        write(0x135c39u,g[4]+0x14u,g[2]&255u,1);
    }
    bool visible(){s_.flags.logic(s_.gp[0]&255u,1);return (s_.gp[0]&255u)!=0;}
    // Inlined 135AE0: plane side bytes, then the |distance| < radius test.
    void sides(){
        auto&g=s_.gp;auto&x=s_.xmm;scope_.require_capacity(2);
        push(0x135c45u,g[6]);g[2]=g[4]+0x38u;g[6]=g[4]+0x17u;g[1]=g[3];g[0]=g[7];push(0x135c52u,0x135c57u);
        push(0x135ae0u,g[1]);s_.flags.logic(read(0x135ae1u,g[0]+0x2a60u)&0x200u,4);
        const bool clipping=!s_.flags.zf;
        if(clipping){
            x[0]=N::scalar(read(0x135aedu,g[0]+0x2a90u));x[0]=N::multiply(x[0],N::scalar(read(0x135af5u,g[1]+8u)));
            x[1]=N::scalar(read(0x135afau,g[0]+0x2a8cu));x[1]=N::multiply(x[1],N::scalar(read(0x135b02u,g[1]+4u)));
            x[0]=N::add(x[0],x[1]);x[1]=N::scalar(read(0x135b0bu,g[1]));x[1]=N::multiply(x[1],N::scalar(read(0x135b0fu,g[0]+0x2a88u)));
            x[0]=N::add(x[0],x[1]);x[0]=N::subtract(x[0],N::scalar(read(0x135b1bu,g[0]+0x2a94u)));x[1]=_mm_setzero_ps();
            auto comparison=native_geometry_visibility_detail::compare(x[1],x[0]);s_.flags.load(comparison.flags);
            write(0x135b29u,g[2],0u,1);write(0x135b2cu,g[4],bits(x[0]));write(0x135b31u,g[6],0u,1);
            if(!s_.flags.cf&&!s_.flags.zf)write(0x135b36u,g[6],1u,1);
            comparison=native_geometry_visibility_detail::compare(x[0],x[1]);s_.flags.load(comparison.flags);
            if(!s_.flags.cf&&!s_.flags.zf)write(0x135b3eu,g[2],1u,1);
            auto value=read(0x135b41u,g[4]);asm volatile("flds %0;fabs"::"m"(value):"memory","st");
            value=read(0x135b46u,g[4]+8u);std::uint32_t ah;unsigned char overflow;
            asm volatile("flds %2;fcomip %%st(1),%%st;fstp %%st(0);lahf;seto %1":"=a"(ah),"=qm"(overflow):"m"(value):"cc","memory","st","st(1)");
            s_.flags.load(((ah>>8u)&0xd5u)|(std::uint32_t(overflow)<<11u));
        }
        // Unclipped contexts take both sides; otherwise |distance| < radius
        // (JB, including unordered) keeps the classified bytes.
        if(!clipping||!s_.flags.cf){write(0x135b50u,g[2],1u,1);write(0x135b53u,g[6],1u,1);}
        g[1]=pop(0x135b56u);pop(0x135b57u);g[4]+=4u;
    }
    // Inlined 135620: the six side/fog/containment bytes become row flags.
    void record_flags(){
        auto&g=s_.gp;g[0]=read(0x135c57u,g[4]+0x30u);g[1]=read(0x135c5bu,g[4]+0x14u);g[2]=read(0x135c5fu,g[4]+0x1cu);
        push(0x135c63u,g[5]);push(0x135c64u,g[0]);byte(0,read(0x135c65u,g[4]+0x3cu,1));push(0x135c69u,g[1]);
        byte(1,read(0x135c6au,g[4]+0x1fu,1));push(0x135c6eu,g[2]);byte(2,read(0x135c6fu,g[4]+0x22u,1));push(0x135c73u,0x135c78u);
        const auto mask=[&](unsigned reg,std::uint32_t bit){byte(reg,s_.flags.minus(0u,g[reg]&255u,1));g[reg]=s_.flags.borrow(g[reg],g[reg],4);g[reg]=s_.flags.logic(g[reg]&bit,4);};
        byte(0,s_.flags.minus(0u,g[0]&255u,1));push(0x135622u,g[3]);byte(3,read(0x135623u,g[4]+12u,1));
        g[0]=s_.flags.borrow(g[0],g[0],4);g[0]=s_.flags.logic(g[0]&0x4000u,4);
        mask(3,0x2000u);g[0]=s_.flags.logic(g[0]|g[3],4);
        byte(3,read(0x13563au,g[4]+8u,1));mask(3,0x1000u);g[0]=s_.flags.logic(g[0]|g[3],4);
        byte(2,s_.flags.minus(0u,g[2]&255u,1));g[3]=pop(0x13564cu);g[2]=s_.flags.borrow(g[2],g[2],4);g[2]=s_.flags.logic(g[2]&0x800u,4);
        g[0]=s_.flags.logic(g[0]|g[2],4);
        byte(2,read(0x135657u,g[4]+12u,1));mask(2,0x400u);g[0]=s_.flags.logic(g[0]|g[2],4);
        byte(2,read(0x135667u,g[4]+16u,1));mask(2,0x200u);g[0]=s_.flags.logic(g[0]|g[2],4);
        mask(1,0x8000u);g[0]=s_.flags.logic(g[0]|g[1],4);
        pop(0x135683u);g[4]+=16u;
    }
    bool contained(){auto&g=s_.gp;g[6]=g[0];g[0]=s_.flags.logic(0xffffffffu,4);s_.flags.logic(g[6]&0x800u,4);return !s_.flags.zf;}
    bool association_requested(){
        auto&g=s_.gp;g[1]=read(0x135c85u,g[4]+0x20u);g[2]=read(0x135c89u,g[1]+4u);g[2]=s_.flags.right(g[2],31u,4,false);
        s_.flags.logic(g[2]&1u,1);return !s_.flags.zf;
    }
    void allocate_association(){
        auto&g=s_.gp;g[0]=read(0x135c94u,g[4]+0x2cu);push(0x135c98u,g[0]);g[2]=g[7];push(0x135c9bu,0x135ca0u);
        scope_.child([&]{call_(s_,Child::association,before_);});
    }
    // Inlined 166B60: bounded four-stream queue row append.
    bool append(){
        auto&g=s_.gp;g[1]=read(0x135ca0u,g[7]+0x14u);push(0x135ca3u,g[0]);push(0x135ca4u,0u);push(0x135ca6u,g[6]);
        g[6]=read(0x135ca7u,g[4]+0x34u);push(0x135cabu,g[6]);push(0x135cacu,0x135cb1u);
        g[2]=read(0x166b60u,g[1]+4u,2);push(0x166b64u,g[6]);g[6]=read(0x166b65u,g[1]);byte(0,s_.flags.logic(0u,1));
        g[6]=s_.flags.increment(g[6],4,true);s_.flags.minus(g[2],g[6],4);
        if(less()){
            g[0]=read(0x166b6eu,g[1]+12u);g[6]=read(0x166b71u,g[4]+8u);write(0x166b75u,g[0]+g[2]*4u,g[6]);
            g[2]=read(0x166b78u,g[1]+4u,2);g[0]=read(0x166b7cu,g[1]+8u);s_.set(6,read(0x166b7fu,g[4]+12u,2),2,0);write(0x166b84u,g[0]+g[2]*2u,g[6]&0xffffu,2);
            g[2]=read(0x166b88u,g[1]+4u,2);g[0]=read(0x166b8cu,g[1]+16u);g[6]=read(0x166b8fu,g[4]+16u);write(0x166b93u,g[0]+g[2]*4u,g[6]);
            g[2]=read(0x166b96u,g[1]+4u,2);g[0]=read(0x166b9au,g[1]+20u);s_.set(6,read(0x166b9du,g[4]+20u,2),2,0);write(0x166ba2u,g[0]+g[2]*2u,g[6]&0xffffu,2);
            modify(0x166ba6u,g[1]+4u,2,[](auto&flags,auto value){return flags.increment(value,2,false);});byte(0,1u);
        }
        g[6]=pop(0x166bacu);pop(0x166badu);g[4]+=16u;
        s_.flags.logic(g[0]&255u,1);return (g[0]&255u)!=0;
    }
    // Inlined C0CA0: publish the frame stamp unless it already is current.
    void stamp(){
        auto&g=s_.gp;g[0]=g[6];push(0x135cbbu,0x135cc0u);
        g[1]=read(0xc0ca0u,0x4eabccu);g[0]=s_.flags.logic(g[0]&0xffffu,4);push(0xc0cabu,g[6]);g[6]=read(0xc0cacu,0x4eabd0u+g[0]*4u);
        byte(2,s_.flags.logic(0u,1));s_.flags.minus(g[6],g[1],4);g[6]=pop(0xc0cb7u);
        if(!s_.flags.zf){write(0xc0cbau,0x4eabd0u+g[0]*4u,g[1]);byte(0,1u);pop(0xc0cc3u);}
        else{byte(0,g[2]&255u);pop(0xc0cc6u);}
    }
    void finish(std::uint32_t epilogue){
        auto&g=s_.gp;g[7]=pop(epilogue);g[6]=pop(epilogue+1u);g[5]=pop(epilogue+2u);g[3]=pop(epilogue+3u);
        g[4]=s_.flags.plus(g[4],0x14u,4);pop(epilogue+7u);g[4]+=16u;
    }
};
template<class State,class Before>bool secondary_push_io(State&s){
    if constexpr(!std::is_same_v<Before,Noop>)return false;
    if(reference_push_io() || native_raw_x87_detail::scoped_access_reference() ||
       s.observe || s.fp.count || s.fp.top || !s.live_instruction_cursor() || s.gp[0]!=0x5598f8u)
        return false;
    const auto sp=s.gp[4];
    if(sp<0x60000080u || sp>0x600ffffcu || (sp&3u))return false;
    std::uint32_t caller{};
    return native_raw_x87_detail::direct_read(s.memory,sp,std::as_writable_bytes(std::span(&caller,1))) &&
           (caller==0x134e2fu || caller==0x134ee5u);
}

template<class State,class Call,class Before=Noop>bool run_with(State&s,Call call,Before before={},
    bool borrow_geometry=false,bool try_push_io=false){
    if(!admitted(s))return false;
    if(auto* timing=pstencil::projected_object_timing)++timing->work.native_object_admissions;
    native_raw_x87_detail::Scope scope{s};const bool eligible=borrow_geometry && !reference_geometry_scope() && !pstencil::projected_object_timing;
    const bool callback_free_pushes=try_push_io && secondary_push_io<State,Before>(s);
    View view{s,call,before,scope,eligible,callback_free_pushes};
    native_object_admission_detail::admit_object(view);return true;
}
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call fallback,Before before={}){
    return native_object_admission_detail::run_with(s,[&](auto&child,Child which,auto&observe){
        if(which==Child::geometry){
            const auto geometry=[&] {
                if(native_geometry_visibility_detail::bridge(child,[&](auto&state,auto kind,auto&progress){fallback(state,kind==native_geometry_visibility_detail::Child::frustum?Child::geometry_frustum:Child::geometry_auxiliary,progress);},observe))return;
                fallback(child,which,observe);
            };
            if(auto* timing=pstencil::projected_object_timing) {
                const pstencil::ProjectedVisibilityTimingBinding nested(nullptr,pstencil::projected_object_timing);
                const pstencil::CampaignLightPhaseTimer timer(timing,pstencil::CampaignLightPhase::projected_visibility_object_geometry);
                geometry();
            } else geometry();
            return;
        }
        if(which==Child::association && native_visibility_association_detail::bridge(child,observe))return;
        fallback(child,which,observe);
    },std::move(before),true,true);
}
// Exact generated owners install this bridge with their pinned canonical
// fallback and real child-dispatch boundaries.
#else
template<class State,class...Args>bool run_with(State&,Args&&...){return false;}
template<class State,class...Args>bool bridge(State&,Args&&...){return false;}
#endif
} // namespace halo2::native_object_admission_detail
