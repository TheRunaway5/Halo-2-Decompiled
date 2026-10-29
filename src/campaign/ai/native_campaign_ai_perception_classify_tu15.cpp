#include "halo2/native_campaign_ai_perception_classify_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_initialize_tu15.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include "halo2/native_campaign_audio_stop_callback.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8u);return v;}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(k*8u));m.write(p,b);}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
struct Header {
    const SourcePhysicsContactMemory&m;std::uint32_t address;
    std::int16_t salt()const{return signed16(read(m,address,2));}
    bool present()const{return address!=0;}
    std::uint8_t type()const{return std::uint8_t(read(m,address+3u,1));}
    std::uint32_t body()const{return read(m,address+8u);}
};
struct Objects {
    const SourcePhysicsContactMemory&m;std::uint32_t owner;
    std::int32_t extent()const{return std::bit_cast<std::int32_t>(read(m,owner+0x38u));}
    Header indexed(std::uint32_t index)const{const auto stride=read(m,owner+0x24u),rows=read(m,owner+0x44u);return {m,rows+index*stride};}
};
struct ObjectQuery {
    const SourcePhysicsContactMemory&m;
    Objects objects()const{return {m,read(m,0x4ecbd0u)};}
};
// C0CA0: actual retained generation stamp, with no count or validity clamp.
bool stamp_object(SourcePhysicsContactMemory&m,std::uint32_t object){
    const auto generation=read(m,0x4eabccu),cell=0x4eabd0u+(object&0xffffu)*4u;
    if(read(m,cell)==generation)return false;
    write(m,cell,generation);return true;
}
//29A990's iterator is a genuine caller-private pair, not retained scratch.
struct AttachmentIterator {
    std::uint32_t next,previous=none;
    std::uint32_t advance(const SourcePhysicsContactMemory&m){
        if(next==none)return 0;
        const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),body=read(m,rows+(next&0xffffu)*12u+8u);
        previous=next;
        if(read(m,body+0x134u)==0){
            const auto extension=body+std::uint32_t(std::int32_t(signed16(read(m,body+0x13au,2))));
            if(extension){next=read(m,extension+0xcu);return body;}
        }
        next=none;return body;
    }
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp&fp_;unsigned ambient_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),ambient_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native perception classification requires its original SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(ambient_);}
};
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool above(__m128 a,__m128 b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a),"x"(b):"cc","memory");return v!=0;}
void squared_separation(const SourcePhysicsContactMemory&m,std::uint32_t clump,
        const std::array<std::uint32_t,3>&point,NativeTu15PhysicsFrameFp&fp){
    const auto target_z=read(m,clump+0xcu);SimdEnvironment environment(fp);
    const auto z=sub(scalar(point[2]),scalar(target_z));
    const auto y=sub(scalar(point[1]),scalar(read(m,clump+8u)));
    const auto x=sub(scalar(point[0]),scalar(read(m,clump+4u)));
    const auto zz=mul(z,z),yy=mul(y,y);const auto zy=add(zz,yy);const auto xx=mul(x,x);(void)add(zy,xx);
}
#endif
}
std::optional<NativeCampaignAiPerceptionClassificationTu15>
native_campaign_ai_perception_classify_new_group_tu15(SourcePhysicsContactMemory&m,
        std::uint32_t object,std::uint32_t clump,NativeTu15PhysicsFrameFp&fp){
    NativeCampaignAiPerceptionClassificationTu15 result{0,0,object};ObjectQuery query{m};
    const auto body=native_campaign_sound_object(query,object,none);
    if(!body)return result;
    if(!stamp_object(m,object))return result;
    const auto point=native_campaign_object_world_point_value_tu15(m,object,fp);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    squared_separation(m,clump,point,fp);
#else
    (void)point;throw std::runtime_error("Exact native perception classification arithmetic unavailable");
#endif
    const auto type=read(m,body+0xaau,1);
    if(type==0u||type==1u){
        if(type==1u&&read(m,body+0x12cu)==none){result.kind=4;result.priority=1;return result;}
        const auto special=(read(m,body+0x10au,1)&4u)!=0;
        const auto actor=read(m,body+0x12cu);std::int16_t team;bool player_owned=false;
        if(actor!=none){const auto actors=read(m,0x5020c8u),rows=read(m,actors+0x44u);team=signed16(read(m,rows+(actor&0xffffu)*0x888u+0x24u,2));}
        else{
            const auto player=read(m,body+0x13cu);
            if(player!=none){const auto players=read(m,0x4f54f4u),rows=read(m,players+0x44u);team=std::bit_cast<std::int8_t>(std::uint8_t(read(m,rows+(player&0xffffu)*0x21cu+0xc0u,1)));player_owned=true;}
            else team=signed16(read(m,body+0x138u,2));
        }
        const auto clump_team=signed16(read(m,clump+0x12u,2));
        const bool relation=team!=clump_team&&native_campaign_ai_perception_team_relation_tu15(m,team,clump_team);
        if(special){result.kind=relation?6u:7u;result.priority=1;}
        else if(relation){result.kind=1;result.priority=3;}
        else{result.kind=2;if(player_owned)result.priority=3;}
        return result;
    }
    if(type==12u){
        if(read(m,body+0x134u)!=0)return result;
        const auto extension=body+std::uint32_t(std::int32_t(signed16(read(m,body+0x13au,2))));
        const auto clump_team=signed16(read(m,clump+0x12u,2)),team=signed16(read(m,body+0x12eu,2));
        if(native_campaign_ai_perception_team_relation_tu15(m,team,clump_team)){result.kind=1;result.priority=3;}else result.kind=2;
        const auto attachment=read(m,extension+8u);
        if(attachment!=none){
            const auto attachments=read(m,0x510fa8u),rows=read(m,attachments+0x44u);
            AttachmentIterator members{read(m,rows+(attachment&0xffffu)*0x34u+0x18u)};
            for(auto current=members.advance(m);current;current=members.advance(m))(void)stamp_object(m,members.previous);
        }
        return result;
    }
    if(type==5u){
        const auto tag=read(m,body),tags=read(m,0x4f0414u),definition=read(m,tags+(tag&0xffffu)*16u+8u);
        const auto size=read(m,definition+0xd0u),threshold=read(m,0x46a0e8u);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        SimdEnvironment environment(fp);
        if(!above(scalar(size),scalar(threshold)))return result;
#endif
        if(read(m,body+0x14u)==none||(read(m,body+0x12cu)&0x20u)){result.kind=3;result.priority=3;}
        return result;
    }
    return std::nullopt;
}
}
