#include "halo2/native_campaign_ai_goal_evaluation_tu15.hpp"
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_ai_goal_scoring_tu15.hpp"
#include "halo2/native_campaign_ai_navigation_filter_tu15.hpp"
#include "halo2/native_campaign_ai_path_result_tu15.hpp"
#include "halo2/native_campaign_ai_path_search_tu15.hpp"
#include "halo2/native_campaign_ai_path_workspace_tu15.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include <array>
#include <optional>
#include <sstream>

namespace halo2 {
namespace {
using namespace goal_tu15;
constexpr u32 none_cell=0x47d6c0u,actor_pool=0x5020c8u;

// 26A380's private frame locals (ESP after its four pushes = entry - 8B0).
struct Locals {
    u32 goal{};             // +18: the NONE goal, then the chosen goal datum
    u32 best{0xffffffffu};  // +1C: chosen candidate index (word)
    u32 row{};              // +20: actor row
    u32 offset{};           // +24: actor row offset
    u32 best_score{};       // +28: best evaluated score (0.0)
    bool near{},far{};      // +16/+17
};

// CALL/PUSH words land in the real stack without an operand observer.
void stack_store(SourcePhysicsContactMemory &memory,u32 address,u32 value) {
    std::array<std::byte,4> raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
    memory.write(address,raw);
}
u32 stack_word(const SourcePhysicsContactMemory &memory,u32 address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);u32 value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);
    return value;
}

// 26A380 through 26A70D: the prologue stores, the no-candidate exit and the
// request-shape branches. Returns the goal when 26A380 is complete.
std::optional<u32> prefix(const Port &p,const NativeCampaignAiGoalEvaluationTu15 &call,Locals &locals) {
    const auto pool=p.r32(actor_pool);
    const auto offset=(call.actor&0xffffu)*0x888u;
    locals.row=p.r32(pool+0x44u)+offset;locals.offset=offset;
    locals.goal=p.r32(none_cell);
    p.w8(0x26a3ceu,call.flag_out,0);
    const auto ai_globals=p.r32(0x5020a8u);
    p.w8(0x26a3f2u,ai_globals+3u,1);
    if(!(call.count&0xffffu)) {
        // No candidates: forget the goal history and return NONE.
        const auto row=p.r32(pool+0x44u)+offset;
        p.w16(0x26a3fdu,row+0x3feu,3);
        for(u32 n=0;n<4;++n){const auto none=p.r32(none_cell);p.w32(0x26a416u,row+0x402u+n*6u,none);}
        return p.r32(none_cell);
    }
    const auto request=call.request;
    if(p.r8(request+0x618u))throw NativeCampaignAiPerformRequiredOwnerTu15(0x26a438u,0x26a43eu);
    if(p.r8(request+0x54u))throw NativeCampaignAiPerformRequiredOwnerTu15(0x26a5b5u,0x26a5b7u);
    if(p.r8(request+0x55u))throw NativeCampaignAiPerformRequiredOwnerTu15(0x26a628u,0x26a62eu);
    if(p.r8(request+0x56u))throw NativeCampaignAiPerformRequiredOwnerTu15(0x26a6fbu,0x26a846u);
    return std::nullopt;
}

#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
// 140DB0 on the [56AEB0] owner table (only the original 267A30 hash and
// 267A40 equality callbacks are native), then 26AE4A's REP MOVSD/MOVSB of
// the entry value into 26A380's [esp+10] local. The copy runs over the
// frame window [esp+10, esp+28): the owner (+10, NONE before the copy),
// dead locals (+14..+17), the goal (+18), the index (+1C), the actor row
// (+20) and the chosen candidate (+24). Its later reads see what the copy
// left; values longer than the window are a named refusal.
void owner_lookup(const Port &p,u32 goal,std::array<std::byte,0x18> &window) {
    const auto table=p.r32(0x56aeb0u);
    const auto hash_owner=p.r32(table+0x2cu);
    if(hash_owner!=0x267a30u)throw NativeCampaignAiPerformRequiredOwnerTu15(0x140db8u,hash_owner);
    const auto hash=(goal&0xffu)<<2;
    const auto buckets=p.r32(table+0x20u);
    if(!buckets)throw std::runtime_error("Native 26A380 refuses the 140DBF divide by a zero owner-table bucket count");
    auto node=p.r32(table+(hash%buckets)*4u+0x3cu);
    for(;node;node=p.r32(node+8u)) {
        if(hash!=p.r32(node+4u))continue;
        const auto key=p.r32(node);
        const auto equal_owner=p.r32(table+0x30u);
        if(equal_owner!=0x267a40u)throw NativeCampaignAiPerformRequiredOwnerTu15(0x140dd9u,equal_owner);
        if(key==goal)break;
    }
    if(!node)return;
    const auto size=p.r32(table+0x28u);
    if(size>window.size()) {
        std::ostringstream text;text<<"Native 26A380 refuses a 26AE59 owner value of "<<size<<" bytes that overruns its frame locals";
        throw std::runtime_error(text.str());
    }
    u32 at=0;
    for(u32 n=0;n<size/4u;++n,at+=4u)p.memory.read(node+0xcu+at,std::span(window).subspan(at,4));
    for(u32 n=0;n<size%4u;++n,++at)p.memory.read(node+0xcu+at,std::span(window).subspan(at,1));
}
u32 window_word(const std::array<std::byte,0x18> &window,u32 offset) {
    u32 value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<u32>(window[offset+n])<<(n*8u);
    return value;
}
void window_store(std::array<std::byte,0x18> &window,u32 offset,u32 value) {
    for(unsigned n=0;n<4;++n)window[offset+n]=std::byte(value>>(n*8u));
}

// 2013B0 with its CALL words in the real stack, or the named refusal.
void path_query(const Port &p,const NativeCampaignAiGoalEvaluationContextTu15 &context,u32 site,u32 actor,
    NativeTu15PhysicsFrameFp &fp) {
    if(!context.path_query)throw NativeCampaignAiPerformRequiredOwnerTu15(site,0x2013b0u);
    const auto esp=context.stack-0x8b0u;
    stack_store(p.memory,esp-4u,actor);stack_store(p.memory,esp-8u,site+5u);
    context.path_query(p.memory,actor,esp+0x44u,esp-8u,fp,p.observe);
}

// 26A712 through 26AF82, after 2013B0 returned.
u32 remainder(const Port &p,const Simd &s,NativeTu15PhysicsFrameFp &fp,const NativeCampaignAiGoalEvaluationTu15 &call,
    const NativeCampaignAiGoalEvaluationContextTu15 &context,u32 request,Locals &locals) {
    const auto esp=context.stack-0x8b0u,query=esp+0x44u,filter=esp+0x94u,indices=esp+0xb0u;
    const auto row=locals.row;
    // 26A712: the query's goal fields.
    const auto wide=p.r8(request+0x46u);
    const auto reach=p.r32(request+0x1cu);
    p.w8(0x26a71eu,query+0x45u,1);
    p.w32(0x26a725u,query+0x48u,reach);
    if(wide && p.r8(request+0x618u)) {
        const auto target=p.r32(request+0x664u);
        u32 sector=0xffffffffu;
        if(target!=0xffffffffu) {
            const auto props=p.r32(0x50eef0u),rows=p.r32(props+0x44u);
            sector=p.r32((target&0xffffu)*0x3cu+rows+0x20u);
        }
        const auto range=p.r32(request+0x4cu);
        p.w8(0x26a766u,query+0x28u,1);
        const auto x=p.r32(request+0x620u);p.w32(0x26a772u,query+0x2cu,x);
        const auto y=p.r32(request+0x624u),z=p.r32(request+0x628u);
        p.w32(0x26a77cu,query+0x30u,y);
        const auto exact=p.r8(request+0x50u);
        p.w32(0x26a788u,query+0x3cu,range);
        const auto spread=p.r32(request+0x48u);
        p.w32(0x26a796u,query+0x34u,z);p.w32(0x26a79au,query+0x38u,sector);
        p.w8(0x26a79eu,query+0x44u,exact==0);
        p.w32(0x26a7fdu,query+0x40u,spread);
    } else if(sx16(p.r16(row+0x358u))>0) {
        const auto range=p.r32(row+0x36cu);
        const auto x=p.r32(row+0x370u);p.w32(0x26a7c5u,query+0x2cu,x);
        const auto y=p.r32(row+0x374u),z=p.r32(row+0x378u);
        p.w32(0x26a7cfu,query+0x30u,y);
        const auto sector=p.r32(row+0x360u);
        p.w32(0x26a7d9u,query+0x3cu,range);
        const auto spread=p.r32(0x46a110u);
        p.w8(0x26a7eau,query+0x28u,1);
        p.w32(0x26a7eeu,query+0x34u,z);p.w32(0x26a7f2u,query+0x38u,sector);
        p.w8(0x26a7f6u,query+0x44u,1);
        p.w32(0x26a7fdu,query+0x40u,spread);
    }
    // 26A806: 201500 navigation filter into ESP+94, 27B010, 27B2B0.
    stack_store(p.memory,esp-4u,call.actor);stack_store(p.memory,esp-8u,0x26a81au);
    native_campaign_ai_navigation_filter_tu15(p.memory,call.actor,filter,p.observe);
    stack_store(p.memory,esp-4u,0);stack_store(p.memory,esp-8u,query);stack_store(p.memory,esp-12u,0x26a831u);
    {
        const NativeCampaignAiPathMemoryWordsTu15 main(p.memory,query),transport(p.memory,filter);
        native_campaign_ai_path_workspace_initialize_tu15(p.memory,call.workspace,main,nullptr,&transport,0,p.observe);
    }
    stack_store(p.memory,esp-4u,0x26a836u);
    if(native_campaign_ai_path_search_tu15(p.memory,call.workspace,esp-4u,fp,p.observe))p.w8(0x26a841u,call.flag_out,1);
    const auto count=call.count;
    // 26A84A: candidate distances and path costs.
    if(sx16(count)>0) {
        for(u32 k=0;k<(count&0xffffu);++k) {
            const auto c=call.candidates+k*0x78u;
            if(p.r8(request+0x618u)) {
                const auto cx=p.r32(c+0xcu),a=s.sub(cx,p.r32(request+0x620u));
                const auto cz=p.r32(c+0x14u),b=s.sub(cz,p.r32(request+0x628u));
                const auto cy=p.r32(c+0x10u),d=s.sub(cy,p.r32(request+0x624u));
                auto sum=s.mul(a,a);sum=s.add(sum,s.mul(b,b));sum=s.add(sum,s.mul(d,d));
                p.w32(0x26a8bdu,c+0x30u,sum);
            }
            const auto cx=p.r32(c+0xcu),x6=s.sub(cx,p.r32(row+0x238u));
            const auto cz=p.r32(c+0x14u),x5=s.sub(cz,p.r32(row+0x240u));
            const auto cy=p.r32(c+0x10u),x4=s.sub(cy,p.r32(row+0x23cu));
            auto d2=s.mul(x6,x6);d2=s.add(d2,s.mul(x5,x5));d2=s.add(d2,s.mul(x4,x4));
            const auto radius=p.r32(request+0x1cu);
            if(s.above(s.mul(radius,radius),d2)) {
                bool direct=p.r8(request+0x56u)!=0;
                if(!direct && p.r8(c+0x58u))direct=p.r8(request+0x5au)!=0;
                if(!direct && p.r8(c+0x59u))direct=p.r8(request+0x58u)!=0;
                if(!direct) {
                    // 26A979: 27A460 path cost, clearance and (+51) direction.
                    const auto steer=p.r8(request+0x51u);
                    const auto record=p.r32(c);
                    const auto key=p.r32(record+0x14u);
                    (void)native_campaign_ai_path_result_tu15(p.memory,call.workspace,key,record,c+0x18u,c+0x2cu,
                        steer?c+0x1cu:0u,fp,context.occupied_x87,p.observe);
                } else {
                    // 26A983: straight-line distances to the request target.
                    u32 squared;
                    if(s.above(d2,p.r32(0x46a0ecu))) {
                        const auto ty=p.r32(request+0x624u),tz=p.r32(request+0x628u);
                        const auto x3=s.sub(ty,p.r32(row+0x23cu));
                        const auto x7=s.sub(tz,p.r32(row+0x240u));
                        const auto tx=p.r32(request+0x620u),x2=s.sub(tx,p.r32(row+0x238u));
                        auto t=s.mul(x7,x5);t=s.add(t,s.mul(x3,x4));t=s.add(t,s.mul(x2,x6));t=s.div(t,d2);
                        u32 clamped;
                        if(s.above(0u,t))clamped=0u;
                        else {const auto one=p.r32(0x46a0d0u);clamped=s.above(t,one)?one:t;}
                        const auto negative=s.sub(0u,clamped);
                        const auto px=s.add(s.mul(negative,x6),x2);
                        const auto py=s.add(s.mul(x4,negative),x3);
                        const auto pz=s.add(s.mul(x5,negative),x7);
                        auto q=s.mul(pz,pz);const auto yy=s.mul(py,py);
                        q=s.add(q,yy);squared=s.add(q,s.mul(px,px));
                    } else {
                        const auto az=p.r32(row+0x240u),bz=s.sub(az,p.r32(request+0x628u));
                        const auto ay=p.r32(row+0x23cu),by=s.sub(ay,p.r32(request+0x624u));
                        const auto ax=p.r32(row+0x238u),bx=s.sub(ax,p.r32(request+0x620u));
                        auto q=s.mul(bz,bz);q=s.add(q,s.mul(by,by));squared=s.add(q,s.mul(bx,bx));
                    }
                    p.w32(0x26aaa1u,c+0x2cu,X87::root(fp,squared));
                    const auto eps=p.r32(0x46a0ecu);
                    const auto [root,tiny]=X87::root_eps_above_abs(fp,d2,eps);
                    u32 length=0u;std::array<u32,3> direction{x6,x4,x5};
                    if(!tiny) {
                        const auto one=p.r32(0x46a0d0u),inverse=s.div(one,root);
                        direction={s.mul(inverse,x6),s.mul(x4,inverse),s.mul(x5,inverse)};
                        length=root;
                    }
                    p.w32(0x26aaf4u,c+0x18u,length);
                    if(p.r8(request+0x51u)) {
                        p.w32(0x26ab0bu,c+0x1cu,direction[0]);
                        p.w32(0x26ab11u,c+0x20u,direction[1]);
                        p.w32(0x26ab14u,c+0x24u,direction[2]);
                    }
                }
            }
            // 26AB17: inside the request radius, or beyond 45695C.
            const auto limit=p.r32(request+0x1cu);
            if(s.above(limit,p.r32(c+0x18u)))locals.near=true;
            else {
                const auto far_limit=p.r32(0x45695cu);
                const bool beyond=s.above(far_limit,p.r32(c+0x18u));
                p.w8(0x26ab35u,c+0x4cu,0);
                if(beyond)locals.far=true;
            }
        }
    }
    // 26AB52: the actor's +5E8 failed-evaluation count.
    bool reset=locals.near || locals.far || p.r8(request+0x56u) || p.r8(row+0x264u);
    if(!reset && p.r32(row+0x28cu)!=0xffffffffu)
        reset=!p.r8(call.flag_out) || sx16(count)<=0 || sx16(p.r16(call.workspace+0xaeu))>=8;
    if(reset)p.w16(0x26aba7u,row+0x5e8u,0);
    else {const auto failures=p.r16(row+0x5e8u);p.w16(0x26ab9eu,row+0x5e8u,failures+1u);}
    if(p.r8(request+0x15u) && !locals.near) {
        // 26ABC7: nothing in reach; one random candidate, history reset.
        const auto state=p.r32(0x4f3cd8u);
        const auto next=p.r32(state)*0x19660du+0x3c6ef35fu;
        p.w32(0x26abe1u,state,next);
        locals.best=((next>>16)*u32(sx16(count)))>>16;
        const auto pool=p.r32(actor_pool);
        p.w8(0x26abfcu,call.flag_out,0);
        const auto history=p.r32(pool+0x44u)+locals.offset;
        p.w16(0x26ac04u,history+0x3feu,3);
        for(u32 n=0;n<4;++n){const auto none=p.r32(none_cell);p.w32(0x26ac26u,history+0x402u+n*6u,none);}
        const auto candidate=u32(sx16(locals.best))*0x78u+call.candidates;
        if(!native_campaign_ai_admit_candidate_tu15(p.memory,call.actor,request,candidate,fp,p.observe))return locals.goal;
    } else {
        // 26AC5A: score, sort and evaluate in rank order.
        native_campaign_ai_score_candidates_tu15(p.memory,call.actor,request,count,call.candidates,fp,p.observe);
        const auto ranked=sx16(count);
        for(i32 n=0;n<ranked;++n)p.w32(0x26ac90u,indices+u32(n)*4u,u32(n));
        p.w16(0x26acafu,0x52d874u,count);
        p.w32(0x26acb6u,0x52d878u,call.candidates);
        if(ranked<0)throw std::runtime_error("Native 26A380 refuses a negative 140910 candidate count");
        native_campaign_ai_sort_candidates_tu15(p.memory,indices,u32(ranked),0x269df0u,esp+0x24u,fp,p.observe);
        p.w32(0x26acc6u,request+0x680u,0);
        const bool admitted=native_campaign_ai_evaluate_candidate_tu15(p.memory,call.actor,request,0,fp,p.observe,true);
        p.w8(0x26ad08u,request+0x67cu,admitted);
        if(ranked<=0)return locals.goal;
        for(u32 n=0;;) {
            const auto index=p.r16(indices+u32(sx16(n))*4u);
            const auto c=u32(sx16(index))*0x78u+call.candidates;
            if(!p.r8(c+0x4cu))break;
            if(p.r8(request+0x67cu)) {
                const auto score=p.r32(c+0x54u),total=s.add(score,p.r32(request+0x680u));
                if(s.above_equal(locals.best_score,total))break;
            }
            if(p.r8(request+0x618u))throw NativeCampaignAiPerformRequiredOwnerTu15(0x26ad7eu,0x2694c0u);
            const auto score=p.r32(c+0x54u);
            p.w32(0x26ad90u,c+0x50u,score);
            if(native_campaign_ai_evaluate_candidate_tu15(p.memory,call.actor,request,c,fp,p.observe)) {
                const auto value=p.r32(c+0x54u);
                if(s.above(value,locals.best_score)){locals.best=index;locals.best_score=value;}
            }
            ++n;
            if(!(sx16(n)<ranked))break;
        }
    }
    // 26ADC9: publish the chosen candidate.
    if(u16(locals.best)==0xffffu)return locals.goal;
    const auto chosen=u32(sx16(locals.best))*0x78u+call.candidates;
    if(call.candidate_out)
        for(u32 n=0;n<0x1eu;++n){const auto word=p.r32(chosen+n*4u);p.w32(0x26adfdu,call.candidate_out+n*4u,word);}
    locals.goal=p.r32(chosen+4u);
    auto pinned_row=row,pinned_choice=chosen;
    if(call.owner_out) {
        std::array<std::byte,0x18> window{};
        window_store(window,0,0xffffffffu);window_store(window,8,locals.goal);window_store(window,0xc,locals.best);
        window_store(window,0x10,row);window_store(window,0x14,chosen);
        owner_lookup(p,locals.goal,window);
        const auto owner=window_word(window,0);
        locals.goal=window_word(window,8);pinned_row=window_word(window,0x10);pinned_choice=window_word(window,0x14);
        if(owner!=call.actor)p.w32(0x26ae76u,call.owner_out,owner);
        else p.w32(0x26ae81u,call.owner_out,0xffffffffu);
    }
    if(p.r8(pinned_row+0x3f2u) && !p.r8(pinned_row+0x220u) && !p.r8(pinned_choice+0x5bu))p.w8(0x26aeaau,pinned_row+0x3f2u,0);
    if(p.r8(request+0x56u)) {
        // 26AEBC: a second path query, then the workspace for its search.
        path_query(p,context,0x26aec8u,call.actor,fp);
        const auto ws=call.workspace;
        p.w8(0x26aed6u,ws+0x54u,0);p.w16(0x26aed9u,ws+0x90u,0xffffu);p.w16(0x26aee2u,ws+0xaeu,0);
        p.w16(0x26aee9u,ws+0x110b0u,0);p.w16(0x26aef0u,ws+0x70u,0);p.w16(0x26aef4u,ws+0x72u,0);
        const auto bsp=p.r32(0x4ecc18u);
        const auto clusters=i32(p.r32(bsp+0xc4u));
        const auto navigation=clusters>0?p.r32(bsp+0xc8u):0u;
        p.w32(0x26af16u,ws+0x8cu,navigation);p.w32(0x26af1cu,ws+0x50u,0);p.w8(0x26af1fu,ws+0xacu,0);
        for(u32 n=0;n<0x14u;++n){const auto word=p.r32(query+n*4u);p.w32(0x26af30u,ws+n*4u,word);}
        constexpr std::array<u32,7> sites{0x26af3au,0x26af3cu,0x26af3fu,0x26af42u,0x26af45u,0x26af48u,0x26af4bu};
        for(u32 n=0;n<7;++n)p.w32(sites[n],ws+0x140b8u+n*4u,0);
        p.w16(0x26af4eu,ws+0x14188u,0);
        for(u32 n=0;n<0x2du;++n){const auto word=p.r32(request+0x6a0u+n*4u);p.w32(0x26af6du,ws+0x140d4u+n*4u,word);}
        {const auto half=p.r16(request+0x6a0u+0xb4u);p.w16(0x26af6fu,ws+0x140d4u+0xb4u,half);}
        p.w8(0x26af71u,call.flag_out,1);
    }
    return locals.goal;
}
#endif
u32 evaluate(SourcePhysicsContactMemory &memory,const NativeCampaignAiGoalEvaluationTu15 &call,
    const NativeCampaignAiGoalEvaluationContextTu15 &context,const SourcePhysicsContactRecordObserver &observe) {
    const Port port{memory,observe};Locals locals;
    if(const auto goal=prefix(port,call,locals))return *goal;
    if(!context.path_query)throw NativeCampaignAiPerformRequiredOwnerTu15(0x26a70du,0x2013b0u);
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if(!context.fp)throw std::invalid_argument("Native 26A380 requires the caller's FP controls");
    auto &fp=*context.fp;
    path_query(port,context,0x26a70du,call.actor,fp);
    const Simd simd(fp);
    return remainder(port,simd,fp,call,context,call.request,locals);
#else
    throw std::runtime_error("Exact native 26A380 arithmetic backend unavailable");
#endif
}
}

std::uint32_t native_campaign_ai_evaluate_goals_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiGoalEvaluationTu15 &call,const NativeCampaignAiGoalEvaluationContextTu15 &context,
    const SourcePhysicsContactRecordObserver &observe) {
    return evaluate(memory,call,context,observe);
}
std::uint32_t native_campaign_ai_evaluate_goals_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiGoalEvaluationTu15 &call,const SourcePhysicsContactRecordObserver &observe) {
    return evaluate(memory,call,{},observe);
}

void native_campaign_ai_evaluate_goals_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    const auto sp=abi.gp[4];
    std::array<std::uint32_t,8> words{};
    for(unsigned n=0;n<8;++n)words[n]=stack_word(memory,sp+4u+n*4u);
    const NativeCampaignAiGoalEvaluationTu15 call{words[0],words[1],words[2],words[3],words[4],words[5],words[6],words[7]};
    const bool empty=!(call.count&0xffffu);
    const auto goal=native_campaign_ai_evaluate_goals_tu15(memory,call,observe);
    abi.gp[0]=goal;abi.gp[4]=sp+0x24u;
    if(empty){abi.gp[1]=0;abi.gp[2]=goal;abi.xmm[0]={0,0,0,0};}
}

void native_campaign_ai_evaluate_goals_resume_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>4u)
        throw std::runtime_error("Native 26A712 requires retained FP controls and four free x87 temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication{SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}} publication{abi,fp};
    const auto esp=abi.gp[4];
    std::array<std::uint32_t,8> words{};
    for(unsigned n=0;n<8;++n)words[n]=stack_word(memory,esp+0x8b4u+n*4u);
    const NativeCampaignAiGoalEvaluationTu15 call{words[0],words[1],words[2],words[3],words[4],words[5],words[6],words[7]};
    Locals locals;
    locals.goal=stack_word(memory,esp+0x18u);locals.best=stack_word(memory,esp+0x1cu);
    locals.row=stack_word(memory,esp+0x20u);locals.offset=stack_word(memory,esp+0x24u);
    locals.best_score=stack_word(memory,esp+0x28u);
    const auto flags=stack_word(memory,esp+0x14u);
    locals.near=(flags>>16)&0xffu;locals.far=(flags>>24)&0xffu;
    NativeCampaignAiGoalEvaluationContextTu15 context{esp+0x8b0u,&fp,abi.x87_depth,{}};
    const Port port{memory,observe};const Simd simd(fp);
    const auto goal=remainder(port,simd,fp,call,context,abi.gp[5],locals);
    abi.gp[0]=goal;
    abi.gp[7]=stack_word(memory,esp);abi.gp[3]=stack_word(memory,esp+4u);
    abi.gp[6]=stack_word(memory,esp+8u);abi.gp[5]=stack_word(memory,esp+12u);
    abi.gp[4]=esp+0x8d4u;
#else
    (void)memory;(void)abi;(void)observe;
    throw std::runtime_error("Exact native 26A712 arithmetic backend unavailable");
#endif
}
}
