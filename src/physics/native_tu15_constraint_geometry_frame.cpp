#include "halo2/native_tu15_constraint_geometry_frame.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){_mm_setcsr(fp.mxcsr);}
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char value;asm volatile("comiss %2,%1; seta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
}
void native_tu15_constraint_geometry_frame(SourcePhysicsContactMemory&m,std::uint32_t instance,
    std::uint32_t datum,std::uint32_t first_slot,NativeTu15ConstraintGeometryOutputs out,bool flag,
    NativeTu15ConstraintGeometryScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintGeometryStats*stats){
    if(stats)++stats->calls;
    if(depth>3u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
       (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
        if(stats)++stats->unsupported_profiles;throw std::runtime_error("Native TU1.5 constraint geometry requires its unsupported numerical-fault owner");}
    if(std::uint64_t(scratch.first_basis)+36u>0x100000000ull||std::uint64_t(scratch.second_basis)+36u>0x100000000ull||
       (std::uint64_t(scratch.first_basis)<std::uint64_t(scratch.second_basis)+36u&&
        std::uint64_t(scratch.second_basis)<std::uint64_t(scratch.first_basis)+36u))
        throw std::runtime_error("Native TU1.5 constraint geometry requires separate caller-owned basis DATA");
    store(m,observer,0x22e50au,out.normal,read(m,instance+0x5cu));
    store(m,observer,0x22e50fu,out.normal+4u,read(m,instance+0x60u));
    store(m,observer,0x22e515u,out.normal+8u,read(m,instance+0x64u));
    store(m,observer,0x22e51fu,out.position,read(m,instance+0x68u));
    store(m,observer,0x22e524u,out.position+4u,read(m,instance+0x6cu));
    store(m,observer,0x22e52du,out.position+8u,read(m,instance+0x70u));
    if(read(m,instance+0x9cu,2)!=0xffffu){
        const auto component_datum=read(m,instance+0x24u),pool=read(m,0x52be5cu),rows=read(m,pool+0x44u);
        const auto frame=native_tu15_model_constraint_body_frame(m,{rows+(component_datum&0xffffu)*160u},first_slot,fp,
            stats?&stats->frames:nullptr);
        store(m,observer,0x22e57au,out.position,frame.position[0]);
        store(m,observer,0x22e580u,out.position+4u,frame.position[1]);
        store(m,observer,0x22e587u,out.position+8u,frame.position[2]);
        store(m,observer,0x22e58eu,out.first_axis,frame.forward[0]);
        store(m,observer,0x22e595u,out.first_axis+4u,frame.forward[1]);
        store(m,observer,0x22e59cu,out.first_axis+8u,frame.forward[2]);
        store(m,observer,0x22e5a3u,out.normal,frame.up[0]);
        store(m,observer,0x22e5a9u,out.normal+4u,frame.up[1]);
        store(m,observer,0x22e5b0u,out.normal+8u,frame.up[2]);
        store(m,observer,0x22e5bau,out.second_axis,frame.side[0]);
        store(m,observer,0x22e5c0u,out.second_axis+4u,frame.side[1]);
        store(m,observer,0x22e5cau,out.second_axis+8u,frame.side[2]);
        store(m,observer,0x22e5d6u,out.first_extent,0);store(m,observer,0x22e5dau,out.second_extent,0);
        if(stats)++stats->retained_node_frames;return;
    }
    bool measured=std::bit_cast<std::int16_t>(std::uint16_t(read(m,instance+8u,2)))>1;
    if(!measured&&read(m,instance+0xeu,1)!=0)measured=flag;
    if(!measured){
        if(stats)++stats->simple_axes;auto fallback=read(m,0x474d10u);
        store(m,observer,0x22e61cu,out.first_extent,0);store(m,observer,0x22e620u,out.second_extent,0);
        const auto cross=[&](std::uint32_t axis,bool alternate){
            Environment environment(fp);
            auto z=scalar(read(m,axis+4u)),y=scalar(read(m,out.normal+4u));y=mul(y,scalar(read(m,axis)));
            z=mul(z,scalar(read(m,out.normal)));auto other=scalar(read(m,out.normal));other=mul(other,scalar(read(m,axis+8u)));
            auto x=scalar(read(m,axis+4u));x=mul(x,scalar(read(m,out.normal+8u)));z=sub(z,y);
            y=scalar(read(m,out.normal+8u));y=mul(y,scalar(read(m,axis)));y=sub(y,other);
            other=scalar(read(m,axis+8u));other=mul(other,scalar(read(m,out.normal+4u)));x=sub(other,x);
            store(m,observer,alternate?0x22e6d6u:0x22e66au,out.first_axis,bits(x));
            store(m,observer,alternate?0x22e6dbu:0x22e66fu,out.first_axis+4u,bits(y));
            store(m,observer,alternate?0x22e6e0u:0x22e674u,out.first_axis+8u,bits(z));
        };
        cross(fallback,false);auto length=native_tu15_normalize_constraint_axis(m,out.first_axis,fp,depth,observer,stats?&stats->normalize:nullptr);
        bool alternate{};{Environment environment(fp);alternate=greater(scalar(read(m,0x46a180u)),scalar(length));}
        if(alternate){fallback=read(m,0x474d08u);cross(fallback,true);
            native_tu15_normalize_constraint_axis(m,out.first_axis,fp,depth,observer,stats?&stats->normalize:nullptr);
            if(stats)++stats->alternate_axes;}
        {
            Environment environment(fp);
            auto z=scalar(read(m,out.normal)),y=scalar(read(m,out.first_axis));y=mul(y,scalar(read(m,out.normal+4u)));
            z=mul(z,scalar(read(m,out.first_axis+4u)));auto other=scalar(read(m,out.normal));other=mul(other,scalar(read(m,out.first_axis+8u)));
            auto x=scalar(read(m,out.first_axis+4u));x=mul(x,scalar(read(m,out.normal+8u)));z=sub(z,y);
            y=scalar(read(m,out.first_axis));y=mul(y,scalar(read(m,out.normal+8u)));y=sub(y,other);
            other=scalar(read(m,out.normal+4u));other=mul(other,scalar(read(m,out.first_axis+8u)));x=sub(other,x);
            store(m,observer,0x22e737u,out.second_axis,bits(x));store(m,observer,0x22e73bu,out.second_axis+4u,bits(y));
            store(m,observer,0x22e740u,out.second_axis+8u,bits(z));
        }
        native_tu15_normalize_constraint_axis(m,out.second_axis,fp,depth,observer,stats?&stats->normalize:nullptr);return;
    }
    if(stats)++stats->measured_bounds;
    const auto first=read(m,instance+0x24u);const auto high=scalar(read(m,0x45695cu)),low=scalar(read(m,0x44aac8u));
    const auto pool=read(m,0x52be5cu),components=read(m,pool+0x44u),component=components+(first&0xffffu)*160u;
    const bool paired=read(m,instance+0x28u)!=0xffffffffu;
    auto u_max=low,u_min=high,v_max=low,v_min=high,u2_max=low,u2_min=high,v2_max=low,v2_min=high;
    native_tu15_constraint_point_basis(m,first,instance,scratch.first_basis,fp,depth,observer,stats?&stats->bases:nullptr);
    if(paired){const auto second=read(m,instance+0x28u);
        native_tu15_constraint_point_basis(m,second,instance,scratch.second_basis,fp,depth,observer,stats?&stats->bases:nullptr);}
    Environment environment(fp);
    const auto count=std::bit_cast<std::int32_t>(read(m,component+0x8cu));
    if(count>0){
        const auto rows=read(m,component+0x88u);const auto first_x=scalar(read(m,scratch.first_basis+0xcu)),first_y=scalar(read(m,scratch.first_basis+0x10u));
        for(std::uint32_t n=0;n<std::uint32_t(count);++n){
            if(stats)++stats->local_rows;const auto row=rows+n*0x48u;
            if(read(m,row+8u)!=datum)continue;if(stats)++stats->matching_rows;
            auto dy=scalar(read(m,row+0x20u));dy=sub(dy,scalar(read(m,out.position+4u)));
            auto dz=scalar(read(m,row+0x24u));dz=sub(dz,scalar(read(m,out.position+8u)));
            auto u=scalar(read(m,scratch.first_basis+8u)),other=scalar(read(m,scratch.first_basis+4u));
            auto dx=scalar(read(m,row+0x1cu));dx=sub(dx,scalar(read(m,out.position)));
            other=mul(other,dy);u=mul(u,dz);u=add(u,other);other=scalar(read(m,scratch.first_basis));other=mul(other,dx);u=add(u,other);
            auto v=scalar(read(m,scratch.first_basis+0x14u));v=mul(v,dz);other=mul(first_y,dy);v=add(v,other);other=mul(first_x,dx);v=add(v,other);
            if(!greater(u_max,u))u_max=u;if(greater(u_min,u))u_min=u;if(!greater(v_max,v))v_max=v;if(greater(v_min,v))v_min=v;
            if(!paired)continue;
            u=scalar(read(m,scratch.second_basis+0x18u));other=scalar(read(m,scratch.second_basis+0xcu));other=mul(other,dy);u=mul(u,dz);u=add(u,other);
            other=scalar(read(m,scratch.second_basis));other=mul(other,dx);u=add(u,other);
            v=scalar(read(m,scratch.second_basis+0x1cu));v=mul(v,dz);other=scalar(read(m,scratch.second_basis+0x10u));other=mul(other,dy);
            auto last=scalar(read(m,scratch.second_basis+4u));last=mul(last,dx);
            const bool keep_max=greater(u2_max,u);v=add(v,other);v=add(v,last);if(!keep_max)u2_max=u;
            if(greater(u2_min,u))u2_min=u;if(!greater(v2_max,v))v2_max=v;if(greater(v2_min,v))v2_min=v;
        }
    }
    bool use_second=false;auto second_width=_mm_setzero_ps(),second_height=_mm_setzero_ps();
    if(paired){
        auto width=sub(u_max,u_min),height=sub(v_max,v_min);const auto area=mul(height,width);
        second_width=sub(u2_max,u2_min);second_height=sub(v2_max,v2_min);const auto second_area=mul(second_width,second_height);
        use_second=!greater(area,second_area);
    }
    if(use_second){
        if(stats)++stats->second_basis_selected;const auto half=scalar(read(m,0x46a0ccu));
        second_height=mul(second_height,half);auto u_center=add(u2_max,u2_min),v_center=add(v2_max,v2_min);second_width=mul(second_width,half);
        store(m,observer,0x22e9ebu,out.first_extent,bits(second_width));const auto x=read(m,scratch.second_basis);
        store(m,observer,0x22e9f3u,out.second_extent,bits(second_height));const auto y=read(m,scratch.second_basis+4u);
        store(m,observer,0x22e9fdu,out.first_axis,x);const auto z=read(m,scratch.second_basis+8u);
        store(m,observer,0x22ea03u,out.first_axis+4u,y);store(m,observer,0x22ea06u,out.first_axis+8u,z);
        store(m,observer,0x22ea16u,out.second_axis,read(m,scratch.second_basis+0xcu));
        store(m,observer,0x22ea1cu,out.second_axis+4u,read(m,scratch.second_basis+0x10u));
        store(m,observer,0x22ea23u,out.second_axis+8u,read(m,scratch.second_basis+0x14u));
        auto value=scalar(read(m,out.first_axis));u_center=mul(u_center,half);value=mul(value,u_center);value=add(value,scalar(read(m,out.position)));
        store(m,observer,0x22ea37u,out.position,bits(value));
        for(unsigned k=1;k<3;++k){value=scalar(read(m,out.first_axis+k*4u));value=mul(value,u_center);value=add(value,scalar(read(m,out.position+k*4u)));
            store(m,observer,k==1?0x22ea49u:0x22ea5cu,out.position+k*4u,bits(value));}
        value=scalar(read(m,out.second_axis));v_center=mul(v_center,half);value=mul(value,v_center);value=add(value,scalar(read(m,out.position)));
        store(m,observer,0x22ea71u,out.position,bits(value));
        for(unsigned k=1;k<3;++k){value=scalar(read(m,out.second_axis+k*4u));value=mul(value,v_center);value=add(value,scalar(read(m,out.position+k*4u)));
            store(m,observer,k==1?0x22ea83u:0x22ea98u,out.position+k*4u,bits(value));}
        return;
    }
    const auto half=scalar(read(m,0x46a0ccu));auto width=mul(sub(u_max,u_min),half);
    store(m,observer,0x22eacbu,out.first_extent,bits(width));const auto x=read(m,scratch.first_basis);
    const auto height=mul(sub(v_max,v_min),half);store(m,observer,0x22eae4u,out.second_extent,bits(height));const auto y=read(m,scratch.first_basis+4u);
    store(m,observer,0x22eaeeu,out.first_axis,x);const auto z=read(m,scratch.first_basis+8u);
    store(m,observer,0x22eaf4u,out.first_axis+4u,y);store(m,observer,0x22eaf7u,out.first_axis+8u,z);
    const auto side_x=read(m,scratch.first_basis+0xcu);const auto u_center=mul(add(u_max,u_min),half);
    store(m,observer,0x22eb0fu,out.second_axis,side_x);store(m,observer,0x22eb15u,out.second_axis+4u,read(m,scratch.first_basis+0x10u));
    store(m,observer,0x22eb1cu,out.second_axis+8u,read(m,scratch.first_basis+0x14u));
    __m128 cached_z{};
    for(unsigned k=0;k<3;++k){auto value=scalar(read(m,out.first_axis+k*4u));value=mul(value,u_center);value=add(value,scalar(read(m,out.position+k*4u)));
        store(m,observer,k==0?0x22eb2cu:k==1?0x22eb3eu:0x22eb51u,out.position+k*4u,bits(value));if(k==2)cached_z=value;}
    auto value=scalar(read(m,out.second_axis));const auto v_center=mul(add(v_max,v_min),half);value=mul(value,v_center);value=add(value,scalar(read(m,out.position)));
    store(m,observer,0x22eb6au,out.position,bits(value));value=scalar(read(m,out.second_axis+4u));value=mul(value,v_center);value=add(value,scalar(read(m,out.position+4u)));
    store(m,observer,0x22eb7cu,out.position+4u,bits(value));value=scalar(read(m,out.second_axis+8u));value=mul(value,v_center);value=add(value,cached_z);
    store(m,observer,0x22eb90u,out.position+8u,bits(value));
}
} // namespace halo2
