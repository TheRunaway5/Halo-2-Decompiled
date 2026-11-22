#include "halo2/native_campaign_ai_behavior_129_perform_tu15.hpp"
#include "halo2/native_campaign_ai_behavior_129_probe_tu15.hpp"
#include "halo2/native_campaign_ai_actor_path_query_tu15.hpp"
#include "halo2/native_campaign_ai_navigation_filter_tu15.hpp"
#include "halo2/native_campaign_ai_path_arena_tu15.hpp"
#include "halo2/native_campaign_ai_path_workspace_tu15.hpp"
#include "halo2/native_campaign_ai_path_search_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <vector>
namespace halo2 {
namespace {
using U=std::uint32_t;
struct Perform {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;
    const SourcePhysicsContactRecordObserver &observe;
    U r(U at,unsigned width=4)const {std::array<std::byte,4>b{};memory.read(at,std::span(b).first(width));U v{};for(unsigned i=0;i<width;++i)v|=std::to_integer<U>(b[i])<<(i*8u);return v;}
    void w(U pc,U at,U value,unsigned width=4)const {
        const bool watched=native_tu15_observes_retained_store(memory,observe,pc,at,width);
        SourcePhysicsContactRecordStore e{pc,at,{},std::vector<std::byte>(width)};
        if(watched){e.before.resize(width);memory.read(at,e.before);}
        for(unsigned i=0;i<width;++i)e.written[i]=std::byte(value>>(i*8u));
        memory.write(at,e.written);if(watched)observe(e);
    }
    void push(U &sp,U value)const {sp-=4u;std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(i*8u));memory.write(sp,b);}
    U acquire(U sp,U pc)const {
        push(sp,pc+5u);
        return services.protection?native_campaign_ai_path_workspace_acquire_tu15(memory,*services.protection,observe):native_campaign_ai_path_workspace_acquire_tu15(memory,observe);
    }
    void release(U sp,U pc,U workspace)const {push(sp,pc+5u);native_campaign_ai_path_workspace_release_tu15(memory,workspace,observe);}
    void child(U pc,U target,std::array<U,8> gp,std::initializer_list<U> arguments={}) {
        const auto caller=gp[4];for(auto it=arguments.end();it!=arguments.begin();)push(gp[4],*--it);push(gp[4],pc+5u);abi.gp=gp;
        if(target==0x26a2e0u)native_campaign_ai_goal_selection_call_tu15(memory,abi,services,observe);
        else if(target==0x275e90u||target==0x2013b0u)native_campaign_ai_actor_path_query_call_tu15(memory,target,abi,observe);
        else if(target==0x27b2b0u)native_campaign_ai_path_search_call_tu15(memory,abi,observe);
        else if(target==0x1ca900u||target==0x1caa20u)native_campaign_ai_behavior_129_probe_call_tu15(memory,target,abi,services,observe);
        else throw NativeCampaignAiPerformRequiredOwnerTu15(pc,target);
        if(abi.gp[4]!=caller)throw std::runtime_error("Native behavior129 child returned another stack");
    }
    void run(const std::array<U,8> &saved) {
        U sp=saved[4];push(sp,saved[5]);const U bp=sp;sp=(sp&~7u)-0x7d4u;
        const auto pool=r(0x5020c8u),rows=r(pool+0x44u);push(sp,saved[3]);push(sp,saved[6]);
        const auto actor=r(bp+8u),record=rows+(actor&0xffffu)*0x888u;
        const auto disabled=r(record+0x605u,1);push(sp,saved[7]);const U f=sp;
        if(disabled||r(record+0x264u,1)||!r(record+0x40u,1))return;
        w(0x1cad35u,record+0x5e8u,1,2);
        const auto workspace=acquire(f,0x1cad3cu);
        for(U n=0;n<0x1d6u;++n)w(0x1cad51u,f+0x88u+n*4u,0);
        // The four arguments and stores are interleaved in the original.
        U call=f;push(call,f+0x13u);push(call,workspace);w(0x1cad5cu,f+0x14u,workspace);
        push(call,f+0x18u);w(0x1cad65u,f+0x9du,1,1);push(call,0);
        w(0x1cad74u,f+0x13u,0,1);w(0x1cad79u,f+0x88u,4,2);
        push(call,0x1cad88u);abi.gp={0,f+0x13u,f+0x18u,f+0x88u,call,bp,record,actor};
        native_campaign_ai_goal_selection_call_tu15(memory,abi,services,observe);
        if(abi.gp[4]!=f)throw std::runtime_error("Native behavior129 goal selection returned another stack");
        if(!r(record+0x5e8u,2))w(0x1cad95u,r(bp+0xcu)+0x10u,1,1);
        release(f,0x1cad9du,r(f+0x14u));
        auto state=r(bp+0xcu);if(r(state+0x10u,1))return;
        const auto who=r(bp+8u);
        child(0x1cadb5u,0x275e90u,{abi.gp[0],state,who,f+0x88u,f,bp,who,actor});
        call=f;push(call,who);push(call,0x1cadc4u);
        native_campaign_ai_navigation_filter_tu15(memory,who,f+0x1cu,observe);
        child(0x1cadc9u,0x2013b0u,{abi.gp[0],abi.gp[1],abi.gp[2],f+0x38u,f,bp,who,f+0x1cu},{who});
        abi.xmm[0]={r(0x46a118u),0,0,0};w(0x1cadd6u,f+0x7du,1,1);w(0x1caddbu,f+0x80u,abi.xmm[0][0]);
        const auto path=acquire(f,0x1cade4u);
        call=f;push(call,0);push(call,f+0x38u);push(call,0x1cadf9u);
        // 1CADF4 passes EBX as the transport words and clears ECX (policy).
        NativeCampaignAiPathMemoryWordsTu15 query(memory,f+0x38u),transport(memory,f+0x1cu);
        native_campaign_ai_path_workspace_initialize_tu15(memory,path,query,nullptr,&transport,0,observe);
        child(0x1cadf9u,0x27b2b0u,{path,0,f+0x38u,f+0x1cu,f,bp,path,f+0x1cu});
        const bool found=(abi.gp[0]&0xffu)!=0;state=r(bp+0xcu);
        if(!(r(state+0xcu,1)&1u)) {
            const auto who_again=r(bp+8u),selected=found?path:0u;
            child(0x1cae16u,0x1ca900u,{selected,who_again,abi.gp[2],((f+0x1cu)&0xffffff00u)|U(found),f,bp,path,state},{who_again,selected});
            const auto flags=r(state+0xcu)|1u;w(0x1cae23u,state+0xcu,flags);
            if(abi.gp[0]&0xffu){release(f,0x1cae57u,path);return;}
        }
        if(!(r(state+0xcu,1)&4u)) {
            const auto who_again=r(bp+8u);child(0x1cae37u,0x1caa20u,{abi.gp[0],abi.gp[1],abi.gp[2],found?path:0u,f,bp,path,who_again});
            state=r(bp+0xcu);const auto flags=r(state+0xcu)|4u;w(0x1cae47u,state+0xcu,flags);
            if(abi.gp[0]&0xffu){release(f,0x1cae57u,path);return;}
        }
        w(0x1cae4eu,state+0xcu,0);release(f,0x1cae57u,path);
    }
};
}
void native_campaign_ai_behavior_129_perform_call_tu15(SourcePhysicsContactMemory &memory,U entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0x1cace0u)throw std::invalid_argument("Unknown behavior129 perform entry");
    if(!abi.floating_point||abi.active_mmx||abi.x87_depth||abi.eflags&0x400u)
        throw std::invalid_argument("Native behavior129 perform requires forward DF, retained FP and empty x87");
    const auto saved=abi.gp;Perform{memory,abi,services,observe}.run(saved);
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;
    for(unsigned i:{3u,5u,6u,7u})abi.gp[i]=saved[i];abi.gp[4]=saved[4]+12u;
}
}
