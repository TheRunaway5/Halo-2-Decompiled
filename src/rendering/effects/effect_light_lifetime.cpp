#include "halo2/effect_light_lifetime.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
using Record=SourceEffectLightDatumPool::Record;
float scalar(const Record& r,unsigned at) {return std::bit_cast<float>(r.at(at/4));}
Vec3 vector(const Record& r,unsigned at) {return {scalar(r,at),scalar(r,at+4),scalar(r,at+8)};}
void scalar(Record& r,unsigned at,float v) {r.at(at/4)=std::bit_cast<std::uint32_t>(v);}
void vector(Record& r,unsigned at,Vec3 v) {scalar(r,at,v.x);scalar(r,at+4,v.y);scalar(r,at+8,v.z);}
std::uint16_t flags(const Record& r) {return std::uint16_t(r[0]>>16);}
void flags(Record& r,std::uint16_t v) {r[0]=(r[0]&0xffffu)|(std::uint32_t(v)<<16);}
void valid_pool(const SourceEffectLightDatumPool& p) {
    if(p.cursor>p.capacity || p.highwater>p.capacity || p.active_count>p.capacity)
        throw FormatError("Effect light datum pool exceeds original350 records");
}
SourceEffectLightRegistration registration(const Record& r,std::uint32_t id) {
    SourceEffectLightRegistration s;s.identity=id;s.flags=flags(r);s.reference_head=r[0x10/4];
    s.spatial_center=vector(r,0x18);s.spatial_radius=scalar(r,0x24);
    s.cull_center=vector(r,0x28);s.cull_radius=scalar(r,0x34);s.location_center=vector(r,0x38);
    s.location={r[0x44/4],std::int16_t(r[0x48/4]),std::int16_t(r[0x48/4]>>16)};
    s.position=vector(r,0x84);s.extension_radius=scalar(r,0x9c);s.extension_center=vector(r,0xa0);return s;
}
void registration(Record& r,const SourceEffectLightRegistration& s) {
    flags(r,s.flags);r[0x10/4]=s.reference_head;
    vector(r,0x18,s.spatial_center);scalar(r,0x24,s.spatial_radius);
    vector(r,0x28,s.cull_center);scalar(r,0x34,s.cull_radius);vector(r,0x38,s.location_center);
    r[0x44/4]=s.location.leaf;r[0x48/4]=std::uint16_t(s.location.cluster)|(std::uint32_t(std::uint16_t(s.location.bsp))<<16);
}
void valid_standalone(const Record& r) {
    if(r[0x4c/4]!=none || r[0x58/4]!=none || std::uint16_t(r[0x54/4])!=0xffffu ||
        std::uint16_t(r[0x5c/4])!=0xffffu || (flags(r)&4))
        throw FormatError("Effect light lifetime requires original attachment/segment producer");
}
}
void SourceEffectLightDatumPool::activate(bool debug) {
    valid_pool(*this);cursor=highwater=active_count=0;next_generation=0xe96c;debug_fill=debug;occupied.fill(0);
    for(auto& r:records) {if(debug)r.fill(0xbabababau);r[0]&=0xffff0000u;}
}
std::uint32_t SourceEffectLightDatumPool::allocate() {
    valid_pool(*this);auto i=none;
    for(auto n=cursor;n<highwater;++n)if(!(occupied[n/32]&(1u<<(n&31)))) {i=n;break;}
    if(i==none && highwater<capacity)i=highwater;
    if(i==none)return none;
    occupied[i/32]|=1u<<(i&31);++active_count;cursor=i+1;if(highwater<=i)highwater=i+1;
    auto& r=records[i];r.fill(0);r[0]=next_generation;++next_generation;
    if(next_generation==0xffffu)next_generation=0x8000;
    return (r[0]<<16)|i;
}
void SourceEffectLightDatumPool::release(std::uint32_t id) {
    valid_pool(*this);const auto i=id&0xffffu;
    if(i>=capacity || !(occupied[i/32]&(1u<<(i&31))) || !active_count)
        throw FormatError("Effect light release requires an occupied original slot");
    auto& r=records[i];if(debug_fill)r.fill(0xbabababau);
    occupied[i/32]&=~(1u<<(i&31));r[0]&=0xffff0000u;if(i<cursor)cursor=i;
    if(i+1==highwater)do {--highwater;}while(highwater && !(records[highwater-1][0]&0xffffu));
    --active_count;
}
bool SourceEffectLightDatumPool::contains(std::uint32_t id) const {
    const auto i=id&0xffffu;
    return id!=none && i<capacity && i<highwater && (occupied[i/32]&(1u<<(i&31))) &&
        std::uint16_t(records[i][0])==std::uint16_t(id>>16) && std::uint16_t(records[i][0])!=0;
}
Record& SourceEffectLightDatumPool::record(std::uint32_t id) {
    if(!contains(id))throw FormatError("Effect light identity has no retained lifetime");
    return records[id&0xffffu];
}
const Record& SourceEffectLightDatumPool::record(std::uint32_t id) const {
    if(!contains(id))throw FormatError("Effect light identity has no retained lifetime");
    return records[id&0xffffu];
}
std::vector<std::uint32_t> SourceEffectLightDatumPool::identities() const {
    valid_pool(*this);std::vector<std::uint32_t> out;
    for(unsigned i=0;i<highwater;++i)if(occupied[i/32]&(1u<<(i&31)))out.push_back((records[i][0]<<16)|i);
    return out;
}
SourceStandaloneEffectLight source_standalone_effect_light(const SourceEffectLightDatumPool& p,std::uint32_t id) {
    const auto& r=p.record(id);valid_standalone(r);SourceStandaloneEffectLight out;
    out.definition=r[1];out.birth=r[0x14/4];out.visited_epoch=r[3];out.registration=registration(r,id);
    out.forward=vector(r,0xac);out.up=vector(r,0xb8);
    out.state={scalar(r,0xc4),scalar(r,0xd0),vector(r,0xd4),vector(r,0xe0),scalar(r,0xec)};return out;
}
void source_standalone_effect_light_update(SourceEffectLightDatumPool& p,SourceEffectLightSpatialPool& spatial,
    const EffectLightDefinition& definition,std::uint32_t id,const SourceEffectLightLifetimeWorld& world) {
    auto& r=p.record(id);valid_standalone(r);
    const auto elapsed=std::bit_cast<std::int32_t>(world.tick-r[0x14/4]);
    const auto state=source_effect_light_state(definition,scalar(r,0xcc),elapsed,world.tick_rate,world.fade_curve);
    scalar(r,0xd0,state.control);vector(r,0xd4,state.diffuse);vector(r,0xe0,state.specular);
    scalar(r,0xec,state.brightness);scalar(r,0xc8,0);scalar(r,0xc4,state.size);
    const bool active=(flags(r)&1) && state.size>0 &&
        (state.diffuse.x!=0 || state.diffuse.y!=0 || state.diffuse.z!=0 ||
         state.specular.x!=0 || state.specular.y!=0 || state.specular.z!=0);
    if(!active) {
        if(flags(r)&2) {
            if(flags(r)&8) {auto s=registration(r,id);source_effect_light_unregister(s,spatial);registration(r,s);}
            flags(r,flags(r)&std::uint16_t(~2u));
        }
    } else if(!(flags(r)&2)) {
        flags(r,flags(r)|2);
        if(world.registration_enabled) {
            auto s=registration(r,id);source_effect_light_register(definition,s,world.locations,world.selected_bsp,
                world.memberships,spatial,world.terrain);registration(r,s);
        }
    }
}
std::uint32_t source_standalone_effect_light_create(SourceEffectLightDatumPool& p,SourceEffectLightSpatialPool& spatial,
    const EffectLightDefinition& d,std::uint32_t definition,float control,Vec3 position,Vec3 forward,Vec3 up,
    const SourceEffectLightLifetimeWorld& world) {
    const auto id=p.allocate();if(id==none)return id;auto& r=p.record(id);
    r[1]=definition;r[0x14/4]=world.tick;r[2]=r[0x4c/4]=none;r[0x54/4]=0xffffu;
    scalar(r,0xcc,control);scalar(r,0xd0,control);flags(r,d.duration>0?0x11:1);r[0x10/4]=none;
    vector(r,0x84,position);vector(r,0x90,{position.x+forward.x,position.y+forward.y,position.z+forward.z});
    vector(r,0xac,forward);vector(r,0xb8,up);r[0x58/4]=none;r[0x5c/4]=0xffffu;
    source_standalone_effect_light_update(p,spatial,d,id,world);r[3]=world.epoch-1;r[0x10c/4]=none;return id;
}
void source_standalone_effect_light_delete(SourceEffectLightDatumPool& p,SourceEffectLightSpatialPool& spatial,std::uint32_t id) {
    auto& r=p.record(id);valid_standalone(r);
    if((flags(r)&0xa)==0xa) {auto s=registration(r,id);source_effect_light_unregister(s,spatial);registration(r,s);}
    p.release(id);
}
void source_standalone_effect_lights_advance(SourceEffectLightDatumPool& p,SourceEffectLightSpatialPool& spatial,
    const std::function<const EffectLightDefinition&(std::uint32_t)>& definition,const SourceEffectLightLifetimeWorld& world) {
    // Snapshotting occupied identities preserves the source ascending iteration;
    // standalone update/deletion never allocates another light.
    for(const auto id:p.identities()) {
        const auto& d=definition(p.record(id)[1]);source_standalone_effect_light_update(p,spatial,d,id,world);
        const auto& r=p.record(id);const auto elapsed=std::bit_cast<std::int32_t>(world.tick-r[0x14/4]);
        if((flags(r)&0x10) && float(world.tick_rate)*d.duration<float(elapsed))
            source_standalone_effect_light_delete(p,spatial,id);
    }
}
}
