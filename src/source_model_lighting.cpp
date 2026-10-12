#include "halo2/source_model_lighting.hpp"
#include "halo2/object_lighting.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <set>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
template<class T>T read(const SourceModelResourceWorld& w,std::uint32_t at) {
    if(!w.banks.read)throw FormatError("source model lighting reader absent");
    const auto bytes=w.banks.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source model lighting input truncated");
    T value;std::memcpy(&value,bytes.data(),sizeof(T));return value;
}
template<class T>void put(const SourceModelResourceWorld& w,std::uint32_t at,T value) {
    if(!w.write)throw FormatError("source model lighting writer absent");
    auto bytes=w.write(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("source model lighting output truncated");
    std::memcpy(bytes.data(),&value,sizeof(T));
}
std::uint32_t body(const SourceModelResourceWorld& w,std::uint32_t owner) {
    return read<std::uint32_t>(w,read<std::uint32_t>(w,w.banks.objects+0x44)+(owner&65535u)*12+8);
}
std::uint32_t root(const SourceModelResourceWorld& w,std::uint32_t owner) {
    std::set<std::uint32_t> visited;
    while(owner!=none) {
        if(!visited.insert(owner).second)throw FormatError("source model lighting parent cycle");
        const auto parent=read<std::uint32_t>(w,body(w,owner)+0x14);
        if(parent==none)return owner;
        owner=parent;
    }
    return none;
}
std::uint32_t record(const SourceModelResourceWorld& w,std::uint32_t token) {
    if((token&65535u)>=256)throw FormatError("source model lighting cache token escapes256 records");
    return read<std::uint32_t>(w,w.render_cache_pool+0x44)+(token&65535u)*256;
}
std::uint32_t acquire(const SourceModelResourceWorld& w,std::uint32_t owner) {
    const auto token=source_model_acquire_cache(owner,w);
    if(w.cache_acquired)w.cache_acquired(owner,token);
    return token;
}
void copy(const SourceModelResourceWorld& w,std::uint32_t dst,std::uint32_t src,unsigned words) {
    for(unsigned i=0;i<words;++i)put(w,dst+i*4,read<std::uint32_t>(w,src+i*4));
}
std::pair<std::int32_t,bool> type(const SourceModelResourceWorld& w,std::uint32_t owner) {
    if(owner==none)return {-1,false};
    const auto b=body(w,owner);
    const auto d=read<std::uint32_t>(w,w.banks.tag_table+(read<std::uint32_t>(w,b)&65535u)*16+8);
    return {read<std::int16_t>(w,d),(read<std::uint32_t>(w,b+4)&0x2000)!=0 || (read<std::uint8_t>(w,d+2)&2)!=0};
}
bool typed_owner(const SourceModelResourceWorld& w,std::uint32_t owner) {
    if(owner==none)return false;
    const auto pool=w.banks.objects,index=owner&65535u;
    if(std::int32_t(index)>=read<std::int32_t>(w,pool+0x38))return false;
    const auto header=read<std::uint32_t>(w,pool+0x44)+index*read<std::uint32_t>(w,pool+0x24);
    const auto salt=read<std::int16_t>(w,header);
    if(!salt || std::int32_t(salt)!=(std::bit_cast<std::int32_t>(owner)>>16))return false;
    if(!(0x80u&(1u<<(read<std::uint8_t>(w,header+3)&31u))))return false;
    return read<std::uint32_t>(w,header+8)!=0;
}
void rgb(const SourceModelResourceWorld& w,std::uint32_t dst,std::uint32_t src,float step) {
    const auto a=read<std::uint32_t>(w,dst),b=read<std::uint32_t>(w,src);
    Vec3 result;
    float* channels[]={&result.x,&result.y,&result.z};
    for(unsigned i=0;i<3;++i) {
        const auto shift=16-i*8;
        const float current=float((a>>shift)&255u)*(1.f/255.f);
        const float target=float((b>>shift)&255u)*(1.f/255.f);
        float delta=target-current;
        if(-step>delta)delta=-step;
        else if(delta>step)delta=step;
        *channels[i]=current+delta;
    }
    put(w,dst,pack_object_lighting_rgb(result));
}
double source_length(Vec3 v) {
    //24590: captured x87 PC23F selects53-bit precision, X/Y then Z.
    const double x=v.x,y=v.y,z=v.z;
    const double xy=x*x+y*y;return std::sqrt(xy+z*z);
}
}
void source_model_lighting_update(std::uint32_t token,std::uint32_t owner,float metric,
    const SourceModelLightingWorld& world) {
    const auto& w=world.memory;const auto c=record(w,token);
    const auto last=read<std::uint32_t>(w,c+8);
    auto elapsed=std::bit_cast<std::int32_t>(w.render_frame-last);
    if(!elapsed)return;
    const auto b=body(w,owner);
    const auto rendered=read<std::uint32_t>(w,c+12);
    const bool initial=!read<std::uint8_t>(w,c+3) || !read<std::uint8_t>(w,c+2);
    if(elapsed<0)elapsed=1;
    bool discontinuity=initial;
    if(!initial && (read<std::uint8_t>(w,b+0xc1)&1))return;
    const auto interval=metric>400.f?3:metric>200.f?6:metric>100.f?9:metric>50.f?15:metric>10.f?45:60;
    if(rendered==none || std::bit_cast<std::int32_t>(w.render_frame-rendered)>1)discontinuity=true;
    bool due=last==none || elapsed>interval;
    if(due && read<std::uint8_t>(w,c+2)) {
        const auto origin=read<Vec3>(w,b+0x30),previous=read<Vec3>(w,c+0x10);
        const float z=origin.z-previous.z,x=origin.x-previous.x,y=origin.y-previous.y;
        const float zx=z*z+x*x;const float squared=zx+y*y;
        due=!(.002500000176951289f>squared);
    }
    if(due) {
        if(!world.query)throw FormatError("source model lighting D1850 query owner absent");
        put(w,c+8,w.render_frame);
        if(world.query(owner,initial,c+0x30)) {
            put<std::uint8_t>(w,c+2,1);copy(w,c+0x10,b+0x30,3);
        }
    }
    if(!read<std::uint8_t>(w,c+2))return;
    if(!read<std::uint8_t>(w,c+3) || !world.interpolation || discontinuity)copy(w,c+0x1c,c+0x30,5);
    else {
        if(!world.velocity)throw FormatError("source model lighting BA1D0 velocity owner absent");
        world.velocity(owner,world.scratch+20);
        const double speed=source_length(read<Vec3>(w,world.scratch+20));
        const double difference=speed-double(.15000000596046448f);
        float rate=float(difference*double(.03350083902478218f));
        if(0.f>rate)rate=0.f;
        else if(rate>1.f)rate=1.f;
        bool smooth=rate>0.f;
        if(!smooth) {
            const auto origin=read<Vec3>(w,b+0x30),previous=read<Vec3>(w,c+0x10);
            //3EA30 retains its53-bit x87 differences through the square root.
            const double x=double(origin.x)-previous.x,y=double(origin.y)-previous.y,z=double(origin.z)-previous.z;
            const auto displacement=std::sqrt((x*x+y*y)+z*z);
            // The subsequent COMISS rate,0 remains a second gate. Movement
            // alone never admits interpolation; a valid type7 owner can.
            smooth=(displacement>.05000000074505806f && rate>0.f) || typed_owner(w,owner);
        }
        if(smooth) {
            if(!world.incident)throw FormatError("source model lighting3E5A0 incident owner absent");
            world.incident(c+0x30,c+0x1c,rate*.009999999776482582f);
            rgb(w,c+0x2c,c+0x40,rate*.5f);
            rgb(w,c+0x28,c+0x3c,rate*.5f);
        }
    }
    if(!world.convert)throw FormatError("source model lighting D4080 conversion owner absent");
    const auto [kind,flag]=type(w,read<std::uint32_t>(w,c+4));
    world.convert(c+0x1c,kind,flag,c+0x54);
    put<std::uint8_t>(w,c+3,1);
}
std::uint32_t source_model_lighting(std::uint32_t owner,float metric,
    const SourceModelLightingWorld& world) {
    const auto& w=world.memory;
    const auto b=body(w,owner);
    if(read<std::uint8_t>(w,b+7)&1)return world.default_lighting+0x1c;
    const auto probe=read<std::int16_t>(w,b+0xd0);
    if(probe!=-1)return read<std::uint32_t>(w,world.bsp+0x230)+std::uint32_t(std::int32_t(probe))*0x5c+8;
    const auto ancestor=root(w,owner),token=acquire(w,ancestor);
    if(token!=none) {
        const auto c=record(w,token);
        if(ancestor==owner)source_model_lighting_update(token,owner,metric,world);
        else {
            const auto child=acquire(w,owner);
            if(child!=none) {
                const auto target=record(w,child);
                copy(w,target+0x54,c+0x54,21);
                copy(w,target+0x30,c+0x1c,5);copy(w,target+0x1c,c+0x1c,5);
            }
        }
        return c+0x54;
    }
    if(!world.query || !world.convert)throw FormatError("source model lighting direct query owners absent");
    const auto [kind,flag]=type(w,ancestor);
    (void)world.query(ancestor,true,world.scratch);
    world.convert(world.scratch,kind,flag,world.fallback_lighting);
    return world.fallback_lighting;
}
}
