#include "halo2/native_tu15_overlap_query_handoff.hpp"
#include "halo2/native_tu15_sphere_cleanup_handoff.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
constexpr std::uint32_t base=0x1000000u,sp=base+0x9000u;
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct Memory:SourcePhysicsContactMemory {
    std::array<std::byte,0x10000>data{};std::array<std::byte,8>profile{};unsigned writes{};mutable unsigned reads{};
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        ++reads;
        if(p>=0x46a0e8u&&std::uint64_t(p-0x46a0e8u)+b.size()<=4u){std::fill(b.begin(),b.end(),std::byte{});return;}
        if(p>=0x41ca40u&&std::uint64_t(p-0x41ca40u)+b.size()<=8u){
            constexpr std::array table{std::byte{0x50},std::byte{0x0a},std::byte{0x2f},std::byte{},
                std::byte{0x60},std::byte{0xb9},std::byte{0x30},std::byte{}};
            std::copy_n(table.begin()+p-0x41ca40u,b.size(),b.begin());return;
        }
        if(p>=0x48c974u&&std::uint64_t(p-0x48c974u)+b.size()<=profile.size()){
            std::copy_n(profile.begin()+p-0x48c974u,b.size(),b.begin());return;
        }
        check(p>=base&&std::uint64_t(p-base)+b.size()<=data.size(),"Missing declared query DATA");
        std::copy_n(data.begin()+p-base,b.size(),b.begin());
    }
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        check(p>=base&&std::uint64_t(p-base)+b.size()<=data.size(),"Missing declared query output");
        std::copy(b.begin(),b.end(),data.begin()+p-base);++writes;
    }
    void put(std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));write(p,b);}
};
Memory initial(){Memory m;
    m.put(sp,0x2f4d3fu);m.put(sp+4u,base+0x100u);m.put(sp+8u,base+0x200u);
    m.put(sp+12u,base+0x1000u);m.put(sp+16u,base+0x1100u);
    for(auto body:{base+0x100u,base+0x200u}){m.put(body,base+0x300u);m.put(body+8u,base+0x500u);}
    m.put(base+0x30cu,base+0x900u);
    for(unsigned axis=0;axis<4;++axis){m.put(base+0x310u+axis*20u,0x3f800000u);m.put(base+0x520u+axis*20u,0x3f800000u);}
    m.put(base+0x80cu,base+0x400u);m.put(base+0x400u,base+0x700u);m.put(base+0x704u,0x8eb50u);
    m.put(base+0x1104u,0xa5a5a5a5u);m.writes=0;return m;
}
SourcePhysicsContactRuntimeAbi abi(){SourcePhysicsContactRuntimeAbi a{};a.gp={11u,base+0x800u,22u,33u,sp,44u,55u,66u};a.eflags=2u;a.fs_base=base+0x1200u;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u};return a;}
NativeTu15OverlapQueryHandoffPolicy policy(){return{base,base+0x10000u,base+0x4000u,0x2000u,true};}
}
int main()try{
    const SourcePhysicsContactRuntimeCall call{0x2f4d3cu,0x2f73c0u,false};
    {
        auto m=initial();auto a=abi();const auto before=a;NativeTu15OverlapQueryHandoffActivity stats;
        check(native_tu15_handoff_overlap_query(m,call,a,policy(),{},{},&stats),"Actual query caller was not owned");
        auto expected=before.gp;expected[4]+=20u;check(a.gp==expected,"Void query changed an ordinary register or RET16");
        check(a.fs_base==before.fs_base&&a.eflags==before.eflags&&a.x87_depth==0&&a.x87_status==0&&
            a.floating_point->x87_control_word==0x27fu&&a.floating_point->mxcsr==0x1f80u,"Native query changed its caller FP/control contract");
        check(stats.attempts==1&&stats.completed==1&&stats.refusals==0&&stats.queries.reversed==1&&
            stats.queries.empty==1&&stats.queries.calls==2&&stats.queries.completed==2&&stats.simplex.attempts==0,
            "No-op child query did not complete its genuine typed path");
        std::array<std::byte,4>flag{};m.read(base+0x1104u,flag);check(flag==std::array{std::byte{0xa5},std::byte{0xa5},std::byte{0xa5},std::byte{0xa5}},"No-op query fabricated a contact");
    }
    unsigned refused{};
    for(unsigned input=0;input<12;++input){
        auto m=initial();auto a=abi();auto p=policy();auto c=call;bool trace=true;
        switch(input){
        case 0:c.instruction=0x2f4d3du;break;case 1:c.tail=true;break;
        case 2:m.put(sp,0x2f4d40u);break;case 3:p.observers_read_only=false;break;
        case 4:p.first_trace_active=&trace;break;case 5:p.second_trace_active=&trace;break;
        case 6:a.x87_depth=1;break;case 7:a.floating_point.reset();break;
        case 8:p.data_first+=1;break;case 9:p.data_bytes=0x1fffu;break;
        case 10:p.data_first=sp-0x1000u;break;case 11:p.stack_end=sp+19u;break;
        }
        m.writes=0;const auto before=a.gp;NativeTu15OverlapQueryHandoffActivity stats;bool caught{};
        try{native_tu15_handoff_overlap_query(m,c,a,p,{},{},&stats);}catch(const std::runtime_error&){caught=true;}
        check(caught&&stats.attempts==1&&stats.refusals==1&&stats.completed==0&&m.writes==0&&a.gp==before,
            "Invalid query caller entered native work or altered its continuation");++refused;
    }
    {
        Memory m;m.put(sp,0x2f6c8bu);m.put(base+0x100u,base+0x200u);
        m.put(base+0x10cu,0xa5a5ffffu);m.put(base+0x200u,0x2e4d60u);m.writes=0;
        auto a=abi();a.gp[1]=base+0x100u;const auto before=a;const auto data=m.data;
        NativeTu15SphereCleanupHandoffActivity activity;
        check(native_tu15_handoff_sphere_cleanup(m,{0x2f6c88u,0x2f9960u,false},a,
            {base,base+0x10000u,true},base+0x4000u,{},{},&activity),"Actual sphere cleanup caller was not owned");
        auto expected=before.gp;expected[4]+=4u;
        check(a.gp==expected&&a.eflags==before.eflags&&a.fs_base==before.fs_base&&
            a.x87_status==before.x87_status&&a.floating_point->x87_control_word==before.floating_point->x87_control_word&&
            a.floating_point->mxcsr==before.floating_point->mxcsr&&m.data==data&&m.writes==0,
            "Sphere FFFF/no-op cleanup changed caller DATA/control/FP");
        check(activity.attempts==1&&activity.completed==1&&!activity.refusals&&!activity.scalar_deletions&&
            !activity.points.attempts,"Sphere no-point path fabricated a removal or deletion");
    }
    for(unsigned input=0;input<15;++input){
        Memory m;m.put(sp,0x2f6c8bu);m.writes=0;auto a=abi();
        SourcePhysicsContactRuntimeCall c{0x2f6c88u,0x2f9960u,false};
        NativeTu15SmallContactFactoryPolicy p{base,base+0x10000u,true};auto data=base+0x4000u;bool trace=true;
        switch(input){
        case 0:c.instruction++;break;case 1:c.tail=true;break;case 2:m.put(sp,0x2f6c8cu);break;
        case 3:p.observers_read_only=false;break;case 4:p.first_trace_active=&trace;break;
        case 5:p.second_trace_active=&trace;break;case 6:a.x87_depth=1;break;
        case 7:a.floating_point.reset();break;case 8:a.active_mmx=SourcePhysicsMmxAlias{};break;
        case 9:p.stack_first=sp+1u;break;case 10:p.stack_end=sp+3u;break;
        case 11:data++;break;case 12:data=sp-0x80u;break;case 13:data=base-0x10u;break;
        case 14:a.gp[4]=base-4u;break;
        }
        m.writes=0;const auto before=a.gp;NativeTu15SphereCleanupHandoffActivity activity;bool caught{};
        try{native_tu15_handoff_sphere_cleanup(m,c,a,p,data,{},{},&activity);}catch(const std::runtime_error&){caught=true;}
        check(caught&&activity.attempts==1&&activity.refusals==1&&!activity.completed&&m.writes==0&&a.gp==before,
            "Invalid sphere cleanup caller entered work or altered its continuation");
    }
    for(const auto distance:{0u,0x3f800000u,0x40000000u,0x40400000u,0x7fc12345u}){
        auto m=initial();m.put(sp,0x2f6faeu);m.put(base+0x30cu,0x3f800000u);
        m.put(base+0x200u,base+0x300u);m.put(base+0x208u,base+0x600u);
        m.put(base+0x650u,distance);m.put(base+0x1100u,base+0x700u);m.put(base+0x704u,0x30ba80u);
        auto a=abi();const auto before=a;NativeTu15OverlapQueryHandoffActivity stats;
        check(native_tu15_handoff_overlap_query(m,{0x2f6fabu,0x2f9ca0u,false},a,policy(),{},{},&stats),
            "Actual sphere query caller was not owned");
        auto expected=before.gp;expected[4]+=20u;
        check(a.gp==expected&&a.eflags==before.eflags&&a.fs_base==before.fs_base,
            "Sphere query altered its void continuation");
        const bool overlap=distance==0u||distance==0x3f800000u;
        check(stats.attempts==1&&stats.completed==1&&!stats.refusals&&stats.queries.sphere==1&&
            stats.queries.marked_pairs==unsigned(overlap)&&!stats.simplex.attempts,
            "Sphere query entered the wrong native owner");
        std::array<std::byte,4>flag{};m.read(base+0x1104u,flag);
        check(flag[0]==std::byte(overlap?1u:0xa5u)&&flag[1]==std::byte{0xa5}&&
            flag[2]==std::byte{0xa5}&&flag[3]==std::byte{0xa5},"Sphere query changed its strict overlap result");
    }
    for(unsigned input=0;input<13;++input){
        auto m=initial();m.put(sp,0x2f6faeu);m.writes=0;auto a=abi();auto p=policy();
        SourcePhysicsContactRuntimeCall c{0x2f6fabu,0x2f9ca0u,false};bool trace=true;
        switch(input){
        case 0:c.instruction=0x2f4d3cu;break;case 1:c.tail=true;break;case 2:m.put(sp,0x2f4d3fu);break;
        case 3:p.observers_read_only=false;break;case 4:p.first_trace_active=&trace;break;
        case 5:p.second_trace_active=&trace;break;case 6:a.x87_depth=1;break;
        case 7:a.floating_point.reset();break;case 8:a.active_mmx=SourcePhysicsMmxAlias{};break;
        case 9:p.data_first++;break;case 10:p.data_bytes=0x1fffu;break;
        case 11:p.stack_end=sp+19u;break;case 12:p.data_first=sp-0x1000u;break;
        }
        m.writes=0;const auto before=a.gp;NativeTu15OverlapQueryHandoffActivity stats;bool caught{};
        try{native_tu15_handoff_overlap_query(m,c,a,p,{},{},&stats);}catch(const std::runtime_error&){caught=true;}
        check(caught&&stats.attempts==1&&stats.refusals==1&&!stats.completed&&m.writes==0&&a.gp==before,
            "Invalid sphere query caller entered native work");
    }
    const SourcePhysicsContactRuntimeCall capsule_call{0x2f1806u,0x2fab70u,false};
    const auto capsule_input=[](std::uint32_t distance){
        auto m=initial();m.put(sp,0x2f180du);
        m.put(base+0x100u,base+0x300u);m.put(base+0x108u,base+0x500u);
        m.put(base+0x200u,base+0x400u);m.put(base+0x208u,base+0x600u);
        m.put(base+0x30cu,0x3f800000u);m.put(base+0x40cu,0x3f800000u);
        for(unsigned n=0;n<8;++n)m.put(base+0x310u+n*4u,0u);
        m.put(base+0x310u,0xbf800000u);m.put(base+0x320u,0x3f800000u);
        m.put(base+0x654u,distance);
        m.put(base+0x1100u,base+0x700u);m.put(base+0x704u,0x30ba80u);
        m.writes=0;m.reads=0;return m;
    };
    for(const auto distance:{0u,0x3f800000u,0x40000000u,0x40400000u,0x7fc12345u}){
        auto m=capsule_input(distance);auto a=abi();const auto before=a;const auto arguments=m.data;
        NativeTu15OverlapQueryHandoffActivity stats;
        check(native_tu15_handoff_overlap_query(m,capsule_call,a,policy(),{},{},&stats),
            "Actual cdecl sphere/capsule query caller was not owned");
        auto expected=before.gp;expected[4]+=4u;
        check(a.gp==expected&&a.eflags==before.eflags&&a.fs_base==before.fs_base,
            "Sphere/capsule cdecl query changed a register or removed caller-owned arguments");
        check(std::equal(arguments.begin()+sp-base+4u,arguments.begin()+sp-base+20u,
            m.data.begin()+sp-base+4u),"Sphere/capsule query changed its four ordinary caller arguments");
        const bool overlap=distance==0u||distance==0x3f800000u;
        check(stats.attempts==1&&stats.completed==1&&!stats.refusals&&
            stats.queries.sphere_capsule==1&&stats.queries.reversed_sphere_capsule==1&&
            stats.queries.marked_pairs==unsigned(overlap)&&stats.queries.reversed_pair_callbacks==unsigned(overlap)&&
            !stats.simplex.attempts,"Sphere/capsule query entered the wrong owner or fabricated a solver result");
        std::array<std::byte,4>flag{};m.read(base+0x1104u,flag);
        check(flag[0]==std::byte(overlap?1u:0xa5u)&&flag[1]==std::byte{0xa5}&&
            flag[2]==std::byte{0xa5}&&flag[3]==std::byte{0xa5},"Sphere/capsule query changed strict overlap or collector padding");
    }
    for(unsigned input=0;input<16;++input){
        auto m=capsule_input(0u);auto a=abi();auto p=policy();auto c=capsule_call;bool trace=true;
        switch(input){
        case 0:c.instruction++;break;case 1:c.tail=true;break;case 2:m.put(sp,0x2f1809u);break;
        case 3:p.observers_read_only=false;break;case 4:p.first_trace_active=&trace;break;
        case 5:p.second_trace_active=&trace;break;case 6:a.x87_depth=1;break;
        case 7:a.floating_point.reset();break;case 8:a.active_mmx=SourcePhysicsMmxAlias{};break;
        case 9:p.data_first++;break;case 10:p.data_bytes=0x1fffu;break;
        case 11:p.stack_end=sp+19u;break;case 12:p.data_first=sp-0x1000u;break;
        case 13:p.data_first=base-16u;break;case 14:p.stack_first=sp+1u;break;
        case 15:c.instruction=0x2f6fabu;break;
        }
        m.writes=0;const auto before=a.gp;NativeTu15OverlapQueryHandoffActivity stats;bool caught{};
        try{native_tu15_handoff_overlap_query(m,c,a,p,{},{},&stats);}catch(const std::runtime_error&){caught=true;}
        check(caught&&stats.attempts==1&&stats.refusals==1&&!stats.completed&&
            !stats.queries.calls&&m.writes==0&&a.gp==before,
            "Invalid cdecl sphere/capsule caller entered DATA work or changed its continuation");
    }
    for(const auto excluded:{0xffffffffu,7u,8u}){
        auto m=capsule_input(0x3f800000u);
        // The failing live World supplied a30B960 adapter whose actual
        // destination was4615B8/1CDFA0. Preserve that two-reversal chain.
        m.put(base+0x1100u,0x41ca40u);m.put(base+0x1108u,base+0x1200u);
        m.put(base+0x1200u,base+0x1300u);m.put(base+0x1304u,0x1cdfa0u);
        m.put(base+0x1204u,0xa5a5a5a5u);m.put(base+0x1208u,0xa5a5a5a5u);m.put(base+0x120cu,excluded);
        for(unsigned n=0;n<2;++n){
            const auto descriptor=base+0x100u+n*0x100u,body=base+0x1400u+n*0x200u;
            m.put(descriptor+12u,0);m.put(descriptor+0x18u,1);m.put(descriptor+0x20u,body);
            m.put(body+0x30u,body+0x100u);m.put(body+0x34u,1);
            m.put(body+0x100u,0x2001u);m.put(body+0x104u,7u+n);
        }
        m.put(base+0x300u,base+0x1800u);m.put(base+0x1814u,0x31ede0u);m.put(base+0x308u,0);
        m.put(base+0x400u,base+0x1900u);m.put(base+0x1914u,0x2e8cf0u);m.put(base+0x408u,0);
        auto a=abi();const auto before=a.gp;NativeTu15OverlapQueryHandoffActivity activity;
        check(native_tu15_handoff_overlap_query(m,capsule_call,a,policy(),{},{},&activity),
            "Actual object-overlap collector chain was not owned");
        auto expected=before;expected[4]+=4u;check(a.gp==expected,"Object collector changed cdecl caller ownership");
        const bool accepted=excluded!=7u;
        std::array<std::byte,4>hit{},dirty{};m.read(base+0x1204u,hit);m.read(base+0x1208u,dirty);
        check(hit[0]==std::byte(accepted?1u:0xa5u)&&dirty[0]==std::byte(accepted?1u:0xa5u)&&
            hit[1]==std::byte{0xa5}&&dirty[1]==std::byte{0xa5},
            "Object collector lost the actual reversed second component or changed padding");
        check(activity.queries.reversed_pair_callbacks==2&&activity.queries.object_pair_callbacks==1&&
            !activity.queries.marked_pairs&&!activity.refusals,"Object overlap used a substitute result collector");
    }
    {
        auto m=capsule_input(0u);auto a=abi();const auto before=a;NativeTu15OverlapQueryHandoffActivity stats;
        check(!native_tu15_handoff_overlap_query(m,{0x2f1806u,0x2fab71u,false},a,policy(),{},{},&stats)&&
            !stats.attempts&&!m.reads&&!m.writes&&a.gp==before.gp,
            "Unknown overlap query owner was admitted or read ordinary inputs");
        NativeTu15OverlapQueryBindingStats queries;NativeTu15OverlapQueryBinding binding(m,
            {base+0x4000u,512u},{},{},&queries);bool caught{};
        NativeTu15PhysicsFrameFp fp;
        try{binding.query({0},{0x2fab71u},{1},{2},{3},{4},fp);}
        catch(const std::runtime_error&){caught=true;}
        check(caught&&queries.calls==1&&queries.refusals==1&&!queries.completed&&!m.reads&&!m.writes,
            "Unknown typed child query entered DATA work");
    }
    std::cout<<"Native query RET16/no-op and "<<refused<<" query controls; sphere cleanup and15controls; sphere overlap5cases and13caller controls; sphere/capsule cdecl5cases,16controls andunknownowners pass\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
