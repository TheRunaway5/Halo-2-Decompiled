#include "halo2/native_campaign_ai_route_smoothing_tu15.hpp"
#include "halo2/native_campaign_ai_route_obstacles_tu15.hpp"
#include "halo2/native_campaign_ai_route_traversal_tu15.hpp"
#include "halo2/native_campaign_ai_route_corridor_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_vector_math_tu15.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <cstring>
#include <vector>
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;
using i16=std::int16_t;using i32=std::int32_t;
constexpr u32 none=0xffffffffu;
i32 sw(u32 v){return std::bit_cast<i16>(u16(v));}
u32 row_offset(u32 v){return u32(sw(v))*28u;}
float real(u32 bits){return std::bit_cast<float>(bits);}
u32 bits(float value){return std::bit_cast<u32>(value);}
struct Scalar {
    NativeTu15PhysicsFrameFp &fp;
    template<class F> u32 operation(u32 a,u32 b,F calculate)const {
        u32 host{},csr=fp.mxcsr;asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(host):"m"(csr):"memory");
        auto x=real(a);calculate(x,real(b));
        asm volatile("stmxcsr %0\n\tldmxcsr %1":"=m"(fp.mxcsr):"m"(host):"memory");return bits(x);
    }
    u32 add(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 sub(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 mul(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");});}
    u32 div(u32 a,u32 b)const{return operation(a,b,[](float &x,float y){asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");});}
    bool above(u32 a,u32 b)const {
        u32 host{},csr=fp.mxcsr;u8 answer{};const auto x=real(a),y=real(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[y],%[x]\n\tseta %[answer]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
          :[host]"+m"(host),[csr]"+m"(csr),[answer]"=&q"(answer):[x]"x"(x),[y]"x"(y):"memory","cc");
        fp.mxcsr=csr;return answer!=0;
    }
    bool zero(u32 a,u32 b)const {
        u32 host{},csr=fp.mxcsr;u8 equal{},ordered{};const auto x=real(a),y=real(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tucomiss %[y],%[x]\n\tsete %[equal]\n\tsetnp %[ordered]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
          :[host]"+m"(host),[csr]"+m"(csr),[equal]"=&q"(equal),[ordered]"=&q"(ordered):[x]"x"(x),[y]"x"(y):"memory","cc");
        fp.mxcsr=csr;return equal&&ordered;
    }
    bool below(u32 a,u32 b)const {
        u32 host{},csr=fp.mxcsr;u8 answer{};const auto x=real(a),y=real(b);
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcomiss %[y],%[x]\n\tsetb %[answer]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
          :[host]"+m"(host),[csr]"+m"(csr),[answer]"=&q"(answer):[x]"x"(x),[y]"x"(y):"memory","cc");fp.mxcsr=csr;return answer!=0;
    }
};
struct Smoothing {
    SourcePhysicsContactMemory &memory;SourcePhysicsWorldRuntimeAbi &abi;
    const NativeCampaignAiPerformServicesTu15 &services;NativeTu15PhysicsFrameFp &fp;
    const SourcePhysicsContactRecordObserver &observe;Scalar scalar{fp};
    u32 r(u32 address,unsigned width=4)const {
        std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));u32 value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);
        return value;
    }
    void w(u32 site,u32 address,u32 value,unsigned width=4)const {
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
        if(observe){event.before.resize(width);memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
        memory.write(address,event.written);if(observe)observe(event);
    }
    void push(u32 &sp,u32 value)const {
        sp-=4u;std::array<std::byte,4> raw{};
        for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
        memory.write(sp,raw);
    }
    void copy(u32 source,u32 destination,u32 site,unsigned words=7)const {
        for(unsigned n=0;n<words;++n){auto value=r(source+n*4u);w(site,destination+n*4u,value);}
    }

    void child(u32 site,u32 target,std::array<u32,8> gp,u32 cleanup) {
        const auto caller_sp=gp[4];push(gp[4],site+5u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        auto import_fp=[&](){if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}fp.status=abi.x87_status;};
        try {
            if(target==0x2cabe0u)native_campaign_ai_route_obstacles_call_tu15(memory,target,abi,services,observe);
            else if(target==0x2769c0u)native_campaign_ai_route_traversal_call_tu15(memory,target,abi,services,observe);
            else if(target==0x2cdeb0u)native_campaign_ai_route_corridor_call_tu15(memory,target,abi,services,observe);
            else if(target==0x2cbe30u || target==0x2cbcf0u)native_campaign_ai_route_smoothing_call_tu15(memory,target,abi,services,observe);
            else if(services.callee)services.callee(site,target,abi);
            else throw NativeCampaignAiPerformRequiredOwnerTu15(site,target);
        }catch(...){import_fp();throw;}
        if(!abi.floating_point || abi.gp[4]!=caller_sp+cleanup)throw std::runtime_error("Native smoothing child returned another ABI");
        import_fp();
    }
    void cluster(u32 entry,const std::array<u32,8> &caller);
    void connected(u32 entry,const std::array<u32,8> &caller,u32 margin);
    u8 run(u32 entry,const std::array<u32,8> &caller);
};

void Smoothing::connected(u32 entry,const std::array<u32,8> &caller,u32 margin) {
    const auto records=r(entry+4u);const auto count=sw(r(records+2u,2));const auto words=u32((count+31)>>5);
    auto sp=entry-0x80u;push(sp,caller[5]);push(sp,caller[6]);push(sp,caller[7]);const auto f=sp,mask=caller[3];
    for(u32 n=0;n<words;++n)w(0x2cbd16u,mask+n*4u,0);
    const auto first=caller[0]&0xffffu;if(first==0xffffu)return;
    const auto first_index=u32(sw(first)),bit=1u<<(first_index&31u),word=mask+u32(sw(first)>>5)*4u;auto flags=r(word);
    w(0x2cbd41u,f+0xcu,first,2);w(0x2cbd48u,word,flags|bit);u32 pending=1;
    for(;;) {
        --pending;const auto current=r(f+u32(sw(pending))*2u+0xcu,2),node=records+u32(sw(current))*20u+0xcu;
        u32 current_records=records;
        if(sw(r(current_records+2u,2))>0)for(u32 candidate=0;;++candidate) {
            const auto index=u32(sw(candidate)),at=mask+u32(sw(candidate)>>5)*4u,flag=1u<<(index&31u);
            if(!(r(at)&flag)) {
                current_records=r(f+0x90u);auto radius=r(node+0x10u);radius=scalar.add(radius,margin);
                auto other_radius=r(current_records+index*20u+0x1cu),dy=r(current_records+index*20u+0x18u);dy=scalar.sub(dy,r(node+0xcu));
                const auto other=current_records+index*20u+0xcu;other_radius=scalar.add(other_radius,margin);auto limit=scalar.add(other_radius,radius);
                auto dx=r(other+8u);dx=scalar.sub(dx,r(node+8u));auto distance=scalar.add(scalar.mul(dx,dx),scalar.mul(dy,dy));limit=scalar.mul(limit,limit);
                if(!scalar.below(limit,distance)){auto before=r(at);w(0x2cbdfau,at,before|flag);w(0x2cbdffu,f+u32(sw(pending))*2u+0xcu,candidate,2);++pending;}
            }
            current_records=r(f+0x90u);if(sw(candidate+1u)>=sw(r(current_records+2u,2)))break;
        }
        if(sw(pending)<=0)break;
    }
}

void Smoothing::cluster(u32 entry,const std::array<u32,8> &caller) {
    auto sp=entry-8u;const auto records=caller[6];const auto first_count=r(records+2u,2);w(0x2cbe39u,records,0,2);
    if(sw(first_count)>0)for(u32 n=0;;++n){w(0x2cbe47u,records+u32(sw(n))*20u+0xeu,0xffffu,2);if(sw(n+1u)>=sw(r(records+2u,2)))break;}
    push(sp,caller[5]);if(sw(r(records+2u,2))<=0)return;
    const auto margin=r(sp+0x10u);push(sp,caller[3]);push(sp,caller[7]);const auto f=sp;
    for(u32 n=0;;++n) {
        if(r(records+u32(sw(n))*20u+0xeu,2)==0xffffu) {
            const auto group=r(records,2);auto call_sp=f;push(call_sp,records);w(0x2cbe8eu,records,group+1u,2);abi.xmm[3][0]=margin;
            child(0x2cbe93u,0x2cbcf0u,{n,abi.gp[1],abi.gp[2],f+0xcu,call_sp,n,records,group},4u);
            if(sw(r(records+2u,2))>0)for(u32 candidate=0;;++candidate){auto index=u32(sw(candidate)),flag=1u<<(index&31u);if(r(f+u32(sw(candidate)>>5)*4u+0xcu)&flag)w(0x2cbebdu,records+index*20u+0xeu,group,2);if(sw(candidate+1u)>=sw(r(records+2u,2)))break;}
        }
        if(sw(n+1u)>=sw(r(records+2u,2)))break;
    }
}

u8 Smoothing::run(u32 entry,const std::array<u32,8> &caller) {
    auto sp=entry;push(sp,0x2ce03au);push(sp,caller[1]);(void)r(entry-0x1000u);(void)r(entry-0x1f88u);
    (void)r(entry-8u);auto return_pc=r(entry-4u);sp=entry-0x1f88u;push(sp,return_pc);sp+=4u;
    auto request=r(sp+0x1f90u),width=r(request),minimum=r(0x46a16cu),actor=r(sp+0x1f8cu);
    push(sp,caller[3]);auto navigation=r(request+0x8cu);push(sp,caller[5]);push(sp,caller[6]);auto object_out=r(sp+0x1fb8u);
    auto wide=scalar.above(width,minimum);push(sp,caller[7]);const auto f=sp;auto obstacle_flag=r(f+0x1fc4u);
    w(0x2ce071u,f+0x13u,1,1);w(0x2ce076u,f+0x4cu,0);w(0x2ce07au,f+0x44u,navigation);if(wide)minimum=width;
    auto complete=r(f+0x1fb8u),complete_byte=r(complete,1);w(0x2ce08cu,f+0x2bu,complete_byte,1);w(0x2ce097u,f+0x2cu,minimum);
    auto scale=r(0x46a120u);w(0x2ce0a5u,f+0x6cu,scale);w(0x2ce0abu,f+0x68u,0);w(0x2ce0afu,f+0x14u,0);u32 actor_record{};
    if(actor!=none){auto pool=r(0x5020c8u),actors=r(pool+0x44u);actor_record=actors+(actor&0xffffu)*0x888u;auto object=r(actor_record+0x26cu);w(0x2ce0d6u,f+0x4cu,actor_record);if(object!=none){scale=r(0x46a118u);w(0x2ce0e4u,f+0x6cu,scale);}}
    if(object_out)w(0x2ce0eeu,object_out,none);
    auto cache=r(request+0x50u);if(cache && !r(cache+0x147b0u,1))w(0x2ce100u,cache+0x147b2u,0,2);
    if(obstacle_flag)w(0x2ce10du,obstacle_flag,0,1);
    if(actor_record) {
        auto object=r(actor_record+0x26cu);w(0x2ce11eu,f+0x14u,1);
        if(object==none) {
            auto mode=r(actor_record+0x86u,2);w(0x2ce138u,f+0x14u,3);
            bool expand=sw(mode)>=3;if(!expand)expand=r(f+0x1facu,1)!=0;
            if(expand) {
                auto flags=r(request+0x140b8u);w(0x2ce155u,f+0x14u,7);
                if((flags&8u) && (r(request+0x140bcu,1)&3u))w(0x2ce168u,f+0x14u,15);
                if((flags&16u) && (r(request+0x140bcu,1)&0xe0u)){auto mask=r(f+0x14u);w(0x2ce17du,f+0x14u,mask|16u);}
                if((flags&32u) && (r(request+0x140bcu)&0x1800u)){auto mask=r(f+0x14u);w(0x2ce191u,f+0x14u,mask|32u);}
                if(r(f+0x1facu,1)){auto controls=r(0x47b49cu),state=r(controls+8u),disabled=r(0x47b908u);if(state!=disabled){auto active=r(controls+4u),wanted=r(0x4fafc0u);if((active&wanted)==wanted && (r(0x56ae84u)&0x200u)){auto mask=r(f+0x14u);w(0x2ce1cbu,f+0x14u,mask|64u);}}}
            }
        }
    }
    auto count=r(f+0x1fa4u,2);w(0x2ce1ddu,f+0x38u,0);if(sw(count)<=0)return u8(r(f+0x13u,1));w(0x2ce1ebu,f+0x70u,u32(sw(count)-1));
    u32 index=0;
    for(;;) {
        auto last=r(f+0x70u);w(0x2ce1fbu,f+0x1cu,f+0x11e8u);u32 obstacles=f+0xcdcu;
        bool terminal=false;if(u32(sw(index))==last){auto original_complete=r(f+0x2bu,1);w(0x2ce213u,f+0x30u,1,1);terminal=original_complete!=0;}
        if(!terminal)w(0x2ce21au,f+0x30u,0,1);
        auto count_output=r(f+0x1fb0u),already=r(count_output,2);w(0x2ce22au,f+0x3cu,none);
        if(sw(already)>=4){auto completion=r(f+0x1fb8u);w(0x2ceb83u,completion,0,1);return u8(r(f+0x13u,1));}
        u32 previous{},sector{};if(sw(index)>0){sector=r(f+0x18u);previous=f+0x5cu;}else{previous=request+0x14u;sector=r(request+0x24u);}
        w(0x2ce24du,f+0x48u,previous);w(0x2ce256u,f+0x34u,sector);auto rows=r(f+0x1fa8u),row=rows+row_offset(index),kind=r(row,2),target_sector=r(row+4u);w(0x2ce26au,f+0x24u,row);w(0x2ce26eu,f+0x18u,target_sector);
        if(kind==0xffffu) {
            request=r(f+0x1fa0u);auto frame=r(row+0x18u,2);w(0x2ce285u,f+0x40u,frame,2);cache=r(request+0x50u);
            if(cache){obstacles=cache+u32(sw(index))*0x50cu+0x147b4u;w(0x2ce2adu,f+0x1cu,cache+u32(sw(index))*0xdb0u+0x15be4u);}
            previous=r(f+0x48u);auto x=r(row+0xcu);x=scalar.sub(x,r(previous));w(0x2ce2c1u,f+0x9cu,x);auto y=r(row+0x10u);y=scalar.sub(y,r(previous+4u));w(0x2ce2d4u,f+0xa0u,y);auto z=r(row+0x14u);z=scalar.sub(z,r(previous+8u));w(0x2ce2eeu,f+0xa4u,z);
            auto call_sp=f;push(call_sp,0x2ce2fcu);native_campaign_ai_normalize_vector_tu15(memory,f+0x9cu,fp,observe);
            actor_record=r(f+0x4cu);if(actor_record){auto a=r(actor_record+0x290u),b=r(actor_record+0x294u),c=r(actor_record+0x298u);w(0x2ce311u,f+0xa8u,a);w(0x2ce318u,f+0xacu,b);w(0x2ce31fu,f+0xb0u,c);}
            auto frame_word=r(f+0x40u);call_sp=f;push(call_sp,previous);push(call_sp,0x2ce337u);
            if(!native_campaign_ai_resolve_frame_tu15(memory,u16(frame_word),previous,f+0x90u,fp,observe)){auto a=r(previous),b=r(previous+4u),c=r(previous+8u);w(0x2ce343u,f+0x90u,a);w(0x2ce34au,f+0x94u,b);w(0x2ce351u,f+0x98u,c);}
            for(auto part:std::array<std::array<u32,2>,5>{{{0,0x2ce35au},{2,0x2ce35eu},{4,0x2ce362u},{6,0x2ce366u},{8,0x2ce36au}}})w(part[1],obstacles+part[0],0,2);
            auto parameter_c=r(request+0xcu),parameter_8=r(request+8u);call_sp=f;push(call_sp,parameter_c);actor_record=r(f+0x4cu);push(call_sp,parameter_8);scale=r(f+0x6cu);push(call_sp,f+0x9cu);push(call_sp,scale);push(call_sp,f+0x90u);push(call_sp,f+0xd0u);auto mask=r(f+0x14u);push(call_sp,obstacles);push(call_sp,request+0x140b8u);actor=r(f+0x1f9cu);push(call_sp,mask);push(call_sp,actor);
            for(auto part:std::array<std::array<u32,2>,5>{{{0,0x2ce3b9u},{2,0x2ce3c1u},{4,0x2ce3c9u},{6,0x2ce3d1u},{8,0x2ce3d9u}}})w(part[1],f+0xd0u+part[0],0,2);
            child(0x2ce3e1u,0x2cabe0u,{mask,actor_record?f+0xa8u:0u,actor,request,call_sp,obstacles,previous,0},0x28u);
            if(sw(r(obstacles+2u,2))>0) {
                for(u32 n=0;;++n){const auto obstacle=obstacles+u32(sw(n))*20u+0xcu;auto px=r(obstacle+8u),fw=r(f+0x40u);w(0x2ce404u,f+0x78u,px);auto py=r(obstacle+0xcu);w(0x2ce413u,f+0x7cu,py);w(0x2ce41eu,f+0x80u,0);call_sp=f;push(call_sp,0x2ce42cu);native_campaign_ai_inverse_frame_point_tu15(memory,u16(fw),f+0x78u,f+0x78u,fp,observe);px=r(f+0x78u);w(0x2ce432u,obstacle+8u,px);py=r(f+0x7cu);w(0x2ce43eu,obstacle+0xcu,py);if(sw(n+1u)>=sw(r(obstacles+2u,2)))break;}
                request=r(f+0x1fa0u);
            }
            if(sw(r(f+0xd2u,2))>0){for(u32 n=0;;++n){const auto obstacle=f+u32(sw(n))*20u+0xdcu;auto px=r(obstacle+8u),fw=r(f+0x40u);w(0x2ce47au,f+0x84u,px);auto py=r(obstacle+0xcu);w(0x2ce48fu,f+0x88u,py);w(0x2ce49du,f+0x8cu,0);call_sp=f;push(call_sp,0x2ce4abu);native_campaign_ai_inverse_frame_point_tu15(memory,u16(fw),f+0x84u,f+0x84u,fp,observe);px=r(f+0x84u);w(0x2ce4b4u,obstacle+8u,px);py=r(f+0x88u);w(0x2ce4c3u,obstacle+0xcu,py);if(sw(n+1u)>=sw(r(f+0xd2u,2)))break;}request=r(f+0x1fa0u);}
            if(r(request+0x28u,1) && r(request+0x44u,1)){auto fw=r(f+0x40u);call_sp=f;push(call_sp,0x2ce4feu);native_campaign_ai_inverse_frame_point_tu15(memory,u16(fw),request+0x2cu,f+0xb4u,fp,observe);auto req=r(f+0x1fa0u),radius=r(req+0x3cu),object=r(req+0x38u),n=r(obstacles+2u,2);if(n!=0x40u){auto px=r(f+0xb4u),at=obstacles+u32(sw(n))*20u+0xcu;auto active=r(obstacles+4u,2);w(0x2ce52du,obstacles+4u,active+1u,2);w(0x2ce531u,obstacles+2u,n+1u,2);w(0x2ce535u,at,1,2);w(0x2ce53au,at+2u,0xffffu,2);w(0x2ce540u,at+4u,object);w(0x2ce543u,at+8u,px);auto py=r(f+0xb8u);w(0x2ce551u,at+0xcu,py);w(0x2ce556u,at+0x10u,radius);}}
            auto margin=r(f+0x2cu);call_sp=f;push(call_sp,margin);child(0x2ce562u,0x2cbe30u,{abi.gp[0],margin,abi.gp[2],request,call_sp,obstacles,obstacles,abi.gp[7]},4u);
            auto req=r(f+0x1fa0u),cache_again=r(req+0x50u);if(cache_again && !r(cache_again+0x147b0u,1)){auto seen=r(cache_again+0x147b2u,2);w(0x2ce57fu,cache_again+0x147b2u,seen+1u,2);}
        }
        row=r(f+0x24u);const auto route_kind=r(row,2);bool routed=false;
        if(route_kind==0xffffu && sw(r(obstacles+2u,2))>0) {
            auto terminal_word=r(f+0x30u);previous=r(f+0x48u);auto margin=r(f+0x2cu),corridor=r(f+0x1cu);auto call_sp=f;push(call_sp,0);push(call_sp,terminal_word);navigation=r(f+0x44u);push(call_sp,previous);push(call_sp,margin);request=r(f+0x1fa0u);auto mode=r(request+4u,1);push(call_sp,obstacles);push(call_sp,mode);target_sector=r(f+0x18u);push(call_sp,request);push(call_sp,navigation);sector=r(f+0x34u);
            child(0x2ce5d5u,0x2cdeb0u,{corridor,target_sector,sector,previous,call_sp,obstacles,row+0xcu,corridor},0x20u);
            // The actual geometric corridor owner determines success. Its
            // retry and row-publication continuation follows below.
            if(!(abi.gp[0]&0xffu)) {
                auto retry_policy=r(f+0x1facu,1);
                if(!retry_policy){if(sw(r(obstacles+4u,2))<=0)goto failed;auto flag=r(f+0x30u),radius=r(f+0x2cu);call_sp=f;push(call_sp,1);push(call_sp,flag);request=r(f+0x1fa0u);push(call_sp,previous);auto mode2=r(request+4u,1);push(call_sp,radius);push(call_sp,obstacles);push(call_sp,mode2);push(call_sp,request);navigation=r(f+0x44u);push(call_sp,navigation);}
                else {
                    bool usable=sw(r(obstacles+6u,2))>0;if(usable)usable=r(corridor+0xdaeu,2)!=0xffffu;if(!usable && sw(r(obstacles+4u,2))<=0)goto failed;
                    if(sw(r(obstacles+2u,2))>0)for(u32 n=0;;++n){auto at=obstacles+u32(sw(n))*20u+0xcu,group=r(at+2u,2),blocked=r(corridor+0xdaeu,2);if(group==blocked){auto flags=r(at,2);if(flags&8u)w(0x2ce666u,at,flags|1u,2);else{auto low=r(at,1);w(0x2ce66bu,at,low&0xfeu,1);}}else{auto low=r(at,1);w(0x2ce66bu,at,low&0xfeu,1);}if(sw(n+1u)>=sw(r(obstacles+2u,2)))break;}
                    auto row_index=r(f+0x38u,2),blocked=r(corridor+0xdaeu,2);w(0x2ce682u,f+0x3cu,blocked,2);if(sw(row_index)>0){auto source=r(f+0x24u);previous=source-0x10u;auto sec=r(source-0x18u);w(0x2ce693u,f+0x34u,sec);}
                    auto flag=r(f+0x30u);request=r(f+0x1fa0u);auto radius=r(f+0x2cu);row=r(f+0x24u);call_sp=f;push(call_sp,1);push(call_sp,flag);auto mode2=r(request+4u,1);push(call_sp,previous);push(call_sp,radius);navigation=r(f+0x44u);push(call_sp,obstacles);push(call_sp,mode2);push(call_sp,request);push(call_sp,navigation);
                }
                sector=r(f+0x34u);target_sector=r(f+0x18u);child(0x2ce6c9u,0x2cdeb0u,{corridor,target_sector,sector,previous,call_sp,obstacles,row+0xcu,corridor},0x20u);if(!(abi.gp[0]&0xffu))goto failed;
            }
            {
                auto complete_route=r(corridor+0x28u,1);w(0x2ce6dfu,f+0x23u,0,1);u32 terminal_sector{};
                if(complete_route){auto x=r(row+0xcu);w(0x2ce6e9u,f+0x5cu,x);auto y=r(row+0x10u),z=r(row+0x14u);w(0x2ce6f3u,f+0x60u,y);terminal_sector=r(f+0x18u);w(0x2ce6fbu,f+0x64u,z);}
                else {auto end=r(corridor+0x1eu,2),point=corridor+u32(sw(end))*0x34u+0x2cu;auto x=r(point);terminal_sector=r(point+8u);w(0x2ce716u,f+0x5cu,x);auto y=r(point+4u);w(0x2ce721u,f+0x60u,y);auto z=r(row+0x14u);w(0x2ce72cu,f+0x64u,z);}
                w(0x2ce732u,f+0x18u,terminal_sector);auto parent=r(corridor+0x1eu,2);u32 reversed=0;bool truncated=false;
                if(parent){auto source=r(f+0x24u),frame=r(source+0x18u,2);w(0x2ce74bu,f+0x1cu,frame,2);for(;;){auto point=corridor+u32(sw(parent))*0x34u+0x2cu,sec=r(point+8u),out=f+row_offset(reversed)+0x5dcu;w(0x2ce76bu,out+4u,sec);auto x=r(point);w(0x2ce770u,out+0xcu,x);auto y=r(point+4u);parent=u32(sw(r(point+0x28u,2)));w(0x2ce77au,out+0x10u,y);auto z=r(row+0x14u);++reversed;w(0x2ce785u,out+0x14u,z);frame=r(f+0x1cu,2);w(0x2ce78du,out,0xffffu,2);w(0x2ce792u,out+8u,0xffffu,2);w(0x2ce798u,out+2u,0,2);w(0x2ce79eu,out+0x18u,frame,2);if(sw(reversed)>=64){truncated=true;break;}if(!u16(parent)){truncated=r(f+0x23u,1)!=0;break;}}}
                count_output=r(f+0x1fb0u);auto output_count=r(count_output,2);for(auto n=reversed-1u;sw(n)>=0;--n){if(sw(output_count)>=4){truncated=true;break;}auto output=r(f+0x1fb4u);copy(f+row_offset(n)+0x5dcu,output+row_offset(output_count),0x2ce7edu);++output_count;}
                auto destination=r(f+0x1fb0u);w(0x2ce7feu,destination,output_count,2);if(truncated){auto complete_out=r(f+0x1fb8u);w(0x2ce80au,complete_out,0,1);return u8(r(f+0x13u,1));}routed=true;
            }
        }
        if(!routed){count_output=r(f+0x1fb0u);auto output_count=r(count_output,2),output=r(f+0x1fb4u),at=output+row_offset(output_count);copy(row,at,0x2ce83bu);auto current_count=r(count_output,2);w(0x2ce83du,count_output,current_count+1u,2);auto x=r(at+0xcu),sec=r(at+4u);w(0x2ce848u,f+0x5cu,x);auto y=r(at+0x10u),z=r(at+0x14u);w(0x2ce852u,f+0x60u,y);w(0x2ce856u,f+0x64u,z);w(0x2ce85au,f+0x18u,sec);}
        row=r(f+0x24u);
        if(r(row,2)==0xffffu) {
            object_out=r(f+0x1fbcu);
            if(object_out && r(object_out)==none) {
                auto secondary_count=r(f+0xd2u,2);auto blocked=r(f+0x3cu,2);
                if(sw(secondary_count)>0 || blocked!=0xffffu) {
                    if(blocked!=0xffffu && sw(r(obstacles+2u,2))>0) {
                        auto remaining=r(obstacles+2u,2),point=obstacles+0x14u;
                        for(;;) {
                            auto group=r(f+0x3cu,2);
                            if(r(point-6u,2)==group) {
                                auto x=r(point),y=r(point+4u),radius=r(point+8u),flags=r(point-8u,2),object=r(point-4u);
                                if(secondary_count!=64u){auto at=f+u32(sw(secondary_count))*20u+0xdcu;++secondary_count;w(0x2ce8fcu,f+0xd2u,secondary_count,2);
                                    if(flags&1u){auto n=r(f+0xd4u,2);w(0x2ce906u,f+0xd4u,n+1u,2);}if(flags&8u){auto n=r(f+0xd6u,2);w(0x2ce913u,f+0xd6u,n+1u,2);}
                                    w(0x2ce91bu,at,flags,2);w(0x2ce91eu,at+2u,0xffffu,2);w(0x2ce924u,at+4u,object);w(0x2ce927u,at+8u,x);w(0x2ce92cu,at+0xcu,y);w(0x2ce931u,at+0x10u,radius);secondary_count=r(f+0xd2u,2);
                                }
                            }
                            point+=20u;if(!--remaining)break;
                        }
                    }
                    auto output_index=r(f+0x68u),out_count=r(f+0x1fb0u);auto total=r(out_count,2);
                    if(sw(output_index)<sw(total)) {
                        navigation=r(f+0x44u);auto radius=r(f+0x2cu);
                        for(;;) {
                            auto output=r(f+0x1fb4u),at=output+row_offset(output_index);bool hit=false;
                            if(r(at,2)==0xffffu) {
                                u32 start{},start_sector{};
                                if(!u16(output_index)){auto req=r(f+0x1fa0u);start=req+0x14u;start_sector=r(req+0x24u);}else{start_sector=r(at-0x18u);start=at-0x10u;}
                                bool small{};
                                {detail::NativeTu15ArithmeticEnvironment environment(fp);auto x=real(r(at+0xcu)),origin=real(r(start));float difference{};
                                    asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(difference):"m"(x),"m"(origin):"memory","st");w(0x2ce9aau,f+0x50u,bits(difference));
                                    x=real(r(at+0x10u));origin=real(r(start+4u));asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(difference):"m"(x),"m"(origin):"memory","st");w(0x2ce9b4u,f+0x54u,bits(difference));
                                    x=real(r(f+0x50u));asm volatile("flds %0"::"m"(x):"memory","st");x=real(r(f+0x50u));asm volatile("fmuls %0"::"m"(x):"memory","st");x=real(r(f+0x54u));asm volatile("flds %0"::"m"(x):"memory","st");x=real(r(f+0x54u));float length{};asm volatile("fmuls %1\n\tfaddp\n\tfsqrt\n\tfsts %0":"=m"(length):"m"(x):"memory","st","st(1)");w(0x2ce9ccu,f+0x1cu,bits(length));asm volatile("fabs":::"st");x=real(r(0x46a0ecu));u8 tiny{};asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(tiny):"m"(x):"memory","cc","st","st(1)");small=tiny!=0;
                                }
                                u32 length{};if(!small){auto one=r(0x46a0d0u);length=r(f+0x1cu);auto inverse=scalar.div(one,length),dy=scalar.mul(inverse,r(f+0x54u)),dx=scalar.mul(inverse,r(f+0x50u));w(0x2ce9ffu,f+0x54u,dy);w(0x2cea08u,f+0x50u,dx);}
                                auto terminal_byte=r(f+0x30u,1);w(0x2cea1cu,f+0x74u,length);w(0x2cea22u,f+0x58u,0);
                                if(terminal_byte){auto count_pointer=r(f+0x1fb0u),written=r(count_pointer,2);if(sw(output_index)==sw(written)-1){length=scalar.sub(length,r(0x46a130u));w(0x2cea41u,f+0x74u,length);}}
                                if(scalar.above(length,0)) {
                                    auto call_sp=f;push(call_sp,f+0xc0u);auto maximum=r(f+0x74u);push(call_sp,0);push(call_sp,0);push(call_sp,0);push(call_sp,1);push(call_sp,maximum);push(call_sp,radius);push(call_sp,f+0x50u);push(call_sp,none);push(call_sp,start_sector);push(call_sp,start);push(call_sp,none);push(call_sp,f+0xd0u);push(call_sp,navigation);
                                    child(0x2cea76u,0x2769c0u,{f+0x50u,f+0xd0u,start_sector,navigation,call_sp,output_index,u32(sw(output_index)),radius},0x38u);
                                    auto obstruction=r(f+0xccu);if(sw(obstruction)>=0 && sw(obstruction)<sw(r(f+0xd2u,2))) {
                                        auto object_pointer=r(f+0x1fbcu),obstacle=f+u32(sw(obstruction))*20u+0xdcu,object=r(obstacle+4u),flags=r(obstacle,2);w(0x2ceac7u,object_pointer,object);u32 category{},site{};
                                        if(flags&0x10u){category=2;site=0x2cead2u;}else if(flags&0x40u){category=3;site=0x2ceae4u;}else if(flags&0x20u){category=1;site=0x2ceaf6u;}else if(flags&0x100u){category=4;site=0x2ceb09u;}else if(flags&0x80u){category=5;site=0x2ceb1bu;}else if(flags&8u){category=6;site=0x2ceb2du;}else if(flags&0x200u){category=7;site=0x2ceb40u;}
                                        if(site){auto kind_out=r(f+0x1fc0u);w(site,kind_out,category,2);}hit=true;
                                    }
                                }
                            }
                            if(hit)break;
                            out_count=r(f+0x1fb0u);++output_index;if(sw(output_index)>=sw(r(out_count,2)))break;
                        }
                    }
                }
            }
        }
        {
            auto next=r(f+0x38u),out_count=r(f+0x1fb0u),written=r(out_count,2);++next;auto limit=r(f+0x1fa4u,2);w(0x2ceb5cu,f+0x68u,written,2);w(0x2ceb61u,f+0x38u,next);if(sw(next)>=sw(limit))return u8(r(f+0x13u,1));request=r(f+0x1fa0u);index=next;continue;
        }
failed:
        {
            auto current_index=r(f+0x38u,2),last_index=r(f+0x70u);if(u32(sw(current_index))==last_index && sw(r(obstacles+2u,2))>0){auto optional=r(f+0x1fc4u);for(u32 n=0;;++n){auto flags=r(obstacles+u32(sw(n))*20u+0xcu,1);if((flags&2u)&&optional)w(0x2cebc1u,optional,1,1);if(sw(n+1u)>=sw(r(obstacles+2u,2)))break;}}return 0;
        }
    }
}

struct FpPublication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~FpPublication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}};
}
void native_campaign_ai_route_smoothing_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth)throw std::invalid_argument("Native smoothing requires retained FP and empty x87");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};FpPublication publish{abi,fp};
    if((fp.control_word&0x3fu)!=0x3fu || (fp.mxcsr&0x1f80u)!=0x1f80u)throw std::invalid_argument("Native smoothing requires masked FP exceptions");
    const auto caller=abi.gp;Smoothing owner{memory,abi,services,fp,observe};const auto sp=caller[4];
    if(entry==0x2ce030u){const auto result=owner.run(sp,caller);const auto f=sp-0x1f98u;abi.gp=caller;abi.gp[0]=(caller[0]&0xffffff00u)|result;abi.gp[7]=owner.r(f);abi.gp[6]=owner.r(f+4u);abi.gp[5]=owner.r(f+8u);abi.gp[3]=owner.r(f+0xcu);abi.gp[4]=sp+0x30u;return;}
    if(entry==0x2cbcf0u){owner.connected(sp,caller,abi.xmm[3][0]);abi.gp=caller;abi.gp[7]=owner.r(sp-0x8cu);abi.gp[6]=owner.r(sp-0x88u);abi.gp[5]=owner.r(sp-0x84u);abi.gp[4]=sp+8u;return;}
    if(entry==0x2cbe30u){owner.cluster(sp,caller);abi.gp=caller;abi.gp[4]=sp+8u;return;}
    throw std::invalid_argument("Native smoothing has no such entry");
}
#else
void native_campaign_ai_route_smoothing_call_tu15(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 &,const SourcePhysicsContactRecordObserver &){throw std::runtime_error("Exact native smoothing arithmetic unavailable");}
#endif
}
