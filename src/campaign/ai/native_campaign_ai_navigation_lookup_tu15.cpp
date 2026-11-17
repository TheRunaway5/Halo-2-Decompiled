#include "halo2/native_campaign_ai_navigation_lookup_tu15.hpp"
#include "halo2/native_tu15_camera_query_object_context.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"

namespace halo2 {
namespace {
using namespace goal_tu15;
constexpr u32 none=0xffffffffu;
i32 signed32(u32 value){return std::bit_cast<i32>(value);}
void push(const Port &p,SourcePhysicsWorldRuntimeAbi &a,u32 value) {
    std::array<std::byte,4> bytes{};
    for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));
    a.gp[4]-=4;p.memory.write(a.gp[4],bytes);
}
void ax(SourcePhysicsWorldRuntimeAbi &a,u32 value){a.gp[0]=(a.gp[0]&0xffff0000u)|(value&65535u);}

// 1F7E00 already has a qualified semantic owner. Its memory adapter retains
// the real saved-register words and stack operand without repeating its world
// lookup or supplying any result. Captured operands recover caller scratch.
void collision_context(const Port &p,SourcePhysicsWorldRuntimeAbi &a,u32 object) {
    const auto sp=a.gp[4];const auto saved=a.gp;
    struct FrameMemory final:SourcePhysicsContactMemory {
        const Port &p;SourcePhysicsWorldRuntimeAbi &a;
        mutable std::array<u32,12> values{};mutable unsigned ordinal{};
        FrameMemory(const Port &port,SourcePhysicsWorldRuntimeAbi &abi):p(port),a(abi){}
        void read(u32 address,std::span<std::byte> bytes)const override {
            p.memory.read(address,bytes);
            const auto n=ordinal++;
            if(n>=values.size())return;
            for(unsigned k=0;k<bytes.size();++k)values[n]|=std::to_integer<u32>(bytes[k])<<(8*k);
            if(n==1){push(p,a,a.gp[5]);a.gp[5]=p.r32(a.gp[4]+8u);}
            if(n==3)push(p,a,a.gp[7]);
            if(n==6 && values[n]!=none)push(p,a,a.gp[3]);
        }
        void write(u32 address,std::span<const std::byte> bytes)override{p.memory.write(address,bytes);}
    } memory(p,a);
    const bool found=native_tu15_resolve_camera_collision_context(memory,object,a.gp[6],p.observe);
    a.gp[1]=memory.values[2];
    a.gp[2]=found?memory.values[10]:none;
    a.gp[0]=found?((memory.values[2]+u32(sx16(memory.values[9])))&0xffffff00u)|1u:
        ((memory.values[3]&65535u)<<4u)&0xffffff00u;
    for(auto n:{3u,5u,7u})a.gp[n]=saved[n];a.gp[4]=sp+8u;
}

void moving_frame(const Port &p,SourcePhysicsWorldRuntimeAbi &a) {
    const auto sp=a.gp[4];const auto saved=a.gp;
    a.gp[4]-=0x14u;push(p,a,a.gp[3]);push(p,a,a.gp[5]);a.gp[3]=a.gp[0];
    a.gp[0]=p.r32(0x4ecc18u);push(p,a,a.gp[6]);a.gp[6]=a.gp[1];
    a.gp[1]=p.r32(a.gp[0]+0xc4u);a.gp[5]=none;
    const auto finish=[&]{ax(a,a.gp[5]);for(auto n:{3u,5u,6u,7u})a.gp[n]=saved[n];a.gp[4]=sp+4u;};
    if(signed32(a.gp[1])<=0){finish();return;}
    a.gp[2]=p.r32(a.gp[0]+0xc8u);if(!a.gp[2]){finish();return;}
    a.gp[1]=p.r32(0x4ecbd0u);a.gp[1]=p.r32(a.gp[1]+0x44u);
    a.gp[0]=(a.gp[6]&65535u)*3u;a.gp[0]=p.r32(a.gp[1]+a.gp[0]*4u+8u);
    const auto kind=p.r8(a.gp[0]+0xaau);a.gp[1]=(a.gp[1]&0xffffff00u)|kind;
    if(kind==6u)ax(a,p.r16(a.gp[0]+0x130u));
    else if(kind==7u)ax(a,p.r16(a.gp[0]+0x1d4u));
    else{finish();return;}
    if(sx16(a.gp[0])<0){finish();return;}
    a.gp[1]=p.r32(a.gp[2]+0x30u);a.gp[0]=u32(sx16(a.gp[0]));
    if(signed32(a.gp[0])>=signed32(a.gp[1])){finish();return;}
    a.gp[1]=p.r32(a.gp[2]+0x34u);a.gp[0]=a.gp[0]*0x1cu+a.gp[1];
    push(p,a,a.gp[7]);a.gp[7]=a.gp[0];
    if((p.r8(a.gp[7])&1u) && a.gp[3]!=none) {
        push(p,a,a.gp[6]);const auto object=a.gp[6];a.gp[6]=a.gp[4]+0x14u;
        push(p,a,0x218673u);collision_context(p,a,object);
        if(a.gp[0]&255u) {
            a.gp[3]&=255u;a.gp[0]=a.gp[3];
            a.gp[1]=p.r32(a.gp[7]+0x14u);a.gp[0]=u32(sx16(a.gp[0]));
            if(signed32(a.gp[0])<signed32(a.gp[1])) {
                a.gp[2]=p.r32(a.gp[7]+0x18u);
                a.gp[5]=(a.gp[5]&0xffff0000u)|p.r16(a.gp[2]+a.gp[0]*4u);
            }
        }
    }
    finish();
}

// 29FF40: the navigation-model index belongs only to object kinds 6 and 7.
void model_index(const Port &p,SourcePhysicsWorldRuntimeAbi &a) {
    a.gp[2]=p.r32(0x4ecbd0u);a.gp[2]=p.r32(a.gp[2]+0x44u);
    a.gp[1]=(a.gp[1]&65535u)*3u;a.gp[1]=p.r32(a.gp[2]+a.gp[1]*4u+8u);
    const auto kind=p.r8(a.gp[1]+0xaau);a.gp[2]=(a.gp[2]&0xffffff00u)|kind;a.gp[0]=none;
    if(kind==6u)ax(a,p.r16(a.gp[1]+0x130u));else if(kind==7u)ax(a,p.r16(a.gp[1]+0x1d4u));
    a.gp[4]+=4u;
}
// 2028A0: a surface index selects a BSP navigation root only in world space.
void surface_root(const Port &p,SourcePhysicsWorldRuntimeAbi &a) {
    const auto saved=a.gp[6];push(p,a,saved);a.gp[6]=p.r32(a.gp[4]+8u);a.gp[0]=0;
    if(a.gp[6]==none && signed32(a.gp[1])>=0 && signed32(a.gp[1])<signed32(p.r32(a.gp[2]+0x10u)))
        a.gp[0]=p.r32(a.gp[2]+0x14u)+a.gp[1]*4u;
    a.gp[6]=saved;a.gp[4]+=12u;
}

#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
void plane(const Port &p,SourcePhysicsWorldRuntimeAbi &a,NativeTu15PhysicsFrameFp &fp) {
    const auto sp=a.gp[4],saved_esi=a.gp[6],saved_edi=a.gp[7];
    push(p,a,a.gp[6]);a.gp[6]=p.r32(a.gp[4]+8u);a.gp[1]=a.gp[2]&0x7fffu;
    push(p,a,a.gp[7]);a.gp[7]=p.r32(a.gp[6]+0xcu);a.gp[1]=a.gp[7]+a.gp[1]*16u;
    if(a.gp[2]&0x8000u) {
        Simd s(fp);a.xmm[0]={};a.xmm[1]=a.xmm[0];
        a.xmm[1][0]=s.sub(0,p.r32(a.gp[1]));p.w32(0x14ce54u,a.gp[0],a.xmm[1][0]);
        a.xmm[1]=a.xmm[0];a.xmm[1][0]=s.sub(0,p.r32(a.gp[1]+4u));p.w32(0x14ce60u,a.gp[0]+4u,a.xmm[1][0]);
        a.xmm[1]=a.xmm[0];a.xmm[1][0]=s.sub(0,p.r32(a.gp[1]+8u));p.w32(0x14ce6du,a.gp[0]+8u,a.xmm[1][0]);
        a.xmm[0][0]=s.sub(0,p.r32(a.gp[1]+12u));a.gp[7]=saved_edi;p.w32(0x14ce78u,a.gp[0]+12u,a.xmm[0][0]);
    } else {
        a.gp[6]=p.r32(a.gp[1]);a.gp[2]=a.gp[0];p.w32(0x14ce85u,a.gp[2],a.gp[6]);
        a.gp[6]=p.r32(a.gp[1]+4u);p.w32(0x14ce8au,a.gp[2]+4u,a.gp[6]);
        a.gp[6]=p.r32(a.gp[1]+8u);p.w32(0x14ce90u,a.gp[2]+8u,a.gp[6]);
        a.gp[1]=p.r32(a.gp[1]+12u);a.gp[7]=saved_edi;p.w32(0x14ce97u,a.gp[2]+12u,a.gp[1]);
    }
    a.gp[6]=saved_esi;a.gp[4]=sp+8u;
}
u32 absolute(NativeTu15PhysicsFrameFp &fp,u32 value) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 out{};
    asm volatile("flds %1\n\tfabs\n\tfstps %0":"=m"(out):"m"(value):"memory","st");return out;
}
bool z_below_y(NativeTu15PhysicsFrameFp &fp,u32 y,u32 z) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);unsigned char below{};
    asm volatile("flds %1\n\tflds %2\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tsetb %0"
        :"=q"(below):"m"(y),"m"(z):"memory","cc","st","st(1)");return below!=0;
}
// The value-only dominant-axis helper does not expose the source's three
// rounded stack stores, x87 status or XMM scratch. This frame form retains them.
void dominant_axis(const Port &p,SourcePhysicsWorldRuntimeAbi &a,NativeTu15PhysicsFrameFp &fp) {
    a.gp[4]-=12u;const auto frame=a.gp[4];
    p.w32(0x123217u,frame+8u,absolute(fp,p.r32(a.gp[0])));
    const auto y=p.r32(a.gp[0]+4u);a.xmm[0]={p.r32(frame+8u),0,0,0};
    p.w32(0x123226u,frame+4u,absolute(fp,y));
    p.w32(0x12322fu,frame,absolute(fp,p.r32(a.gp[0]+8u)));
    const auto rounded_y=p.r32(frame+4u),rounded_z=p.r32(frame);
    bool fallback=z_below_y(fp,rounded_y,rounded_z);Simd s(fp);
    if(!fallback){a.xmm[1]={p.r32(frame),0,0,0};fallback=!s.above_equal(a.xmm[1][0],a.xmm[0][0]);}
    if(fallback){a.xmm[1]={p.r32(frame+4u),0,0,0};ax(a,s.above_equal(a.xmm[1][0],a.xmm[0][0])?1u:0u);}
    else ax(a,2u);
    a.gp[4]+=16u;
}
void project(const Port &p,SourcePhysicsWorldRuntimeAbi &a) {
    const auto saved=a.gp[6];push(p,a,saved);a.gp[6]=p.r8(a.gp[4]+8u);
    a.gp[1]=(a.gp[6]+u32(sx16(a.gp[1]))*2u)*6u;
    a.gp[6]=u32(sx16(p.r16(a.gp[1]+0x44ac8eu)));
    a.gp[1]=u32(sx16(p.r16(a.gp[1]+0x44ac8cu)));
    a.xmm[0]={p.r32(a.gp[2]+a.gp[6]*4u),0,0,0};a.gp[2]=p.r32(a.gp[2]+a.gp[1]*4u);
    p.w32(0x1231f7u,a.gp[0],a.gp[2]);p.w32(0x1231f9u,a.gp[0]+4u,a.xmm[0][0]);
    a.gp[6]=saved;a.gp[4]+=12u;
}
bool positive_side(NativeTu15PhysicsFrameFp &fp,u32 cy,u32 y,u32 cx,u32 x,u32 d,
    const std::array<std::byte,8> &eps) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);unsigned char above{};
    asm volatile("flds %1\n\tfmuls %2\n\tflds %3\n\tfmuls %4\n\tfaddp\n\tfsubs %5\n\tfldl %6\n\tfxch %%st(1)\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tseta %0"
        :"=q"(above):"m"(cy),"m"(y),"m"(cx),"m"(x),"m"(d),"m"(eps):"memory","cc","st","st(1)");return above!=0;
}
void tree(const Port &p,SourcePhysicsWorldRuntimeAbi &a,NativeTu15PhysicsFrameFp &fp) {
    const auto saved=a.gp[6];push(p,a,saved);
    while(a.gp[0]!=none && signed32(a.gp[0])<0) {
        a.gp[0]&=0x7fffffffu;
        if(signed32(a.gp[0])>=signed32(p.r32(a.gp[2]+0x18u))){a.gp[0]=none;break;}
        a.gp[6]=p.r32(a.gp[2]+0x1cu);a.gp[0]*=5u;
        const auto cy=p.r32(a.gp[6]+a.gp[0]*4u+4u);a.gp[0]=a.gp[6]+a.gp[0]*4u;
        const auto y=p.r32(a.gp[1]+4u),cx=p.r32(a.gp[0]),x=p.r32(a.gp[1]),d=p.r32(a.gp[0]+8u);
        const auto eps=p.r64(0x46a0f0u);
        a.gp[0]=p.r32(a.gp[0]+(positive_side(fp,cy,y,cx,x,d,eps)?12u:16u));
    }
    a.gp[6]=saved;a.gp[4]+=8u;
}
bool unordered_equal(NativeTu15PhysicsFrameFp &fp,u32 left,u32 right) {
    const auto x=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<i32>(left)));
    const auto y=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<i32>(right)));
    u32 host{},csr=fp.mxcsr;unsigned char same{},ordered{};
    asm volatile("stmxcsr %2\n\tldmxcsr %3\n\tucomiss %5,%4\n\tsete %0\n\tsetnp %1\n\tstmxcsr %3\n\tldmxcsr %2"
        :"=q"(same),"=q"(ordered),"+m"(host),"+m"(csr):"x"(x),"x"(y):"cc","memory");
    fp.mxcsr=csr;return same&&ordered;
}
// 145200 frame form: the semantic inverse-point owner intentionally keeps
// SIMD scratch private. A numeric caller also retains each MOVSS/MOVAPS result.
void inverse_point_frame(const Port &p,SourcePhysicsWorldRuntimeAbi &a,NativeTu15PhysicsFrameFp &fp) {
    Simd s(fp);const auto load=[&](unsigned n,u32 address){a.xmm[n]={p.r32(address),0,0,0};};
    const auto mul=[&](unsigned x,unsigned y){a.xmm[x][0]=s.mul(a.xmm[x][0],a.xmm[y][0]);};
    const auto add=[&](unsigned x,unsigned y){a.xmm[x][0]=s.add(a.xmm[x][0],a.xmm[y][0]);};
    load(1,a.gp[1]);a.xmm[0]={};
    if(unordered_equal(fp,a.xmm[1][0],0)) {
        for(auto [pc,offset]:std::array<std::array<u32,2>,3>{{{0x1452dfu,0},{0x1452e3u,4},{0x1452e8u,8}}})p.w32(pc,a.gp[2]+offset,0);
    } else {
        load(4,a.gp[1]);load(3,0x46a0d0u);load(0,a.gp[6]);load(1,a.gp[6]+4u);load(2,a.gp[6]+8u);
        for(unsigned n=0;n<3;++n)a.xmm[n][0]=s.sub(a.xmm[n][0],p.r32(a.gp[1]+0x28u+n*4u));
        if(!unordered_equal(fp,a.xmm[4][0],a.xmm[3][0])) {
            a.xmm[3][0]=s.div(a.xmm[3][0],a.xmm[4][0]);a.xmm[4]=a.xmm[3];mul(4,0);a.xmm[0]=a.xmm[4];
            a.xmm[4]=a.xmm[3];mul(4,1);mul(3,2);a.xmm[1]=a.xmm[4];a.xmm[2]=a.xmm[3];
        }
        load(3,a.gp[1]+12u);load(4,a.gp[1]+8u);mul(3,2);mul(4,1);add(3,4);load(4,a.gp[1]+4u);mul(4,0);add(3,4);
        p.w32(0x145288u,a.gp[2],a.xmm[3][0]);
        load(3,a.gp[1]+0x18u);load(4,a.gp[1]+0x14u);mul(3,2);mul(4,1);add(3,4);load(4,a.gp[1]+0x10u);mul(4,0);add(3,4);
        p.w32(0x1452afu,a.gp[2]+4u,a.xmm[3][0]);
        load(3,a.gp[1]+0x24u);mul(3,2);load(2,a.gp[1]+0x20u);mul(2,1);load(1,a.gp[1]+0x1cu);add(3,2);mul(1,0);add(3,1);
        p.w32(0x1452d7u,a.gp[2]+8u,a.xmm[3][0]);
    }
    a.gp[0]=a.gp[2];a.gp[4]+=4u;
}
void inverse_permutation_frame(const Port &p,SourcePhysicsWorldRuntimeAbi &a) {
    const auto saved=a.gp[6];push(p,a,saved);a.gp[6]=p.r8(a.gp[4]+8u);
    a.gp[1]=(a.gp[6]+u32(sx16(a.gp[1]))*2u)*6u;
    a.gp[6]=u32(sx16(p.r16(a.gp[1]+0x44ac90u)));a.xmm[0]={p.r32(a.gp[2]+a.gp[6]*4u),0,0,0};
    a.gp[6]=u32(sx16(p.r16(a.gp[1]+0x44ac8eu)));a.gp[1]=u32(sx16(p.r16(a.gp[1]+0x44ac8cu)));
    a.xmm[1]={p.r32(a.gp[2]+a.gp[6]*4u),0,0,0};a.gp[2]=p.r32(a.gp[2]+a.gp[1]*4u);
    p.w32(0x219013u,a.gp[0],a.gp[2]);p.w32(0x219015u,a.gp[0]+4u,a.xmm[1][0]);p.w32(0x21901au,a.gp[0]+8u,a.xmm[0][0]);
    a.gp[6]=saved;a.gp[4]+=12u;
}
void inverse_frame(const Port &p,SourcePhysicsWorldRuntimeAbi &a,NativeTu15PhysicsFrameFp &fp) {
    const auto sp=a.gp[4];const auto saved=a.gp;
    a.gp[4]-=12u;push(p,a,a.gp[6]);a.gp[6]=a.gp[1];a.gp[1]&=0xffffff00u;
    const auto finish=[&]{for(auto n:{5u,6u,7u})a.gp[n]=saved[n];a.gp[4]=sp+4u;};
    const auto al=[&](u32 value){a.gp[0]=(a.gp[0]&0xffffff00u)|(value&255u);};
    if(u16(a.gp[0])==0xffffu) {
        a.gp[0]=p.r32(a.gp[6]);p.w32(0x218a2au,a.gp[3],a.gp[0]);
        a.gp[1]=p.r32(a.gp[6]+4u);p.w32(0x218a2fu,a.gp[3]+4u,a.gp[1]);
        a.gp[2]=p.r32(a.gp[6]+8u);p.w32(0x218a35u,a.gp[3]+8u,a.gp[2]);al(1);finish();return;
    }
    push(p,a,a.gp[5]);a.gp[5]=p.r32(0x505e78u);push(p,a,a.gp[7]);a.gp[7]=u32(sx16(a.gp[0]))*16u;
    a.gp[0]=p.r32(a.gp[7]+a.gp[5]);a.gp[7]+=a.gp[5];
    if(a.gp[0]==none) {
        a.gp[2]=p.r32(a.gp[6]);p.w32(0x218a11u,a.gp[3],a.gp[2]);
        a.gp[0]=p.r32(a.gp[6]+4u);p.w32(0x218a16u,a.gp[3]+4u,a.gp[0]);
        a.gp[2]=p.r32(a.gp[6]+8u);p.w32(0x218a1eu,a.gp[3]+8u,a.gp[2]);al(a.gp[1]);finish();return;
    }
    a.gp[1]=p.r32(0x4ecbd0u);a.gp[2]=p.r32(a.gp[1]+0x44u);a.gp[0]=(a.gp[0]&65535u)*3u;
    a.gp[5]=p.r32(a.gp[2]+a.gp[0]*4u+8u);ax(a,p.r16(a.gp[7]+6u));
    bool valid=sx16(a.gp[0])>=0;
    if(valid) {
        a.gp[2]=u32(sx16(p.r16(a.gp[5]+0x114u)));a.gp[1]=u32(sx16(a.gp[0]));
        const auto product=std::uint64_t(0x4ec4ec4fu)*a.gp[2];a.gp[0]=u32(product);a.gp[2]=u32(product>>36u);
        valid=signed32(a.gp[1])<signed32(a.gp[2]);
    }
    if(!valid) {
        a.gp[2]=p.r32(a.gp[6]);p.w32(0x2189f8u,a.gp[3],a.gp[2]);
        a.gp[0]=p.r32(a.gp[6]+4u);p.w32(0x2189fdu,a.gp[3]+4u,a.gp[0]);
        a.gp[1]=p.r32(a.gp[6]+8u);p.w32(0x218a05u,a.gp[3]+8u,a.gp[1]);al(0);finish();return;
    }
    a.gp[0]=u32(sx16(p.r16(a.gp[5]+0x116u)));a.gp[1]*=52u;a.gp[0]+=a.gp[5];a.gp[1]+=a.gp[0];a.gp[2]=a.gp[4]+12u;
    push(p,a,0x2189dbu);inverse_point_frame(p,a,fp);
    a.gp[1]=p.r8(a.gp[7]+11u);a.gp[0]=a.gp[3];push(p,a,a.gp[1]);
    a.gp[1]=(a.gp[1]&0xffff0000u)|u16(std::int16_t(std::int8_t(p.r8(a.gp[7]+10u))));
    push(p,a,0x2189edu);inverse_permutation_frame(p,a);al(1);finish();
}
#endif

void sector(const Port &p,SourcePhysicsWorldRuntimeAbi &a,NativeTu15PhysicsFrameFp &fp) {
    const auto sp=a.gp[4];const auto saved=a.gp;
    a.gp[4]-=0x18u;push(p,a,a.gp[3]);push(p,a,a.gp[5]);a.gp[5]=p.r32(a.gp[4]+0x28u);
    push(p,a,a.gp[6]);push(p,a,a.gp[7]);const auto frame=a.gp[4];a.gp[7]=none;a.gp[3]=a.gp[0];
    a.gp[0]=a.gp[1]!=none?4u:(a.gp[3]!=none?3u:1u);
    a.gp[2]=p.r32(0x4ecc18u);a.gp[6]=p.r32(a.gp[2]+0xc4u);
    const auto finish=[&]{a.gp[0]=a.gp[7];for(auto n:{3u,5u,6u,7u})a.gp[n]=saved[n];a.gp[4]=sp+16u;};
    if(signed32(a.gp[6])<=0){finish();return;}
    a.gp[6]=p.r32(a.gp[2]+0xc8u);if(!a.gp[6]){finish();return;}
    if(a.gp[0]==4u) {
        push(p,a,0x2026c6u);model_index(p,a);
        if(sx16(a.gp[0])<0){finish();return;}
        a.gp[1]=p.r32(a.gp[6]+0x30u);a.gp[0]=u32(sx16(a.gp[0]));
        if(signed32(a.gp[0])>=signed32(a.gp[1])){finish();return;}
        a.gp[1]=p.r32(a.gp[6]+0x34u);a.gp[0]=a.gp[0]*0x1cu+a.gp[1];a.gp[1]=a.gp[0];
        a.gp[3]=p.r32(a.gp[1]+12u);a.gp[2]=0;if(signed32(a.gp[3])<=0){finish();return;}
        a.gp[1]=p.r32(a.gp[1]+16u);a.gp[0]=0;
        for(;;) {
            a.gp[0]=a.gp[0]*16u+a.gp[1];if(p.r32(a.gp[0])==a.gp[5])break;
            ++a.gp[2];a.gp[0]=u32(sx16(a.gp[2]));
            if(signed32(a.gp[0])>=signed32(a.gp[3])){finish();return;}
        }
        a.gp[7]=p.r32(a.gp[0]+4u);
        if(a.gp[7]!=none) {
            a.gp[1]=p.r32(frame+0x2cu);
            if(signed32(a.gp[1])>=0) {
                a.gp[0]=p.r32(a.gp[0]+8u)-a.gp[7];
                if(signed32(a.gp[1])<=signed32(a.gp[0])) {
                    a.gp[7]+=a.gp[1];
                    if(signed32(a.gp[7])>=0 && signed32(a.gp[7])<signed32(p.r32(a.gp[6]))){finish();return;}
                }
            }
        }
        a.gp[7]=none;finish();return;
    }
    if(a.gp[0]==3u) {
        if(signed32(a.gp[3])<0 || signed32(a.gp[3])>=signed32(p.r32(a.gp[6]+0x40u))){finish();return;}
        a.gp[1]=p.r32(a.gp[6]+0x44u);ax(a,p.r16(a.gp[1]+a.gp[3]*4u));
        if(sx16(a.gp[0])<0){finish();return;}
        a.gp[1]=p.r32(a.gp[6]+0x30u);a.gp[0]=u32(sx16(a.gp[0]));
        if(signed32(a.gp[0])>=signed32(a.gp[1])){finish();return;}
        a.gp[7]=p.r32(a.gp[6]+0x34u);a.gp[0]*=0x1cu;a.gp[1]=p.r32(a.gp[0]+a.gp[7]+4u);a.gp[0]+=a.gp[7];
        if(a.gp[1]!=none) {
            a.gp[2]=p.r32(frame+0x2cu);
            if(signed32(a.gp[2])>=0) {
                a.gp[0]=p.r32(a.gp[0]+8u)-a.gp[1];
                if(signed32(a.gp[2])<=signed32(a.gp[0])) {
                    a.gp[7]=a.gp[1]+a.gp[2];
                    if(signed32(a.gp[7])>=0 && signed32(a.gp[7])<signed32(p.r32(a.gp[6]))){finish();return;}
                }
            }
        }
        a.gp[7]=none;finish();return;
    }
    a.gp[5]=p.r32(0x4ecc10u);if(!a.gp[5]){finish();return;}
    a.gp[0]=p.r32(frame+0x2cu);if(a.gp[0]==none){finish();return;}
    push(p,a,a.gp[1]);a.gp[1]=a.gp[0];a.gp[2]=a.gp[6];push(p,a,0x2027e5u);surface_root(p,a);
    a.gp[3]=a.gp[0];if(!a.gp[3]){finish();return;}
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    a.gp[1]=p.r32(a.gp[5]+0x2cu);a.gp[2]=p.r32(frame+0x2cu);a.gp[2]=u32(sx16(p.r16(a.gp[1]+a.gp[2]*8u)));
    push(p,a,a.gp[5]);a.gp[0]=a.gp[4]+0x1cu;push(p,a,0x202800u);plane(p,a,fp);
    push(p,a,0x202805u);dominant_axis(p,a,fp);
    p.w8(0x202805u,frame+0x2cu,1);a.gp[1]=a.gp[0];a.gp[0]=u32(sx16(a.gp[1]));
    a.xmm[0]={p.r32(frame+a.gp[0]*4u+0x18u),0,0,0};Simd s(fp);
    if(!s.above(a.xmm[0][0],p.r32(0x46a0e8u)))p.w8(0x20281eu,frame+0x2cu,0);
    a.gp[2]=p.r32(frame+0x2cu);push(p,a,a.gp[2]);a.gp[2]=p.r32(a.gp[4]+0x38u);a.gp[0]=a.gp[4]+0x14u;
    push(p,a,0x202835u);project(p,a);a.gp[0]=p.r32(a.gp[3]);a.gp[2]=a.gp[6];a.gp[1]=frame+0x10u;
    push(p,a,0x202842u);tree(p,a,fp);a.gp[7]=a.gp[0];finish();
#else
    throw std::runtime_error("Native BSP navigation projection requires x86 scalar arithmetic");
#endif
}
}

void native_campaign_ai_navigation_lookup_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    const Port p{memory,observe};
    if(entry==0x2185e0u){moving_frame(p,abi);return;}
    if(entry!=0x202660u && entry!=0x218960u)throw std::invalid_argument("Unsupported native navigation lookup entry");
    if(!abi.floating_point)throw std::invalid_argument("Native sector lookup requires retained FP controls");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    try{
        if(entry==0x202660u)sector(p,abi,fp);
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
        else inverse_frame(p,abi,fp);
#else
        else throw std::runtime_error("Native inverse-frame projection requires x86 scalar arithmetic");
#endif
        publish();
    }catch(...){publish();throw;}
}
}
