#include "halo2/native_campaign_ai_publication_tu15.hpp"
#include "halo2/native_campaign_ai_actor_path_query_tu15.hpp"
#include "halo2/native_campaign_ai_action_identity_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_campaign_ai_navigation_filter_tu15.hpp"
#include "halo2/native_campaign_ai_orders_tu15.hpp"
#include "halo2/native_campaign_ai_movement_resume_tu15.hpp"
#include "halo2/native_campaign_ai_path_arena_tu15.hpp"
#include "halo2/native_campaign_ai_path_search_tu15.hpp"
#include "halo2/native_campaign_ai_path_workspace_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_route_tu15.hpp"
#include "halo2/native_tu15_data_array_lookup.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#include <vector>

namespace halo2 {
namespace {
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;using i16=std::int16_t;using i32=std::int32_t;
constexpr u32 actor_pool=0x5020c8u,none_cell=0x47d6c0u,object_pool=0x4ecbd0u;
i32 sx16(u32 value){return std::bit_cast<i16>(u16(value));}

// SSE compares under the caller's MXCSR in one asm block each; the sticky
// flags they raise (invalid, denormal) stay in mxcsr.
bool comiss_above(u32 &mxcsr,u32 left,u32 right) {
    unsigned char above{};u32 host{},csr=mxcsr;
    const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[b],%[a]\n\tseta %[above]\n\t"
                 "stmxcsr %[csr]\n\tldmxcsr %[host]"
        :[above]"=&q"(above),[host]"+m"(host),[csr]"+m"(csr):[a]"x"(a),[b]"x"(b):"cc","memory");
    mxcsr=csr;return above;
}
// UCOMISS / LAHF / TEST AH,44 / JP: true when ordered and equal.
bool ucomiss_equal(u32 &mxcsr,u32 left,u32 right) {
    unsigned char equal{},ordered{};u32 host{},csr=mxcsr;
    const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tucomiss %[b],%[a]\n\tsete %[equal]\n\tsetnp %[ordered]\n\t"
                 "stmxcsr %[csr]\n\tldmxcsr %[host]"
        :[equal]"=&q"(equal),[ordered]"=&q"(ordered),[host]"+m"(host),[csr]"+m"(csr):[a]"x"(a),[b]"x"(b):"cc","memory");
    mxcsr=csr;return equal&&ordered;
}
u32 mulss(u32 &mxcsr,u32 left,u32 right) {
    u32 host{},csr=mxcsr;auto a=std::bit_cast<float>(left);const auto b=std::bit_cast<float>(right);
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tmulss %[b],%[a]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
        :[a]"+x"(a),[host]"+m"(host),[csr]"+m"(csr):[b]"x"(b):"memory");
    mxcsr=csr;return std::bit_cast<u32>(a);
}

class Movement {
public:
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;NativeTu15PhysicsFrameFp &fp;
    const SourcePhysicsContactRecordObserver &observe;

    u32 r(u32 address,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));u32 value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);
        return value;
    }
    // An operand store of the original instruction at `site`.
    void w(u32 site,u32 address,u32 value,unsigned width)const {
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
        if(observe){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observe)observe(event);
    }
    // PUSH and CALL words: implicit stack stores without an operand.
    void push(u32 &sp,u32 value)const {
        sp-=4u;std::array<std::byte,4> raw{};
        for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
        memory.write(sp,raw);
    }
    u32 row_of(u32 actor)const{return r(r(actor_pool)+0x44u)+(actor&0xffffu)*0x888u;}

    // A callee without a native owner: the named refusal at its actual CALL.
    // Reference oracles only may transport the translated body; gp holds the
    // original registers with ESP before the CALL's return word.
    void unported(u32 site,u32 entry,std::array<u32,8> gp,u32 cleanup) {
        if(!services.callee)throw NativeCampaignAiPerformRequiredOwnerTu15(site,entry);
        const auto caller_sp=gp[4];push(gp[4],site+5u);
        abi.gp=gp;abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        services.callee(site,entry,abi);
        if(!abi.floating_point)throw std::runtime_error("Native 26C3C0 transported callee lost its FP controls");
        fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;
        if(abi.gp[4]!=caller_sp+cleanup)throw std::runtime_error("Native 26C3C0 transported callee returned another stack");
    }

    void resume(u32 site,u32 entry,std::array<u32,8> gp,u32 cleanup) {
        const auto caller_sp=gp[4];push(gp[4],site+5u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto publish_fp=[&]{fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;};
        try {native_campaign_ai_movement_resume_call_tu15(memory,entry,abi,services,observe);}
        catch(...){publish_fp();throw;}
        publish_fp();
        if(abi.gp[4]!=caller_sp+cleanup)throw std::runtime_error("Native movement resume returned another stack");
    }

    void route(u32 site,std::array<u32,8> gp) {
        const auto caller_sp=gp[4];push(gp[4],site+5u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto publish_fp=[&]{fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;};
        try {native_campaign_ai_route_call_tu15(memory,0x27a640u,abi,services,observe);}
        catch(...){publish_fp();throw;}
        publish_fp();
        if(abi.gp[4]!=caller_sp+8u)throw std::runtime_error("Native 27A640 returned another stack");
    }

    // 200960 (EAX actor): clear the movement state.
    void clear(u32 actor) {
        const auto row=row_of(actor);
        w(0x20097bu,row+0x50cu,0,1);w(0x200981u,row+0x5acu,0xffffffffu,4);w(0x200987u,row+0x5b0u,0xffffu,2);
        w(0x20098eu,row+0x5b4u,0,2);w(0x200995u,row+0x5b6u,0,2);w(0x20099cu,row+0x4acu,0,2);w(0x2009a3u,row+0x504u,0,2);
    }
    // 200A10 (EAX actor): count a failed update.
    void fail(u32 actor) {
        const auto row=row_of(actor);
        const auto failures=r(row+0x508u,2);w(0x200a26u,row+0x508u,failures+1u,2);
        w(0x200a2du,row+0x50cu,0,1);w(0x200a34u,row+0x504u,3,2);
    }
    // 27B0D0 (ECX workspace, EAX point, XMM0 scalar, [esp+4] frame): the goal.
    void goal(u32 workspace,u32 point,u32 scalar,u32 frame) {
        static constexpr std::array<u32,4> sites{0x27b0dau,0x27b0dfu,0x27b0e5u,0x27b0ebu};
        w(0x27b0d1u,workspace+0x54u,1,1);
        for(unsigned n=0;n<4;++n){const auto value=r(point+n*4u);w(sites[n],workspace+0x58u+n*4u,value,4);}
        w(0x27b0f2u,workspace+0x68u,frame,4);w(0x27b0f5u,workspace+0x6cu,scalar,4);
    }
    // 4-dword copy in the original r/w interleave.
    std::array<u32,4> copy4(const std::array<u32,4> &sites,u32 source,u32 destination) {
        std::array<u32,4> values{};
        for(unsigned n=0;n<4;++n){values[n]=r(source+n*4u);w(sites[n],destination+n*4u,values[n],4);}
        return values;
    }

    // Each owner reads its arguments from the real stack at `entry` (ESP at
    // its first instruction). `caller` holds the caller's EBX/EBP/ESI/EDI,
    // which the owner pushes as its saved registers.
    struct Saved {u32 ebx,ebp,esi,edi;};
    u8 update(u32 entry,const Saved &caller);
    void after_query(u32 frame,u32 row,u32 who);
    void after_search(u32 frame,u32 row,u32 leased,bool found);
    u8 conclude(u32 frame,u32 row);
    u8 fail_tail(u32 frame);
    u8 begin(u32 entry,const Saved &caller);
    u8 reset(u32 entry,const Saved &caller);
    u8 command_point(u32 entry,const Saved &caller);
    u32 publication(u32 entry,const Saved &caller);
};

// 200D30 ([esp+4] actor, [esp+8] workspace; RET 8; AL).
u8 Movement::update(u32 entry,const Saved &caller) {
    const auto pool=r(actor_pool),rows=r(pool+0x44u);
    const auto actor=r(entry+4u);
    const auto offset=(actor&0xffffu)*0x888u,row=offset+rows;
    // sub esp,80; push ebx/ebp/esi.
    u32 sp=entry-0x80u;push(sp,caller.ebx);push(sp,caller.ebp);push(sp,caller.esi);
    const auto regs=[&](u32 eax,u32 ecx,u32 edx,u32 ebx,u32 esp,u32 edi){return std::array<u32,8>{eax,ecx,edx,ebx,esp,row,actor,edi};};
    if(r(row+0x267u,1)){clear(actor);return 0;}
    if(r(row+0x480u,1))return 1;
    if(sx16(r(row+0x504u,2))==2) {
        // ECX actor; 1EBBD0 returns XMM0, then 1FB4F0(XMM0 * [46A0E4]).
        resume(0x200d95u,0x1ebbd0u,regs(pool&0xffffff00u,actor,abi.gp[2],offset,sp,caller.edi),0);
        const auto scaled=mulss(fp.mxcsr,abi.xmm[0][0],r(0x46a0e4u));
        u32 call_sp=sp;push(call_sp,scaled);
        resume(0x200daau,0x1fb4f0u,regs(actor,abi.gp[1],abi.gp[2],offset,call_sp,caller.edi),4);
        if(abi.gp[0]&0xffu) {
            call_sp=sp;push(call_sp,0);
            resume(0x200db7u,0x200a40u,regs(actor,abi.gp[1],abi.gp[2],offset,call_sp,caller.edi),4);
            return 1;
        }
    }
    const auto action=r(row+0x4acu,2);
    if(action==0 || action==1){clear(actor);return 1;}
    u32 mode=r(row+0x229u,1)?1u:0u;
    const auto unit=r(row+0x18u);
    push(sp,caller.edi);
    const auto frame=sp;  // F: [F+12] word/result byte, [F+14] path mode, [F+94] actor, [F+98] workspace
    w(0x200df9u,frame+0x14u,mode,4);
    {
        // E9600 (ECX unit, EAX &[F+12]): the unit's current action kind.
        const auto objects=r(object_pool),body=r(r(objects+0x44u)+(unit&0xffffu)*12u+8u);
        const auto block=u32(sx16(r(body+0x346u,2)));
        w(0xe9622u,frame+0x12u,r(block+body+0x36u,2),2);
    }
    if(r(frame+0x12u,2)==5u) {
        const auto character=r(row+0x54u);u32 ecx{};
        const auto vision=native_campaign_ai_character_property_tu15(memory,character,0x34u,ecx);
        const auto behavior=native_campaign_ai_character_property_tu15(memory,character,0x5cu,ecx);
        if(vision && !(r(vision,1)&2u) && behavior && (r(behavior,1)&0x20u))w(0x200e2du,frame+0x14u,1,4);
    }
    const auto row2=r(r(actor_pool)+0x44u)+offset;
    w(0x200e42u,row2+0x50cu,0,1);w(0x200e49u,row2+0x5acu,0xffffffffu,4);w(0x200e4fu,row2+0x5b0u,0xffffu,2);
    const auto selector=sx16(r(row+0x4acu,2))-2;
    w(0x200e63u,row+0x504u,0,2);w(0x200e6cu,row+0x5d2u,0,1);
    bool failed=false;
    const auto zero_scalar=[&](u32 site){w(site,row+0x500u,0,4);};
    if(u32(selector)>5u)failed=true;
    else switch(selector) {
    case 0:  // mode 2: the action's point and frame
        {
            const auto x=r(row+0x4b8u);w(0x200e8eu,row+0x4ecu,x,4);
            const auto y=r(row+0x4bcu);w(0x200e93u,row+0x4f0u,y,4);
            const auto z=r(row+0x4c0u),f=r(row+0x4c4u);w(0x200e9cu,row+0x4f4u,z,4);w(0x200e9fu,row+0x4f8u,f,4);
            w(0x200ea8u,row+0x4fcu,r(row+0x4c8u),4);zero_scalar(0x200f12u);
        }
        break;
    case 1:  // mode 3
        {
            const auto x=r(row+0x4b8u);w(0x200ebeu,row+0x4ecu,x,4);
            const auto y=r(row+0x4bcu);w(0x200ec3u,row+0x4f0u,y,4);
            const auto z=r(row+0x4c0u),f=r(row+0x4c4u);w(0x200eccu,row+0x4f4u,z,4);w(0x200ecfu,row+0x4f8u,f,4);
            w(0x200f4fu,row+0x4fcu,0xffffffffu,4);zero_scalar(0x200f55u);w(0x200f5du,frame+0x14u,1,4);
        }
        break;
    case 2:  // mode 4: firing position
        if(const auto point=native_campaign_ai_goal_definition_tu15(memory,r(row+0x4b8u))) {
            copy4({0x200ef2u,0x200ef7u,0x200efdu,0x200f03u},point,row+0x4ecu);
            w(0x200f09u,row+0x4fcu,r(point+0x14u),4);zero_scalar(0x200f12u);
        } else failed=true;
        break;
    case 3:  // mode 5
        if(const auto point=native_campaign_ai_goal_definition_tu15(memory,r(row+0x4b8u))) {
            copy4({0x200f38u,0x200f3du,0x200f43u,0x200f49u},point,row+0x4ecu);
            w(0x200f4fu,row+0x4fcu,0xffffffffu,4);zero_scalar(0x200f55u);w(0x200f5du,frame+0x14u,1,4);
        } else failed=true;
        break;
    case 4:  // mode 6
        {
            const auto point=native_campaign_ai_goal_definition_tu15(memory,r(row+0x4b8u));
            if(!point){failed=true;break;}
            u32 ecx{};const auto behavior=native_campaign_ai_character_property_tu15(memory,r(row+0x54u),0x5cu,ecx);
            if(!behavior){failed=true;break;}
            if(!(r(behavior,1)&0x10u))break;
            const auto copied=copy4({0x200fdau,0x200fdfu,0x200fe5u,0x200febu},point,row+0x4ecu);
            w(0x200ff1u,row+0x4fcu,r(point+0x14u),4);zero_scalar(0x200ff7u);
            if(r(behavior,1)&0x20u){w(0x200f5du,frame+0x14u,1,4);break;}
            if(r(frame+0x14u,2))break;
            const auto goal=r(row+0x4b8u);
            u32 call_sp=frame;push(call_sp,goal);
            unported(0x201022u,0x201840u,regs(actor,ecx,abi.gp[2],copied[2],call_sp,0xffffffffu),4);
            if(abi.gp[0]&0xffu)w(0x20102fu,frame+0x14u,2,4);
        }
        break;
    case 5:  // mode 7: 275FE0 then 2673A0
        {
            const auto goal=r(row+0x4b8u);
            const auto props=r(0x50eef0u),rows7=r(props+0x44u);
            const auto record=(goal&0xffffu)*0x3cu+rows7;
            unported(0x201058u,0x275fe0u,regs(goal,props,rows7,record,frame,0xffffffffu),0);
            unported(0x20105fu,0x2673a0u,regs(abi.gp[0],record,abi.gp[2],record,frame,0xffffffffu),0);
            const auto source=abi.gp[0];
            if(r(frame+0x14u,2)==1u) {
                const auto x=r(source+0x10u);w(0x201077u,row+0x4ecu,x,4);
                const auto y=r(source+0x14u);w(0x20107cu,row+0x4f0u,y,4);
                const auto z=r(source+0x18u);w(0x201082u,row+0x4f4u,z,4);
                w(0x201085u,row+0x4f8u,0xffffu,2);
            } else copy4({0x201099u,0x20109eu,0x2010a4u,0x2010aau},source+0x48u,row+0x4ecu);
            const auto f=r(source+0x44u),scalar=r(row+0x4d0u);
            w(0x2010b6u,row+0x4fcu,f,4);w(0x2010bcu,row+0x500u,scalar,4);
        }
        break;
    }
    // 200F65: the path mode selects the route owner.
    u8 result=0;
    if(!failed) {
        const auto path_mode=sx16(r(frame+0x14u,2));
        if(path_mode==0) {
            const auto scalar=r(row+0x500u);
            if(ucomiss_equal(fp.mxcsr,scalar,r(0x46a0e8u)) && r(row+0x4fcu)==0xffffffffu)failed=true;
        } else if(path_mode<0 || path_mode>2)failed=true;
        if(!failed) {
            if(path_mode==0) {
                const auto lease=r(frame+0x98u);
                if(lease) {
                    // 27B0D0(ECX lease, EAX +4EC, XMM0 +500, [esp+4] +4FC), then 27A640.
                    const auto f=r(row+0x4fcu),scalar=r(row+0x500u);
                    u32 call_sp=frame;push(call_sp,f);push(call_sp,0x201215u);
                    goal(lease,row+0x4ecu,scalar,f);
                    const auto who=r(frame+0x94u);
                    call_sp=frame;push(call_sp,row+0x50cu);push(call_sp,who);
                    route(0x201226u,regs(lease,lease,f,abi.gp[3],call_sp,0xffffffffu));
                    result=u8(abi.gp[0]);w(0x20122bu,frame+0x12u,result,1);
                } else {
                    const auto who=r(frame+0x94u);
                    const auto query=frame+0x40u;
                    u32 call_sp=frame;push(call_sp,who);
                    w(0x201240u,frame+0x12u,0,1);
                    push(call_sp,0x20124au);
                    abi.gp={abi.gp[0],0,abi.gp[2],query,call_sp,row,who,0xffffffffu};
                    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
                    const auto publish=[&]{fp.control_word=abi.floating_point->x87_control_word;
                        fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;};
                    try {native_campaign_ai_actor_path_query_call_tu15(memory,0x2013b0u,abi,observe);}
                    catch(...){publish();throw;}
                    publish();
                    if(abi.gp[4]!=frame)throw std::runtime_error("Native 2013B0 returned another movement stack");
                    after_query(frame,row,who);
                }
            } else if(path_mode==1) {
                // 201135: the flying route through 1D2CF0 and 2A8440.
                const auto action_mode=r(row+0x4acu,2);u32 target=r(none_cell),object=0xffffffffu;
                if(action_mode==4u || action_mode==5u || action_mode==6u)target=r(row+0x4b8u);
                else if(action_mode==7u) {
                    const auto goal_id=r(row+0x4b8u),props=r(0x50eef0u),rows7=r(props+0x44u);
                    const auto device=r((goal_id&0xffffu)*0x3cu+rows7+0x20u);
                    unported(0x201178u,0xbcfb0u,regs(props,device,(goal_id&0xffffu)*0x3cu,0xffffffffu,frame,target),0);
                    object=abi.gp[0];
                }
                const auto restriction=r(row+0x4e4u);
                if(restriction!=0xffffffffu)object=restriction;
                auto subject=r(row+0x26cu);
                if(subject==0xffffffffu)subject=r(row+0x18u);
                unported(0x2011a5u,0x1d2cf0u,std::array<u32,8>{subject,abi.gp[1],abi.gp[2],object,frame,row,frame+0x18u,target},0);
                auto lease=r(frame+0x98u);if(lease)lease+=0x140d4u;
                const auto who=r(frame+0x94u);
                u32 call_sp=frame;push(call_sp,row+0x50cu);push(call_sp,object);push(call_sp,lease);push(call_sp,target);
                push(call_sp,row+0x4ecu);push(call_sp,frame+0x18u);push(call_sp,who);
                unported(0x2011e2u,0x2a8440u,std::array<u32,8>{row+0x4acu,frame+0x18u,who,object,call_sp,row,frame+0x18u,target},28);
                result=u8(abi.gp[0]);w(0x2011e7u,frame+0x12u,result,1);
            } else {
                // 201113: path mode 2, 201750(ESI +50C, EAX actor, [esp+4] goal).
                const auto goal_id=r(row+0x4b8u),who=r(frame+0x94u);
                u32 call_sp=frame;push(call_sp,goal_id);
                unported(0x201127u,0x201750u,std::array<u32,8>{who,goal_id,abi.gp[2],abi.gp[3],call_sp,row,row+0x50cu,0xffffffffu},4);
                result=u8(abi.gp[0]);w(0x20112cu,frame+0x12u,result,1);
            }
            return conclude(frame,row);
        }
    }
    w(0x2010f1u,frame+0x12u,0,1);
    return fail_tail(frame);
}

// 20124A..20133D: after 2013B0 filled the query at F+40 (ESI the actor).
void Movement::after_query(u32 frame,u32 row,u32 who) {
    const auto query=frame+0x40u,filter=frame+0x24u;
    u32 call_sp;
    const auto restriction=r(row+0x4e4u);
    if(restriction!=0xffffffffu)w(0x201254u,frame+0x4cu,restriction,4);
    if(sx16(r(row+0x358u,2))>0 && !r(row+0x35eu,1)) {
        const auto target=r(row+0x360u);
        call_sp=frame;push(call_sp,1);push(call_sp,0x41200000u);push(call_sp,target);
        abi.xmm[0]={r(row+0x36cu),0,0,0};
        unported(0x20128cu,0x27a2d0u,std::array<u32,8>{query,row+0x370u,abi.gp[2],query,call_sp,row,who,0xffffffffu},12);
    }
    call_sp=frame;push(call_sp,who);push(call_sp,0x20129bu);
    native_campaign_ai_navigation_filter_tu15(memory,who,filter,observe);
    call_sp=frame;push(call_sp,0x2012a0u);
    const auto leased=services.protection
        ?native_campaign_ai_path_workspace_acquire_tu15(memory,*services.protection,observe)
        :native_campaign_ai_path_workspace_acquire_tu15(memory,observe);
    call_sp=frame;push(call_sp,0);push(call_sp,query);
    w(0x2012b3u,frame+0x14u,leased,4);
    push(call_sp,0x2012bcu);
    {
        const NativeCampaignAiPathMemoryWordsTu15 main(memory,query),policy(memory,row+0x5b4u),transport(memory,filter);
        native_campaign_ai_path_workspace_initialize_tu15(memory,leased,main,&policy,&transport,0,observe);
    }
    const auto f=r(row+0x4fcu),scalar=r(row+0x500u);
    call_sp=frame;push(call_sp,f);push(call_sp,0x2012d8u);
    goal(leased,row+0x4ecu,scalar,f);
    call_sp=frame;push(call_sp,0x2012dfu);
    if(abi.x87_depth)throw std::invalid_argument("Native 200D30 path search requires an empty x87 stack");
    const bool found=native_campaign_ai_path_search_tu15(memory,leased,frame-4u,fp,observe);
    after_search(frame,row,leased,found);
}

// 2012DF..20133D: 27B2B0 returned `found` for the lease in EDI.
void Movement::after_search(u32 frame,u32 row,u32 leased,bool found) {
    const auto filter=frame+0x24u;
    u32 call_sp;
    if(!found) {
        for(unsigned n=0;n<7;++n){const auto value=r(leased+0x70u+n*4u);w(0x201335u,row+0x5b4u+n*4u,value,4);}
    } else {
        const auto again=r(frame+0x94u);
        call_sp=frame;push(call_sp,row+0x50cu);push(call_sp,again);
        route(0x2012f4u,std::array<u32,8>{leased,row+0x50cu,abi.gp[2],filter,call_sp,row,again,leased});
        const auto result=u8(abi.gp[0]);w(0x2012fbu,frame+0x12u,result,1);
        if(!result && r(row+0x5b2u,1) && r(row+0x4acu,2)==4u) {
            const auto goal_id=r(row+0x4b8u);
            call_sp=frame;push(call_sp,goal_id);push(call_sp,0x201325u);
            native_campaign_ai_action_history_tu15(memory,again,goal_id,0,observe);
        }
    }
    const auto release=r(frame+0x14u);
    call_sp=frame;push(call_sp,0x201342u);
    native_campaign_ai_path_workspace_release_tu15(memory,release,observe);
}

// 201342: the route owner's result byte at F+12.
u8 Movement::conclude(u32 frame,u32 row) {
    const auto result=u8(r(frame+0x12u,1));
    w(0x201348u,row+0x506u,1,1);
    if(result){w(0x201357u,row+0x504u,1,2);w(0x201360u,row+0x5e8u,0,2);return result;}
    return fail_tail(frame);
}
// 2010F6: 200A10 for [F+94], AL = [F+12].
u8 Movement::fail_tail(u32 frame) {
    const auto who=r(frame+0x94u);
    {u32 call_sp=frame;push(call_sp,0x201102u);}
    fail(who);
    return u8(r(frame+0x12u,1));
}

// 1FC9B0 ([esp+4] actor, [esp+8] mode, [esp+C] goal, [esp+10] workspace,
// [esp+14] flag; RET 14; AL).
u8 Movement::begin(u32 entry,const Saved &caller) {
    const auto rows=r(r(actor_pool)+0x44u);
    u32 sp=entry;push(sp,caller.ebx);push(sp,caller.ebp);
    const auto actor=r(entry+4u),row=(actor&0xffffu)*0x888u+rows;
    const auto mode=r(entry+8u,2);
    if(r(row+0x4acu,2)==mode && r(row+0x4b8u)==r(entry+0xcu)) {
        if(!r(row+0x40u,1) || r(row+0x506u,1))return 1;
        const auto workspace=sx16(r(row+0x5b4u,2))<=0?r(entry+0x10u):0u;
        u32 call_sp=sp;push(call_sp,workspace);push(call_sp,actor);push(call_sp,0x1fca17u);
        return update(call_sp,Saved{row,actor,caller.esi,caller.edi});
    }
    {u32 call_sp=sp;push(call_sp,0x1fca23u);}
    clear(actor);
    const auto workspace=r(entry+0x10u);
    if(workspace) {
        u32 copy_sp=sp;push(copy_sp,caller.esi);push(copy_sp,caller.edi);
        for(unsigned n=0;n<7;++n){const auto value=r(workspace+0x70u+n*4u);w(0x1fca3du,row+0x5b4u+n*4u,value,4);}
    }
    const auto new_mode=r(entry+8u,2);
    w(0x1fca49u,row+0x4acu,0,2);w(0x1fca50u,row+0x4aeu,0,1);w(0x1fca56u,row+0x4d4u,0,1);w(0x1fca5cu,row+0x4d5u,0,1);
    w(0x1fca62u,row+0x4e8u,0,1);w(0x1fca68u,row+0x4b0u,0,4);w(0x1fca70u,row+0x4b4u,0,4);w(0x1fca78u,row+0x4ccu,0,4);
    w(0x1fca80u,row+0x4d0u,0,4);w(0x1fca88u,row+0x4e4u,0xffffffffu,4);w(0x1fca92u,row+0x4acu,new_mode,2);
    const auto goal_id=r(entry+0xcu);
    w(0x1fca9du,row+0x4bcu,0,1);w(0x1fcaa3u,row+0x3f1u,0,1);
    const auto flag=r(entry+0x14u,1);
    u32 call_sp=sp;push(call_sp,workspace);push(call_sp,actor);
    w(0x1fcaafu,row+0x4b8u,goal_id,4);w(0x1fcab5u,row+0x4aeu,flag,1);
    push(call_sp,0x1fcac0u);
    return update(call_sp,Saved{row,actor,caller.esi,caller.edi});
}

// 1FC540 ([esp+4] actor; RET 4; AL).
u8 Movement::reset(u32 entry,const Saved &caller) {
    const auto rows=r(r(actor_pool)+0x44u);
    u32 sp=entry-0x10u;push(sp,caller.ebx);push(sp,caller.ebp);
    const auto actor=r(entry+4u),row=(actor&0xffffu)*0x888u+rows;
    push(sp,caller.esi);
    const auto none=[&](u32 at){return r(at);};
    if(r(row+0x270u,2)!=4u || !r(row+0x5d0u,1)) {
        const auto key=none(none_cell);
        u32 call_sp=sp;push(call_sp,key);push(call_sp,actor);push(call_sp,0x1fc6edu);
        native_campaign_ai_order_script_assignment_tu15(memory,actor,key);
        if(r(row+0x4acu,2)!=1u)w(0x1fc6fbu,row+0x4acu,1,2);
        call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc709u);
        return update(call_sp,Saved{0,actor,row,caller.edi});
    }
    // The local framed point at entry-10: +238 XYZ and frame word FFFF. Its
    // upper frame word is never written: the +4C4 copy keeps stack history.
    const auto point=entry-0x10u;
    const auto x=r(row+0x238u),y=r(row+0x23cu),z=r(row+0x240u);
    w(0x1fc58du,point+8u,z,4);
    const auto flying=r(row+0x229u,1);
    push(sp,caller.edi);
    w(0x1fc59du,point,x,4);w(0x1fc5a1u,point+4u,y,4);w(0x1fc5a5u,point+0xcu,0xffffu,2);w(0x1fc5aau,entry+4u,1,1);
    if(!flying) {
        {u32 call_sp=sp;push(call_sp,0x1fc5b8u);}
        clear(actor);
        w(0x1fc5bbu,entry+4u,0,1);return 0;
    }
    const auto key=r(none_cell);
    {u32 call_sp=sp;push(call_sp,key);push(call_sp,actor);push(call_sp,0x1fc5d5u);}
    native_campaign_ai_order_script_assignment_tu15(memory,actor,key);
    u32 frame_word=0xffffffffu;
    bool restart=true;
    if(r(row+0x4acu,2)==2u && r(row+0x4c8u)==0xffffffffu) {
        {u32 call_sp=sp;push(call_sp,0x1fc5f6u);}
        const auto distance=native_campaign_ai_point_distance_squared_tu15(memory,point,row+0x4b8u,fp);
        abi.xmm[0]={distance,0,0,0};
        if(!comiss_above(fp.mxcsr,distance,r(0x46a560u))) {
            restart=false;
            if(r(row+0x40u,1) && !r(row+0x506u,1)) {
                u32 call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc61bu);
                const auto result=update(call_sp,Saved{0,actor,row,0xffffffffu});
                w(0x1fc61eu,entry+4u,result,1);return result;
            }
        }
    }
    if(restart) {
        {u32 call_sp=sp;push(call_sp,0x1fc633u);}
        clear(actor);
        const auto px=r(point),py=r(point+4u);
        w(0x1fc63eu,row+0x4acu,0,2);w(0x1fc645u,row+0x4aeu,0,1);w(0x1fc64bu,row+0x4b0u,0,4);w(0x1fc653u,row+0x4b4u,0,4);
        w(0x1fc65bu,row+0x4ccu,0,4);w(0x1fc663u,row+0x4d0u,0,4);w(0x1fc66bu,row+0x4d4u,0,1);w(0x1fc671u,row+0x4d5u,0,1);
        w(0x1fc677u,row+0x4e4u,frame_word,4);w(0x1fc67du,row+0x4e8u,0,1);
        w(0x1fc689u,row+0x4b8u,px,4);
        const auto pz=r(point+8u);
        w(0x1fc68fu,row+0x4bcu,py,4);
        const auto pf=r(point+0xcu);
        w(0x1fc696u,row+0x4c0u,pz,4);
        const auto flying_now=r(row+0x229u,1);
        w(0x1fc69fu,row+0x4acu,2,2);w(0x1fc6a8u,row+0x4aeu,1,1);w(0x1fc6afu,row+0x4c4u,pf,4);
        w(0x1fc6b2u,row+0x4c8u,frame_word,4);w(0x1fc6b8u,row+0x4e4u,frame_word,4);
        if(flying_now)w(0x1fc6c0u,row+0x656u,0,2);
        u32 call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc6ceu);
        const auto result=update(call_sp,Saved{0,actor,row,frame_word});
        w(0x1fc6ceu,entry+4u,result,1);
    }
    return u8(r(entry+4u,1));
}

// 1FC720: EBX actor; [esp+4] framed point, [esp+8] navigation reference,
// [esp+C] object, [esp+10] flag byte. AL result, RET10. A ground actor
// requires the authored navigation reference; no replacement goal is made.
u8 Movement::command_point(u32 entry,const Saved &caller) {
    const auto pool=r(actor_pool),rows=r(pool+0x44u);
    u32 sp=entry;push(sp,caller.ebp);
    const auto point=r(entry+4u);push(sp,caller.esi);
    const auto actor=caller.ebx,row=rows+(actor&0xffffu)*0x888u;
    if(!r(row+0x229u,1) && r(entry+8u)==0xffffffffu) {
        {u32 call_sp=sp;push(call_sp,0x1fc757u);}
        clear(actor);return 0;
    }
    const auto key=r(none_cell);
    {u32 call_sp=sp;push(call_sp,key);push(call_sp,actor);push(call_sp,0x1fc76bu);}
    native_campaign_ai_order_script_assignment_tu15(memory,actor,key);
    bool restart=true;
    if(r(row+0x4acu,2)==2u) {
        const auto reference=r(entry+8u);
        if(r(row+0x4c8u)==reference) {
            push(sp,caller.edi);
            {u32 call_sp=sp;push(call_sp,0x1fc78fu);}
            const auto distance=native_campaign_ai_point_distance_squared_tu15(memory,point,row+0x4b8u,fp);
            abi.xmm[0]={distance,0,0,0};
            restart=comiss_above(fp.mxcsr,distance,r(0x46a560u));
            sp+=4u;
            if(!restart) {
                if(!r(row+0x40u,1) || r(row+0x506u,1))return 1;
                u32 call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc7bau);
                return update(call_sp,Saved{actor,point,row,caller.edi});
            }
        }
    }
    if(restart) {
        {u32 call_sp=sp;push(call_sp,0x1fc7c6u);}
        clear(actor);
        const auto flag=r(entry+0x10u,1);
        w(0x1fc7cdu,row+0x4b0u,0,4);w(0x1fc7d5u,row+0x4b4u,0,4);
        w(0x1fc7ddu,row+0x4ccu,0,4);w(0x1fc7e5u,row+0x4d0u,0,4);
        w(0x1fc7edu,row+0x4e4u,0xffffffffu,4);w(0x1fc7f9u,row+0x4acu,0,2);
        w(0x1fc800u,row+0x4aeu,0,1);w(0x1fc806u,row+0x4d4u,0,1);
        w(0x1fc80cu,row+0x4d5u,0,1);w(0x1fc812u,row+0x4e8u,0,1);
        w(0x1fc818u,row+0x4aeu,flag,1);w(0x1fc81eu,row+0x4acu,2,2);
        copy4({0x1fc830u,0x1fc835u,0x1fc83bu,0x1fc841u},point,row+0x4b8u);
        const auto object=r(entry+0xcu),reference=r(entry+8u);
        w(0x1fc84cu,row+0x4e4u,object,4);
        const auto flying=r(row+0x229u,1);
        w(0x1fc85au,row+0x4c8u,reference,4);
        if(flying)w(0x1fc862u,row+0x656u,0,2);
        u32 call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc870u);
        return update(call_sp,Saved{actor,point,row,caller.edi});
    }
    return 1;
}

// 26C3C0 ([esp+4] actor, [esp+8] goal, [esp+C] owner actor, [esp+10]
// workspace, [esp+14] path-found byte, [esp+18] begin byte; RET 18; EAX).
u32 Movement::publication(u32 entry,const Saved &caller) {
    const auto none=r(none_cell);
    u32 sp=entry;push(sp,caller.ebx);
    const auto actor=r(entry+4u);
    push(sp,caller.ebp);
    const auto goal_id=r(entry+8u);
    push(sp,caller.esi);
    const auto pool=r(actor_pool),rows=r(pool+0x44u);
    push(sp,caller.edi);
    const auto row=(actor&0xffffu)*0x888u+rows;
    if(goal_id==none) {
        {u32 call_sp=sp;push(call_sp,actor);push(call_sp,0x26c3f4u);reset(call_sp,Saved{actor,goal_id,pool,row});}
        const auto key=r(none_cell);
        {u32 call_sp=sp;push(call_sp,key);push(call_sp,actor);push(call_sp,0x26c400u);}
        native_campaign_ai_order_script_assignment_tu15(memory,actor,key);
        return r(row+0x418u);
    }
    const auto current=r(row+0x418u);
    if(current!=none && current!=goal_id) {
        u32 call_sp=sp;push(call_sp,current);push(call_sp,0x26c427u);
        native_campaign_ai_action_history_tu15(memory,actor,current,1,observe);
    }
    auto esi=pool;
    const auto owner=r(entry+0xcu);
    if(owner!=0xffffffffu && native_tu15_data_array_try_lookup(memory,pool,owner)) {
        esi=owner;
        {u32 call_sp=sp;push(call_sp,owner);push(call_sp,0x26c447u);reset(call_sp,Saved{actor,goal_id,owner,row});}
        const auto key=r(none_cell);
        {u32 call_sp=sp;push(call_sp,key);push(call_sp,owner);push(call_sp,0x26c454u);}
        native_campaign_ai_order_script_assignment_tu15(memory,owner,key);
    }
    u32 flag;
    if(r(row+0x418u)!=goal_id) {
        {u32 call_sp=sp;push(call_sp,goal_id);push(call_sp,actor);push(call_sp,0x26c463u);}
        native_campaign_ai_order_script_assignment_tu15(memory,actor,goal_id);
        flag=r(entry+0x14u,1);
        w(0x26c46cu,row+0x3f0u,flag==0?1u:0u,1);w(0x26c472u,row+0x3f1u,0,1);
    } else flag=r(entry+0x14u,1);
    if(!r(entry+0x18u,1))return r(row+0x418u);
    bool begun=false;
    {u32 call_sp=sp;push(call_sp,goal_id);push(call_sp,0x26c48du);}
    if(const auto definition=native_campaign_ai_goal_definition_tu15(memory,goal_id)) {
        const auto kind=r(definition+0xeu,2);
        u32 mode,begin_flag;
        if(kind&0x20u){mode=6;begin_flag=0;}
        else if(kind&0x40u){mode=4;begin_flag=0;}
        else {mode=5;begin_flag=1;}
        const auto workspace=flag?r(entry+0x10u):0u;
        const auto who=r(entry+4u);
        u32 call_sp=sp;push(call_sp,begin_flag);push(call_sp,workspace);push(call_sp,goal_id);push(call_sp,mode);push(call_sp,who);
        push(call_sp,0x26c4e5u);
        begun=begin(call_sp,Saved{workspace,goal_id,esi,row})!=0;
    }
    if(!begun) {
        const auto key=r(none_cell),who=r(entry+4u);
        {u32 call_sp=sp;push(call_sp,key);push(call_sp,who);push(call_sp,0x26c4fau);}
        native_campaign_ai_order_script_assignment_tu15(memory,who,key);
    }
    return r(row+0x418u);
}

struct FpPublication {
    SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
    ~FpPublication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
};
}

void native_campaign_ai_publication_call_tu15(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx)
        throw std::invalid_argument("Native 26C3C0 publication requires retained FP controls");
    if((abi.floating_point->mxcsr&0x1f80u)!=0x1f80u || (abi.floating_point->x87_control_word&0x3fu)!=0x3fu)
        throw std::invalid_argument("Native 26C3C0 publication requires masked FP exceptions");
    if(abi.x87_depth)throw std::invalid_argument("Native 26C3C0 publication requires an empty x87 stack (27B2B0)");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    FpPublication publication{abi,fp};
    const auto entry_abi=abi.gp;
    const Movement::Saved caller{entry_abi[3],entry_abi[5],entry_abi[6],entry_abi[7]};
    Movement movement{memory,abi,services,fp,observe};
    const auto sp=entry_abi[4];
    std::uint32_t eax,cleanup;
    switch(entry) {
    case 0x26c3c0u:eax=movement.publication(sp,caller);cleanup=0x1cu;break;
    case 0x1fc540u:eax=(entry_abi[0]&0xffffff00u)|u32(movement.reset(sp,caller));cleanup=8u;break;
    case 0x1fc720u:eax=(entry_abi[0]&0xffffff00u)|u32(movement.command_point(sp,caller));cleanup=0x14u;break;
    case 0x1fc9b0u:eax=(entry_abi[0]&0xffffff00u)|u32(movement.begin(sp,caller));cleanup=0x18u;break;
    case 0x200d30u:eax=(entry_abi[0]&0xffffff00u)|u32(movement.update(sp,caller));cleanup=0xcu;break;
    case 0x20124au:case 0x2012dfu:case 0x201342u: {
        // Component qualification of 200D30's segments after 2013B0, 27B2B0
        // and a route owner, from declared post-call states: ESP is the
        // frame F, EBP the actor row, ESI the actor (20124A), EDI the lease
        // and AL the search result (2012DF). Exit is 200D30's own RET 8.
        const auto frame=sp,row=entry_abi[5];
        u8 result;
        if(entry==0x20124au){movement.after_query(frame,row,entry_abi[6]);result=movement.conclude(frame,row);}
        else if(entry==0x2012dfu){movement.after_search(frame,row,entry_abi[7],(entry_abi[0]&0xffu)!=0);result=movement.conclude(frame,row);}
        else result=movement.conclude(frame,row);
        abi.gp[0]=(entry_abi[0]&0xffffff00u)|u32(result);abi.gp[1]=entry_abi[1];abi.gp[2]=entry_abi[2];
        abi.gp[7]=movement.r(frame);abi.gp[6]=movement.r(frame+4u);abi.gp[5]=movement.r(frame+8u);abi.gp[3]=movement.r(frame+0xcu);
        abi.gp[4]=frame+0x9cu;return;
    }
    default:throw std::invalid_argument("Native 26C3C0 publication has no such entry");
    }
    abi.gp[0]=eax;abi.gp[1]=entry_abi[1];abi.gp[2]=entry_abi[2];
    abi.gp[3]=caller.ebx;abi.gp[5]=caller.ebp;abi.gp[6]=caller.esi;abi.gp[7]=caller.edi;abi.gp[4]=sp+cleanup;
}

void native_campaign_ai_publication_tu15(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    native_campaign_ai_publication_call_tu15(memory,0x26c3c0u,abi,services,observe);
}
}
