// Native kernels: listener selection routines (TU1.5 sound manager).
//
// A sound source record (the request the sound manager plays) keeps its flags
// word at +0 (bit 0 of +0: never cut off by clusters; bits 8 and 11: no
// distance limit), a signed byte at +2 (nonzero: listeners are measured in a
// vertical column from the source), its spatial mode in the low nibble of +3
// (0 none, 1 positional, others absolute), its position at +0xC/+0x10/+0x14,
// its location at +0x28, its BSP cluster at +0x2C (word, -1 for none) and the
// column height at +0x30.
// The listener globals keep four 0x48-byte listener rows from +0x88: location
// +0, cluster +4 (word, -1 for none), active byte +6, position +0x30/+0x34/
// +0x38. The structure's cluster tables are strictly triangular: the byte for
// the cluster pair (low < high) of N clusters (+0x9C) is at
// table + (N-1)*low - low*(low+1)/2 + high - 1.
#include "audio/native_tu15_sound_kernel.hpp"

namespace halo2::native_tu15_sound {
namespace {
// Retail globals (game runtime data).
constexpr std::uint32_t listener_globals=0x4f2c50u;  // +0x88: four 0x48-byte listener rows
constexpr std::uint32_t local_players=0x4f54f0u;     // +0xC: four local player slots, -1 for none
constexpr std::uint32_t structure_bsp=0x4ecc18u;     // +0x9C clusters, +0xE8 cluster bytes,
                                                     // +0x224 scale count, +0x228 scale table
// Retail .rdata constants (TU1.5 XBE).
constexpr std::uint32_t flt_max=0x7f7fffffu;         // 0x45695C: FLT_MAX
constexpr std::uint32_t scale_preload=0x437f0000u;   // 0x46A148: 255.0 (loaded, then always replaced)
constexpr std::uint32_t full_scale=0x43800000u;      // 0x46A26C: 256.0
constexpr std::uint32_t step_scale=0x3b81848eu;      // 0x46A2D8: 1/253
constexpr std::uint32_t byte_scale=0x40010204u;      // 0x46A2D4: 256/127

constexpr std::uint32_t none=0xffffffffu;
__m128 single(std::uint32_t bits){return _mm_set_ss(std::bit_cast<float>(bits));}
std::uint32_t low_byte(std::uint32_t reg,std::uint32_t value){return (reg&~0xffu)|(value&0xffu);}
std::uint32_t sign16(std::uint32_t value){return std::uint32_t(std::int32_t(std::int16_t(value)));}

// COMISS/UCOMISS x,y: the flags the compiled branches test, raising exactly
// the original instruction's MXCSR exceptions.
struct Compared {bool cf,zf,pf;};
Compared compare(__m128 x,__m128 y,bool unordered) {
    unsigned char carry,zero,parity;
    if(unordered)asm volatile("ucomiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(x),"x"(y):"cc");
    else asm volatile("comiss %4,%3\n\tsetb %0\n\tsete %1\n\tsetp %2"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(x),"x"(y):"cc");
    return {carry!=0,zero!=0,parity!=0};
}
// COMISS x,y; JA: x > y, ordered.
bool above(__m128 x,__m128 y){const auto f=compare(x,y,false);return !f.cf && !f.zf;}

// An x87 m32 operand loaded as its own FLD, as the reference routines do.
long double load_single(float x){volatile long double v=static_cast<long double>(x);return v;}
// The original's x87 loads need `slots` free registers. A caller stack that
// would overflow is refused exactly where the reference routine refuses it
// (no game caller has one).
void require_x87_room(const Abi &a,unsigned slots) {
    if(a.x87_depth+slots>8u)throw FormatError("Required original solver x87 stack-fault owner unavailable");
}

// The triangular cluster-pair row: (clusters-1)*low - low*(low+1)/2, in the
// original's 32-bit IMUL/CDQ/SAR arithmetic (triangle is the second term).
std::uint32_t pair_row(std::uint32_t low,std::uint32_t clusters,std::uint32_t &triangle) {
    triangle=(low+1u)*low;
    triangle-=std::int32_t(triangle)<0?none:0u;          // CDQ; SUB EAX,EDX
    triangle=std::uint32_t(std::int32_t(triangle)>>1);   // SAR EAX,1
    return (clusters-1u)*low-triangle;                   // DEC EDX; IMUL EDX,low; SUB EDX,EAX
}

// 2531E0: the structure's cluster byte (+0xE8 table) for the clusters in AX
// and CX; EDI: structure. Equal clusters give AL 0. Otherwise EAX holds the
// higher cluster (sign-extended) with the byte in AL, ECX the lower cluster
// and EDX the pair's row in the table.
void cluster_byte(const M &m,Abi &a,std::uint32_t structure) {
    const auto first=a.gp[0],second=a.gp[1];
    if(std::uint16_t(first)==std::uint16_t(second)){a.gp[0]=low_byte(a.gp[0],0u);return;}
    const bool swap=std::int16_t(first)>std::int16_t(second);
    const auto low=sign16(swap?second:first),high=sign16(swap?first:second);
    a.gp[1]=low;
    std::uint32_t triangle;
    auto row=pair_row(low,u32(m,structure+0x9cu),triangle);
    row+=u32(m,structure+0xe8u);
    a.gp[2]=row;
    a.gp[0]=low_byte(high,u8(m,row+high-1u));
}

// 253070: the cluster-pair distance scale (0 to 256) between the clusters in
// EAX and ECX; EDX: structure. Equal clusters scale 0. A structure with
// scale rows (+0x224 > 0) reads the pair's byte from the scale table's +0x28
// bytes: 0 scales 0, 0xFF the full 256, others interpolate the table's
// +4..+8 range by (byte-1)/253. Otherwise the structure's cluster byte
// (2531E0), without bit 7, scales by 256/127. Returns XMM0.
void cluster_scale(const M &m,Abi &a) {
    const auto first=a.gp[0],second=a.gp[1],structure=a.gp[2];
    const auto zero=_mm_setzero_ps();
    set_xmm(a,1,zero);set_xmm(a,0,single(scale_preload));
    if(first==second){set_xmm(a,0,zero);return;}
    const auto rows=u32(m,structure+0x224u);
    a.gp[2]=rows;
    if(std::int32_t(rows)<=0) {
        cluster_byte(m,a,structure);
        a.gp[1]=(a.gp[0]&0xffu)&0xffffff7fu;
        auto scale=_mm_cvtsi32_ss(xmm(a,0),std::int32_t(a.gp[1]));
        scale=_mm_mul_ss(scale,single(byte_scale));
        set_xmm(a,0,scale);
        return;
    }
    const auto low=std::int32_t(first)<=std::int32_t(second)?first:second;   // CMOVLE
    const auto high=std::int32_t(first)>std::int32_t(second)?first:second;   // CMOVG
    a.gp[1]=high;
    const auto table=u32(m,structure+0x228u);
    std::uint32_t triangle;
    auto row=pair_row(low,u32(m,structure+0x9cu),triangle);
    row+=u32(m,table+0x28u);
    a.gp[2]=row;
    const auto cell=u8(m,row+high-1u);
    a.gp[0]=low_byte(triangle,cell);
    if(cell==0u){set_xmm(a,0,zero);return;}
    if(cell==0xffu){set_xmm(a,0,single(full_scale));return;}
    const auto lower=single(u32(m,table+4u));
    const auto upper=single(u32(m,table+8u));
    a.gp[0]=cell-1u;
    auto scale=_mm_cvtsi32_ss(xmm(a,0),std::int32_t(a.gp[0]));
    scale=_mm_mul_ss(scale,single(step_scale));
    const auto span=_mm_sub_ss(upper,lower);
    scale=_mm_mul_ss(scale,span);
    scale=_mm_add_ss(scale,lower);
    set_xmm(a,0,scale);set_xmm(a,1,lower);set_xmm(a,2,span);
}

// 2287B0: whether the source (EDX) is cut off from the listener (ECX, -1 for
// none); stack: the source's distance over its limit (ret 4). Returns AL: 1
// with no listener, 0 for sources that ignore clusters (+0 bit 0) or are not
// positional; 1 when the source's or the listener's cluster is unknown; 0 in
// the same location or cluster; otherwise 1 when the cluster scale times the
// distance ratio reaches 256 (any NaN cuts off).
template<class Ratio> void listener_cut_off(const M &m,Abi &a,Ratio ratio) {
    const auto listener=a.gp[1],source=a.gp[2];
    a.gp[0]=low_byte(a.gp[0],listener==none);   // CMP ECX,-1; SETE AL
    if(listener==none)return;
    if(u8(m,source)&1u)return;
    if((u8(m,source+3u)&0xfu)!=1u)return;
    a.gp[0]=listener*9u;
    const auto row=u32(m,listener_globals)+a.gp[0]*8u+0x88u;
    a.gp[1]=row;
    const auto source_cluster=u16(m,source+0x2cu);
    a.gp[0]=(a.gp[0]&0xffff0000u)|source_cluster;
    if(source_cluster==0xffffu){a.gp[0]=low_byte(a.gp[0],1u);return;}
    const auto listener_cluster=u16(m,row+4u);
    if(listener_cluster==0xffffu){a.gp[0]=low_byte(a.gp[0],1u);return;}
    a.gp[1]=u32(m,row);
    if(a.gp[1]==u32(m,source+0x28u) || listener_cluster==source_cluster){a.gp[0]=low_byte(a.gp[0],0u);return;}
    a.gp[2]=u32(m,structure_bsp);
    a.gp[1]=sign16(source_cluster);a.gp[0]=sign16(listener_cluster);
    cluster_scale(m,a);
    const auto scaled=_mm_mul_ss(xmm(a,0),single(ratio()));
    const auto full=single(full_scale);
    set_xmm(a,0,scaled);set_xmm(a,1,full);
    a.gp[0]=low_byte(a.gp[0],!above(full,scaled));
}

// 12A930: a source's (ECX) distance measure to a listener (EDX index), in
// XMM0. Mode 0: 0. Positional (1): the squared distance to the listener; a
// source measured in a vertical column (+2 nonzero) gives the squared
// horizontal distance while the listener's height above the source is
// within [0, +0x30], else FLT_MAX. Other modes: the length of the source's
// position (x87). ECX returns the pushed slot, which the x87 path overwrites
// with its result.
void listener_distance(const M &m,Abi &a,Floating &floating) {
    const auto source=a.gp[1];
    const auto mode=std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(u8(m,source+3u)<<4)))>>4);
    a.gp[0]=mode;
    if(mode==0u){set_xmm(a,0,_mm_setzero_ps());return;}
    if(mode!=1u) {
        a.gp[0]=mode-1u;
        // FLD z, y, x; x*x + y*y + z*z; FSQRT; FSTP m32 (five registers at most).
        const auto z=load_single(f32(m,source+0x14u));
        require_x87_room(a,1);
        const auto y=load_single(f32(m,source+0x10u));
        require_x87_room(a,2);
        const auto x=load_single(f32(m,source+0xcu));
        require_x87_room(a,5);
        volatile long double xx=x*x;
        const volatile long double yy=y*y;
        xx=xx+yy;
        const volatile long double zz=z*z;
        xx=xx+zz;
        const auto length=floating.store_single(root(xx));
        set_xmm(a,0,_mm_set_ss(length));a.gp[1]=std::bit_cast<std::uint32_t>(length);
        return;
    }
    const auto listeners=u32(m,listener_globals);
    const auto slot=a.gp[2]*9u;
    const auto listener_z=single(u32(m,listeners+slot*8u+0xc0u));
    const auto dz=_mm_sub_ss(listener_z,single(u32(m,source+0x14u)));
    const auto row=listeners+slot*8u+0x88u;
    a.gp[2]=row;
    const auto column=s8(m,source+2u);
    a.gp[0]=column;
    if(column==0u) {
        const auto listener_y=single(u32(m,row+0x34u));
        const auto dy=_mm_sub_ss(listener_y,single(u32(m,source+0x10u)));
        const auto listener_x=single(u32(m,row+0x30u));
        const auto dx=_mm_sub_ss(listener_x,single(u32(m,source+0xcu)));
        const auto dy2=_mm_mul_ss(dy,dy);
        auto sum=_mm_mul_ss(dz,dz);
        sum=_mm_add_ss(sum,dy2);
        const auto dx2=_mm_mul_ss(dx,dx);
        sum=_mm_add_ss(sum,dx2);
        set_xmm(a,0,sum);set_xmm(a,1,dx);set_xmm(a,2,dx2);set_xmm(a,3,dy2);
        return;
    }
    auto clamped=_mm_setzero_ps();
    if(!above(clamped,dz)) {
        const auto column_height=single(u32(m,source+0x30u));   // COMISS m32, then MOVSS re-reads it
        clamped=above(dz,column_height)?single(u32(m,source+0x30u)):dz;
    }
    // UCOMISS; LAHF; TEST AH,44h; JP: AH keeps the comparison's flags.
    const auto f=compare(clamped,dz,true);
    const auto ah=std::uint32_t(f.cf)|2u|(std::uint32_t(f.pf)<<2)|(std::uint32_t(f.zf)<<6);
    a.gp[0]=(a.gp[0]&0xffff00ffu)|(ah<<8);
    set_xmm(a,1,clamped);
    if(!f.zf || f.pf){set_xmm(a,0,single(flt_max));return;}
    const auto source_y=single(u32(m,source+0x10u));
    const auto dy=_mm_sub_ss(source_y,single(u32(m,row+0x34u)));
    const auto source_x=single(u32(m,source+0xcu));
    auto sum=_mm_sub_ss(source_x,single(u32(m,row+0x30u)));
    sum=_mm_mul_ss(sum,sum);
    const auto dy2=_mm_mul_ss(dy,dy);
    sum=_mm_add_ss(sum,dy2);
    set_xmm(a,0,sum);set_xmm(a,1,dy);set_xmm(a,2,dy2);
}

// The first occupied local player slot (index), or -1; ECX ends at the slot
// the scan stopped on.
std::uint32_t first_local_player(const M &m,Abi &a) {
    auto slot=u32(m,local_players)+0xcu;
    for(std::uint32_t i=0;i<4u;++i,slot+=4u)if(u32(m,slot)!=none){a.gp[1]=slot;return i;}
    a.gp[1]=slot;return none;
}

// 12A810: the listener a source plays to. Stack: source, distance limit,
// optional result pointer (ret 0Ch). Non-positional sources take the first
// local player, distance 0. Positional sources take the active listener with
// the least distance measure (12A930); the distance is its square root. A
// listener cut off by clusters (2287B0, with distance/limit) is dropped. A
// source without a distance limit (+0 bit 8 or 11) then falls back to the
// first local player; otherwise a distance beyond the limit selects none,
// distance FLT_MAX. Returns the listener (-1 for none) in EAX and the
// distance in XMM0, also stored through the result pointer when not null.
void select_listener(M &m,Abi &a,FP fp,const O &o,const Services &) {
    Floating floating(a,fp);
    const auto entry=a.gp[4];
    const auto source=argument(m,a,0);
    std::uint32_t chosen=none;
    __m128 result;
    if((u8(m,source+3u)&0xfu)!=1u) {
        chosen=first_local_player(m,a);
        result=_mm_setzero_ps();
    } else {
        const auto ceiling=single(flt_max);
        set_xmm(a,5,ceiling);
        auto best=ceiling;
        auto active=u32(m,listener_globals)+0x8eu;
        for(std::uint32_t i=0;i<4u;++i,active+=0x48u) {
            if(!u8(m,active))continue;
            a.gp[1]=source;a.gp[2]=i;
            listener_distance(m,a,floating);
            const auto measure=xmm(a,0);
            if(above(best,measure)){chosen=i;best=measure;}
        }
        set_xmm(a,4,best);
        // FLD m32; FSQRT; FST m32 (the distance); FDIV m32 (the limit); FSTP m32.
        const auto loaded=load_single(_mm_cvtss_f32(best));
        require_x87_room(a,1);
        floating.push(loaded);
        const long double length=root(loaded);
        const auto distance=floating.store_single(length);
        const volatile long double ratio=length/load_single(f32(m,entry+8u));
        const auto ratio_single=floating.store_single(ratio);
        floating.pop();
        a.gp[1]=chosen;a.gp[2]=source;
        listener_cut_off(m,a,[&]{return std::bit_cast<std::uint32_t>(ratio_single);});
        if(std::uint8_t(a.gp[0]))chosen=none;
        const auto flags=u16(m,source);
        result=_mm_set_ss(distance);
        if(!(flags&0x100u) && !(flags&0x800u)) {
            const auto limit=single(u32(m,entry+8u));
            const auto squared=_mm_mul_ss(limit,limit);
            set_xmm(a,1,squared);
            if(above(best,squared)){chosen=none;result=ceiling;}
        } else if(chosen==none)chosen=first_local_player(m,a);
    }
    set_xmm(a,0,result);
    if(const auto out=u32(m,entry+0xcu))store(m,o,0x12a920u,out,std::bit_cast<std::uint32_t>(_mm_cvtss_f32(result)));
    a.gp[0]=chosen;
    pop_return(m,a,12);
}

void distance_entry(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);
    listener_distance(m,a,floating);
    pop_return(m,a);
}
void cut_off_entry(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);
    const auto entry=a.gp[4];
    listener_cut_off(m,a,[&]{return u32(m,entry+4u);});
    pop_return(m,a,4);
}
void cluster_scale_entry(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);
    cluster_scale(m,a);
    pop_return(m,a);
}
void cluster_byte_entry(M &m,Abi &a,FP fp,const O &,const Services &) {
    Floating floating(a,fp);
    cluster_byte(m,a,a.gp[7]);
    pop_return(m,a);
}

// Every register is reproduced exactly (the full default contract).
constexpr Kernel kernels[]{
    {0x12a810u,select_listener},       // EAX listener, XMM0 distance; ret 0Ch
    {0x12a930u,distance_entry},        // XMM0 distance measure
    {0x2287b0u,cut_off_entry},         // AL cut off; ret 4
    {0x253070u,cluster_scale_entry},   // XMM0 cluster scale (also called by the object-loop group)
    {0x2531e0u,cluster_byte_entry},    // AL cluster byte
};
} // namespace
std::span<const Kernel> listener_kernels(){return kernels;}
} // namespace halo2::native_tu15_sound
