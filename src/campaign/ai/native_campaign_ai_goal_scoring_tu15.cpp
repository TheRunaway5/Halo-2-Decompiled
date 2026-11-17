#include "halo2/native_campaign_ai_goal_scoring_tu15.hpp"
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"
#include <array>
#include <cstring>
#include <stdexcept>

namespace halo2 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
namespace {
using namespace goal_tu15;
constexpr u32 actor_pool=0x5020c8u,none_cell=0x47d6c0u;

u32 actor_row(const Port &p,u32 actor) {
    const auto pool=p.r32(actor_pool);
    return (actor&0xffffu)*0x888u+p.r32(pool+0x44u);
}

// 26C5A0.
bool history_contains(const Port &p,u32 actor,u32 goal) {
    const auto pool=p.r32(actor_pool);
    const auto rows=p.r32(pool+0x44u);
    const auto none=p.r32(none_cell);
    const auto row=(actor&0xffffu)*0x888u+rows;
    if(goal==none)return false;
    for(i32 n=0;n<4;++n)if(goal==p.r32(row+u32(n+0xab)*6u))return true;
    return false;
}

// 1209C0: |point - clamp(origin + t*vector)|^2. The vector is a caller
// value (a local in 267A60) or retained memory.
struct Vector {
    const Port *port{};u32 address{};std::array<u32,3> value{};
    u32 at(unsigned k)const{return port?port->r32(address+k*4u):value[k];}
};
u32 segment_distance(const Port &p,const Simd &s,const Vector &v,u32 origin,u32 point) {
    const auto vy=v.at(1),vz=v.at(2),vx=v.at(0);
    const auto zz=s.mul(vz,vz),xx=s.mul(vx,vx);
    const auto length=s.add(s.add(s.mul(vy,vy),zz),xx);
    if(!s.above(length,p.r32(0x46a0ecu))) {
        const auto ay=p.r32(origin+4u),dy=s.sub(ay,p.r32(point+4u));
        const auto ax=p.r32(origin),dx=s.sub(ax,p.r32(point));
        const auto az=p.r32(origin+8u),dz=s.sub(az,p.r32(point+8u));
        return s.add(s.add(s.mul(dz,dz),s.mul(dy,dy)),s.mul(dx,dx));
    }
    const auto pz=p.r32(point+8u),px=p.r32(point);
    const auto dz=s.sub(pz,p.r32(origin+8u)),dx=s.sub(px,p.r32(origin));
    const auto wy=v.at(1),wx=v.at(0);
    const auto py=p.r32(point+4u),dy=s.sub(py,p.r32(origin+4u));
    auto t=s.add(s.add(s.mul(wx,dx),s.mul(dz,vz)),s.mul(dy,wy));
    const auto denominator=s.add(s.add(s.mul(wy,wy),s.mul(vz,vz)),xx);
    t=s.div(t,denominator);
    if(s.above(0u,t))t=0u;
    else {
        const auto one=p.r32(0x46a0d0u);
        if(s.above(t,one))t=one;
    }
    const auto ux=v.at(0);
    const auto negative=s.sub(0u,t);
    const auto uy=v.at(1);
    auto ry=s.mul(negative,uy);
    auto rx=s.mul(ux,negative);
    const auto uz=v.at(2);
    auto rz=s.mul(negative,uz);
    rx=s.add(rx,dx);ry=s.add(ry,dy);rz=s.add(rz,dz);
    const auto yy=s.mul(ry,ry);
    return s.add(s.add(s.mul(rz,rz),yy),s.mul(rx,rx));
}

// x87 sequences of 267A60 (each from an empty stack).
// 267B8B: sqrt(z*z + y*y + x*x) extended, FLD bound, FXCH, FCOMIP: JA.
bool x87_length_above(NativeTu15PhysicsFrameFp &fp,u32 z,u32 y,u32 x,u32 bound) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);unsigned char above{};
    asm volatile("flds %1\n\tfmuls %1\n\tflds %2\n\tfmuls %2\n\t.byte 0xde,0xc1\n\tflds %3\n\tfmuls %3\n\t.byte 0xde,0xc1\n\t"
                 ".byte 0xd9,0xfa\n\tflds %4\n\t.byte 0xd9,0xc9\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tseta %0"
        :"=q"(above):"m"(z),"m"(y),"m"(x),"m"(bound):"cc","memory","st","st(1)");
    return above!=0;
}
// 267BC8: |a - b| with a at ECX, b at EDX (EBP-4) as extended differences.
u32 x87_distance(NativeTu15PhysicsFrameFp &fp,const std::array<u32,6> &operands) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 out{};
    asm volatile("flds %1\n\tfsubs %2\n\tflds %3\n\tfsubs %4\n\tflds %5\n\tfsubs %6\n\t"
                 ".byte 0xd9,0xc0\n\t.byte 0xd8,0xc9\n\t.byte 0xd9,0xc2\n\t.byte 0xd8,0xcb\n\t.byte 0xde,0xc1\n\t"
                 ".byte 0xd9,0xc3\n\t.byte 0xd8,0xcc\n\t.byte 0xde,0xc1\n\t.byte 0xd9,0xfa\n\t"
                 ".byte 0xdd,0xdb\n\t.byte 0xdd,0xd8\n\t.byte 0xdd,0xd8\n\tfstps %0"
        :"=m"(out):"m"(operands[0]),"m"(operands[1]),"m"(operands[2]),"m"(operands[3]),"m"(operands[4]),"m"(operands[5])
        :"memory","st","st(1)","st(2)","st(3)","st(4)");
    return out;
}
// 267C41: (sqrt(v) - a) * b, FSTP m32.
u32 x87_root_offset_scale(NativeTu15PhysicsFrameFp &fp,u32 value,u32 offset,u32 scale) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 out{};
    asm volatile("flds %1\n\tfsqrt\n\tfsubs %2\n\tfmuls %3\n\tfstps %0"
        :"=m"(out):"m"(value),"m"(offset),"m"(scale):"memory","st");
    return out;
}
// 267E42: sqrt(v) * b, FSTP m32.
u32 x87_root_scale(NativeTu15PhysicsFrameFp &fp,u32 value,u32 scale) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 out{};
    asm volatile("flds %1\n\tfsqrt\n\tfmuls %2\n\tfstps %0":"=m"(out):"m"(value),"m"(scale):"memory","st");
    return out;
}
// 267DB1: FLD QWORD bound, FLD DWORD radius, FCOMIP ST(1), FSTP ST(0): JA.
bool x87_single_above_double(NativeTu15PhysicsFrameFp &fp,const std::array<std::byte,8> &bound,u32 value) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);unsigned char above{};
    asm volatile("fldl %1\n\tflds %2\n\t.byte 0xdf,0xf1\n\t.byte 0xdd,0xd8\n\tseta %0"
        :"=q"(above):"m"(bound),"m"(value):"cc","memory","st","st(1)");
    return above!=0;
}
// 267FD3: (z*a + y*b + x*c) / sqrt(d), extended; FSTP m32.
u32 x87_projection(NativeTu15PhysicsFrameFp &fp,u32 z,u32 a,u32 y,u32 b,u32 x,u32 c,u32 squared) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 out{};
    asm volatile("flds %1\n\tfmuls %2\n\tflds %3\n\tfmuls %4\n\t.byte 0xde,0xc1\n\tflds %5\n\tfmuls %6\n\t.byte 0xde,0xc1\n\t"
                 "flds %7\n\t.byte 0xd9,0xfa\n\t.byte 0xd9,0xc9\n\t.byte 0xd9,0xc9\n\t.byte 0xde,0xf9\n\tfstps %0"
        :"=m"(out):"m"(z),"m"(a),"m"(y),"m"(b),"m"(x),"m"(c),"m"(squared):"memory","st","st(1)","st(2)");
    return out;
}

class Scoring {
public:
    const Port &p;const Simd &s;NativeTu15PhysicsFrameFp &fp;
    void history(u32 actor,u32 request,u32 count,u32 candidates);
    void distance(u32 request,u32 count,u32 candidates);
    void table(u32 actor,u32 request,u32 count,u32 candidates);
    bool evaluate(u32 actor,u32 request,u32 candidate,u32 site);
    bool evaluators(u32 actor,u32 request,u32 candidate,bool request_loop);
    bool compare(u32 a,u32 b);
    void selection(u32 low,u32 high);
    void sort(u32 base,u32 count);
    bool admit(u32 actor,u32 request,u32 candidate);
};

// 267A60: goal history, the request's +51 target sweep, +70 avoid points,
// +698/+5A adjustments and the +5B facing preference.
void Scoring::history(u32 actor,u32 request,u32 count_word,u32 candidates) {
    const auto pool=p.r32(actor_pool);
    const auto row=(actor&0xffffu)*0x888u+p.r32(pool+0x44u);
    const auto count=sx16(count_word);
    for(i32 n=0;n<count;++n) {
        const auto c=candidates+u32(n)*0x78u;
        if(!p.r8(c+0x4cu))continue;
        const auto goal=p.r32(c+4u);
        if(history_contains(p,actor,goal)) {
            p.w8(0x267ac0u,c+0x4du,1);
            if(!p.r8(request+0x14u)){p.w8(0x267acbu,c+0x4cu,0);continue;}
        }
        if(p.r8(request+0x51u)) {
            // 267ADF: the swept target volume.
            const auto a388=p.r32(row+0x388u),sx=s.sub(a388,p.r32(row+0x370u));
            const auto c14=p.r32(c+0x14u),dz=s.sub(c14,p.r32(row+0x3a8u));
            const auto c10=p.r32(c+0x10u),dy=s.sub(c10,p.r32(row+0x3a4u));
            const auto a39c=p.r32(row+0x39cu),reach=s.add(a39c,p.r32(0x46a104u));
            const auto a38c=p.r32(row+0x38cu),sy=s.sub(a38c,p.r32(row+0x374u));
            const auto a390=p.r32(row+0x390u),sz=s.sub(a390,p.r32(row+0x378u));
            const auto c0c=p.r32(c+0xcu),dx=s.sub(c0c,p.r32(row+0x3a0u));
            auto d2=s.mul(dz,dz);
            d2=s.add(d2,s.mul(dy,dy));
            d2=s.add(d2,s.mul(dx,dx));
            if(s.above(s.mul(reach,reach),d2)) {
                u32 distance;
                const auto bound=p.r32(0x46a0e8u);
                if(x87_length_above(fp,sz,sy,sx,bound)) {
                    distance=segment_distance(p,s,Vector{nullptr,0,{sx,sy,sz}},row+0x370u,c+0xcu);
                } else {
                    std::array<u32,6> operands{};
                    operands[0]=p.r32(row+0x370u);operands[1]=p.r32(c+0xcu);
                    operands[2]=p.r32(row+0x374u);operands[3]=p.r32(c+0x10u);
                    operands[4]=p.r32(row+0x378u);operands[5]=p.r32(c+0x14u);
                    distance=x87_distance(fp,operands);
                }
                const auto radius=p.r32(row+0x36cu);
                u32 bonus=0;bool scored=true;
                if(s.above(s.mul(radius,radius),distance)) {
                    p.w8(0x267c11u,c+0x4du,1);
                    if(!p.r8(request+0x14u)){p.w8(0x267c1cu,c+0x4cu,0);scored=false;}
                } else {
                    const auto r=p.r32(row+0x36cu),outer=s.add(r,p.r32(0x46a104u));
                    if(s.above(s.mul(outer,outer),distance)) {
                        const auto offset=p.r32(row+0x36cu);
                        const auto scale=p.r32(0x46a128u);
                        bonus=x87_root_offset_scale(fp,distance,offset,scale);
                    } else bonus=p.r32(0x46a118u);
                }
                if(!scored)continue;
                const auto score=p.r32(c+0x54u);
                p.w32(0x267c70u,c+0x54u,s.add(score,bonus));
            }
            // 267C75: the moving target's swept test.
            const auto seen=p.r32(row+0x39cu),late=s.add(seen,p.r32(0x46a0e4u));
            if(s.above(s.mul(late,late),p.r32(row+0x398u))) {
                const auto now=p.r32(row+0x394u);
                if(s.above(now,p.r32(row+0x36cu))) {
                    const auto radius=p.r32(row+0x36cu);
                    const auto swept=segment_distance(p,s,Vector{nullptr,0,{sx,sy,sz}},row+0x370u,row+0x238u);
                    if(s.above(swept,s.mul(radius,radius))) {
                        const auto k=p.r32(0x46a0e4u);
                        const auto vx=p.r32(c+0x1cu),vy=p.r32(c+0x20u),vz=p.r32(c+0x24u);
                        const auto my=s.mul(vy,k),mx=s.mul(vx,k),mz=s.mul(vz,k);
                        auto length=s.mul(mz,mz);
                        length=s.add(length,s.mul(my,my));
                        length=s.add(length,s.mul(mx,mx));
                        if(s.above(length,p.r32(0x46a0ecu))) {
                            (void)p.r32(row+0x36cu);
                            throw NativeCampaignAiPerformRequiredOwnerTu15(0x267d53u,0x120af0u);
                        }
                    }
                }
            }
        }
        // 267D89: the request's +70 avoid points.
        const auto points=i32(p.r32(request+0x70u));
        if(points<=0)continue;
        const auto one=p.r32(0x46a0d0u);
        auto nearest=one;
        for(i32 k=0;k<points;k=i32(i16(u16(k+1)))) {
            const auto bound=p.r64(0x46a0f0u);
            auto ratio=p.r32(0x45695cu);
            const auto record=request+u32(k)*16u;
            const auto radius=p.r32(record+0x74u);
            const auto ex=p.r32(record+0x78u),ey=p.r32(record+0x7cu),ez=p.r32(record+0x80u);
            const auto qx=s.sub(ex,p.r32(c+0xcu));
            const auto qy=s.sub(ey,p.r32(c+0x10u));
            const auto qz=s.sub(ez,p.r32(c+0x14u));
            if(x87_single_above_double(fp,bound,radius)) {
                const auto r=p.r32(record+0x74u);
                auto d=s.mul(qz,qz);
                d=s.add(d,s.mul(qy,qy));
                d=s.add(d,s.mul(qx,qx));
                ratio=s.div(d,s.mul(r,r));
            }
            if(s.above(nearest,ratio))nearest=ratio;
        }
        auto bonus=p.r32(0x46a110u);
        if(s.above(one,nearest))bonus=x87_root_scale(fp,nearest,p.r32(0x46a110u));
        const auto score=p.r32(c+0x54u);
        p.w32(0x267e61u,c+0x54u,s.add(score,bonus));
    }
    // 267E78: the +698 kind mask and +5A untargeted preference.
    if(count>0) {
        const auto bonus=p.r32(0x46a120u);
        for(i32 n=0;n<count;++n) {
            const auto c=candidates+u32(n)*0x78u;
            if(!p.r8(c+0x4cu))continue;
            if(p.r8(request+0x698u)) {
                const auto record=p.r32(c);
                const auto kind=u32(sx16(p.r16(record+0xeu)));
                const auto mask=u32(sx16(p.r16(request+0x69au)));
                if(u16(mask&kind)==u16(kind)) {
                    const auto score=p.r32(c+0x54u);
                    p.w32(0x267ec9u,c+0x54u,s.add(score,bonus));
                }
            }
            if(p.r8(c+0x5au) && p.r32(row+0x26cu)==0xffffffffu) {
                const auto score=p.r32(c+0x54u);
                p.w32(0x267ee4u,c+0x54u,s.add(score,bonus));
            }
        }
    }
    // 267EEE: +5B prefers positions in front of the unit or its target.
    if(p.r8(request+0x5bu)) {
        auto target=p.r32(row+0x26cu);
        const auto objects=p.r32(0x4ecbd0u),object_rows=p.r32(objects+0x44u);
        if(target==0xffffffffu)target=p.r32(row+0x18u);
        const auto body=p.r32((target&0xffffu)*12u+object_rows+8u);
        const auto unit=p.r32(row+0x18u);
        const auto origin=native_campaign_object_world_point_value_tu15(p.memory,unit,fp);
        if(count>0) {
            const auto near=p.r32(0x46a510u);
            for(i32 n=0;n<count;++n) {
                const auto c=candidates+u32(n)*0x78u;
                if(!p.r8(c+0x4cu))continue;
                const auto cz=p.r32(c+0x14u),cx=p.r32(c+0xcu);
                const auto dx=s.sub(cx,origin[0]);
                const auto cy=p.r32(c+0x10u);
                const auto dy=s.sub(cy,origin[1]);
                const auto dz=s.sub(cz,origin[2]);
                auto d2=s.mul(dx,dx);
                d2=s.add(d2,s.mul(dz,dz));
                d2=s.add(d2,s.mul(dy,dy));
                if(X87::eps_above_abs(fp,d2,p.r32(0x46a0ecu)))continue;
                if(!s.above(p.r32(0x46a620u),d2))continue;
                const auto facing=p.r8(request+0x5cu);
                const auto fz=p.r32(body+0x78u),fy=p.r32(body+0x74u),fx=p.r32(body+0x70u);
                const auto dot=x87_projection(fp,dz,fz,dy,fy,dx,fx,d2);
                bool test=facing!=0;
                if(!test) {
                    const auto ax=p.r32(body+0x88u),ay=p.r32(body+0x8cu),az=p.r32(body+0x90u);
                    auto speed=s.mul(ax,ax);
                    speed=s.add(speed,s.mul(ay,ay));
                    speed=s.add(speed,s.mul(az,az));
                    test=s.above(speed,p.r32(0x46a748u));
                }
                if(test && s.above(p.r32(0x46a1a0u),d2) && s.above(p.r32(0x46a794u),dot)) {
                    p.w8(0x268061u,c+0x4du,1);
                    if(!p.r8(request+0x14u)){p.w8(0x26806cu,c+0x4cu,0);continue;}
                }
                auto weight=p.r32(request+0x60u);
                if(s.above(weight,0u))weight=s.mul(weight,p.r32(0x46a0ccu));
                else weight=p.r32(0x46a100u);
                if(s.above(0u,dot)) {
                    auto behind=s.mul(weight,dot);
                    behind=s.mul(behind,p.r32(0x46a0dcu));
                    weight=s.sub(weight,behind);
                } else if(s.above(dot,near)) {
                    auto ahead=s.sub(dot,near);
                    ahead=s.mul(ahead,p.r32(0x46a7e0u));
                    ahead=s.add(ahead,p.r32(0x46a0d0u));
                    weight=s.mul(ahead,weight);
                }
                p.w32(0x2680d0u,c+0x54u,s.add(weight,p.r32(c+0x54u)));
            }
        }
    }
    // 2680DF: +692 prefers one goal datum.
    if(p.r8(request+0x692u) && count>0) {
        for(i32 n=0;n<count;++n) {
            const auto c=candidates+u32(n)*0x78u;
            const auto goal=p.r32(c+4u);
            if(goal!=p.r32(request+0x694u))continue;
            if(!p.r8(c+0x4cu))continue;
            const auto score=p.r32(c+0x54u);
            p.w32(0x26811eu,c+0x54u,s.add(score,p.r32(0x46a110u)));
        }
    }
}

// 2689A0: path-cost falloff and the +68C firing-kind restriction.
void Scoring::distance(u32 request,u32 count_word,u32 candidates) {
    const auto radius=p.r32(request+0x18u);
    const bool positive=s.above(radius,0u);
    const auto count=sx16(count_word);
    if(positive && count>0) {
        const auto one=p.r32(0x46a0d0u),scale=p.r32(0x46a128u);
        for(i32 n=0;n<count;++n) {
            const auto c=candidates+u32(n)*0x78u;
            if(!p.r8(c+0x4cu))continue;
            const auto cost=p.r32(c+0x18u);
            const auto ratio=s.div(cost,p.r32(request+0x18u));
            auto value=s.mul(s.sub(one,ratio),scale);
            if(s.above(0u,value))value=0u;
            p.w32(0x268a00u,c+0x54u,s.add(value,p.r32(c+0x54u)));
        }
    }
    if(p.r8(request+0x68cu) && count>0) {
        for(i32 n=0;n<count;++n) {
            const auto c=candidates+u32(n)*0x78u;
            if(!p.r8(c+0x4cu))continue;
            const auto kind=p.r16(c+6u);
            bool mark=(kind&0x8000u)!=0;
            if(!mark)mark=kind!=p.r16(request+0x68eu);
            if(!mark) {
                const auto record=p.r32(c);
                const auto kind_of_record=p.r16(record+0x10u);
                mark=kind_of_record!=p.r16(request+0x690u);
            }
            if(mark) {
                p.w8(0x268a47u,c+0x4du,1);
                if(!p.r8(request+0x14u))p.w8(0x268a52u,c+0x4cu,0);
            }
        }
    }
}

// 269D70: the 45681C table, one callback per entry whose type mask holds
// the request's type bit.
void Scoring::table(u32 actor,u32 request,u32 count,u32 candidates) {
    if(!p.r32(0x45681cu))return;
    for(u32 entry=0x45681cu;;entry+=8u) {
        const auto type=p.r8(request);
        const auto mask=u32(sx16(p.r16(entry-4u)));
        if(mask&(1u<<(type&31u))) {
            const auto callback=p.r32(entry);
            if(callback==0x267a60u)history(actor,request,count,candidates);
            else if(callback==0x2689a0u)distance(request,count,candidates);
            else throw NativeCampaignAiPerformRequiredOwnerTu15(0x269d9du,callback);
        }
        if(!p.r32(entry+8u))return;
    }
}

// 269870: the visibility evaluator. With a candidate whose visibility the
// request requires, 2A7D60 (line of sight) is a named refusal.
bool Scoring::evaluate(u32 actor,u32 request,u32 candidate,u32 site) {
    (void)site;
    const auto row=actor_row(p,actor);
    bool test=p.r8(request+0x56u)!=0;
    bool penalize=false;
    if(!test) {
        if(p.r8(request+0x5au)) {
            if(!candidate)penalize=true;
            else test=p.r8(candidate+0x58u)!=0;
        }
        if(!test && !penalize) {
            if(p.r8(request+0x58u)) {
                if(!candidate)penalize=true;
                else test=p.r8(candidate+0x59u)!=0;
            }
        }
    }
    if(test && !candidate)penalize=true;
    if(penalize) {
        const auto value=p.r32(request+0x680u);
        p.w32(0x2698d3u,request+0x680u,s.add(value,p.r32(0x46a108u)));
    } else if(test && p.r8(candidate+0x4cu)) {
        (void)p.r32(candidate+4u);(void)p.r32(row+0x26cu);(void)p.r32(candidate);
        throw NativeCampaignAiPerformRequiredOwnerTu15(0x2698ffu,0x2a7d60u);
    }
    return candidate?p.r8(candidate+0x4cu)!=0:true;
}

// 269DB0 / 26ACCE: the 45687C evaluators; AL threads through entries whose
// mask does not match.
bool Scoring::evaluators(u32 actor,u32 request,u32 candidate,bool request_loop) {
    bool result=true;
    for(u32 entry=0x45687cu;;entry+=8u) {
        const auto callback=p.r32(entry);
        if(!callback)return result;
        const auto type=p.r8(request);
        const auto mask=u32(sx16(p.r16(entry-4u)));
        if(mask&(1u<<(type&31u))) {
            if(callback!=0x269870u)throw NativeCampaignAiPerformRequiredOwnerTu15(request_loop?0x26acf6u:0x269ddeu,callback);
            result=evaluate(actor,request,candidate,request_loop?0x26acf6u:0x269ddeu);
        }
        if(!result)return result;
    }
}

// 269DF0 (a, b, context; RET C): candidates at [52D878], 0x78 bytes each.
bool Scoring::compare(u32 a,u32 b) {
    const auto base=p.r32(0x52d878u);
    const auto first=a*0x78u+base,second=b*0x78u+base;
    const auto valid=p.r8(first+0x4cu);
    if(valid!=p.r8(second+0x4cu))return valid==0;
    const auto marked=p.r8(first+0x4du);
    if(marked!=p.r8(second+0x4du))return marked!=0;
    const auto left=p.r32(first+0x54u);
    if(s.above(left,p.r32(second+0x54u)))return false;
    const auto right=p.r32(second+0x54u);
    return s.above(right,p.r32(first+0x54u));
}

// 140BC0: selection sort of [low, high] (inclusive dword addresses).
void Scoring::selection(u32 low,u32 high) {
    if(high<=low)return;
    do {
        u32 best=low;
        for(u32 at=low+4u;at<=high;at+=4u) {
            const auto held=p.r32(best);
            const auto next=p.r32(at);
            if(compare(next,held))best=at;
        }
        const auto last=p.r32(high);
        const auto chosen=p.r32(best);
        p.w32(0x140bffu,best,last);p.w32(0x140c01u,high,chosen);
        high-=4u;
    } while(high>low);
}

// 140910: median-swap quicksort with an explicit 30-entry stack.
void Scoring::sort(u32 base,u32 count) {
    if(count<2u)return;
    std::array<u32,30> lows{},highs{};i32 depth=0;
    u32 low=base,high=base+count*4u-4u;
    const auto pop=[&]{
        if(--depth<0)return false;
        low=lows[std::size_t(depth)];high=highs[std::size_t(depth)];return true;
    };
    const auto push=[&](u32 a,u32 b) {
        if(depth>=30)throw std::runtime_error("Native 140910 exceeds its original 30-entry partition stack");
        lows[std::size_t(depth)]=a;highs[std::size_t(depth)]=b;++depth;
    };
    for(;;) {
        const auto size=u32(i32(high-low)>>2)+1u;
        if(size<=8u) {
            selection(low,high);
            if(!pop())return;
            continue;
        }
        {
            const auto first=p.r32(low);
            const auto middle=p.r32(low+(size>>1)*4u);
            p.w32(0x14097cu,low+(size>>1)*4u,first);p.w32(0x14097eu,low,middle);
        }
        u32 left=low,right=high+4u;
        for(;;) {
            for(;;) {
                left+=4u;
                if(left>high)break;
                const auto pivot=p.r32(low);
                const auto value=p.r32(left);
                if(compare(value,pivot))break;
            }
            for(;;) {
                right-=4u;
                if(right<=low)break;
                const auto value=p.r32(right);
                const auto pivot=p.r32(low);
                if(compare(pivot,value))break;
            }
            if(right<left)break;
            const auto a=p.r32(right);
            const auto b=p.r32(left);
            p.w32(0x1409cdu,left,a);p.w32(0x1409cfu,right,b);
        }
        {
            const auto pivot=p.r32(low);
            const auto value=p.r32(right);
            p.w32(0x1409d7u,low,value);p.w32(0x1409d9u,right,pivot);
        }
        const auto lower=i32((right-low-4u)&~3u),upper=i32((high-left)&~3u);
        if(lower<upper) {
            if(left<high)push(left,high);
            if(low+4u<right){high=right-4u;continue;}
        } else {
            if(low+4u<right)push(low,right-4u);
            if(left<high){low=left;continue;}
        }
        if(!pop())return;
    }
}

// 269E70: one random candidate, scored and evaluated alone.
bool Scoring::admit(u32 actor,u32 request,u32 candidate) {
    const auto bsp=p.r32(0x4ecc18u);
    const auto count=i32(p.r32(bsp+0xc4u));
    const auto clusters=count>0?p.r32(bsp+0xc8u):0u;
    p.w32(0x269e93u,candidate+0x54u,0);p.w32(0x269e98u,candidate+0x50u,0);
    p.w8(0x269e9du,candidate+0x4cu,1);p.w8(0x269ea1u,candidate+0x4du,0);
    if(!p.r8(request+0x56u)) {
        bool valid=false;
        if(clusters) {
            const auto record=p.r32(candidate);
            const auto cluster=i32(p.r32(record+0x14u));
            valid=cluster>=0 && cluster<i32(p.r32(clusters));
        }
        if(!valid){p.w8(0x269ebau,candidate+0x4cu,0);p.w8(0x269ebdu,candidate+0x4du,1);}
    }
    if(p.r8(candidate+0x4cu)) {
        table(actor,request,1,candidate);
        if(p.r8(candidate+0x4cu)) {
            if(p.r8(request+0x618u))throw NativeCampaignAiPerformRequiredOwnerTu15(0x269ee2u,0x2694c0u);
            const auto score=p.r32(candidate+0x54u);
            p.w32(0x269eebu,candidate+0x50u,score);
            p.w8(0x269ef3u,candidate+0x4cu,evaluators(actor,request,candidate,false));
        }
    }
    return p.r8(candidate+0x4cu)!=0;
}

u32 stack_word(const SourcePhysicsContactMemory &memory,u32 address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);u32 value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);
    return value;
}
}

bool native_campaign_ai_goal_history_contains_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor,std::uint32_t goal) {
    static const SourcePhysicsContactRecordObserver none;
    return history_contains(Port{memory,none},actor,goal);
}
std::uint32_t native_campaign_ai_segment_distance_tu15(SourcePhysicsContactMemory &memory,std::uint32_t vector,
    std::uint32_t origin,std::uint32_t point,NativeTu15PhysicsFrameFp &fp) {
    static const SourcePhysicsContactRecordObserver none;const Port port{memory,none};const Simd simd(fp);
    return segment_distance(port,simd,Vector{&port,vector,{}},origin,point);
}
void native_campaign_ai_score_candidates_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor,
    std::uint32_t request,std::uint32_t count,std::uint32_t candidates,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    const Port port{memory,observe};const Simd simd(fp);Scoring scoring{port,simd,fp};
    scoring.table(actor,request,count,candidates);
}
bool native_campaign_ai_evaluate_candidate_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor,
    std::uint32_t request,std::uint32_t candidate,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe,bool request_loop) {
    const Port port{memory,observe};const Simd simd(fp);Scoring scoring{port,simd,fp};
    return scoring.evaluators(actor,request,candidate,request_loop);
}
void native_campaign_ai_sort_candidates_tu15(SourcePhysicsContactMemory &memory,std::uint32_t array,
    std::uint32_t count,std::uint32_t comparator,std::uint32_t context,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    (void)context;
    const Port port{memory,observe};const Simd simd(fp);Scoring scoring{port,simd,fp};
    if(comparator!=0x269df0u && count>=2u)throw NativeCampaignAiPerformRequiredOwnerTu15(count>8u?0x14099au:0x140bebu,comparator);
    scoring.sort(array,count);
}
bool native_campaign_ai_admit_candidate_tu15(SourcePhysicsContactMemory &memory,std::uint32_t actor,
    std::uint32_t request,std::uint32_t candidate,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    const Port port{memory,observe};const Simd simd(fp);Scoring scoring{port,simd,fp};
    return scoring.admit(actor,request,candidate);
}
void native_campaign_ai_goal_scoring_call_tu15(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>4u)
        throw std::runtime_error("Native 26A380 scoring requires retained FP controls and four free x87 temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication{SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}} publication{abi,fp};
    const auto sp=abi.gp[4];
    const auto arg=[&](unsigned n){return stack_word(memory,sp+4u*n);};
    const auto al=[&](bool value){abi.gp[0]=(abi.gp[0]&0xffffff00u)|u32(value);};
    const Port port{memory,observe};const Simd simd(fp);Scoring scoring{port,simd,fp};
    switch(entry) {
    case 0x26c5a0u:al(history_contains(port,abi.gp[0],arg(1)));abi.gp[4]=sp+8u;break;
    case 0x1209c0u:abi.xmm[0][0]=segment_distance(port,simd,Vector{&port,abi.gp[0],{}},abi.gp[1],abi.gp[2]);abi.gp[4]=sp+4u;break;
    case 0x269d70u:scoring.table(arg(1),abi.gp[7],arg(2),abi.gp[3]);abi.gp[0]=0;abi.gp[4]=sp+12u;break;
    case 0x267a60u:scoring.history(arg(1),arg(2),arg(3),arg(4));abi.gp[4]=sp+0x14u;break;
    case 0x2689a0u:scoring.distance(arg(2),arg(3),arg(4));abi.gp[4]=sp+0x14u;break;
    case 0x269870u:al(scoring.evaluate(arg(1),arg(2),arg(3),0));abi.gp[4]=sp+0x10u;break;
    case 0x269db0u:al(scoring.evaluators(arg(1),abi.gp[7],abi.gp[3],false));abi.gp[4]=sp+8u;break;
    case 0x140910u:
        if(arg(2)!=0x269df0u && abi.gp[0]>=2u)throw NativeCampaignAiPerformRequiredOwnerTu15(abi.gp[0]>8u?0x14099au:0x140bebu,arg(2));
        scoring.sort(abi.gp[1],abi.gp[0]);abi.gp[4]=sp+0x10u;break;
    case 0x269e70u:al(scoring.admit(abi.gp[6],abi.gp[1],abi.gp[0]));abi.gp[4]=sp+4u;break;
    default:throw std::invalid_argument("Native 26A380 scoring has no owner at this entry");
    }
}
#else
// Exact SSE/x87 replication requires an x86 host.
bool native_campaign_ai_goal_history_contains_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t)
    {throw std::runtime_error("Exact native 26C5A0 backend unavailable");}
std::uint32_t native_campaign_ai_segment_distance_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,
    std::uint32_t,NativeTu15PhysicsFrameFp &){throw std::runtime_error("Exact native 1209C0 backend unavailable");}
void native_campaign_ai_score_candidates_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,std::uint32_t,
    std::uint32_t,NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &)
    {throw std::runtime_error("Exact native 269D70 backend unavailable");}
bool native_campaign_ai_evaluate_candidate_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,std::uint32_t,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &,bool)
    {throw std::runtime_error("Exact native 269DB0 backend unavailable");}
void native_campaign_ai_sort_candidates_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,std::uint32_t,
    std::uint32_t,NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &)
    {throw std::runtime_error("Exact native 140910 backend unavailable");}
bool native_campaign_ai_admit_candidate_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,std::uint32_t,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &)
    {throw std::runtime_error("Exact native 269E70 backend unavailable");}
void native_campaign_ai_goal_scoring_call_tu15(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &){throw std::runtime_error("Exact native 26A380 scoring backend unavailable");}
#endif
}
