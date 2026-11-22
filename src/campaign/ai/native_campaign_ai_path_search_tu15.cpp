#include "halo2/native_campaign_ai_path_search_tu15.hpp"
#include "halo2/native_campaign_ai_frame_geometry_tu15.hpp"
#include "halo2/native_campaign_ai_jump_arc_tu15.hpp"
#include "halo2/native_campaign_ai_path_heap_tu15.hpp"
#include "halo2/native_campaign_ai_path_influence_tu15.hpp"
#include "halo2/native_campaign_ai_path_query_start_tu15.hpp"
#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include "halo2/detail/physics_scalar_memory.hpp"
#include <array>
#include <bit>
#include <sstream>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <xmmintrin.h>
#endif

namespace halo2 {
NativeCampaignAiPathSearchOverrunTu15::NativeCampaignAiPathSearchOverrunTu15(std::uint32_t site,std::int32_t row)
    :std::runtime_error([&]{std::ostringstream text;text<<"Native 27B340 path search: 27BD30 row "<<row
        <<" at "<<std::hex<<site<<" overruns the original 64-row frame (its return address)";return text.str();}()),
     site_(site),row_(row){}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using u8=std::uint8_t;using u16=std::uint16_t;using u32=std::uint32_t;using i16=std::int16_t;using i32=std::int32_t;
constexpr u32 none16=0xffffu;
u32 sx16(u32 value){return u32(i32(i16(u16(value))));}

// Retained memory in original order and width. Stores carry their PC.
class Port {
public:
    SourcePhysicsContactMemory &memory;const SourcePhysicsContactRecordObserver &observe;
    u32 r(u32 address,unsigned width)const {
        return physics_step_detail::r(memory,address,width);
    }
    u32 r8(u32 address)const{return r(address,1);}
    u32 r16(u32 address)const{return r(address,2);}
    u32 r32(u32 address)const{return r(address,4);}
    void w(u32 site,u32 address,u32 value,unsigned width)const {
        physics_step_detail::w(memory,observe,site,address,width,value);
    }
    void w8(u32 site,u32 address,u32 value)const{w(site,address,value,1);}
    void w16(u32 site,u32 address,u32 value)const{w(site,address,value,2);}
    void w32(u32 site,u32 address,u32 value)const{w(site,address,value,4);}
};

// One SSE scalar instruction per call under the caller's MXCSR, so sticky
// flags, DAZ/FTZ and rounding are the original's.
class Simd {
    NativeTu15PhysicsFrameFp &fp_;
    static __m128 scalar(u32 value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<i32>(value)));}
    static u32 bits(__m128 value){return std::bit_cast<u32>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
public:
    explicit Simd(NativeTu15PhysicsFrameFp &fp):fp_(fp) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u) || (fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u))
            throw std::invalid_argument("Native 27B2B0 path search requires masked original FP exceptions");
    }
#define HALO2_PATH_SEARCH_BINARY(name,instruction) \
    u32 name(u32 a,u32 b)const { \
        __m128 x=scalar(a);const __m128 y=scalar(b);u32 host{},csr=fp_.mxcsr; \
        asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\t" instruction " %[y],%[x]\n\tstmxcsr %[c]\n\tldmxcsr %[h]" \
            :[x]"+x"(x),[h]"+m"(host),[c]"+m"(csr):[y]"x"(y):"memory"); \
        fp_.mxcsr=csr;return bits(x); \
    }
    HALO2_PATH_SEARCH_BINARY(add,"addss")
    HALO2_PATH_SEARCH_BINARY(sub,"subss")
    HALO2_PATH_SEARCH_BINARY(mul,"mulss")
    HALO2_PATH_SEARCH_BINARY(div,"divss")
#undef HALO2_PATH_SEARCH_BINARY
    // COMISS a,b: {JA, JB}. Unordered is "below" and never "above".
    std::array<bool,2> order(u32 a,u32 b)const {
        const __m128 x=scalar(a),y=scalar(b);u32 host{},csr=fp_.mxcsr;unsigned char above{},below{};
        asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\tcomiss %[y],%[x]\n\tseta %[a]\n\tsetb %[b]\n\tstmxcsr %[c]\n\tldmxcsr %[h]"
            :[a]"=&q"(above),[b]"=&q"(below),[h]"+m"(host),[c]"+m"(csr):[x]"x"(x),[y]"x"(y):"cc","memory");
        fp_.mxcsr=csr;return {above!=0,below!=0};
    }
    bool above(u32 a,u32 b)const{return order(a,b)[0];}
    bool below(u32 a,u32 b)const{return order(a,b)[1];}
    i32 truncate(u32 a)const {
        const __m128 x=scalar(a);u32 host{},csr=fp_.mxcsr;i32 result{};
        asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\tcvttss2si %[x],%[r]\n\tstmxcsr %[c]\n\tldmxcsr %[h]"
            :[r]"=&r"(result),[h]"+m"(host),[c]"+m"(csr):[x]"x"(x):"memory");
        fp_.mxcsr=csr;return result;
    }
};

// SUBSS of two retained operands: the minuend is read first.
u32 difference_of(const Port &p,const Simd &s,u32 minuend,u32 subtrahend) {
    const auto a=p.r32(minuend);return s.sub(a,p.r32(subtrahend));
}
// x87 FLD/FSQRT/FSTP under the caller's control word, from an empty stack.
u32 x87_root(NativeTu15PhysicsFrameFp &fp,u32 squared) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 root{};
    asm volatile("flds %1\n\tfsqrt\n\tfstps %0":"=m"(root):"m"(squared):"memory","st");
    return root;
}
// 218C40's retained extended root, then FST m32 / FADD m32 / FSTP m32.
std::array<u32,2> x87_root_sum(NativeTu15PhysicsFrameFp &fp,u32 squared,u32 addend) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);u32 root{},sum{};
    asm volatile("flds %2\n\tfsqrt\n\tfsts %0\n\tfadds %3\n\tfstps %1":"=m"(root),"=m"(sum):"m"(squared),"m"(addend):"memory","st");
    return {root,sum};
}

// 218C40 without its FSQRT: the squared distance of two framed points
// (ESI first, EDI second). Equal frames subtract locally; otherwise both
// points are projected through native 218B20 first.
u32 distance_squared(const Port &p,const Simd &s,NativeTu15PhysicsFrameFp &fp,u32 first,u32 second) {
    if(p.r16(first+0xcu)==p.r16(second+0xcu)) {
        const auto x=difference_of(p,s,second,first);
        const auto z=difference_of(p,s,second+8u,first+8u);
        const auto y=difference_of(p,s,second+4u,first+4u);
        const auto xz=s.add(s.mul(x,x),s.mul(z,z));
        return s.add(xz,s.mul(y,y));
    }
    const auto a=native_campaign_ai_project_point_value_tu15(p.memory,first,fp);
    const auto b=native_campaign_ai_project_point_value_tu15(p.memory,second,fp);
    const auto z=s.sub(b[2],a[2]),y=s.sub(b[1],a[1]),x=s.sub(b[0],a[0]);
    const auto zy=s.add(s.mul(z,z),s.mul(y,y));
    return s.add(zy,s.mul(x,x));
}

// 186E30: the device bit-field row for a link's device index.
u32 device_bits(const Port &p,u32 index) {
    const auto group=sx16(p.r16(0x474c24u));const auto base=p.r32(0x4f9b50u);
    if(index==0xffffffffu)return (group<<5u)+base+1u;
    return (group<<10u)+base+index+0x201u;
}
// 202970: ECX sector record, EDX navigation. The first kind-7 link of the
// sector decides: a device-gated link whose device bit is clear blocks.
bool special_link_blocked(const Port &p,u32 sector_record,u32 navigation) {
    u32 link=sx16(p.r16(sector_record+2u));
    if(link==0xffffffffu)return false;
    const auto links=p.r32(navigation+0x3cu);
    u32 record{};
    for(;;) {
        record=links+link*20u;
        if(p.r16(record)==7u)break;
        link=sx16(p.r16(record+2u));
        if(link==0xffffffffu)return false;
    }
    const auto devices=p.r32(p.r32(p.r32(0x4ecc18u)+0x18u)+0x2cu);
    const auto device=p.r32(record+4u);
    if(!(p.r8(devices+device*8u+4u)&8u))return false;
    const auto argument=p.r32(record+8u);
    const auto bit=p.r8(devices+device*8u+5u);
    const auto field=device_bits(p,argument);
    return !(p.r32(field+(bit>>5u)*4u)&(1u<<(bit&31u)));
}

// 27BD30. Locals ([esp+..] after its four pushes): 14 node frame word,
// 10 sectors, 18 sector, 24 link index, 28 target flags/vertex D, 1C k.
// Its 218780 outputs are its frame at entry-0x18 (A) and entry-0xC (C).
i16 neighbors(const Port &p,const Simd &s,NativeTu15PhysicsFrameFp &fp,u32 navigation,u32 node,
    u32 rows,u32 workspace,u32 stack,u32 capacity) {
    const u32 frame_base=stack-0x44u;
    const u32 resolved_a=frame_base+0x2cu,resolved_c=frame_base+0x38u;
    const auto sector=p.r32(node+8u);
    const auto node_frame=p.r16(node+0x24u);
    const auto sectors=p.r32(navigation+4u);
    const auto link_count=i32(p.r32(navigation+0x38u));
    u32 link_index=sx16(p.r16(sectors+sector*8u+2u));
    i16 count=0;
    auto row_at=[&](u32 site) {
        if(capacity && u32(i32(count))>=capacity)throw NativeCampaignAiPathSearchOverrunTu15(site,count);
        return rows+u32(i32(count))*0x30u;
    };
    auto vertex=[&](u32 index){return p.r32(navigation+0x2cu)+index*12u;};
    if(link_count>0 && link_index!=0xffffffffu) {
        do {
            if(count>=0x40)break;
            const auto link=p.r32(navigation+0x3cu)+link_index*20u;
            const auto kind=sx16(p.r16(link));
            const auto bit=1u<<(kind&31u);
            bool publish_jump=false;u32 flags{};
            if((p.r32(workspace+0x140b8u)&bit) && kind<=6u) switch(kind) {
            case 0: {
                const auto edge=p.r32(link+4u);
                const auto edges=p.r32(navigation+0xcu);
                const auto record=edges+edge*16u;
                const auto side_b=p.r16(record+0xeu);
                const auto other=p.r16(record+0xcu+(sector!=side_b ? 2u : 0u));
                if(other==none16)break;
                const auto row=row_at(0x27bdf9u);
                p.w32(0x27bdf9u,row,other);
                p.w32(0x27bdfbu,row+0xcu,edge);
                p.w16(0x27be02u,row+8u,none16);
                p.w16(0x27be0cu,row+4u,p.r16(sectors+other*8u));
                const auto a_index=p.r16(record);
                const auto vertices=p.r32(navigation+0x2cu);
                const auto a=vertices+a_index*12u;
                const auto b=vertices+p.r16(record+2u)*12u;
                p.w32(0x27be33u,row+0x10u,p.r32(a));
                p.w32(0x27be38u,row+0x14u,p.r32(a+4u));
                p.w32(0x27be3eu,row+0x18u,p.r32(a+8u));
                p.w16(0x27be4eu,row+0x1cu,node_frame);
                p.w32(0x27be5eu,row+0x20u,difference_of(p,s,b,a));
                p.w32(0x27be6du,row+0x24u,difference_of(p,s,b+4u,a+4u));
                p.w32(0x27be7cu,row+0x28u,difference_of(p,s,b+8u,a+8u));
                p.w8(0x27be8bu,row+0x2cu,(p.r8(record+4u)>>7u)&1u);
                p.w8(0x27c29bu,row+0x2du,1);p.w8(0x27c29fu,row+0x2eu,0);++count;
                break;
            }
            case 1: case 6: {
                const auto a_index=p.r16(link+4u);
                (void)p.r16(link+6u);(void)p.r16(link+8u);(void)p.r16(link+0xau);
                if(!p.r8(workspace+0x140c4u)) {
                    const auto mask=u32(i32(std::int8_t(p.r8(link+0xdu))));
                    if(i32(p.r32(workspace+0x140c0u)&mask)<=0)break;
                }
                const auto target=p.r16(link+0xeu);
                if(target==none16)break;
                flags=sx16(p.r16(sectors+target*8u));
                if(p.r8(node+0xdu)) {
                    const auto v=vertex(a_index);
                    const auto z=difference_of(p,s,node+0x20u,v+8u);
                    const auto y=difference_of(p,s,node+0x1cu,v+4u);
                    const auto x=difference_of(p,s,node+0x18u,v);
                    const auto zy=s.add(s.mul(z,z),s.mul(y,y));
                    if(s.above(s.add(zy,s.mul(x,x)),p.r32(0x46a4c0u)))break;
                }
                if(!(p.r8(link+0xcu)&1u)) {
                    const auto frame_a=p.r16(link+0x10u);
                    if(frame_a!=p.r16(link+0x12u)) {
                        const auto c_index=p.r16(link+8u);const auto vertices=p.r32(navigation+0x2cu);
                        const auto c=vertices+c_index*12u,a=vertices+p.r16(link+4u)*12u;
                        (void)native_campaign_ai_resolve_frame_tu15(p.memory,u16(frame_a),a,resolved_a,fp,p.observe);
                        (void)native_campaign_ai_resolve_frame_tu15(p.memory,u16(p.r16(link+0x12u)),c,resolved_c,fp,p.observe);
                        bool feasible=false;
                        for(u32 k=0;;) {
                            const auto kind_bit=1u<<(k&31u);
                            if(p.r32(workspace+0x140c0u)&kind_bit) {
                                const NativeCampaignAiJumpPointMemoryTu15 from(p.memory,resolved_c),to(p.memory,resolved_a);
                                if(native_campaign_ai_jump_link_tu15(p.memory,i16(k),from,to,nullptr,nullptr,fp)){feasible=true;break;}
                            }
                            if(kind_bit&u32(i32(std::int8_t(p.r8(link+0xdu)))))break;
                            ++k;
                            if(i16(u16(k))>=6)break;
                        }
                        if(!feasible)break;
                    }
                }
                publish_jump=true;
                break;
            }
            case 2: {
                const auto v_index=p.r16(link+6u);
                (void)p.r16(link+8u);
                const auto target=p.r16(link+4u);
                if(target==none16)break;
                const auto v=p.r32(navigation+0x2cu)+v_index*12u;
                const auto mode=p.r16(link+0xau);
                if(mode==1u || mode==2u) {
                    const auto y=difference_of(p,s,node+0x1cu,v+4u);
                    const auto x=difference_of(p,s,node+0x18u,v);
                    if(s.above(s.add(s.mul(y,y),s.mul(x,x)),p.r32(0x46a4c0u)))break;
                }
                const auto row=row_at(0x27c152u);
                p.w32(0x27c152u,row+0xcu,link_index);
                p.w16(0x27c159u,row+8u,2);
                const auto t=p.r16(link+4u);
                p.w32(0x27c163u,row,t);
                p.w16(0x27c16du,row+4u,p.r16(sectors+t*8u)|1u);
                p.w32(0x27c176u,row+0x10u,p.r32(v));
                p.w32(0x27c17bu,row+0x14u,p.r32(v+4u));
                p.w32(0x27c181u,row+0x18u,p.r32(v+8u));
                p.w16(0x27c189u,row+0x1cu,node_frame);
                const auto up=p.r32(0x474d04u);
                p.w32(0x27c198u,row+0x20u,p.r32(up));
                p.w32(0x27c19du,row+0x24u,p.r32(up+4u));
                p.w32(0x27c1a3u,row+0x28u,p.r32(up+8u));
                p.w8(0x27c1a6u,row+0x2cu,1);p.w8(0x27c1aau,row+0x2du,1);
                p.w8(0x27c1b6u,row+0x2eu,p.r16(link+0xau)==3u);
                ++count;
                break;
            }
            case 5: {
                (void)p.r16(link+6u);
                const auto v_index=p.r16(link+8u);
                (void)p.r16(link+0xau);
                if(p.r16(link+4u)==none16)break;
                const auto policy=(p.r32(workspace+0x140bcu)>>8u)&0xffu;
                bool admitted=false;
                if((policy&4u) && (p.r8(link+0xcu)&1u))admitted=true;
                if(!admitted && (policy&8u) && (p.r8(link+0xcu)&2u))admitted=true;
                if(!admitted && (!(policy&0x10u) || !(p.r8(link+0xcu)&4u)))break;
                const auto v=vertex(v_index);
                const auto row=row_at(0x27c247u);
                p.w32(0x27c247u,row+0xcu,link_index);
                p.w16(0x27c24eu,row+8u,5);
                const auto t=p.r16(link+4u);
                p.w32(0x27c258u,row,t);
                p.w16(0x27c25eu,row+4u,p.r16(sectors+t*8u));
                p.w32(0x27c267u,row+0x10u,p.r32(v));
                p.w32(0x27c26cu,row+0x14u,p.r32(v+4u));
                p.w32(0x27c272u,row+0x18u,p.r32(v+8u));
                p.w16(0x27c27au,row+0x1cu,node_frame);
                const auto up=p.r32(0x474d04u);
                p.w32(0x27c289u,row+0x20u,p.r32(up));
                p.w32(0x27c28eu,row+0x24u,p.r32(up+4u));
                p.w32(0x27c294u,row+0x28u,p.r32(up+8u));
                p.w8(0x27c297u,row+0x2cu,1);
                p.w8(0x27c29bu,row+0x2du,1);p.w8(0x27c29fu,row+0x2eu,0);++count;
                break;
            }
            default:break;
            }
            if(publish_jump) {
                const auto vertices=p.r32(navigation+0x2cu);
                const auto c=vertices+p.r16(link+8u)*12u;
                const auto d=vertices+p.r16(link+0xau)*12u;
                const auto row=row_at(0x27c044u);
                p.w16(0x27c044u,row+8u,p.r16(link));
                p.w32(0x27c048u,row+0xcu,link_index);
                p.w32(0x27c04fu,row,p.r16(link+0xeu));
                p.w8(0x27c051u,row+0x2cu,1);p.w8(0x27c055u,row+0x2du,1);
                p.w16(0x27c05cu,row+4u,flags|1u);
                p.w32(0x27c067u,row+0x10u,p.r32(c));
                p.w32(0x27c06cu,row+0x14u,p.r32(c+4u));
                p.w32(0x27c072u,row+0x18u,p.r32(c+8u));
                p.w16(0x27c07du,row+0x1cu,p.r16(link+0x12u));
                p.w8(0x27c088u,row+0x2eu,p.r16(link)==6u);
                p.w32(0x27c097u,row+0x20u,difference_of(p,s,d,c));
                p.w32(0x27c0a6u,row+0x24u,difference_of(p,s,d+4u,c+4u));
                p.w32(0x27c0b5u,row+0x28u,difference_of(p,s,d+8u,c+8u));
                ++count;
            }
            link_index=sx16(p.r16(link+2u));
        } while(link_index!=0xffffffffu);
    }
    // 27C2B2: the sector's boundary-edge ring.
    const auto sector_record=sectors+sector*8u;
    if(!(p.r8(sector_record)&1u)) {
        if(count!=0)return count;
        if(p.r16(node+2u)!=none16)return count;
    }
    u32 edge=p.r32(sector_record+4u);
    for(;;) {
        const auto record=p.r32(navigation+0xcu)+edge*16u;
        const bool side_b=sector==p.r16(record+0xeu);
        const auto other=p.r16(record+0xcu+(side_b ? 0u : 2u));
        if(other!=none16) {
            const auto row=row_at(0x27c32du);
            p.w32(0x27c32du,row+0xcu,edge);
            p.w32(0x27c337u,row,other);
            p.w16(0x27c339u,row+8u,none16);
            p.w16(0x27c343u,row+4u,p.r16(sectors+other*8u));
            const auto portal=p.r8(record+4u);
            p.w8(0x27c34cu,row+0x2du,0);p.w8(0x27c350u,row+0x2eu,0);
            p.w8(0x27c35au,row+0x2cu,(portal>>7u)&1u);
            const auto vertices=p.r32(navigation+0x2cu);
            const auto a_index=p.r16(record),b_index=p.r16(record+2u);
            const auto b=vertices+b_index*12u,a=vertices+a_index*12u;
            p.w32(0x27c37eu,row+0x10u,p.r32(a));
            p.w32(0x27c383u,row+0x14u,p.r32(a+4u));
            p.w32(0x27c38du,row+0x18u,p.r32(a+8u));
            p.w16(0x27c399u,row+0x1cu,node_frame);
            p.w32(0x27c3a9u,row+0x20u,difference_of(p,s,b,a));
            p.w32(0x27c3b8u,row+0x24u,difference_of(p,s,b+4u,a+4u));
            p.w32(0x27c3c7u,row+0x28u,difference_of(p,s,b+8u,a+8u));
            ++count;
        }
        if(count==0x40)return count;
        edge=p.r16(record+8u+(side_b ? 2u : 0u));
        if(edge==p.r32(sector_record+4u))return count;
    }
}

// 27B340. Frame F = entry ESP - 0xC54 (its locals are [esp+n] = F+n-0x10
// after the four pushes). Guest-stack locals: candidate point F+1C (its
// frame word F+28 is a WORD store read back as a DWORD), the goal distance
// F+2C, 218EB0's difference F+48 and the neighbor rows F+54.
bool expand(const Port &p,const Simd &s,NativeTu15PhysicsFrameFp &fp,u32 workspace,u32 stack) {
    const u32 frame=stack-0xc54u;
    const u32 candidate=frame+0x1cu,goal_distance=frame+0x2cu,difference=frame+0x48u,rows=frame+0x54u;
    const u32 W=workspace;
    u32 clamp=p.r32(0x46a16cu);
    if(!s.above(clamp,p.r32(W)))clamp=p.r32(W);
    const auto penalty=p.r32(W+0x4cu);
    for(;;) {
        if(i16(u16(p.r16(W+0x110b0u)))<=1)break;
        const auto current=p.r16(W+0x110b6u);
        const auto record=W+sx16(current)*0x44u;
        p.w16(0x27b3a4u,record+0xf0u,none16);
        p.w16(0x27b3adu,W+0x110b0u,p.r16(W+0x110b0u)-1u);
        const auto remaining=i16(u16(p.r16(W+0x110b0u)));
        if(remaining>1) {
            p.w32(0x27b3ceu,W+0x110b6u,p.r32(W+u32(i32(remaining))*4u+0x110b2u));
            native_campaign_ai_path_heap_sift_down_tu15(p.memory,W,1,p.observe);
        }
        if(current==none16)break;
        const auto node=record+0xb0u;
        if(p.r8(W+0x54u)) {
            if(const auto sector=p.r32(node+8u);sector==p.r32(W+0x68u)) {
                // 27BAC4: the goal sector is reached.
                const auto goal=W+0x58u;
                p.w32(0x27bad2u,W+0x9cu,p.r32(goal));
                p.w32(0x27bad7u,W+0xa0u,p.r32(goal+4u));
                const auto z=p.r32(goal+8u),goal_frame=p.r32(goal+0xcu);
                p.w32(0x27bae0u,W+0xa4u,z);
                p.w16(0x27bae3u,W+0x90u,current);
                p.w32(0x27baeau,W+0x94u,0);
                p.w32(0x27baf2u,W+0xa8u,goal_frame);
                break;
            }
            auto bound=p.r32(0x46a100u);
            if(!s.above(bound,p.r32(W+0x94u)))bound=p.r32(W+0x94u);
            bound=s.mul(bound,p.r32(0x46a110u));
            bound=s.add(bound,p.r32(W+0x98u));
            if(s.above(p.r32(node+0x38u),bound))break;
        }
        const auto navigation=p.r32(W+0x8cu);
        const auto produced=neighbors(p,s,fp,navigation,node,rows,W,frame-0x24u,64);
        if(produced<=0)continue;
        for(u32 left=u16(produced),row=rows;left;--left,row+=0x30u) {
            // 27B470: the parent's own sector is skipped unless either
            // side is a special (+E / row +2E) transition.
            if(const auto target=p.r32(row);target==p.r32(node+4u) && !p.r8(row+0x2eu) && !p.r8(node+0xeu))continue;
            const auto row_flags=p.r16(row+4u);
            if(!(row_flags&1u))continue;
            if(p.r8(node+0xcu) && (row_flags&0x1000u))continue;
            bool excluded=false;
            if(i16(u16(p.r16(W+0x70u)))>0) {
                const auto total=i16(u16(p.r16(W+0x72u)));
                if(total>0) {
                    const auto mask=sx16(p.r16(W+0x70u));
                    for(i16 n=0;n<total;++n) {
                        if(!(mask&(1u<<(u32(n)&31u))))continue;
                        if(const auto kind=p.r16(row+8u);kind!=p.r16(W+u32(n)*8u+0x74u))continue;
                        if(const auto link=p.r32(row+0xcu);link==p.r32(W+u32(n)*8u+0x78u)){excluded=true;break;}
                    }
                }
            }
            if(excluded)continue;
            if(!p.r8(W+4u) && (p.r8(row+4u)&2u)) {
                const auto sector_navigation=p.r32(W+0x8cu);
                const auto target=p.r32(row);
                if(special_link_blocked(p,p.r32(sector_navigation+4u)+target*8u,sector_navigation))continue;
            }
            // 27B518: edge length and the clearance gate.
            const auto ey=p.r32(row+0x24u),ez=p.r32(row+0x28u),ex=p.r32(row+0x20u);
            const auto portal=p.r8(row+0x2cu);
            const auto yy=s.mul(ey,ey),xx=s.mul(ex,ex);
            const auto length_squared=s.add(s.add(xx,yy),s.mul(ez,ez));
            if(portal && !p.r8(row+0x2du)) {
                const auto width=s.mul(clamp,p.r32(0x46a11cu));
                if(s.above(s.mul(width,width),length_squared))continue;
            }
            const auto half=p.r32(0x46a0ccu);
            const auto first_x=p.r32(row+0x20u);
            const bool goal=p.r8(W+0x54u);
            const auto row_frame=p.r16(row+0x1cu);
            p.w32(0x27b595u,candidate,s.add(s.mul(first_x,half),p.r32(row+0x10u)));
            const auto first_y=s.mul(p.r32(row+0x24u),half);
            p.w32(0x27b5a9u,candidate+4u,s.add(first_y,p.r32(row+0x14u)));
            const auto first_z=s.mul(p.r32(row+0x28u),half);
            p.w32(0x27b5bdu,candidate+8u,s.add(first_z,p.r32(row+0x18u)));
            p.w16(0x27b5c3u,candidate+0xcu,row_frame);
            if(goal && s.above(length_squared,p.r32(0x4500b4u))) {
                const auto width=s.mul(clamp,p.r32(0x46a11cu));
                if(s.above(length_squared,s.mul(width,width))) {
                    // 27B5F9: slide the crossing point toward the goal.
                    const auto length=x87_root(fp,length_squared);
                    native_campaign_ai_framed_difference_tu15(p.memory,row+0x10u,W+0x58u,difference,fp,p.observe);
                    const auto z=p.r32(row+0x28u),y=p.r32(row+0x24u),x=p.r32(row+0x20u);
                    auto dot=s.mul(y,p.r32(difference+4u));
                    dot=s.add(dot,s.mul(z,p.r32(difference+8u)));
                    dot=s.add(dot,s.mul(x,p.r32(difference)));
                    const auto x2=s.mul(x,x),y2=s.mul(y,y),z2=s.mul(z,z);
                    const auto margin=s.div(clamp,length);
                    const auto squared=s.add(s.add(x2,y2),z2);
                    auto t=s.div(dot,squared);
                    if(s.above(margin,t))t=margin;
                    else {
                        const auto upper=s.sub(p.r32(0x46a0d0u),margin);
                        if(s.above(t,upper))t=upper;
                    }
                    const auto tx=s.mul(p.r32(row+0x20u),t);
                    p.w32(0x27b69fu,candidate,s.add(tx,p.r32(row+0x10u)));
                    const auto ty=s.mul(p.r32(row+0x24u),t);
                    p.w32(0x27b6b3u,candidate+4u,s.add(ty,p.r32(row+0x14u)));
                    const auto tz=s.mul(p.r32(row+0x28u),t);
                    p.w32(0x27b6c7u,candidate+8u,s.add(tz,p.r32(row+0x18u)));
                }
            }
            // 27B6CD: step distance (218C40's retained extended root).
            const auto position=node+0x18u;
            const auto step_squared=distance_squared(p,s,fp,position,candidate);
            const auto accumulated_base=p.r32(node+0x30u);
            const auto [distance,accumulated]=x87_root_sum(fp,step_squared,accumulated_base);
            u32 nearest{},step{};
            if(p.r8(W+0x28u)) {
                const auto influence=native_campaign_ai_path_influence_value_tu15(p.memory,position,candidate,W,fp,0);
                nearest=influence.distance;
                const auto previous=p.r32(node+0x2cu);
                const bool keep=s.above(previous,nearest);
                step=s.add(influence.influence,p.r32(0x46a0d0u));
                step=s.mul(step,distance);
                if(!keep)nearest=previous;
            } else step=distance;
            if(s.above(penalty,p.r32(0x46a0e8u))) {
                const auto kind=p.r16(row+8u);
                if(kind!=none16) {
                    const auto index=sx16(kind)-1u;
                    if(index<=5u && index!=2u && index!=3u)step=s.add(step,penalty);
                }
            }
            const bool goal_estimate=p.r8(W+0x54u);
            const auto cost=s.add(p.r32(node+0x34u),step);
            auto estimate=cost;
            if(goal_estimate) {
                const auto remaining_squared=distance_squared(p,s,fp,candidate,W+0x58u);
                const auto [remaining_distance,total]=x87_root_sum(fp,remaining_squared,cost);
                p.w32(0x27b791u,goal_distance,remaining_distance);
                estimate=total;
            }
            const auto priority=s.truncate(s.mul(estimate,p.r32(0x46a110u)));
            if(priority>=0x7fff)continue;
            if(p.r8(W+0x45u) && s.above(accumulated,p.r32(W+0x48u)))continue;
            // 27B7DF: hash probe for an existing node of this sector/point.
            u32 slot=(p.r16(row)&0x1ffu)<<3u;
            auto entry=p.r16(W+sx16(slot)*2u+0x120b6u);
            bool found=false;
            if(entry!=none16) {
                const auto key=p.r32(row);
                for(;;) {
                    const auto existing=W+0xb0u+sx16(entry)*0x44u;
                    if(p.r32(existing+8u)==key) {
                        bool same=!p.r8(existing+0xeu) && !p.r8(row+0x2eu);
                        if(!same) {
                            const auto x=difference_of(p,s,existing+0x18u,row+0x10u);
                            const auto z=difference_of(p,s,existing+0x20u,row+0x18u);
                            const auto y=difference_of(p,s,existing+0x1cu,row+0x14u);
                            const auto xz=s.add(s.mul(x,x),s.mul(z,z));
                            same=!s.above(s.add(xz,s.mul(y,y)),p.r32(0x46a4c0u));
                        }
                        if(same){found=true;break;}
                    }
                    slot=(slot+1u)&0xfffu;
                    entry=p.r16(W+sx16(slot)*2u+0x120b6u);
                    if(entry==none16)break;
                }
            }
            u16 index{};
            if(found) {
                const auto existing=W+0xb0u+sx16(entry)*0x44u;
                if(priority>=i32(sx16(p.r16(existing+0x3cu))))continue;
                if(p.r16(existing+0x40u)==none16)continue;
                index=u16(entry);
            } else {
                const auto nodes=p.r16(W+0xaeu);
                if(i16(u16(nodes))>=0x400)continue;
                p.w16(0x27b8b9u,W+0xaeu,nodes+1u);
                p.w16(0x27b8c0u,W+sx16(slot)*2u+0x120b6u,nodes);
                p.w16(0x27b8c8u,W+sx16(nodes)*0x44u+0xf0u,none16);
                index=u16(nodes);
            }
            if(index==none16)continue;
            // 27B8D2: publish the node.
            const auto published=W+0xb0u+sx16(index)*0x44u;
            const auto point_x=p.r32(candidate);
            p.w16(0x27b8f6u,published+2u,current);
            p.w32(0x27b901u,published+4u,p.r32(node+8u));
            p.w32(0x27b907u,published+8u,p.r32(row));
            p.w32(0x27b90fu,published+0x18u,point_x);
            p.w32(0x27b915u,published+0x1cu,p.r32(candidate+4u));
            p.w32(0x27b91cu,published+0x20u,p.r32(candidate+8u));
            p.w32(0x27b923u,published+0x24u,p.r32(candidate+0xcu));
            const auto published_flags=p.r16(row+4u);
            p.w16(0x27b936u,published+0x3cu,u32(priority));
            p.w32(0x27b93au,published+0x28u,distance);
            p.w32(0x27b945u,published+0x2cu,nearest);
            p.w32(0x27b94au,published+0x30u,accumulated);
            p.w32(0x27b94fu,published+0x34u,cost);
            p.w32(0x27b954u,published+0x38u,estimate);
            p.w8(0x27b967u,published+0xcu,(published_flags>>12u)&1u);
            p.w8(0x27b96du,published+0xdu,(published_flags&0x3c0u)!=0);
            p.w8(0x27b973u,published+0xeu,p.r8(row+0x2eu));
            const auto kind_word=p.r32(row+8u);
            p.w32(0x27b97cu,published+0x14u,p.r32(row+0xcu));
            p.w32(0x27b986u,published+0x10u,kind_word);
            u32 depth{};
            switch(sx16(kind_word)-1u) {
            case 0: case 4: case 5:depth=p.r16(node+0x3eu)+2u;break;
            case 1: {
                const auto link=p.r32(published+0x14u);
                const auto links=p.r32(p.r32(W+0x8cu)+0x3cu);
                const bool direct=p.r16(links+link*20u+0xau)!=0;
                depth=p.r16(node+0x3eu)+(direct ? 1u : 2u);
                break;
            }
            default:depth=p.r16(node+0x3eu)+1u;
            }
            p.w16(0x27b9edu,published+0x3eu,depth);
            const auto back=p.r16(published+0x40u);
            if(back==none16) {
                const auto tail=p.r16(W+0x110b0u);
                if(i16(u16(tail))<0x400) {
                    p.w16(0x27ba13u,W+0x110b0u,tail+1u);
                    p.w16(0x27ba1du,W+sx16(tail)*4u+0x110b2u,index);
                    p.w16(0x27ba25u,W+sx16(tail)*4u+0x110b4u,u32(priority));
                    native_campaign_ai_path_heap_sift_up_tu15(p.memory,W,i16(tail),p.observe);
                }
            } else {
                p.w16(0x27ba37u,W+sx16(back)*4u+0x110b4u,u32(priority));
                native_campaign_ai_path_heap_sift_up_tu15(p.memory,W,i16(u16(p.r16(published+0x40u))),p.observe);
            }
            // 27BA4A: track the node closest to the goal.
            if(!p.r8(W+0x54u))continue;
            const auto threshold=p.r32(W+0x6cu);
            const bool tracking=s.above(threshold,p.r32(0x46a0e8u));
            const auto bx=p.r32(published+0x18u),by=p.r32(published+0x1cu),bz=p.r32(published+0x20u),bf=p.r32(published+0x24u);
            if(!tracking)continue;
            const auto best=p.r32(W+0x94u);
            const auto remaining_distance=p.r32(goal_distance);
            if(!s.above(best,remaining_distance))continue;
            p.w32(0x27ba83u,W+0x9cu,bx);
            p.w32(0x27ba85u,W+0xa0u,by);
            p.w32(0x27ba88u,W+0xa4u,bz);
            p.w16(0x27ba8bu,W+0x90u,index);
            p.w32(0x27ba92u,W+0x94u,remaining_distance);
            p.w32(0x27ba9au,W+0xa8u,bf);
            p.w32(0x27ba9du,W+0x98u,estimate);
        }
    }
    // 27BAF5. Without a goal the result is the frame byte set to 1 at entry.
    if(p.r8(W+0x54u)){const auto threshold=p.r32(W+0x6cu);return !s.below(threshold,p.r32(W+0x94u));}
    return true;
}

bool search(const Port &p,const Simd &s,NativeTu15PhysicsFrameFp &fp,u32 workspace,u32 stack) {
    const u32 W=workspace;
    const auto maximum=p.r32(0x45695cu);
    p.w16(0x27b2c0u,W+0xaeu,0);
    p.w16(0x27b2c9u,W+0x110b0u,1);
    for(u32 n=0;n<0x800u;++n)p.w32(0x27b2ddu,W+0x120b6u+n*4u,0xffffffffu);
    p.w16(0x27b2e1u,W+0x90u,none16);
    p.w32(0x27b2e8u,W+0x94u,maximum);
    p.w32(0x27b2f0u,W+0x98u,maximum);
    bool result=false;
    if(native_campaign_ai_path_query_start_tu15(p.memory,W,fp,p.observe)) {
        // 27B301: PUSH ESI, CALL 27B340 below 27B2B0's three saved registers.
        result=expand(p,s,fp,W,stack-0x14u);
        if(result)return true;
    }
    if(p.r16(W+0x70u)==0) {p.w16(0x27b314u,W+0x70u,0x20u);return result;}
    p.w16(0x27b320u,W+0x70u,p.r16(W+0x70u)+1u);
    if(i16(u16(p.r16(W+0x70u)))>0x20)p.w16(0x27b32eu,W+0x70u,0);
    return result;
}

NativeTu15PhysicsFrameFp abi_fp(const SourcePhysicsWorldRuntimeAbi &abi,const char *owner) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth)
        throw std::runtime_error(std::string(owner)+" requires retained FP controls and an empty x87 stack");
    return {abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
}
void publish_fp(SourcePhysicsWorldRuntimeAbi &abi,const NativeTu15PhysicsFrameFp &fp) {
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
}
u32 stack_word(const SourcePhysicsContactMemory &memory,u32 address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);u32 value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<u32>(raw[n])<<(n*8u);
    return value;
}
}

bool native_campaign_ai_path_search_tu15(SourcePhysicsContactMemory &memory,std::uint32_t workspace,
    std::uint32_t stack,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const Port port{memory,observe};const Simd simd(fp);
    return search(port,simd,fp,workspace,stack);
}
void native_campaign_ai_path_search_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    auto fp=abi_fp(abi,"Native 27B2B0");
    struct Publication{SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~Publication(){publish_fp(abi,fp);}} publication{abi,fp};
    const auto sp=abi.gp[4];
    const bool result=native_campaign_ai_path_search_tu15(memory,abi.gp[0],sp,fp,observe);
    abi.gp[0]=result;abi.gp[4]=sp+4u;
}
bool native_campaign_ai_path_expand_tu15(SourcePhysicsContactMemory &memory,std::uint32_t workspace,
    std::uint32_t stack,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    const Port port{memory,observe};const Simd simd(fp);
    return expand(port,simd,fp,workspace,stack);
}
void native_campaign_ai_path_expand_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    auto fp=abi_fp(abi,"Native 27B340");
    struct Publication{SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~Publication(){publish_fp(abi,fp);}} publication{abi,fp};
    const auto sp=abi.gp[4];
    const bool result=native_campaign_ai_path_expand_tu15(memory,stack_word(memory,sp+4u),sp,fp,observe);
    abi.gp[0]=result;abi.gp[4]=sp+8u;
}
std::int16_t native_campaign_ai_path_neighbors_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t navigation,std::uint32_t node,std::uint32_t rows,std::uint32_t workspace,
    std::uint32_t stack,std::uint32_t row_capacity,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
    const Port port{memory,observe};const Simd simd(fp);
    return neighbors(port,simd,fp,navigation,node,rows,workspace,stack,row_capacity);
}
void native_campaign_ai_path_neighbors_call_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    auto fp=abi_fp(abi,"Native 27BD30");
    struct Publication{SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;~Publication(){publish_fp(abi,fp);}} publication{abi,fp};
    const auto sp=abi.gp[4];
    std::array<u32,4> arguments{};
    for(unsigned n=0;n<4;++n)arguments[n]=stack_word(memory,sp+4u+n*4u);
    const auto count=native_campaign_ai_path_neighbors_tu15(memory,arguments[0],arguments[1],arguments[2],
        arguments[3],sp,0,fp,observe);
    abi.gp[0]=u16(count);abi.gp[4]=sp+0x14u;
}
#else
bool native_campaign_ai_path_search_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 27B2B0 path search arithmetic backend unavailable");
}
void native_campaign_ai_path_search_call_tu15(SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 27B2B0 path search arithmetic backend unavailable");
}
bool native_campaign_ai_path_expand_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 27B340 path search arithmetic backend unavailable");
}
void native_campaign_ai_path_expand_call_tu15(SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 27B340 path search arithmetic backend unavailable");
}
std::int16_t native_campaign_ai_path_neighbors_tu15(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,
    std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 27BD30 path neighbor arithmetic backend unavailable");
}
void native_campaign_ai_path_neighbors_call_tu15(SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native TU1.5 27BD30 path neighbor arithmetic backend unavailable");
}
#endif
}
