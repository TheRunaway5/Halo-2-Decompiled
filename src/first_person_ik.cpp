#include "halo2/first_person_ik.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/unit_animation_selection.hpp"
#include <algorithm>
#include <cmath>
#include <functional>

namespace halo2 {
namespace {
constexpr float epsilon=.0001f;
void normalize_source(Vec3 &v) {
    // 30BF0 accumulates and square-roots in x87 before storing the length
    // as a float. The subsequent reciprocal and products use scalar SSE.
    const auto x=static_cast<long double>(v.x),y=static_cast<long double>(v.y),z=static_cast<long double>(v.z);
    const auto extended_magnitude=std::sqrt((x*x+y*y)+z*z);
    const float magnitude=static_cast<float>(extended_magnitude);
    if(std::abs(extended_magnitude)>=epsilon) {
        const float inverse=1.f/magnitude;
        v={v.x*inverse,inverse*v.y,inverse*v.z};
    }
}
float squared_source(Vec3 p) { return (p.z*p.z+p.y*p.y)+p.x*p.x; }
Vec3 source_cross(Vec3 a,Vec3 b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}

} // namespace
Transform source_marker_transform(Vec3 position,const std::array<float,4> &q) {
    for(float f:q)if(!std::isfinite(f))throw FormatError("nonfinite first-person marker quaternion");
    const float norm=((q[0]*q[0]+q[1]*q[1])+q[2]*q[2])+q[3]*q[3];
    const float factor=norm>epsilon?2.f/norm:0.f;
    const float x=q[0]*factor,y=q[1]*factor,z=q[2]*factor;
    const float xx=q[0]*x,xy=q[0]*y,xz=q[0]*z,yy=q[1]*y,yz=q[1]*z,zz=q[2]*z;
    const float xw=x*q[3],yw=y*q[3],zw=z*q[3];
    return {1,{{{1.f-(zz+yy),xy+zw,xz-yw},
                 {xy-zw,1.f-(zz+xx),yz+xw},
                 {xz+yw,yz-xw,1.f-(yy+xx)}}},position};
}
bool source_two_bone_ik(Transform &root,Transform &middle,Transform &end,Transform target) {
    const float a2=squared_source(middle.position-root.position);
    const float b2=squared_source(end.position-middle.position);
    const float d2=squared_source(target.position-root.position);
    if(!(a2>epsilon && b2>epsilon && d2>epsilon))return false;
    const float a=std::sqrt(a2),b=std::sqrt(b2),distance=std::sqrt(d2);
    const float inverse=1.f/distance;
    const Vec3 delta=target.position-root.position;
    const Vec3 u={delta.x*inverse,delta.y*inverse,delta.z*inverse};
    Vec3 normal=source_cross(u,middle.position-root.position);
    normalize_source(normal);
    const Vec3 e=source_cross(normal,u);
    float d=distance;
    const float limit=(b+a)*.98f;
    if(limit<d) {target.position={limit*u.x+root.position.x,limit*u.y+root.position.y,limit*u.z+root.position.z};d=limit;}
    const float along=((d*d+a*a)-b*b)/(d*2.f),remaining=d-along;
    const float height2=a*a-along*along;
    const float height=height2<epsilon?0.f:std::sqrt(height2);
    const Vec3 lift={e.x*height,e.y*height,e.z*height};
    root.basis[0]={along*u.x+lift.x,along*u.y+lift.y,along*u.z+lift.z};
    normalize_source(root.basis[0]);
    root.basis[2]=source_cross(root.basis[0],root.basis[1]);
    normalize_source(root.basis[2]);
    root.basis[1]=source_cross(root.basis[2],root.basis[0]);
    const Vec3 new_middle={a*root.basis[0].x+root.position.x,root.basis[0].y*a+root.position.y,root.basis[0].z*a+root.position.z};
    middle.basis[0]={remaining*u.x-lift.x,remaining*u.y-lift.y,remaining*u.z-lift.z};
    normalize_source(middle.basis[0]);
    middle.basis[2]=source_cross(middle.basis[0],middle.basis[1]);
    normalize_source(middle.basis[2]);
    middle.basis[1]=source_cross(middle.basis[2],middle.basis[0]);
    middle.position=new_middle;end=target;
    return true;
}
void apply_first_person_ik(const FirstPersonIkDefinition &definition,
                          std::span<const int> parents,std::span<Transform> arms,
                          std::span<const Transform> weapon,float weight) {
    if(parents.size()!=arms.size() || !std::isfinite(weight))throw FormatError("invalid first-person IK frame");
    if(std::abs(weight)<epsilon)return; // 168644 admission, before 1D90E0.
    for(const auto &constraint:definition.constraints) {
        const auto n=constraint.arms_node;
        if(n>=arms.size() || constraint.weapon_node>=weapon.size())throw FormatError("first-person IK marker node outside model");
        const int p=parents[n];if(p<0)continue;
        if(std::size_t(p)>=arms.size())throw FormatError("first-person IK parent outside model");
        const int grandparent=parents[std::size_t(p)];if(grandparent<0)continue;
        if(std::size_t(grandparent)>=arms.size())throw FormatError("first-person IK grandparent outside model");
        auto target=compose_transform(compose_transform(weapon[constraint.weapon_node],constraint.weapon_local),inverse_transform(constraint.arms_local));
        const auto before=inverse_transform(arms[n]);
        if(std::abs(weight-1.f)>=epsilon) {
            const float remaining=1.f-weight;
            target.position={arms[n].position.x*remaining+target.position.x*weight,
                             arms[n].position.y*remaining+target.position.y*weight,
                             arms[n].position.z*remaining+target.position.z*weight};
        }
        source_two_bone_ik(arms[std::size_t(grandparent)],arms[std::size_t(p)],arms[n],target);
        const auto delta=compose_transform(arms[n],before);
        // 16D8D0 follows child/sibling links. Model node order is retained;
        // every descendant receives the same delta, without rebuilding binds.
        std::vector<unsigned> state(arms.size());
        std::function<bool(unsigned)> descendant=[&](unsigned i) {
            if(i==n)return true;
            if(state[i]==1)throw FormatError("cyclic first-person IK hierarchy");
            if(state[i]>=2)return state[i]==3;
            state[i]=1;const auto parent=parents[i];
            if(parent>=int(parents.size()))throw FormatError("first-person IK hierarchy outside model");
            const bool yes=parent>=0 && descendant(unsigned(parent));state[i]=yes?3:2;return yes;
        };
        for(unsigned i=0;i<arms.size();++i)if(i!=n && descendant(i))arms[i]=compose_transform(delta,arms[i]);
    }
}

FirstPersonIkDefinition Cache::load_first_person_ik(std::uint32_t graph,std::uint32_t weapon,
                                                   std::uint32_t arms,std::uint32_t mode,std::uint32_t weapon_class) const {
    const Bytes data(cache_bytes());
    const auto root=[&](std::uint32_t id,const char *group,std::size_t size) {
        const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
        if(!tag)throw FormatError("first-person IK source tag missing");
        return metadata_offset(tag->address,size);
    };
    const auto graph_root=root(graph,"jmad",0x3c),weapon_root=root(weapon,"mode",0x60),arms_root=root(arms,"mode",0x60);
    const auto marker=[&](std::size_t model,std::uint32_t name)->std::optional<std::pair<unsigned,Transform>> {
        if(!name || name==0xffffffffu)return {};
        const auto groups=TagAddressSpace(data, metadata_magic_).block(model+0x58, 12);
        const auto node_count=data.u32(model+0x48);
        for(unsigned i=0;i<groups.count;++i) {
            const auto p=groups.offset+i*12;if(data.u32(p)!=name)continue;
            const auto markers=TagAddressSpace(data, metadata_magic_).block(p+4, 36);if(!markers.count)return {};
            const auto q=markers.offset;const unsigned node=std::to_integer<unsigned>(data.slice(q+2,1)[0]);
            if(node>=node_count)throw FormatError("first-person IK authored marker outside nodes");
            const Vec3 position{data.f32(q+4),data.f32(q+8),data.f32(q+12)};
            return std::pair{node,source_marker_transform(position,{data.f32(q+16),data.f32(q+20),data.f32(q+24),data.f32(q+28)})};
        }
        return {};
    };
    const auto find_name=[&](TagBlockRange table,std::uint32_t name)->std::optional<std::size_t> {
        for(unsigned i=0;i<table.count;++i)if(data.u32(table.offset+i*20)==name)return table.offset+i*20;
        return {};
    };
    FirstPersonIkDefinition out;const auto modes=TagAddressSpace(data, metadata_magic_).block(graph_root+0x34, 20);
    const std::array<std::pair<std::uint32_t,std::uint32_t>,4> fallbacks{{{mode,weapon_class},{mode,0x030000d9u},{0x030000d9u,weapon_class},{0x030000d9u,0x030000d9u}}};
    unsigned index=0;
    for(const auto [m,c]:fallbacks) {
        const auto selected_mode=find_name(modes,m);
        const auto selected_class=selected_mode?find_name(TagAddressSpace(data, metadata_magic_).block(*selected_mode+4, 20),c):std::optional<std::size_t>{};
        if(!selected_class) {index=0;continue;}
        const auto pairs=TagAddressSpace(data, metadata_magic_).block(*selected_class+12, 8);
        for(;index<pairs.count;++index) {
            const auto p=pairs.offset+index*8;const auto a=data.u32(p),b=data.u32(p+4);
            const auto am=marker(arms_root,a),wm=marker(weapon_root,b);if(!am || !wm)continue;
            out.constraints.push_back({a,b,am->first,wm->first,am->second,wm->second});
        }
    }
    return out;
}

std::array<std::uint32_t,2> Cache::load_weapon_animation_identifiers(std::uint32_t weapon) const {
    const auto tag=query_tag({.handle=TagHandle{weapon}, .group=tag_groups::weapon, .require_address=true});
    if(!tag)throw FormatError("weapon animation identifier source tag missing");
    const auto p=metadata_offset(tag->address,0x290);const Bytes data(cache_bytes());
    return {data.u32(p+0x288),data.u32(p+0x28c)};
}

AnimationClip Cache::load_unit_weapon_action(std::uint32_t graph,std::uint32_t mode,
    std::uint32_t weapon_class,std::uint32_t weapon_type,std::uint32_t action,bool overlay) const {
    return load_unit_weapon_action(graph,mode,weapon_class,weapon_type,action,overlay,
        [this](auto pointer,auto bytes){return raw_data(pointer,bytes);});
}
AnimationClip Cache::load_unit_weapon_action(std::uint32_t graph,std::uint32_t mode,
    std::uint32_t weapon_class,std::uint32_t weapon_type,std::uint32_t action,bool overlay,
    const AnimationResourceReader &resources) const {
    const auto tag=query_tag({.handle=TagHandle{graph}, .group=tag_groups::model_animation_graph, .require_address=true});
    if(!tag)throw FormatError("unit action animation graph missing");
    if(weapon_class==0xffffffffu)throw FormatError("unit action has no weapon class");
    const Bytes data(cache_bytes());const auto root=metadata_offset(tag->address,0x54);
    SourceAnimationGraphMemory memory;
    memory.read=[&](std::uint32_t address,std::size_t extent) {
        return std::span<const std::byte>(cache_bytes()).subspan(metadata_offset(address,extent),extent);
    };
    weapon_class=source_animation_weapon_class(tag->address,weapon_class,weapon_type,memory);
    // Read-only authored lookup. Runtime resource admission remains with the
    // retained manager; this query does not supply readiness or demand success.
    //1CF4C0 exhausts overlay fallbacks before trying ordinary action rows.
    // Some full-body melee actions are authored only in that second bank.
    for(const unsigned table:{12u,4u}) {
     if(table==12 && !overlay)continue;
     std::array<std::byte,28> iterator{};
     const auto put=[&](unsigned at,std::uint32_t v){for(unsigned i=0;i<4;++i)iterator[at+i]=std::byte(v>>(i*8));};
     put(0,tag->address);put(4,mode);put(8,weapon_class);put(12,weapon_type);
     std::array<std::uint32_t,3> resolved{};
     while(const auto weapon=next_source_animation_weapon(iterator,resolved,memory)) {
        const auto row=find_source_animation_record(weapon+table,action,8,memory);if(!row)continue;
        const auto a=metadata_offset(row,8);
        const int inheritance=std::int16_t(data.u16(a+4)),index=std::int16_t(data.u16(a+6));
        if(index==-1)continue;
        if(index<0)throw FormatError("unit action animation index invalid");
        auto resolved_graph=graph;
        if(inheritance!=-1) {
            const auto inherited=TagAddressSpace(data, metadata_magic_).block(root+0x4c, 32, 64);
            if(inheritance<0 || std::size_t(inheritance)>=inherited.count)throw FormatError("unit action inherited graph outside list");
            resolved_graph=data.u32(inherited.offset+std::size_t(inheritance)*32+4);
        }
        auto clip=load_animation(resolved_graph,unsigned(index),resources);
        clip.source_identifier=data.u32(a+4);
        return clip;
     }
    }
    throw FormatError("authored unit weapon action missing: "+string_id(action));
}

void source_aim_blend_select(FirstPersonAimBlend &p,bool eligible,unsigned ticks_per_second) {
    // 1130F0 chooses .267/.06675 seconds; 1D9240 uses current game tick
    // rate and x87's nearest-even float-to-integer conversion.
    const float duration=eligible?.267f:.06675f;
    int n=int(std::nearbyint(float(ticks_per_second)*duration));n=std::clamp(n,1,127)-1;
    if(n<=0) {p.count=0;p.value=0;p.flags=0;return;}
    if(p.count && bool(p.flags&1)==eligible && n==p.count)return;
    int value;
    if(p.count) {const float denominator=std::max(float(p.count),1.f);value=int(float(p.value)/denominator*float(n-1));}
    else value=eligible?1:n-1;
    p.count=std::int8_t(n);p.value=std::int8_t(value);p.flags=eligible?1:0;
}
bool source_aim_blend_advance(FirstPersonAimBlend &p) {
    if(!p.count || (p.flags&2))return false;
    if(p.flags&1) {if(++p.value>=p.count) {p.flags|=2;p.value=p.count;}}
    else if(--p.value<=0) {p.flags|=2;p.value=0;}
    return true;
}
float source_aim_blend_weight(const FirstPersonAimBlend &p) {
    if(!p.count)return 0;
    float t=float(p.value)/std::max(float(p.count),1.f);
    if(p.curve==1) {
        // 10E8E0 accumulates the two polynomial terms separately in SSE.
        const float squared=t*t;
        t=squared*3.f-(squared*t)*2.f;
    } else if(p.curve==2) {
        const float t3=t*t*t,t4=t3*t;t=t4*t*6.f-t4*15.f+t3*10.f;
    } else if(p.curve==3) {t-=1.f;t=1.f-t*t;}
    else if(p.curve==4) {const float u=1.f-t;t=1.f-u*u*u;}
    return std::clamp(t,0.f,1.f);
}
bool source_weapon_aim_eligible(const FirstPersonAimEligibility &p) {
    constexpr std::array<std::uint32_t,27> admitted{
        0xffffffffu,0x0500000au,0x06000006u,0x06000007u,
        0x09000004u,0x09000005u,0x09000008u,0x09000009u,
        0x0a000040u,0x0a0005b9u,0x0a0005bbu,0x0b0005b2u,
        0x0c000073u,0x0c000075u,0x0c000077u,0x0c0006b3u,0x0c0006cdu,
        0x0e000038u,0x0e000607u,0x0e000608u,0x0e000609u,0x0e00060au,0x0e00067du,
        0x11000074u,0x11000076u,0x130005bcu,0x140005b3u};
    constexpr std::array<std::uint32_t,19> denied{
        // C7070's five-entry source table, followed by 1112F0's state cases.
        0x05000049u,0x07000679u,0x0700067au,0x0700067bu,0x0700067cu,
        0x0400004au,0x0600008cu,0x0a00003eu,0x0b000059u,0x0b00005au,
        0x0c000043u,0x0c000058u,0x0c00005bu,0x0d00003fu,0x0d000042u,
        0x0e00003bu,0x0e0000c2u,0x0f00003au,0x1000003cu};
    return !p.dual_wield &&
        std::find(admitted.begin(),admitted.end(),p.secondary_action)!=admitted.end() &&
        std::find(denied.begin(),denied.end(),p.base_state)==denied.end() &&
        !((p.base_animation_flags|p.secondary_animation_flags)&8);
}
} // namespace halo2
