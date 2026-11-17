#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_ai_action_identity_tu15.hpp"
#include "halo2/native_campaign_ai_area_choice_tu15.hpp"
#include "halo2/native_campaign_ai_candidate_collection_tu15.hpp"
#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/native_campaign_ai_actor_path_query_tu15.hpp"
#include "halo2/native_campaign_ai_goal_evaluation_tu15.hpp"
#include "halo2/native_campaign_ai_path_arena_tu15.hpp"
#include "halo2/native_campaign_ai_path_distance_tu15.hpp"
#include "halo2/native_campaign_ai_publication_tu15.hpp"
#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <sstream>
#include <vector>

namespace halo2 {
namespace {
constexpr std::uint32_t none_cell=0x47d6c0u,actor_pool=0x5020c8u;
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
// PUSH/CALL publish their implicit stack word without an operand observer;
// the physical reference sees them only in the retained frame bytes.
void push(SourcePhysicsContactMemory &memory,std::uint32_t &sp,std::uint32_t value) {
    sp-=4u;std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
    memory.write(sp,bytes);
}
std::int32_t signed_word(std::uint32_t word){return std::bit_cast<std::int16_t>(std::uint16_t(word));}
// 1BBF48 COMISS XMM1,XMM0 / JBE under the caller's MXCSR. The sticky flags it
// raises (invalid for NaN, denormal) are part of the retained control state.
bool scalar_above(std::uint32_t &mxcsr,std::uint32_t left,std::uint32_t right) {
    unsigned char above{};std::uint32_t host{},csr=mxcsr;
    const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[b],%[a]\n\tseta %[above]\n\t"
                 "stmxcsr %[csr]\n\tldmxcsr %[host]"
        :[above]"=&q"(above),[host]"+m"(host),[csr]"+m"(csr):[a]"x"(a),[b]"x"(b):"cc","memory");
    mxcsr=csr;return above;
}
struct FpPublication {
    SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
    ~FpPublication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
};
// Rejected retained reads inside the native callees owe their exact extended
// quantities above the caller's live x87 values (the existing binding policy).
void retain_x87(SourcePhysicsWorldRuntimeAbi &abi,std::span<const std::array<std::byte,10>> values) {
    static_assert(sizeof(long double)>=10);
    const auto count=unsigned(values.size());
    if(abi.x87_depth+count>abi.x87.size())throw std::runtime_error("1BBEE0 pending x87 quantities exceed caller capacity");
    for(unsigned n=abi.x87_depth;n;--n)std::memcpy(&abi.x87[n-1u+count],&abi.x87[n-1u],sizeof(long double));
    for(unsigned n=0;n<count;++n){std::memset(&abi.x87[n],0,sizeof(long double));std::memcpy(&abi.x87[n],values[n].data(),10);}
    abi.x87_depth+=count;
}
// Call one callee that is not native yet with its original register inputs.
void transport(const NativeCampaignAiPerformServicesTu15 &services,std::uint32_t site,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &state,NativeTu15PhysicsFrameFp &fp) {
    if(!services.callee)throw NativeCampaignAiPerformRequiredOwnerTu15(site,entry);
    state.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};state.x87_status=fp.status;
    services.callee(site,entry,state);
    if(!state.floating_point)throw std::runtime_error("1BBEE0 callee lost its FP controls");
    fp.control_word=state.floating_point->x87_control_word;fp.mxcsr=state.floating_point->mxcsr;fp.status=state.x87_status;
}
}

NativeCampaignAiPerformRequiredOwnerTu15::NativeCampaignAiPerformRequiredOwnerTu15(std::uint32_t site,std::uint32_t entry)
    :std::runtime_error([&]{std::ostringstream text;text<<"Native 1BBEE0 perform requires an unported owner at "<<std::hex
        <<site<<" -> "<<entry;return text.str();}()),site_(site),entry_(entry) {}

namespace {
// SSE scalar operations under the caller's MXCSR. Each sequence runs in one
// asm block so the compiler cannot move an operation across the control
// switch; the sticky flags it raises are retained in mxcsr.
bool ordered_equal(std::uint32_t &mxcsr,std::uint32_t left,std::uint32_t right) {
    unsigned char equal{},ordered{};std::uint32_t host{},csr=mxcsr;
    const auto a=std::bit_cast<float>(left),b=std::bit_cast<float>(right);
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tucomiss %[b],%[a]\n\tsete %[equal]\n\tsetnp %[ordered]\n\t"
                 "stmxcsr %[csr]\n\tldmxcsr %[host]"
        :[equal]"=&q"(equal),[ordered]"=&q"(ordered),[host]"+m"(host),[csr]"+m"(csr):[a]"x"(a),[b]"x"(b):"cc","memory");
    mxcsr=csr;return equal&&ordered;
}
// 26B659..26B688: x*x, y*y, x2+y2, z*z, +z2, then COMISS against the bound.
bool squared_length_above(std::uint32_t &mxcsr,std::array<std::uint32_t,3> v,std::uint32_t bound,
    std::array<std::uint32_t,3> &squares) {
    unsigned char above{};std::uint32_t host{},csr=mxcsr;
    const auto x=std::bit_cast<float>(v[0]),y=std::bit_cast<float>(v[1]),z=std::bit_cast<float>(v[2]),t=std::bit_cast<float>(bound);
    float r1,r2,r3;
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\t"
                 "movaps %[x],%[r3]\n\tmulss %[x],%[r3]\n\tmovaps %[y],%[r2]\n\tmulss %[y],%[r2]\n\t"
                 "movaps %[z],%[r1]\n\taddss %[r2],%[r3]\n\tmulss %[z],%[r1]\n\taddss %[r1],%[r3]\n\t"
                 "comiss %[t],%[r3]\n\tseta %[above]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
        :[above]"=&q"(above),[r1]"=&x"(r1),[r2]"=&x"(r2),[r3]"=&x"(r3),[host]"+m"(host),[csr]"+m"(csr)
        :[x]"x"(x),[y]"x"(y),[z]"x"(z),[t]"x"(t):"cc","memory");
    mxcsr=csr;squares={std::bit_cast<std::uint32_t>(r1),std::bit_cast<std::uint32_t>(r2),std::bit_cast<std::uint32_t>(r3)};
    return above;
}
// 26B6E1..26B6F8: (a+b) COMISS c.
bool sum_above(std::uint32_t &mxcsr,std::uint32_t left,std::uint32_t addend,std::uint32_t right,std::uint32_t &sum) {
    unsigned char above{};std::uint32_t host{},csr=mxcsr;
    auto a=std::bit_cast<float>(left);const auto b=std::bit_cast<float>(addend),c=std::bit_cast<float>(right);
    asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\taddss %[b],%[a]\n\tcomiss %[c],%[a]\n\tseta %[above]\n\t"
                 "stmxcsr %[csr]\n\tldmxcsr %[host]"
        :[above]"=&q"(above),[a]"+x"(a),[host]"+m"(host),[csr]"+m"(csr):[b]"x"(b),[c]"x"(c):"cc","memory");
    mxcsr=csr;sum=std::bit_cast<std::uint32_t>(a);return above;
}
std::uint32_t object_body(const SourcePhysicsContactMemory &memory,std::uint32_t pool,std::uint32_t object) {
    const auto rows=read(memory,pool+0x44u);
    return read(memory,rows+(object&0xffffu)*12u+8u);
}
std::uint32_t character_property(const SourcePhysicsContactMemory &memory,std::uint32_t datum,
    std::uint32_t count,std::uint32_t &ecx) {
    return native_campaign_ai_character_property_tu15(memory,datum,count,ecx);
}
// 1EA1D0 (EAX actor): the weapon-or-unit object's current marker datum.
std::uint32_t aim_object(const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto pool=read(memory,actor_pool),rows=read(memory,pool+0x44u);
    const auto row=(actor&0xffffu)*0x888u+rows;
    const auto mounted=read(memory,row+0x268u,1);
    const auto objects=read(memory,0x4ecbd0u);
    if(mounted) {
        const auto seat=read(memory,row+0x274u);
        if(seat!=0xffffffffu) {
            const auto body=object_body(memory,objects,seat);
            const auto index=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(memory,body+0x212u,1))));
            std::uint32_t result=0xffffffffu;
            if(index!=-1)result=read(memory,body+0x218u+std::uint32_t(index)*4u);
            if(result!=0xffffffffu)return result;
        }
    }
    const auto unit=read(memory,row+0x18u);
    if(unit==0xffffffffu)return 0xffffffffu;
    const auto body=object_body(memory,objects,unit);
    const auto index=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(memory,body+0x212u,1))));
    if(index==-1)return 0xffffffffu;
    return read(memory,body+0x218u+std::uint32_t(index)*4u);
}
// 1ED530 (ECX actor, [esp+4] definition): the character weapon entry whose
// +8 definition matches, walking the 4F0414 parent chain from actor +54.
std::uint32_t character_weapon(const SourcePhysicsContactMemory &memory,std::uint32_t actor,std::uint32_t definition) {
    const auto pool=read(memory,actor_pool),rows=read(memory,pool+0x44u);
    auto character=read(memory,(actor&0xffffu)*0x888u+rows+0x54u);
    if(character==0xffffffffu)return 0;
    const auto tags=read(memory,0x4f0414u);
    for(;;) {
        const auto tag=read(memory,(character&0xffffu)*16u+tags+8u);
        const auto count=std::bit_cast<std::int32_t>(read(memory,tag+0xccu));
        if(count>0) {
            const auto entries=read(memory,tag+0xd0u);
            for(std::int32_t n=0;n<count;) {
                const auto entry=std::uint32_t(n)*0xccu+entries;
                if(read(memory,entry+8u)==definition)return entry;
                n=std::int16_t(n+1);
            }
        }
        character=read(memory,tag+8u);
        if(character==0xffffffffu)return 0;
    }
}
// 1D2CF0 (EAX object, ESI out): the unit's 0x40000BD marker position, else a
// parentless physics root's origin (BBF20), else the object's +30 origin.
void object_aim_origin(SourcePhysicsContactMemory &memory,std::uint32_t object,std::uint32_t out,std::uint32_t entry_sp,
    std::uint32_t saved_ebx,std::uint32_t saved_edi,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const auto objects=read(memory,0x4ecbd0u);
    const auto rows=read(memory,objects+0x44u);
    std::uint32_t sp=entry_sp-0x70u;push(memory,sp,saved_ebx);push(memory,sp,saved_edi);
    const auto body=read(memory,rows+(object&0xffffu)*12u+8u);
    const auto type=1u<<(read(memory,body+0xaau,1)&31u);
    if((type&3u) || (type&0x1000u)) {
        const auto record=entry_sp-0x70u;
        push(memory,sp,1);push(memory,sp,0x40000bdu);push(memory,sp,object);push(memory,sp,0x1d2d37u);
        (void)native_campaign_object_marker_request_tu15(memory,object,false,0x40000bdu,1,record,fp);
        const auto x=read(memory,record+0x60u),y=read(memory,record+0x64u),z=read(memory,record+0x68u);
        publish(memory,observe,0x1d2d43u,out,x,4);publish(memory,observe,0x1d2d46u,out+4u,y,4);
        publish(memory,observe,0x1d2d49u,out+8u,z,4);
        return;
    }
    if(read(memory,body+0x14u)==0xffffffffu && read(memory,body+0xb4u)!=0xffffffffu) {
        push(memory,sp,0x1d2d6au);
        throw NativeCampaignAiPerformRequiredOwnerTu15(0x1d2d65u,0xbbf20u);
    }
    const auto x=read(memory,body+0x30u);publish(memory,observe,0x1d2d75u,out,x,4);
    const auto y=read(memory,body+0x34u);publish(memory,observe,0x1d2d7au,out+4u,y,4);
    const auto z=read(memory,body+0x38u);publish(memory,observe,0x1d2d81u,out+8u,z,4);
}
// Original 26B220 for the 1BBEE0 type-4 request ([esp+4] actor, [esp+8]
// request; RET 8). Other request types use services.callee.
void candidate_preparation(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &services,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto entry=abi.gp[4];
    const auto actor=read(memory,entry+4u);
    const auto pool=read(memory,actor_pool);
    const auto offset=(actor&0xffffu)*0x888u;
    std::uint32_t sp=entry-0x1cu;push(memory,sp,abi.gp[3]);
    const auto rows=read(memory,pool+0x44u);
    push(memory,sp,abi.gp[5]);
    const auto request=read(memory,entry+8u);
    const auto row=rows+offset;
    const auto mode=signed_word(read(memory,row+0x270u,2));
    push(memory,sp,abi.gp[6]);push(memory,sp,abi.gp[7]);
    const auto frame=sp;
    publish(memory,observe,0x26b24du,frame+0x18u,offset,4);
    publish(memory,observe,0x26b251u,frame+0x14u,row,4);
    std::uint32_t radius;
    if(mode>0)radius=read(memory,0x46a870u);
    else {
        const auto alert=read(memory,row+0x229u,1);
        radius=read(memory,0x46a20cu);
        if(!alert)radius=read(memory,0x46a108u);
    }
    publish(memory,observe,0x26b27bu,request+0x18u,radius,4);
    const auto previous=read(memory,request+0x1cu);
    abi.xmm[0]={previous,0,0,0};
    if(ordered_equal(fp.mxcsr,previous,read(memory,0x46a0e8u))) {
        const auto copied=read(memory,request+0x18u);publish(memory,observe,0x26b295u,request+0x1cu,copied,4);
    }
    auto target=read(memory,row+0x26cu);
    if(target==0xffffffffu)target=read(memory,row+0x18u);
    push(memory,sp,0x26b2b1u);
    object_aim_origin(memory,target,request+0x60cu,sp,row,abi.gp[7],fp,observe);
    sp+=4u;
    const auto type=signed_word(read(memory,request,2));
    publish(memory,observe,0x26b2d4u,request+0x53u,type==5 || type==8 || type==3,1);
    publish(memory,observe,0x26b2d7u,request+0x618u,0,1);
    // 1BBEE0 publishes type 4 itself. Target-driven types (2673A0/275FE0/
    // 1EBDB0) have no native owner and no resumable boundary here.
    if(type!=4)throw NativeCampaignAiPerformRequiredOwnerTu15(0x26b2deu,0x26b2ecu);
    publish(memory,observe,0x26b2e0u,request+0x618u,0,1);
    if(read(memory,request+0x618u,1)) {
        const auto again=signed_word(read(memory,request,2));
        if(again==0 || again==3 || again==2 || again==6)publish(memory,observe,0x26b622u,request+0x55u,1,1);
    }
    const auto self=read(memory,frame+0x30u);
    {std::uint32_t call_sp=sp;push(memory,call_sp,0x26b631u);}
    const auto weapon=aim_object(memory,self);
    const auto objects=read(memory,0x4ecbd0u);
    bool facing=false;
    if(weapon!=0xffffffffu) {
        const auto body=object_body(memory,objects,weapon);
        const auto definition=read(memory,body);
        std::uint32_t call_sp=sp;push(memory,call_sp,definition);push(memory,call_sp,0x26b655u);
        const auto entry_record=character_weapon(memory,self,definition);
        if(entry_record) {
            const std::array<std::uint32_t,3> v{read(memory,entry_record+0x60u),read(memory,entry_record+0x64u),read(memory,entry_record+0x68u)};
            std::array<std::uint32_t,3> squares{};
            facing=squared_length_above(fp.mxcsr,v,read(memory,0x46a0ecu),squares);
            abi.xmm[0]={v[2],0,0,0};abi.xmm[1]={squares[0],0,0,0};abi.xmm[2]={squares[1],0,0,0};abi.xmm[3]={squares[2],0,0,0};
            if(facing) {
                publish(memory,observe,0x26b691u,request+0x5fcu,1,1);
                const auto x=read(memory,entry_record+0x60u);publish(memory,observe,0x26b6a0u,request+0x600u,x,4);
                const auto y=read(memory,entry_record+0x64u);publish(memory,observe,0x26b6a5u,request+0x604u,y,4);
                const auto z=read(memory,entry_record+0x68u);publish(memory,observe,0x26b6abu,request+0x608u,z,4);
            }
        }
    }
    if(!facing)publish(memory,observe,0x26b6b0u,request+0x5fcu,0,1);
    if(signed_word(read(memory,row+0x358u,2))>0) {
        const auto prop=read(memory,row+0x368u);
        const auto props=read(memory,0x50eef0u),prop_rows=read(memory,props+0x44u);
        if(signed_word(read(memory,(prop&0xffffu)*0x3cu+prop_rows+0x24u,2))>=1) {
            std::uint32_t sum{};
            const auto seen=read(memory,row+0x39cu),delay=read(memory,0x46a0e4u),now=read(memory,row+0x394u);
            const bool above=sum_above(fp.mxcsr,seen,delay,now,sum);
            abi.xmm[0]={sum,0,0,0};
            if(above)publish(memory,observe,0x26b6fau,request+0x51u,1,1);
        }
    }
    if(signed_word(read(memory,row+0x270u,2))==4) {
        publish(memory,observe,0x26b708u,request+0x5bu,1,1);
        publish(memory,observe,0x26b70cu,request+0x5cu,1,1);
    }
    {
        const auto alert=read(memory,row+0x229u,1);
        const auto flags=read(memory,request+0x56u,1);
        publish(memory,observe,0x26b716u,request+0x56u,flags|alert,1);
    }
    std::uint32_t ecx{};
    const auto behavior=character_property(memory,read(memory,row+0x54u),0x5cu,ecx);
    if(!behavior) {
        publish(memory,observe,0x26b787u,request+0x59u,0,1);
        publish(memory,observe,0x26b78bu,request+0x58u,0,1);
        publish(memory,observe,0x26b78fu,request+0x5au,0,1);
    } else {
        const auto value=read(memory,behavior);
        if(value&0x20u) {
            const auto allowed=read(memory,request+0x59u,1)&((value>>4)&1u);
            publish(memory,observe,0x26b737u,request+0x59u,allowed,1);
            if(allowed)publish(memory,observe,0x26b73cu,request+0x5au,1,1);
            publish(memory,observe,0x26b740u,request+0x58u,1,1);
        } else publish(memory,observe,0x26b746u,request+0x59u,0,1);
        const auto unit=read(memory,row+0x18u);
        const auto body=object_body(memory,objects,unit);
        const auto definition_offset=std::int32_t(signed_word(read(memory,body+0x346u,2)));
        if(signed_word(read(memory,std::uint32_t(definition_offset)+body+0x36u,2))==5) {
            const auto vision=character_property(memory,read(memory,row+0x54u),0x34u,ecx);
            if(vision && !(read(memory,vision,1)&2u) && (read(memory,behavior,1)&0x20u))
                publish(memory,observe,0x26b781u,request+0x56u,1,1);
        }
    }
    publish(memory,observe,0x26b795u,request+0x274u,0,2);
    publish(memory,observe,0x26b79cu,request+0x276u,0,2);
    publish(memory,observe,0x26b7a3u,request+0x278u,0,2);
    if(signed_word(read(memory,row+0x86u,2))>=5) {
        const auto squad=read(memory,row+0x7cu);
        if(squad!=0xffffffffu) {
            const auto squads=read(memory,0x50eef8u),squad_rows=read(memory,squads+0x44u);
            (void)read(memory,(squad&0xffffu)*0x50u+squad_rows+0x10u,2);
        }
    }
    (void)read(memory,request,2);
    publish(memory,observe,0x26ba0bu,request+0x754u,0,2);
    abi.gp[0]=4u;abi.gp[1]=ecx&0xffffff00u;abi.gp[2]=1u;
    abi.gp[4]=entry+12u;
}
}

void native_campaign_ai_object_aim_origin_tu15(SourcePhysicsContactMemory &memory,std::uint32_t object,
    std::uint32_t out,std::uint32_t stack,std::uint32_t saved_ebx,std::uint32_t saved_edi,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    object_aim_origin(memory,object,out,stack,saved_ebx,saved_edi,fp,observe);
}

namespace {
// Original 26A2E0 under a caller-published FP context. ESP is at its return
// word; EDI actor, EBX request; [esp+4..10] arg0, &actor out, workspace, &flag
// out. The 32A660 probe touches each 4K page of the 0xF000 candidate frame.
void goal_selection(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &services,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto entry=abi.gp[4],actor=abi.gp[7],request=abi.gp[3],saved_esi=abi.gp[6];
    // mov eax,F000; call 32A660 (large path): push ecx, probe, restore, push return.
    std::uint32_t sp=entry;push(memory,sp,0x26a2eau);push(memory,sp,abi.gp[1]);
    for(std::uint32_t cursor=entry-0x1000u;;cursor-=0x1000u) {
        (void)read(memory,cursor);
        if(cursor==entry-0xf000u)break;
    }
    (void)read(memory,entry-0xf000u);
    abi.gp[1]=read(memory,entry-8u);
    const auto returned=read(memory,entry-4u);
    sp=entry-0xf000u;push(memory,sp,returned);sp+=4u;
    const auto pool=read(memory,actor_pool),rows=read(memory,pool+0x44u);
    push(memory,sp,saved_esi);
    const auto row=(actor&0xffffu)*0x888u+rows;
    const auto excluded=read(memory,row+7u,1);
    abi.gp[0]=(pool&0xffffff00u)|excluded;abi.gp[1]=rows;
    if(excluded) {
        abi.gp[0]=read(memory,none_cell);
        abi.gp[6]=saved_esi;abi.gp[4]=entry+0x14u;return;
    }
    const auto frame=sp+4u; // E-F000: the 0x200-entry candidate array.
    abi.gp[6]=row;
    push(memory,sp,request);push(memory,sp,actor);push(memory,sp,0x26a321u);abi.gp[4]=sp;
    if(services.differential_candidate_preparation) {
        // 26B220 publishes only into the request; everything else it touches
        // is its own dead frame below. Compare both owners on the live bank.
        std::vector<std::byte> before(0x758u),native(0x758u),translated(0x758u);
        memory.read(request,before);
        auto native_abi=abi;auto native_fp=fp;
        candidate_preparation(memory,native_abi,services,native_fp,observe);
        memory.read(request,native);memory.write(request,before);
        transport(services,0x26a31cu,0x26b220u,abi,fp);
        memory.read(request,translated);
        for(std::size_t n=0;n<native.size();++n)if(native[n]!=translated[n]) {
            std::ostringstream why;why<<"Native 26B220 differs from its translated body at request+"<<std::hex<<n
                <<" native="<<std::to_integer<unsigned>(native[n])<<" translated="<<std::to_integer<unsigned>(translated[n]);
            throw std::runtime_error(why.str());
        }
        if(native_fp.control_word!=fp.control_word || native_fp.mxcsr!=fp.mxcsr || native_fp.status!=fp.status ||
            native_abi.gp[4]!=abi.gp[4] || native_abi.gp[3]!=abi.gp[3] || native_abi.gp[5]!=abi.gp[5] ||
            native_abi.gp[6]!=abi.gp[6] || native_abi.gp[7]!=abi.gp[7])
            throw std::runtime_error("Native 26B220 FP/stack/preserved registers differ from its translated body");
        ++*services.differential_candidate_preparation;
    } else if(services.reference_candidate_preparation)transport(services,0x26a31cu,0x26b220u,abi,fp);
    else candidate_preparation(memory,abi,services,fp,observe);
    if(abi.gp[4]!=frame-4u)throw std::runtime_error("Native 26A2E0 candidate preparation returned another stack");
    const auto pinned=read(memory,row+0x220u,1);
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|pinned;
    if(pinned)publish(memory,observe,0x26a32bu,row+0x3f2u,1,1);
    sp=frame-4u;push(memory,sp,request);push(memory,sp,0x200u);push(memory,sp,frame);push(memory,sp,actor);
    push(memory,sp,0x26a343u);
    abi.gp[1]=frame;abi.gp[3]=request;abi.gp[4]=sp;abi.gp[6]=row;abi.gp[7]=actor;
    // 26BA30(actor, frame, 0x200, request). Its callees' frames lie below
    // this ESP and are dead on return; EAX is the dword candidate count.
    const auto collect=[&]{
        abi.gp[0]=native_campaign_ai_collect_candidates_tu15(memory,actor,frame,0x200u,request,fp,observe);
        abi.gp[4]=frame-4u;
    };
    if(services.differential_candidate_collection) {
        // 26BA30 publishes only into the candidate array and the request.
        constexpr std::uint32_t array_bytes=0x200u*0x78u;
        std::vector<std::byte> array_before(array_bytes),request_before(0x758u);
        memory.read(frame,array_before);memory.read(request,request_before);
        const auto entry_abi=abi;const auto entry_fp=fp;
        collect();
        const auto native_abi=abi;const auto native_fp=fp;
        std::vector<std::byte> array_native(array_bytes),request_native(0x758u);
        memory.read(frame,array_native);memory.read(request,request_native);
        memory.write(frame,array_before);memory.write(request,request_before);
        abi=entry_abi;fp=entry_fp;
        transport(services,0x26a33eu,0x26ba30u,abi,fp);
        std::vector<std::byte> array_translated(array_bytes),request_translated(0x758u);
        memory.read(frame,array_translated);memory.read(request,request_translated);
        const auto differ=[](const char *what,const std::vector<std::byte> &a,const std::vector<std::byte> &b) {
            for(std::size_t n=0;n<a.size();++n)if(a[n]!=b[n]) {
                std::ostringstream why;why<<"Native 26BA30 differs from its translated body at "<<what<<"+"<<std::hex<<n
                    <<" native="<<std::to_integer<unsigned>(a[n])<<" translated="<<std::to_integer<unsigned>(b[n]);
                throw std::runtime_error(why.str());
            }
        };
        differ("candidates",array_native,array_translated);
        differ("request",request_native,request_translated);
        if(native_abi.gp[0]!=abi.gp[0] || native_fp.control_word!=fp.control_word || native_fp.mxcsr!=fp.mxcsr ||
            native_fp.status!=fp.status || native_abi.gp[4]!=abi.gp[4] || native_abi.gp[3]!=abi.gp[3] ||
            native_abi.gp[5]!=abi.gp[5] || native_abi.gp[6]!=abi.gp[6] || native_abi.gp[7]!=abi.gp[7]) {
            std::ostringstream why;why<<"Native 26BA30 count/FP/stack/preserved registers differ from its translated body: count "
                <<std::hex<<native_abi.gp[0]<<"/"<<abi.gp[0]<<" status "<<native_fp.status<<"/"<<fp.status
                <<" mxcsr "<<native_fp.mxcsr<<"/"<<fp.mxcsr;
            throw std::runtime_error(why.str());
        }
        ++*services.differential_candidate_collection;
    } else if(services.reference_candidate_collection)transport(services,0x26a33eu,0x26ba30u,abi,fp);
    else collect();
    if(abi.gp[4]!=frame-4u)throw std::runtime_error("Native 26A2E0 candidate collection returned another stack");
    const auto count=abi.gp[0];
    sp=frame-4u;
    const auto flag_out=read(memory,entry+0x10u),workspace=read(memory,entry+0xcu);
    push(memory,sp,flag_out);
    const auto actor_out=read(memory,entry+8u);
    push(memory,sp,workspace);
    const auto argument=read(memory,entry+4u);
    push(memory,sp,actor_out);push(memory,sp,argument);push(memory,sp,count);push(memory,sp,frame);
    push(memory,sp,request);push(memory,sp,actor);push(memory,sp,0x26a370u);
    abi.gp[1]=argument;abi.gp[2]=frame;abi.gp[3]=request;abi.gp[4]=sp;abi.gp[6]=row;abi.gp[7]=actor;
    // Native 26A380 exit without candidates: EAX = EDX = NONE, ECX = 0 from
    // its history loop, XMM0 cleared by its XORPS, ESP after RET 20.
    const auto evaluate=[&]{
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        const auto publish=[&]{fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;};
        try {native_campaign_ai_evaluate_goals_with_query_call_tu15(memory,abi,observe);}
        catch(...){publish();throw;}
        publish();const auto goal=abi.gp[0];
        abi.gp[0]=goal;abi.gp[1]=0;abi.gp[2]=goal;abi.xmm[0]={0,0,0,0};abi.gp[4]=frame-4u;
    };
    if(services.differential_goal_evaluation) {
        // This legacy differential probe snapshots only prefix fields. The
        // bound query/search also owns candidate, workspace and stack bytes;
        // refuse that unsupported probe before either body can change them.
        if(count&0xffffu)throw std::invalid_argument("Native 26A380 differential probe requires no candidates");
        // The prefix stores: *flag_out, [5020A8]+3 and the actor's history.
        const auto globals=read(memory,0x5020a8u)+3u,history=row+0x3feu;
        std::array<std::byte,1> flag_before{},global_before{};std::vector<std::byte> history_before(0x1eu);
        memory.read(flag_out,flag_before);memory.read(globals,global_before);memory.read(history,history_before);
        const auto entry_abi=abi;const auto entry_fp=fp;
        bool completed=true;
        try {evaluate();}
        catch(const NativeCampaignAiPerformRequiredOwnerTu15 &){completed=false;}
        const auto native_abi=abi;
        std::array<std::byte,1> flag_native{},global_native{};std::vector<std::byte> history_native(0x1eu);
        memory.read(flag_out,flag_native);memory.read(globals,global_native);memory.read(history,history_native);
        memory.write(flag_out,flag_before);memory.write(globals,global_before);memory.write(history,history_before);
        abi=entry_abi;fp=entry_fp;
        transport(services,0x26a36bu,0x26a380u,abi,fp);
        if(completed) {
            std::array<std::byte,1> flag_translated{},global_translated{};std::vector<std::byte> history_translated(0x1eu);
            memory.read(flag_out,flag_translated);memory.read(globals,global_translated);memory.read(history,history_translated);
            if(flag_native!=flag_translated || global_native!=global_translated || history_native!=history_translated ||
                native_abi.gp[0]!=abi.gp[0] || native_abi.gp[4]!=abi.gp[4])
                throw std::runtime_error("Native 26A380 (no candidates) differs from its translated body");
            ++(*services.differential_goal_evaluation)[0];
        } else ++(*services.differential_goal_evaluation)[1];
    } else if(services.reference_goal_evaluation)transport(services,0x26a36bu,0x26a380u,abi,fp);
    else evaluate();
    if(abi.gp[4]!=frame-4u)throw std::runtime_error("Native 26A2E0 goal evaluation returned another stack");
    abi.gp[6]=saved_esi;abi.gp[4]=entry+0x14u;
}
}

void native_campaign_ai_goal_selection_call_tu15(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>3u)
        throw std::invalid_argument("Native 26A2E0 requires retained FP controls and five free x87 temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    FpPublication publication{abi,fp};
    goal_selection(memory,abi,services,fp,observe);
}

void native_campaign_ai_area_perform_tu15(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>3u)
        throw std::invalid_argument("Native 1BBEE0 requires retained FP controls and five free x87 temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    FpPublication publication{abi,fp};
    const auto entry_sp=abi.gp[4],ebp=entry_sp-4u;
    const auto saved_ebp=abi.gp[5],saved_ebx=abi.gp[3],saved_esi=abi.gp[6],saved_edi=abi.gp[7];
    // push ebp; and esp,-8; sub esp,76C; push ebx/esi/edi.
    std::uint32_t sp=entry_sp;push(memory,sp,saved_ebp);
    sp=(sp&~7u)-0x76cu;
    const auto pool=read(memory,actor_pool),rows=read(memory,pool+0x44u);
    push(memory,sp,saved_ebx);push(memory,sp,saved_esi);push(memory,sp,saved_edi);
    const auto frame=sp;
    const auto actor=read(memory,ebp+8u);
    const auto offset=(actor&0xffffu)*0x888u,row=rows+offset;
    publish(memory,observe,0x1bbf0cu,frame+0x14u,offset,4);
    const auto active=read(memory,row+0x40u,1);
    publish(memory,observe,0x1bbf17u,frame+0xfu,1,1);
    // Exit: pop edi/esi/ebx, mov esp,ebp, pop ebp, ret 8. Only AL is the
    // result; EBX/ESI/EDI/EBP are the caller's. ECX/EDX/upper EAX are owned
    // by the last instruction that wrote them.
    const auto finish=[&](std::uint8_t result) {
        abi.gp[0]=(abi.gp[0]&0xffffff00u)|result;
        abi.gp[3]=saved_ebx;abi.gp[5]=saved_ebp;abi.gp[6]=saved_esi;abi.gp[7]=saved_edi;
        abi.gp[4]=entry_sp+12u;
    };
    if(!active) {
        abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[1]=(offset&0xffffff00u);abi.gp[2]=rows;
        finish(1);return;
    }
    std::uint32_t state;
    bool reached_area=false;
    const auto current=read(memory,row+0x418u);
    if(current!=read(memory,none_cell)) {
        bool choose=signed_word(read(memory,row+0x504u,2))==2;
        if(!choose) {
            // eax=actor; call 200C00 (actual RET); the native owner reads the
            // pool, then publishes XORPS XMM0 before its first actor guard.
            abi.gp[0]=actor;
            std::uint32_t call_sp=frame;push(memory,call_sp,0x1bbf40u);
            const auto distance_pool=read(memory,actor_pool),distance_rows=read(memory,distance_pool+0x44u);
            abi.xmm[0].fill(0);
            std::uint32_t distance;
            try {distance=native_campaign_ai_actor_path_distance_tu15(memory,distance_rows+offset,fp,abi.x87_depth,
                [&](const NativeCampaignAiProjectionCoordinateTu15 &coordinate){abi.xmm[0]={coordinate.bits,0,0,0};});}
            catch(const NativeCampaignAiPathDistancePendingErrorTu15 &error){
                retain_x87(abi,error.pending().previous_z_dy_dx);throw;}
            abi.xmm[0]={distance,0,0,0};
            const auto threshold=read(memory,0x456914u);
            abi.xmm[1]={threshold,0,0,0};
            choose=scalar_above(fp.mxcsr,threshold,distance);
        }
        if(choose) {
            const auto goal=read(memory,row+0x418u);
            std::uint32_t call_sp=frame;push(memory,call_sp,goal);push(memory,call_sp,0x1bbf5du);
            abi.gp[0]=actor;abi.gp[3]=saved_ebx&0xffffff00u;
            native_campaign_ai_action_history_tu15(memory,actor,goal,0,observe);
            state=read(memory,ebp+0xcu);
            call_sp=frame;push(memory,call_sp,state+0xcu);push(memory,call_sp,actor);push(memory,call_sp,0x1bbf6au);
            bool chosen;
            try {chosen=native_campaign_ai_choose_area_tu15(memory,actor,state+0xcu,fp,abi.x87_depth,observe);}
            catch(const NativeCampaignAiAreaScorePendingErrorTu15 &error){
                const auto &pending=error.pending();
                retain_x87(abi,std::span(pending.quantities).first(pending.count));throw;}
            publish(memory,observe,0x1bbf6cu,frame+0xfu,chosen,1);
            if(!chosen){finish(std::uint8_t(read(memory,frame+0xfu,1)));return;}
            reached_area=true;
        }
    }
    if(!reached_area)state=read(memory,ebp+0xcu);
    // call 1D2D90: lease the path workspace.
    {std::uint32_t call_sp=frame;push(memory,call_sp,0x1bbf80u);}
    const auto workspace=services.protection
        ?native_campaign_ai_path_workspace_acquire_tu15(memory,*services.protection,observe)
        :native_campaign_ai_path_workspace_acquire_tu15(memory,observe);
    const auto radius=read(memory,0x46a1a4u);
    abi.xmm[0]={radius,0,0,0};
    const bool near_class=signed_word(read(memory,row+0x328u,2))<=2;
    // rep stosd: 0x1D6 ordered zero DWORDs over the type-4 request.
    const auto request=frame+0x20u;
    for(unsigned n=0;n<0x1d6u;++n)publish(memory,observe,0x1bbf9du,request+n*4u,0,4);
    const auto word12=read(memory,state+0x12u,2),word10=read(memory,state+0x10u,2);
    (void)read(memory,ebp+8u);
    publish(memory,observe,0x1bbfaau,frame+0x6b0u,word12,2);
    std::uint32_t call_sp=frame;
    push(memory,call_sp,frame+0x1cu);push(memory,call_sp,workspace);
    publish(memory,observe,0x1bbfb8u,frame+0x10u,workspace,4);
    publish(memory,observe,0x1bbfc0u,frame+0x6aeu,word10,2);
    push(memory,call_sp,frame+0x18u);push(memory,call_sp,0);
    publish(memory,observe,0x1bbfd2u,request,4,2);
    publish(memory,observe,0x1bbfd9u,request+0x5bu,1,1);
    publish(memory,observe,0x1bbfe1u,request+0x68cu,1,1);
    publish(memory,observe,0x1bbfe9u,request+0x1cu,radius,4);
    publish(memory,observe,0x1bbfefu,request+0x15u,1,1);
    publish(memory,observe,0x1bbff4u,request+0x59u,1,1);
    publish(memory,observe,0x1bbffcu,request+0x69cu,near_class,1);
    push(memory,call_sp,0x1bc008u);
    // 26A2E0(0,&out_actor,workspace,&out_flag) with EDI actor, EBX request.
    abi.gp[0]=(word10&0xff00u)|unsigned(near_class);abi.gp[1]=frame+0x1cu;abi.gp[2]=frame+0x18u;
    abi.gp[3]=request;abi.gp[4]=call_sp;abi.gp[5]=ebp;abi.gp[6]=row;abi.gp[7]=actor;
    goal_selection(memory,abi,services,fp,observe);
    if(abi.gp[4]!=frame)throw std::runtime_error("Native 1BBEE0 goal selection returned another stack");
    const auto goal=abi.gp[0];
    if(goal==read(memory,none_cell)) {
        const auto clear_pool=read(memory,actor_pool),clear_rows=read(memory,clear_pool+0x44u);
        const auto clear_offset=read(memory,frame+0x14u);
        publish(memory,observe,0x1bc01eu,clear_rows+clear_offset+0x40u,0,1);
        const auto lease=read(memory,frame+0x10u);
        publish(memory,observe,0x1bc026u,frame+0xfu,0,1);
        call_sp=frame;push(memory,call_sp,0x1bc02fu);
        native_campaign_ai_path_workspace_release_tu15(memory,lease,observe);
        abi.gp[0]&=0xffffff00u;
        finish(std::uint8_t(read(memory,frame+0xfu,1)));return;
    }
    const auto out_flag=read(memory,frame+0x1cu),lease=read(memory,frame+0x10u);
    (void)read(memory,ebp+8u);
    call_sp=frame;push(memory,call_sp,1);push(memory,call_sp,out_flag);
    const auto out_actor=read(memory,frame+0x18u);
    push(memory,call_sp,lease);push(memory,call_sp,out_actor);push(memory,call_sp,goal);push(memory,call_sp,actor);
    push(memory,call_sp,0x1bc059u);
    abi.gp[1]=out_actor;abi.gp[2]=lease;abi.gp[3]=goal;abi.gp[4]=call_sp;abi.gp[5]=ebp;abi.gp[6]=row;abi.gp[7]=actor;
    // Native 26C3C0 (its unported callees refuse by name inside it).
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
    {
        const auto adopt=[&]{fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;fp.status=abi.x87_status;};
        try{native_campaign_ai_publication_tu15(memory,abi,services,observe);}
        catch(...){adopt();throw;}
        adopt();
    }
    if(abi.gp[4]!=frame)throw std::runtime_error("Native 1BBEE0 publication returned another stack");
    if(abi.gp[0]==read(memory,none_cell)) {
        const auto failures=signed_word(read(memory,row+0x5b4u,2));
        bool forget=failures>4;
        if(!forget && read(memory,row+0x26cu)!=0xffffffffu && failures>2)forget=true;
        if(forget) {
            call_sp=frame;push(memory,call_sp,goal);push(memory,call_sp,0x1bc087u);
            native_campaign_ai_action_history_tu15(memory,actor,goal,0,observe);
        }
    }
    const auto release=read(memory,frame+0x10u);
    call_sp=frame;push(memory,call_sp,0x1bc090u);
    native_campaign_ai_path_workspace_release_tu15(memory,release,observe);
    finish(std::uint8_t(read(memory,frame+0xfu,1)));
}

void native_campaign_ai_area_perform_release_tu15(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto sp=abi.gp[4];
    const auto actor=read(memory,sp+4u);
    const auto pool=read(memory,actor_pool),rows=read(memory,pool+0x44u);
    const auto row=(actor&0xffffu)*0x888u+rows;
    publish(memory,observe,0x1bc0bcu,row+0x4aeu,1,1);
    publish(memory,observe,0x1bc0c2u,row+0x484u,1,1);
    if(read(memory,row+0x26cu)!=0xffffffffu) {
        const auto scalar=read(memory,0x456914u);
        abi.xmm[0]={scalar,0,0,0};
        publish(memory,observe,0x1bc0d9u,row+0x4b4u,scalar,4);
    }
    abi.gp[0]=row;abi.gp[1]=(pool&0xffffff00u)|1u;abi.gp[2]=rows;abi.gp[4]=sp+12u;
}
}
