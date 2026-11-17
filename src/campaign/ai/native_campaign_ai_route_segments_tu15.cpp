#include "halo2/native_campaign_ai_route_segments_tu15.hpp"
#include "halo2/native_campaign_ai_route_traversal_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/native_tu15_look_vector_angle.hpp"
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
struct Segments {
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
        auto caller_sp=gp[4];push(gp[4],site+5u);abi.gp=gp;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
        auto import_fp=[&](){if(abi.floating_point){fp.control_word=abi.floating_point->x87_control_word;fp.mxcsr=abi.floating_point->mxcsr;}fp.status=abi.x87_status;};
        try {
            if(target==0x2769c0u || target==0x2761f0u)
                native_campaign_ai_route_traversal_call_tu15(memory,target,abi,services,observe);
            else if(target==0x2cceb0u || target==0x2cc5c0u || target==0x2cc790u || target==0x2cc8f0u)
                native_campaign_ai_route_segments_call_tu15(memory,target,abi,services,observe);
            else if(services.callee)services.callee(site,target,abi);
            else throw NativeCampaignAiPerformRequiredOwnerTu15(site,target);
        } catch(...) {import_fp();throw;}
        if(!abi.floating_point || abi.gp[4]!=caller_sp+cleanup)
            throw std::runtime_error("Native route segment child returned another ABI");
        import_fp();
    }
    // Preserve the extended Y difference through FST and the following
    // multiply by the rounded stored difference, as in 2CC03F..2CC077.
    bool direction(u32 point,u32 current,u32 out,u32 magnitude,std::array<u32,3> sites={0x2cc047u,0x2cc055u,0x2cc069u},
        std::array<u32,3> before_first={},u32 *rounded_y=nullptr) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);float value{};
        auto a=real(r(point)),b=real(r(current));
        asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(value):"m"(a),"m"(b):"memory","st");
        if(before_first[0])w(before_first[0],before_first[1],before_first[2]);
        w(sites[0],out,bits(value));a=real(r(point+4u));b=real(r(current+4u));
        asm volatile("flds %1\n\tfsubs %2\n\tfsts %0":"=m"(value):"m"(a),"m"(b):"memory","st");
        w(sites[1],out+4u,bits(value));if(rounded_y)*rounded_y=r(out+4u);a=real(r(out+4u));
        asm volatile("fmuls %0"::"m"(a):"memory","st");a=real(r(out));
        asm volatile("flds %0"::"m"(a):"memory","st");a=real(r(out));
        asm volatile("fmuls %1\n\tfaddp\n\tfsqrt\n\tfsts %0":"=m"(value):"m"(a):"memory","st","st(1)");
        w(sites[2],magnitude,bits(value));asm volatile("fabs":::"st");a=real(r(0x46a0ecu));u8 small{};
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(small):"m"(a):"memory","cc","st","st(1)");
        return small!=0;
    }
    bool norm(u32 first,u32 second,u32 magnitude,u32 site) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);float a=real(r(first));
        asm volatile("flds %0"::"m"(a):"memory","st");a=real(r(first));
        asm volatile("fmuls %0"::"m"(a):"memory","st");a=real(r(second));
        asm volatile("flds %0"::"m"(a):"memory","st");a=real(r(second));float length{};
        asm volatile("fmuls %1\n\tfaddp\n\tfsqrt\n\tfsts %0":"=m"(length):"m"(a):"memory","st","st(1)");
        w(site,magnitude,bits(length));asm volatile("fabs":::"st");a=real(r(0x46a0ecu));u8 small{};
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(small):"m"(a):"memory","cc","st","st(1)");return small!=0;
    }
    bool device_blocked(u32 sector,u32 nav) {
        auto index=u32(sw(r(sector+2u,2)));if(index==none)return false;auto links=r(nav+0x3cu),record=0u;
        for(;;){record=links+index*20u;if(r(record,2)==7u)break;index=u32(sw(r(record+2u,2)));if(index==none)return false;}
        auto devices=r(r(r(0x4ecc18u)+0x18u)+0x2cu),device=r(record+4u);
        if(!(r(devices+device*8u+4u,1)&8u))return false;
        auto argument=r(record+8u),bit=r(devices+device*8u+5u,1);const auto group=u32(sw(r(0x474c24u,2))),base=r(0x4f9b50u);
        const auto field=argument==none?(group<<5u)+base+1u:(group<<10u)+base+argument+0x201u;
        return !(r(field+(bit>>5u)*4u)&(1u<<(bit&31u)));
    }
    bool walkable(u32 sector_record,u32 nav) {auto flags=r(sector_record,2);return (flags&1u) && (!(flags&2u)||!device_blocked(sector_record,nav));}
    u32 xsub(u32 a,u32 b) {detail::NativeTu15ArithmeticEnvironment environment(fp);auto x=real(a),y=real(b);float answer{};asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(answer):"m"(x),"m"(y):"memory","st");return bits(answer);}
    bool tiny_double(u32 address) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);auto value=real(r(address));asm volatile("flds %0\n\tfabs"::"m"(value):"memory","st");
        std::array<std::byte,8> raw{};memory.read(0x44acd8u,raw);double epsilon{};std::memcpy(&epsilon,raw.data(),8);u8 answer{};
        asm volatile("fldl %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(answer):"m"(epsilon):"memory","cc","st","st(1)");return answer!=0;
    }
    void normalize2(u32 vector,u32 stack);
    u8 choose_corner(u32 entry,const std::array<u32,8> &caller);
    void tangent(u32 entry,const std::array<u32,8> &caller,u32 radius);
    void tangent_intersection(u32 entry,const std::array<u32,8> &caller,u32 radius);
    u8 corner(u32 entry,const std::array<u32,8> &caller);
    void run(u32 entry,const std::array<u32,8> &caller);
};

void Segments::normalize2(u32 vector,u32 stack) {
    const auto f=stack-4u;bool small{};
    {detail::NativeTu15ArithmeticEnvironment environment(fp);auto y=real(r(vector+4u)),x=real(r(vector));float magnitude{};
        asm volatile("flds %1\n\tflds %2\n\tfld %%st(0)\n\tfmul %%st(1)\n\tfld %%st(2)\n\tfmul %%st(3)\n\tfaddp\n\tfsqrt\n\tfstp %%st(2)\n\tfstp %%st(0)\n\tfsts %0":"=m"(magnitude):"m"(y),"m"(x):"memory","st","st(1)","st(2)","st(3)");
        w(0x2b3f6u,f,bits(magnitude));asm volatile("fabs":::"st");auto epsilon=real(r(0x46a0ecu));u8 answer{};
        asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(answer):"m"(epsilon):"memory","cc","st","st(1)");small=answer!=0;
    }
    if(small)return;
    auto one=r(0x46a0d0u),magnitude=r(f),x=r(vector),inverse=scalar.div(one,magnitude);x=scalar.mul(x,inverse);auto y=scalar.mul(inverse,r(vector+4u));w(0x2b425u,vector,x);w(0x2b429u,vector+4u,y);
}

u8 Segments::choose_corner(u32 entry,const std::array<u32,8> &caller) {
    auto sp=entry-0x34u;const auto previous=caller[0],target=caller[6],left=caller[3],right=caller[7];
    auto a=r(target);push(sp,caller[5]);auto b=r(previous),current=r(sp+0x3cu);const auto f=sp;
    w(0x2cc8fcu,f+0x10u,xsub(a,b));a=r(target+4u);b=r(previous+4u);w(0x2cc906u,f+0x14u,xsub(a,b));
    if(norm(f+0x10u,f+0x14u,f+0x3cu,0x2cc91eu))return 1;
    auto one=r(0x46a0d0u),magnitude=r(f+0x3cu);const auto positive=scalar.below(0,magnitude);auto inverse=scalar.div(one,magnitude),forward_x=scalar.mul(inverse,r(f+0x10u)),forward_y=scalar.mul(inverse,r(f+0x14u));w(0x2cc95au,f+0x10u,forward_x);if(!positive)return 1;
    auto perpendicular=scalar.sub(0,forward_y),left_y=0u,left_x=0u;
    auto small=direction(previous,left,f+0x18u,f+0x3cu,{0x2cc97cu,0x2cc986u,0x2cc9a0u},{0x2cc976u,f+0x30u,perpendicular},&left_y);
    if(!small){inverse=scalar.div(one,r(f+0x3cu));left_x=scalar.mul(inverse,r(f+0x18u));left_y=scalar.mul(left_y,inverse);w(0x2cc9c8u,f+0x18u,left_x);w(0x2cc9ceu,f+0x1cu,left_y);}else left_x=r(f+0x18u);
    small=direction(previous,right,f+0x20u,f+0x3cu,{0x2cc9e0u,0x2cc9eau,0x2cc9feu});
    if(!small){inverse=scalar.div(one,r(f+0x3cu));auto nx=scalar.mul(inverse,r(f+0x20u));w(0x2cca1fu,f+0x20u,nx);auto ny=scalar.mul(r(f+0x24u),inverse);w(0x2cca2fu,f+0x24u,ny);}
    auto left_dot=scalar.add(scalar.mul(left_y,forward_x),scalar.mul(left_x,perpendicular));
    {detail::NativeTu15ArithmeticEnvironment environment(fp);auto ry=real(r(f+0x24u));asm volatile("flds %0"::"m"(ry):"memory","st");auto x=real(r(f+0x10u));asm volatile("fmuls %0"::"m"(x):"memory","st");auto rx=real(r(f+0x20u));asm volatile("flds %0"::"m"(rx):"memory","st");w(0x2cca51u,f+0x10u,left_dot);auto y=real(r(f+0x30u));float value{};asm volatile("fmuls %1\n\tfaddp\n\tfstps %0":"=m"(value):"m"(y):"memory","st","st(1)");w(0x2cca5cu,f+0x3cu,bits(value));}
    auto right_dot=tiny_double(f+0x3cu)?0u:r(f+0x3cu);if(tiny_double(f+0x10u))left_dot=0;
    if(scalar.below(scalar.mul(left_dot,right_dot),0)) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);
        auto load_difference=[&](u32 first,u32 second){auto x=real(r(first)),y=real(r(second));asm volatile("flds %0\n\tfsubs %1"::"m"(x),"m"(y):"memory","st");};
        load_difference(left,current);load_difference(left+4u,current+4u);load_difference(left,target);load_difference(left+4u,target+4u);
        for(const auto part:std::array<std::array<u32,4>,4>{{{right,current,f+0x28u,0x2cce20u},{right+4u,current+4u,f+0x2cu,0x2cce2au},{right,target,f+0x30u,0x2cce32u},{right+4u,target+4u,f+0x34u,0x2cce3cu}}}){
            load_difference(part[0],part[1]);float value{};asm volatile("fstps %0":"=m"(value)::"memory","st");w(part[3],part[2],bits(value));
        }
        asm volatile("fld %%st(0)\n\tfmul %%st(1)\n\tfld %%st(2)\n\tfmul %%st(3)\n\tfaddp\n\tfsqrt\n\tfstp %%st(2)\n\tfstp %%st(0)\n\tfld %%st(1)\n\tfmul %%st(2)\n\tfld %%st(3)\n\tfmul %%st(4)\n\tfaddp\n\tfsqrt\n\tfstp %%st(3)\n\tfxch %%st(2)\n\tfaddp %%st,%%st(2)":::"memory","st","st(1)","st(2)","st(3)","st(4)");
        auto norm_on_stack=[&](u32 y,u32 x,u32 site){auto value=real(r(y));asm volatile("flds %0"::"m"(value):"memory","st");value=real(r(y));asm volatile("fmuls %0"::"m"(value):"memory","st");value=real(r(x));asm volatile("flds %0"::"m"(value):"memory","st");value=real(r(x));float result{};asm volatile("fmuls %1\n\tfaddp\n\tfsqrt\n\tfsts %0":"=m"(result):"m"(value):"memory","st","st(1)");w(site,f+0x3cu,bits(result));};
        norm_on_stack(f+0x34u,f+0x30u,0x2cce76u);norm_on_stack(f+0x2cu,f+0x28u,0x2cce8eu);u8 answer{};
        asm volatile("faddp\n\tfcomip %%st(2),%%st\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tseta %0":"=q"(answer)::"memory","cc","st","st(1)","st(2)","st(3)");return answer;
    }
    for(const auto part:std::array<std::array<u32,7>,4>{{{current,left,f+0x10u,0x2ccaa3u,0x2ccab7u,0x2ccabdu,0},{current,right,f+0x28u,0x2ccacbu,0x2ccadfu,0x2ccae5u,0},{target,left,f+4u,0x2ccaf2u,0x2ccb06u,0x2ccb0cu,0},{target,right,f+0x30u,0x2ccb19u,0x2ccb2du,0x2ccb33u,0}}}){
        auto x=r(part[0]);x=scalar.sub(x,r(part[1]));w(part[3],part[2],x);auto y=r(part[0]+4u);y=scalar.sub(y,r(part[1]+4u));w(part[4],part[2]+4u,y);auto call_sp=f;push(call_sp,part[5]+5u);normalize2(part[2],call_sp);
    }
    auto clamp=[&](u32 value,u32 destination,std::array<u32,3> sites){
        const auto low=r(0x46a0dcu);if(scalar.above(low,value)){w(sites[2],destination,low);return;}
        const auto high=r(0x46a0d0u);if(scalar.above(value,high)){w(sites[2],destination,high);return;}
        if(scalar.above(low,value)){w(sites[0],destination,low);return;}
        const auto above=scalar.above(value,high);w(sites[1],destination,high);if(!above)w(sites[2],destination,value);
    };
    auto ly=r(f+0x14u),lx=r(f+0x10u);lx=scalar.mul(lx,r(f+0x18u));ly=scalar.mul(ly,r(f+0x1cu));clamp(scalar.add(ly,lx),f+0x3cu,{0x2ccb7du,0x2ccb88u,0x2ccb90u});
    // These retained vector reads belong after the acos call in the source;
    // the semantic helper below keeps that ordering while its result is live.
    u32 basis_y{},basis_x{};
    auto signed_angle=[&](u32 cosine,u32 call_site,u32 destination,u32 store_site,u32 vy,u32 vx,u32 ux,u32 uy,bool reverse) {
        detail::NativeTu15ArithmeticEnvironment environment(fp);auto input=real(r(cosine));asm volatile("flds %0"::"m"(input):"memory","st");auto call_sp=f;push(call_sp,call_site+5u);native_tu15_look_acos_active(memory);
        // SIMD is evaluated under the same active environment; publishing
        // its MXCSR before calling Scalar avoids replacing it on scope exit.
        auto a=r(vy),b=r(uy),c=r(vx);float fa=real(a),fb=real(b),fc=real(c),fd=real(r(ux));
        float cross{};u8 negative{};
        if(reverse)asm volatile("mulss %3,%0\n\tmulss %2,%1\n\tsubss %1,%0":"+x"(fa),"+x"(fb):"x"(fc),"x"(fd):"memory");
        else asm volatile("mulss %3,%0\n\tmulss %2,%1\n\tsubss %1,%0":"+x"(fa),"+x"(fb):"x"(fc),"x"(fd):"memory");
        if(!reverse){basis_y=a;basis_x=c;}
        cross=fa;float zero{};asm volatile("comiss %2,%1\n\tseta %0":"=q"(negative):"x"(zero),"x"(cross):"memory","cc");float value{};asm volatile("fstps %0":"=m"(value)::"memory","st");w(store_site,destination,bits(value));return negative!=0;
    };
    auto negative=signed_angle(f+0x3cu,0x2ccb9au,f+0xcu,0x2ccbC8u,f+0x1cu,f+0x18u,f+0x10u,f+0x14u,false);
    if(negative){auto value=scalar.sub(0,r(f+0xcu));w(0x2ccbd4u,f+0xcu,value);}
    auto y=r(f+8u);y=scalar.mul(y,basis_y);auto x=r(f+4u);x=scalar.mul(x,basis_x);clamp(scalar.add(y,x),f+0x3cu,{0x2ccc1bu,0x2ccc26u,0x2ccc2eu});
    negative=signed_angle(f+0x3cu,0x2ccc38u,f+0x3cu,0x2ccc5fu,f+8u,f+4u,f+0x18u,f+0x1cu,true);u32 second_angle{};
    if(negative){second_angle=scalar.sub(0,r(f+0x3cu));w(0x2ccc6eu,f+0x3cu,second_angle);}else second_angle=r(f+0x3cu);
    x=r(f+0x28u);x=scalar.mul(x,r(f+0x20u));w(0x2ccc88u,f+0x18u,second_angle);y=r(f+0x2cu);y=scalar.mul(y,r(f+0x24u));clamp(scalar.add(y,x),f+4u,{0x2cccc7u,0x2cccd2u,0x2cccdau});
    negative=signed_angle(f+4u,0x2ccce4u,f+0x10u,0x2ccd12u,f+0x24u,f+0x20u,f+0x28u,f+0x2cu,false);if(negative){auto value=scalar.sub(0,r(f+0x10u));w(0x2ccd1eu,f+0x10u,value);}
    y=r(f+0x34u);y=scalar.mul(y,basis_y);x=r(f+0x30u);x=scalar.mul(x,basis_x);clamp(scalar.add(y,x),f+4u,{0x2ccd65u,0x2ccd70u,0x2ccd78u});
    negative=signed_angle(f+4u,0x2ccd82u,f+4u,0x2ccda9u,f+0x34u,f+0x30u,f+0x20u,f+0x24u,true);auto fourth=negative?scalar.sub(0,r(f+4u)):r(f+4u);
    auto positive_fourth=scalar.above(fourth,0);auto turn=r(0x44923cu);if(positive_fourth)fourth=scalar.sub(fourth,turn);
    auto second=r(f+0x3cu);const auto negative_second=scalar.above(0,second);second=scalar.add(second,turn);if(!negative_second)second=r(f+0x18u);
    auto left_total=scalar.add(r(f+0xcu),second),right_total=scalar.add(r(f+0x10u),fourth);return u8(scalar.above(scalar.sub(0,right_total),left_total));
}

void Segments::tangent(u32 entry,const std::array<u32,8> &caller,u32 radius) {
    const auto f=entry-0x18u,point=caller[0],center=caller[1],out=caller[2];
    auto dx=r(point);dx=scalar.sub(dx,r(center));auto dy=r(point+4u);dy=scalar.sub(dy,r(center+4u));
    auto length=scalar.add(scalar.mul(dx,dx),scalar.mul(dy,dy));const auto positive=scalar.above(length,0);
    w(0x2cc5edu,f+4u,dx);w(0x2cc5f3u,f+0x14u,dy);const auto factor=positive?scalar.div(radius,length):0u;
    auto discriminant=scalar.sub(length,scalar.mul(radius,radius));const auto external=scalar.above(discriminant,0);w(0x2cc615u,f,discriminant);
    if(external) {
        float root{};{detail::NativeTu15ArithmeticEnvironment environment(fp);auto value=real(r(f));asm volatile("flds %1\n\tfsqrt\n\tfstps %0":"=m"(root):"m"(value):"memory","st");}
        auto xrad=scalar.mul(dx,radius),yrad=scalar.mul(dy,radius);w(0x2cc639u,f,bits(root));auto root_bits=r(f),xroot=scalar.mul(dx,root_bits),yroot=scalar.mul(dy,root_bits);
        auto minus_y=scalar.sub(yrad,xroot),minus_x=scalar.add(scalar.mul(scalar.sub(xrad,yroot),factor),r(center));auto plus_x=scalar.add(xrad,yroot),plus_y=scalar.add(xroot,yrad);
        w(0x2cc661u,f+0xcu,minus_x);auto ax=scalar.sub(minus_x,r(point));plus_y=scalar.add(scalar.mul(plus_y,factor),r(center+4u));plus_x=scalar.add(scalar.mul(plus_x,factor),r(center));minus_y=scalar.add(scalar.mul(minus_y,factor),r(center+4u));
        w(0x2cc688u,f+0x10u,plus_y);auto ay=scalar.sub(plus_y,r(point+4u));w(0x2cc693u,f+4u,plus_x);auto bx=scalar.sub(plus_x,r(point));w(0x2cc69du,f+8u,minus_y);auto by=scalar.sub(minus_y,r(point+4u));
        auto cross=scalar.sub(scalar.mul(ay,bx),scalar.mul(ax,by));auto orient=scalar.above(cross,r(0x46a0e8u));auto side=r(f+0x1cu,1);auto sp=f;push(sp,caller[3]);const auto choice=u32(u32(orient)!=side);
        auto x=r(sp+choice*8u+8u),y=r(sp+choice*8u+0xcu);w(0x2cc6deu,out,x);w(0x2cc6e0u,out+4u,y);return;
    }
    auto x=r(f+4u);w(0x2cc6eeu,f+4u,x);auto y=r(f+0x14u);w(0x2cc6f6u,f+8u,y);
    auto small=norm(f+8u,f+4u,f+0x1cu,0x2cc70eu);u32 nx{},ny{};bool fallback=small;
    if(!small){auto magnitude=r(f+0x1cu),one=r(0x46a0d0u);ny=r(f+8u);fallback=scalar.zero(magnitude,0);auto inverse=scalar.div(one,magnitude);nx=scalar.mul(inverse,r(f+4u));ny=scalar.mul(ny,inverse);}
    if(fallback){auto vector=r(0x474cdcu),sp=f;push(sp,caller[6]);auto a=r(vector),b=r(vector+4u);w(0x2cc759u,sp+8u,a);nx=r(sp+8u);w(0x2cc763u,sp+0xcu,b);ny=r(sp+0xcu);}
    auto a=scalar.add(scalar.mul(nx,radius),r(center));w(0x2cc776u,out,a);auto b=scalar.add(scalar.mul(ny,radius),r(center+4u));w(0x2cc783u,out+4u,b);
}

void Segments::tangent_intersection(u32 entry,const std::array<u32,8> &caller,u32 radius) {
    auto sp=entry-8u;const auto center=caller[0],tangents=caller[1],out=caller[2];
    auto ay=r(tangents+4u),by=r(tangents+0xcu),ax=r(tangents);ay=scalar.sub(ay,r(center+4u));auto bx=r(tangents+8u);by=scalar.sub(by,r(center+4u));ax=scalar.sub(ax,r(center));bx=scalar.sub(bx,r(center));
    push(sp,caller[6]);const auto f=sp,origin=r(f+0x10u);auto cross=scalar.sub(scalar.mul(by,ax),scalar.mul(ay,bx));w(0x2cc7cfu,f+0x10u,cross);
    bool small{};{detail::NativeTu15ArithmeticEnvironment environment(fp);auto value=real(r(f+0x10u));asm volatile("flds %0\n\tfabs"::"m"(value):"memory","st");value=real(r(0x46a0ecu));u8 answer{};asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(answer):"m"(value):"memory","cc","st","st(1)");small=answer!=0;}
    if(!small) {
        ax=scalar.sub(ax,bx);auto square=scalar.mul(radius,radius),limit=scalar.mul(square,r(0x46a120u)),factor=scalar.div(square,cross);auto y=scalar.add(scalar.mul(ax,factor),r(center+4u));ay=scalar.sub(ay,by);auto x=scalar.sub(r(center),scalar.mul(ay,factor));
        w(0x2cc81au,out,x);w(0x2cc81eu,out+4u,y);auto dy=scalar.sub(y,r(center+4u)),dx=scalar.sub(x,r(center));auto distance=scalar.add(scalar.mul(dy,dy),scalar.mul(dx,dx));if(!scalar.above(distance,limit))return;
    }
    small=direction(tangents,origin,f+4u,f+0x10u,{0x2cc84eu,0x2cc858u,0x2cc86cu});u32 nx{},ny{};bool fallback=small;
    if(!small){auto magnitude=r(f+0x10u);fallback=scalar.zero(magnitude,r(0x46a0e8u));auto one=r(0x46a0d0u);ny=r(f+8u);auto inverse=scalar.div(one,magnitude);nx=scalar.mul(inverse,r(f+4u));ny=scalar.mul(ny,inverse);}
    if(fallback){auto vector=r(0x474cdcu),a=r(vector),b=r(vector+4u);w(0x2cc8bau,f+4u,a);nx=r(f+4u);w(0x2cc8c4u,f+8u,b);ny=r(f+8u);}
    auto a=scalar.add(scalar.mul(nx,radius),r(tangents));w(0x2cc8d6u,out,a);auto b=scalar.add(scalar.mul(ny,radius),r(tangents+4u));w(0x2cc8e3u,out+4u,b);
}

u8 Segments::corner(u32 entry,const std::array<u32,8> &caller) {
    auto sp=entry-0x28u;push(sp,caller[3]);push(sp,caller[5]);auto nav=r(sp+0x34u);
    w(0x2ccebfu,sp+0xcu,none);w(0x2ccec3u,sp+8u,none);auto edge=r(sp+0x40u);push(sp,caller[6]);push(sp,caller[7]);const auto f=sp;
    w(0x2ccecdu,f+0x48u,edge);u32 selected{};
    auto output=[&](u32 vertex,u32 first_site,u32 second_site,bool nav_first) {
        u32 vertices{},destination{};if(nav_first){vertices=r(nav+0x2cu);destination=r(f+0x50u);}else vertices=r(nav+0x2cu);
        auto x=r(vertices+vertex*12u);if(!nav_first)destination=r(f+0x50u);w(first_site,destination,x);auto y=r(vertices+vertex*12u+4u);w(second_site,destination+4u,y);return u8(1);
    };
    for(;;) {
        edge=r(f+0x48u);auto edges=r(nav+0xcu),width=r(f+0x44u),record=edges+edge*16u;
        auto left=r(record+0xcu,2);w(0x2cceeeu,f+0x18u,record);
        bool left_ok=left!=0xffffu && walkable(r(nav+4u)+left*8u,nav);w(left_ok?0x2ccf17u:0x2ccf1eu,f+0x3cu,left_ok,1);
        const auto side=r(f+0x3cu,1);auto sector=r(record+(side?0xcu:0xeu),2);if(sector==0xffffu)return 0;
        auto sectors=r(nav+4u);w(0x2ccf52u,f+0x20u,sector*8u);if(!walkable(sectors+sector*8u,nav))return 0;
        auto last=r(f+0x10u);sectors=r(nav+4u);w(0x2ccf7eu,f+0x24u,sectors);
        if(last!=none) {
            const auto link=r(record+6u,2);
            if(link!=0xffffu && std::bit_cast<i32>(r(nav+0x38u))>0) {
                edges=r(nav+0xcu);auto links=r(nav+0x3cu),linked=r(links+u32(sw(link))*20u+4u)*16u+edges;
                const auto right=r(linked+0xeu,2),other=r(linked+(right!=sector?0xeu:0xcu),2);
                if(other!=0xffffu && walkable(sectors+other*8u,nav))return output(r(f+0x10u),0x2cd36cu,0x2cd374u,false);
            }
        }
        const auto orientation=r(f+0x3cu,1);auto a=r(record,2),b=r(record+2u,2),first=orientation?b:a,second=orientation?a:b;
        auto vertices=r(nav+0x2cu),px=vertices+first*12u,qx=vertices+second*12u;
        auto dx=r(qx),dy=r(qx+4u);dx=scalar.sub(dx,r(px));dy=scalar.sub(dy,r(px+4u));auto nx=scalar.sub(0,dx);
        w(0x2cd02du,f+0x2cu,nx);
        // The original starts the first square before storing the second
        // coordinate; retain that ordered frame publication.
        bool small{};{
            detail::NativeTu15ArithmeticEnvironment environment(fp);auto value=real(r(f+0x2cu));asm volatile("flds %0"::"m"(value):"memory","st");value=real(r(f+0x2cu));asm volatile("fmuls %0"::"m"(value):"memory","st");
            w(0x2cd03bu,f+0x34u,dy);value=real(r(f+0x34u));asm volatile("flds %0"::"m"(value):"memory","st");value=real(r(f+0x34u));float length{};
            asm volatile("fmuls %1\n\tfaddp\n\tfsqrt\n\tfsts %0":"=m"(length):"m"(value):"memory","st","st(1)");w(0x2cd052u,f+0x1cu,bits(length));asm volatile("fabs":::"st");value=real(r(0x46a0ecu));u8 answer{};
            asm volatile("flds %1\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0":"=q"(answer):"m"(value):"memory","cc","st","st(1)");small=answer!=0;
        }
        auto ny=dy;if(!small){auto one=r(0x46a0d0u),magnitude=r(f+0x1cu);auto inverse=scalar.div(one,magnitude);nx=scalar.mul(inverse,nx);ny=scalar.mul(inverse,dy);}
        auto current=r(f+0x40u),negative=scalar.sub(0,width),left_x=scalar.add(scalar.mul(ny,width),r(current));auto left_y=scalar.add(scalar.mul(nx,width),r(current+4u));
        auto right_y=scalar.mul(negative,nx),vx=r(px);right_y=scalar.add(right_y,r(current+4u));auto right_x=scalar.add(scalar.mul(negative,ny),r(current));
        auto lx=scalar.sub(vx,left_x),ly=scalar.sub(r(px+4u),left_y),rx=scalar.sub(r(px),right_x),ry=scalar.sub(r(px+4u),right_y);
        auto along=scalar.add(scalar.mul(ly,dy),scalar.mul(lx,dx));const auto behind=scalar.above(0,along);const auto want=r(f+0x4cu,1);w(0x2cd0ffu,f+0x1cu,want);
        bool choose=false;if(u32(behind)==want){auto cross=scalar.sub(scalar.mul(lx,dy),scalar.mul(ly,dx));choose=scalar.above(0,cross);}
        auto cross=scalar.sub(scalar.mul(rx,dy),scalar.mul(ry,dx));if(scalar.above(0,cross))choose=true;if(r(f+0x14u)==none)choose=true;
        auto prior_orientation=r(f+0x3cu,1),desired=r(f+0x1cu);selected=(u32(choose!=bool(prior_orientation))==desired)?b:a;w(selected==b?0x2cd147u:0x2cd14du,f+0x3cu,selected);
        if(selected==r(f+0x10u))return output(selected,0x2cd38eu,0x2cd396u,true);
        auto first_seen=r(f+0x14u);if(selected==first_seen)return 0;
        if(first_seen==none){if(r(record+4u,1)&0x40u)return output(selected,0x2cd3b0u,0x2cd3b8u,true);w(0x2cd178u,f+0x14u,selected);}
        auto sector_offset=r(f+0x20u),sector_base=r(f+0x24u);bool restarted=false;
        if(!(r(sector_base+sector_offset,1)&8u)) {
            auto link=u32(sw(r(sector_base+sector_offset+2u,2)));
            while(link!=none && std::bit_cast<i32>(r(nav+0x38u))>0) {
                auto links=r(nav+0x3cu),link_record=links+link*20u;
                if(r(link_record,2)==0) {
                    auto portal=r(link_record+4u),edges_again=r(nav+0xcu),requested=r(f+0x4cu,1),current_edge=r(f+0x48u);
                    auto portal_record=edges_again+portal*16u;
                    if(r(link_record+(requested?0xcu:0x10u))==current_edge) {
                        auto destination=r(link_record+8u)*16u+edges_again,ls=r(destination+0xcu,2);
                        bool pass=ls!=0xffffu && walkable(r(nav+4u)+ls*8u,nav);
                        if(!pass){auto rs=r(destination+0xeu,2);pass=rs!=0xffffu && walkable(r(nav+4u)+rs*8u,nav);}
                        if(pass){auto next=r(link_record+8u);w(0x2cd2cbu,f+0x48u,next);auto choose_side=r(f+0x4cu,1);auto vertex=r(portal_record+(choose_side?0u:2u),2);w(choose_side?0x2cd2dau:0x2cd2efu,f+0x10u,vertex);w(choose_side?0x2cd2deu:0x2cd2f3u,f+0x14u,none);restarted=true;break;}
                    }
                    record=r(f+0x18u);selected=r(f+0x3cu);
                }
                link=u32(sw(r(link_record+2u,2)));
            }
        }
        if(restarted)continue;
        auto original_edge=r(f+0x48u);w(0x2cd270u,f+0x3cu,0);w(0x2cd278u,f+0x24u,original_edge);
        for(;;) {
            auto vertex=r(record+2u,2),direction=selected==vertex?0u:1u,region=r(record+0xcu+direction*2u,2);
            bool pass=region!=0xffffu && walkable(r(nav+4u)+region*8u,nav);
            if(u32(pass)==r(f+0x4cu,1)){w(0x2cd351u,f+0x10u,selected);break;}
            auto next=r(record+8u+direction*2u,2);w(0x2cd313u,f+0x48u,next);if(next==0xffffu)return 0;
            edges=r(nav+0xcu);record=edges+next*16u;w(0x2cd327u,f+0x18u,record);if(next==r(f+0x24u))return 0;
            auto iterations=r(f+0x3cu)+1u;w(0x2cd33eu,f+0x3cu,iterations);if(sw(iterations)>50)return 0;record=r(f+0x18u);
        }
    }
}

void Segments::run(u32 entry,const std::array<u32,8> &caller) {
    auto sp=entry;push(sp,caller[5]);const auto bp=sp;sp=(sp&~7u)-0x10cu;
    const auto request=r(bp+8u),radius=r(request);w(0x2cbef1u,sp+0x2cu,radius);
    u32 global{},nav_count{};
    {
        detail::NativeTu15ArithmeticEnvironment environment(fp);auto value=real(r(sp+0x2cu));
        asm volatile("flds %0"::"m"(value):"memory","st");global=r(0x4ecc18u);
        std::array<std::byte,8> margin{};memory.read(0x46a8b0u,margin);double extra{};std::memcpy(&extra,margin.data(),8);
        asm volatile("faddl %0"::"m"(extra):"memory","st");nav_count=r(global+0xc4u);push(sp,caller[3]);
        asm volatile("fstps %0":"=m"(value)::"memory","st");w(0x2cbf0fu,sp+0x34u,bits(value));
    }
    push(sp,caller[6]);push(sp,caller[7]);const auto f=sp;
    auto finish=[&](){abi.gp=caller;abi.gp[7]=r(f);abi.gp[6]=r(f+4u);abi.gp[3]=r(f+8u);abi.gp[5]=r(bp);abi.gp[4]=entry+0x1cu;};
    bool absent=std::bit_cast<i32>(nav_count)<=0;
    if(!absent){auto nav=r(global+0xc8u);w(0x2cbf23u,f+0x40u,nav);absent=nav==0;}
    if(absent){auto count=r(bp+0x14u),complete=r(bp+0x1cu);w(0x2cc5a3u,count,0,2);w(0x2cc5a6u,complete,0,1);finish();return;}
    const auto input_count=r(bp+0xcu,2);
    if(sw(input_count)<=1){auto count=r(bp+0x14u),input=r(bp+0x10u),output=r(bp+0x18u);w(0x2cc58du,count,1,2);copy(input,output,0x2cc592u);finish();return;}
    auto y=r(request+0x18u),origin=request+0x14u,x=r(origin);w(0x2cbf46u,f+0x14u,y);
    auto input=r(bp+0x10u);w(0x2cbf4du,f+0x34u,origin);auto sector=r(input+4u);
    w(0x2cbf54u,f+0x24u,0);w(0x2cbf58u,f+0x30u,0);w(0x2cbf5cu,f+0xfu,0,1);w(0x2cbf60u,f+0x10u,x);w(0x2cbf64u,f+0x20u,sector);
    u32 output_count{};
    for(;;) {
        const auto cursor=r(f+0x24u),current=input+row_offset(cursor),kind=r(current,2);
        w(0x2cbf8fu,f+0x18u,none);w(0x2cbf93u,f+0xeu,0,1);w(0x2cbf98u,f+0x74u,none);w(0x2cbf9cu,f+0x6cu,none);w(0x2cbfa0u,f+0x84u,current);
        u32 edge=0xffffu,scan{},frame{},width=radius;bool custom_point=false;
        if(kind!=0xffffu){auto link=r(current+8u,2);w(0x2cbfadu,f+0x18u,cursor);w(0x2cbfb1u,f+0x74u,link,2);w(0x2cbfb6u,f+0x6cu,kind);}
        else {
            scan=cursor;
            if(sw(cursor)<sw(r(bp+0xcu,2)))for(;;){if(r(input+row_offset(scan),2)!=0xffffu){w(0x2cc01au,f+0x18u,scan);w(0x2cc023u,f+0xeu,1,1);break;}++scan;if(sw(scan)>=sw(r(bp+0xcu,2)))break;}
            scan-=1u;bool read_width_after_scan=sw(scan)<sw(cursor),scan_completed=false;
            if(sw(scan)>=sw(cursor))for(;;) {
                input=r(bp+0x10u);const auto candidate=input+row_offset(scan);
                if(direction(candidate+0xcu,f+0x10u,f+0x28u,f+0x1cu)){read_width_after_scan=true;break;}
                auto magnitude=r(f+0x1cu),zero=r(0x46a0e8u);const auto positive=scalar.above(magnitude,zero);
                auto inverse=scalar.div(r(0x46a0d0u),magnitude);auto dx=r(f+0x28u);w(0x2cc0a1u,f+0x28u,scalar.mul(inverse,dx));
                auto dy=r(f+0x2cu);w(0x2cc0b1u,f+0x2cu,scalar.mul(dy,inverse));if(!positive){read_width_after_scan=true;break;}
                width=r(f+0x38u);auto call_sp=f;push(call_sp,f+0xc0u);auto req=r(bp+8u);push(call_sp,req+0x70u);
                const auto last=sw(r(bp+0xcu,2))-1;auto length=r(f+0x1cu);push(call_sp,0);push(call_sp,(u32(sw(scan))&0xffffff00u)|(sw(scan)==last));push(call_sp,0);push(call_sp,length);
                auto target_sector=r(candidate+4u),start_sector=r(f+0x20u);push(call_sp,width);push(call_sp,f+0x28u);push(call_sp,target_sector);auto nav=r(f+0x40u);
                push(call_sp,start_sector);push(call_sp,f+0x10u);push(call_sp,none);push(call_sp,0);push(call_sp,nav);
                child(0x2cc0fdu,0x2769c0u,{start_sector,f+0x10u,nav,edge,call_sp,bp,scan,width},0x38u);
                if(!(abi.gp[0]&0xffu))break;
                edge=r(f+0xc8u);w(0x2cc10du,f+0x18u,scan);--scan;
                const auto again=r(f+0x24u,2);w(0x2cc117u,f+0xeu,1,1);if(sw(scan)<sw(again)){scan_completed=true;break;}
            }
            if(read_width_after_scan)width=r(f+0x38u);
            if(!scan_completed && !r(f+0xeu,1)) {
                const auto count=r(bp+0xcu,2);output_count=r(f+0x30u);input=r(bp+0x10u);auto output=r(bp+0x18u);
                copy(input+row_offset(count)-28u,output+row_offset(output_count),0x2cc55au);++output_count;w(0x2cc55du,f+0xfu,1,1);break;
            }
            if(edge!=0xffffu) {
                const auto index=r(f+0x18u,2);input=r(bp+0x10u);const auto target=input+row_offset(index),target_kind=r(target,2);w(0x2cc153u,f+0x80u,target);
                if(target_kind==0xffffu) {
                    auto req=r(bp+8u),call_sp=f;push(call_sp,f+0x88u);push(call_sp,1);auto nav=r(req+0x8cu);push(call_sp,edge);w(0x2cc177u,f+0x64u,req+4u);
                    push(call_sp,width);push(call_sp,f+0x10u);push(call_sp,nav);
                    child(0x2cc182u,0x2cceb0u,{nav,f+0x88u,f+0x10u,edge,call_sp,bp,target,width},0x18u);
                    const auto left_ok=abi.gp[0]&0xffu;call_sp=f;push(call_sp,f+0x90u);push(call_sp,0);push(call_sp,edge);w(0x2cc192u,f+0xeu,left_ok,1);
                    req=r(bp+8u);nav=r(req+0x8cu);push(call_sp,width);push(call_sp,f+0x10u);push(call_sp,nav);
                    child(0x2cc1a6u,0x2cceb0u,{req,nav,f+0x10u,edge,call_sp,bp,target,width},0x18u);
                    if((abi.gp[0]&0xffu) && r(f+0xeu,1)) {
                        auto previous=sw(r(f+0x18u,2))>0?target-0x10u:r(f+0x34u);call_sp=f;push(call_sp,f+0x10u);w(0x2cc1e4u,f+0x1cu,target+0xcu);
                        child(0x2cc1e8u,0x2cc8f0u,{previous,abi.gp[1],f+0x10u,f+0x88u,call_sp,bp,target+0xcu,f+0x90u},4u);
                        auto current_x=r(f+0x10u),exclusions=r(bp+8u)+0x70u,side_word=((f+0x88u)&0xffffff00u)|(abi.gp[0]&0xffu);
                        w(0x2cc1fbu,f+0xa4u,current_x);auto current_y=r(f+0x14u);w(0x2cc20du,f+0x70u,side_word,1);w(0x2cc211u,f+0x68u,0);w(0x2cc219u,f+0xa0u,0);w(0x2cc222u,f+0xa8u,current_y);w(0x2cc22bu,f+0xacu,0);w(0x2cc234u,f+0x64u,exclusions);
                        for(;;) {
                            if(side_word&0xffu){auto cx=r(f+0x88u),cy=r(f+0x8cu);w(0x2cc24au,f+0x44u,cx);w(0x2cc24eu,f+0x48u,cy);}
                            else {auto cx=r(f+0x90u),cy=r(f+0x94u);w(0x2cc262u,f+0x44u,cx);w(0x2cc266u,f+0x48u,cy);}
                            auto side_argument=r(f+0x70u),widened=r(f+0x3cu);call_sp=f;push(call_sp,side_argument);abi.xmm[6][0]=widened;
                            child(0x2cc284u,0x2cc5c0u,{f+0x10u,f+0x44u,f+0xb0u,side_word,call_sp,bp,target+0xcu,exclusions},4u);
                            auto target_point=r(f+0x1cu);widened=r(f+0x3cu);side_word=(side_word&0xffffff00u)|(!(side_word&0xffu));call_sp=f;push(call_sp,side_word);abi.xmm[6][0]=widened;
                            child(0x2cc2a4u,0x2cc5c0u,{target_point,f+0x44u,f+0xb8u,side_word,call_sp,bp,target+0xcu,exclusions},4u);
                            widened=r(f+0x3cu);call_sp=f;push(call_sp,f+0x10u);abi.xmm[7][0]=widened;
                            child(0x2cc2c3u,0x2cc790u,{f+0x44u,f+0xb0u,f+0x78u,side_word,call_sp,bp,target+0xcu,exclusions},4u);
                            auto start_sector=r(f+0x20u),candidate_x=r(f+0x78u),navigation=r(f+0x40u);call_sp=f;push(call_sp,exclusions);push(call_sp,none);push(call_sp,start_sector);w(0x2cc2dau,f+0x98u,candidate_x);auto candidate_y=r(f+0x7cu);push(call_sp,navigation);w(0x2cc302u,f+0x9cu,candidate_y);
                            child(0x2cc30bu,0x2761f0u,{f+0xa4u,f+0x98u,f+0x10u,side_word,call_sp,bp,f+0xd0u,exclusions},0x10u);
                            if(!r(f+0xd0u,1)) {
                                auto from=r(f+0xe0u);navigation=r(f+0x40u);call_sp=f;push(call_sp,exclusions);auto target_row=r(f+0x80u),to=r(target_row+4u);push(call_sp,to);push(call_sp,from);push(call_sp,navigation);target_point=r(f+0x1cu);
                                child(0x2cc346u,0x2761f0u,{f+0x98u,target_point,to,side_word,call_sp,bp,f+0xf4u,target_row},0x10u);
                                if(!(abi.gp[0]&0xffu)){auto sec=r(f+0xe0u),cx=r(f+0x78u),cy=r(f+0x7cu);frame=r(target_row+0x18u,2);w(0x2cc382u,f+0x20u,sec);w(0x2cc386u,f+0x10u,cx);w(0x2cc38au,f+0x14u,cy);custom_point=true;break;}
                                exclusions=r(f+0x64u);
                            }
                            auto iteration=r(f+0x68u)+1u;w(0x2cc35cu,f+0x70u,side_word,1);w(0x2cc360u,f+0x68u,iteration);if(sw(iteration)>=2)break;
                        }
                    }
                }
            }
        }
        if(!custom_point) {
            const auto index=r(f+0x18u);bool direct=u16(index)==0;
            if(!direct)direct=u16(index)==r(f+0x24u,2);
            input=r(bp+0x10u);const auto target=input+row_offset(index);
            if(direct){frame=r(target+0x18u,2);auto py=r(target+0x10u),px=r(target+0xcu);w(0x2cc3a7u,f+0x14u,py);auto next=r(f+0x24u);w(0x2cc3afu,f+0x10u,px);auto sec=r(target+4u);w(0x2cc3b7u,f+0x20u,sec);w(0x2cc3bbu,f+0x18u,next+1u);}
            else {auto px=r(target-0x10u);frame=r(target-4u,2);auto py=r(target-0xcu),sec=r(target-0x18u);w(0x2cbfecu,f+0x10u,px);w(0x2cbff0u,f+0x14u,py);w(0x2cbff4u,f+0x20u,sec);}
        }
        output_count=r(f+0x30u);auto output=r(bp+0x18u),sec=r(f+0x20u),px=r(f+0x10u);const auto out=output+row_offset(output_count),point=out+0xcu;
        w(0x2cc3d8u,out+4u,sec);auto current_again=r(f+0x84u),pz=r(current_again+0x14u);w(0x2cc3eau,point,px);auto py=r(f+0x14u);
        w(0x2cc3f4u,point+4u,py);w(0x2cc3f9u,point+8u,pz);w(0x2cc3feu,out+0x18u,frame,2);auto flags=r(current_again+2u,2);w(0x2cc406u,out+2u,flags,2);++output_count;
        const auto saved_kind=r(f+0x6cu,2);w(0x2cc415u,f+0x30u,output_count);
        if(saved_kind!=0xffffu){auto kind_again=r(current_again,2);w(0x2cc41eu,out,kind_again,2);auto link=r(f+0x74u,2);w(0x2cc426u,out+8u,link,2);}
        else {w(0x2cc42cu,out,0xffffu,2);w(0x2cc42fu,out+8u,0xffffu,2);}
        if(sw(output_count)>=4)break;
        const auto original=r(f+0x34u),original_frame=r(original+0xcu,2),new_frame=r(point+0xcu,2);u32 dx{},dy{},dz{};
        if(original_frame==new_frame){auto ax=r(point),ay=r(point+4u),az=r(point+8u);dx=scalar.sub(ax,r(original));dy=scalar.sub(ay,r(original+4u));dz=scalar.sub(az,r(original+8u));}
        else {
            bool resolved=false;
            if(original_frame!=0xffffu){auto call_sp=f;push(call_sp,original);push(call_sp,0x2cc47bu);resolved=native_campaign_ai_resolve_frame_tu15(memory,u16(original_frame),original,f+0x58u,fp,observe);}
            if(!resolved){auto source=original_frame==0xffffu?original:r(f+0x34u),a=r(source),b=r(source+4u);if(original_frame==0xffffu){w(0x2cc49eu,f+0x58u,a);auto c=r(source+8u);w(0x2cc4a5u,f+0x5cu,b);w(0x2cc4a9u,f+0x60u,c);}else{auto c=r(source+8u);w(0x2cc48bu,f+0x58u,a);w(0x2cc48fu,f+0x5cu,b);w(0x2cc493u,f+0x60u,c);}}
            auto nf=r(point+0xcu,2);resolved=false;if(nf!=0xffffu){auto call_sp=f;push(call_sp,point);push(call_sp,0x2cc4c3u);resolved=native_campaign_ai_resolve_frame_tu15(memory,u16(nf),point,f+0x4cu,fp,observe);}
            if(!resolved){auto a=r(point),b=r(point+4u),c=r(point+8u);w(0x2cc4cfu,f+0x4cu,a);w(0x2cc4d3u,f+0x50u,b);w(0x2cc4d7u,f+0x54u,c);}
            auto a=r(f+0x4cu),b=r(f+0x50u),c=r(f+0x54u);dx=scalar.sub(a,r(f+0x58u));dy=scalar.sub(b,r(f+0x5cu));dz=scalar.sub(c,r(f+0x60u));output_count=r(f+0x30u);
        }
        const auto yy=scalar.mul(dy,dy),zz=scalar.mul(dz,dz),zy=scalar.add(zz,yy),xx=scalar.mul(dx,dx);if(scalar.above(scalar.add(zy,xx),r(0x46a6e4u)))break;
        auto next=r(f+0x18u),limit=r(bp+0xcu,2);w(0x2cc52eu,f+0x24u,next);if(sw(next)>=sw(limit))break;input=r(bp+0x10u);
    }
    auto count_out=r(bp+0x14u);w(0x2cc565u,count_out,output_count,2);if(!r(f+0xfu,1)){auto complete=r(bp+0x1cu);w(0x2cc573u,complete,0,1);}finish();
}
struct FpPublication {SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~FpPublication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}};
}
void native_campaign_ai_route_segments_call_tu15(SourcePhysicsContactMemory &memory,u32 entry,
    SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiPerformServicesTu15 &services,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth)throw std::invalid_argument("Native route segments require retained FP and empty x87");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};FpPublication publish{abi,fp};
    if((fp.control_word&0x3fu)!=0x3fu || (fp.mxcsr&0x1f80u)!=0x1f80u)throw std::invalid_argument("Native route segments require masked FP exceptions");
    auto caller=abi.gp;Segments owner{memory,abi,services,fp,observe};
    if(entry==0x2cbee0u){owner.run(caller[4],caller);return;}
    if(entry==0x2cceb0u){const auto result=owner.corner(caller[4],caller);const auto f=caller[4]-0x38u;abi.gp=caller;abi.gp[0]=(caller[0]&0xffffff00u)|result;abi.gp[7]=owner.r(f);abi.gp[6]=owner.r(f+4u);abi.gp[5]=owner.r(f+8u);abi.gp[3]=owner.r(f+0xcu);abi.gp[4]=caller[4]+0x1cu;return;}
    if(entry==0x2cc5c0u){owner.tangent(caller[4],caller,abi.xmm[6][0]);abi.gp=caller;abi.gp[4]+=8u;return;}
    if(entry==0x2cc790u){owner.tangent_intersection(caller[4],caller,abi.xmm[7][0]);abi.gp=caller;abi.gp[6]=owner.r(caller[4]-0xcu);abi.gp[4]+=8u;return;}
    if(entry==0x2cc8f0u){auto result=owner.choose_corner(caller[4],caller);abi.gp=caller;abi.gp[0]=(caller[0]&0xffffff00u)|result;abi.gp[5]=owner.r(caller[4]-0x38u);abi.gp[4]+=8u;return;}
    throw std::invalid_argument("Native route segments have no such entry");
}
#else
void native_campaign_ai_route_segments_call_tu15(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,const NativeCampaignAiPerformServicesTu15 &,const SourcePhysicsContactRecordObserver &){throw std::runtime_error("Exact native route segments arithmetic unavailable");}
#endif
}
