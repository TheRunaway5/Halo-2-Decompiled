#include "halo2/source_shadow.hpp"
#include "halo2/transform.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 subtract(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 scale(Vec3 a,float b){return {a.x*b,a.y*b,a.z*b};}
float magnitude(Vec3 a){return std::sqrt((a.x*a.x+a.y*a.y)+a.z*a.z);}
Vec3 normalized(Vec3 a){const auto m=float(std::sqrt((double(a.x)*a.x+double(a.y)*a.y)+double(a.z)*a.z));return std::abs(m)>=0.0001f?scale(a,1.f/m):a;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
ObjectAttachmentFrame normalize_attachment_frame(ObjectAttachmentFrame result,ObjectAttachmentFrame current){
    result.up=cross(cross(result.forward,result.up),result.forward);
    const auto normalize=[](Vec3&v){
        const auto length=std::sqrt((double(v.x)*v.x+double(v.y)*v.y)+double(v.z)*v.z);
        if(length<.0001f)return false;
        const auto inverse=1.f/float(length);v={v.x*inverse,inverse*v.y,inverse*v.z};return true;
    };
    if(!normalize(result.forward)){result.forward=current.forward;result.up=current.up;}
    else if(!normalize(result.up)){
        // 11D090 uses the horizontal perpendicular, or source axis440B30.
        Vec3 perpendicular{result.forward.y,-result.forward.x,0};
        if(!normalize(perpendicular))perpendicular={1,0,0};
        result.up=cross(perpendicular,result.forward);
    }
    return result;
}
}
SourceShadowUniforms source_shadow_uniforms(const SourceShadowFrame &frame,bool shadow_depth_feature){
    SourceShadowUniforms result;result.opacity=frame.opacity;
    const auto d=frame.direction;
    const float half_tangent=float(std::tan(double(frame.cone_angle)*0.5));
    const float inverse=-1.f/(frame.far_offset-frame.near_offset);
    const float tangent_scale=-1.f/half_tangent,half_inverse=inverse*0.5f;
    auto &p=result.projection;
    p[0]={frame.virtual_eye.x,frame.virtual_eye.y,frame.virtual_eye.z,1};
    p[1]={frame.virtual_eye.x,frame.virtual_eye.y,frame.virtual_eye.z,0};
    p[2]={((d.y*frame.up.z-d.z*frame.up.y)*tangent_scale+d.x)*half_inverse,
          ((d.z*frame.up.x-d.x*frame.up.z)*tangent_scale+d.y)*half_inverse,
          ((d.x*frame.up.y-d.y*frame.up.x)*tangent_scale+d.z)*half_inverse,0};
    // C1A80's plane basis is direction × up, opposite the receiver OBB left.
    const auto plane_left=cross(d,frame.up);
    p[3]={half_inverse*((d.y*plane_left.z-d.z*plane_left.y)*tangent_scale+d.x),
          ((d.z*plane_left.x-plane_left.z*d.x)*tangent_scale+d.y)*half_inverse,
          ((plane_left.y*d.x-d.y*plane_left.x)*tangent_scale+d.z)*half_inverse,0};
    p[4]={inverse*d.x,d.y*inverse,d.z*inverse,
          (frame.near_offset+frame.virtual_distance)*inverse};
    const float attenuation=(frame.far_offset-frame.near_offset)*0.02500000037252903f*0.5f;
    p[5]={p[4][0]*attenuation,attenuation*p[4][1],p[4][2]*attenuation,
          (frame.virtual_distance+0.0078125f)*attenuation*inverse+0.5f};
    auto &c=result.constants;
    const auto far_inverse=frame.far_offset!=0?1.f/frame.far_offset:-3.4028234663852886e38f;
    c[0]={d.x*far_inverse,d.y*far_inverse,d.z*far_inverse,0.f-far_inverse*frame.virtual_distance};
    c[1]={0.f-d.x,0.f-d.y,0.f-d.z,0};
    c[2]={128,128,16777215,1};
    const float kernel=shadow_depth_feature?0.f:0.75f;
    c[3]={kernel+0.5f,0.5f-kernel,0,0};
    const float plane=((0.f-d.z)*(d.z*0.01f+frame.center.z)+(0.f-d.y)*(d.y*0.01f+frame.center.y))+
                       (0.f-d.x)*(d.x*0.01f+frame.center.x);
    c[4]={(0.f-d.x)*100.f,(0.f-d.y)*100.f,(0.f-d.z)*100.f,plane*-100.f};
    const float depth_inverse=frame.near_offset<frame.far_offset?1.f/(frame.far_offset-frame.near_offset):1000.f;
    const float depth_plane=(d.z*(d.z*frame.near_offset+frame.center.z)+d.y*(d.y*frame.near_offset+frame.center.y))+
                             d.x*(d.x*frame.near_offset+frame.center.x);
    c[5]={d.x*depth_inverse,d.y*depth_inverse,d.z*depth_inverse,(0.f-depth_plane)*depth_inverse};
    c[6]=p[2];c[7]=p[3];c[8]={1,1,0,0};return result;
}
ShadowCasterSphere source_shadow_sphere(const ShadowCasterDefinition &definition,
    const Transform &node0,float object_scale){
    auto offset=definition.bounding_offset;
    if(node0.scale!=1.f){offset.x*=node0.scale;offset.y*=node0.scale;offset.z*=node0.scale;}
    ShadowCasterSphere result;
    result.center.x=((node0.basis[2].x*offset.z+node0.basis[1].x*offset.y)+node0.basis[0].x*offset.x)+node0.position.x;
    result.center.y=((node0.basis[2].y*offset.z+node0.basis[1].y*offset.y)+node0.basis[0].y*offset.x)+node0.position.y;
    result.center.z=((node0.basis[2].z*offset.z+node0.basis[1].z*offset.y)+node0.basis[0].z*offset.x)+node0.position.z;
    result.radius=object_scale*definition.bounding_radius;
    return result;
}
Transform source_object_transform(Vec3 position,Vec3 forward,Vec3 up,
    float object_scale,bool mirrored,const Transform *parent_node,bool parent_mirrored){
    auto rotation=camera_transform({},forward,up);
    if(mirrored){auto&left=rotation.basis[1];left={0.f-left.x,0.f-left.y,0.f-left.z};}
    if(object_scale!=1.f){Transform scaling;scaling.scale=object_scale;
        rotation=compose_transform(scaling,rotation);}
    Transform translation;translation.position=position;
    if(parent_node){
        auto parent=*parent_node;
        if(parent.scale!=1.f || parent_mirrored){
            if(parent.scale!=1.f){translation.position=scale(translation.position,parent.scale);parent.scale=1.f;}
            if(parent_mirrored){auto&left=parent.basis[1];left={0.f-left.x,0.f-left.y,0.f-left.z};}
        }
        translation=compose_transform(parent,translation);
    }
    return compose_transform(translation,rotation);
}
WeaponAttachmentMarkers select_weapon_attachment(const WeaponAttachmentDefinition &definition,
    std::span<const std::uint8_t> parent_permutations,std::span<const std::uint8_t> child_permutations){
    const auto first=[](const auto&markers,std::span<const std::uint8_t> permutations)->const ObjectAttachmentMarker*{
        for(const auto&m:markers){
            if(permutations.empty() || m.region==-1)return &m;
            if(m.region<0 || std::size_t(m.region)>=permutations.size())
                throw FormatError("attachment marker region outside runtime permutations");
            if(permutations[unsigned(m.region)]==std::uint8_t(m.permutation))return &m;
        }
        return nullptr;
    };
    WeaponAttachmentMarkers result;
    if(const auto p=first(definition.parent,parent_permutations))result.parent=*p;
    if(const auto p=first(definition.preferred_child,child_permutations)){result.child=*p;result.named_child=true;}
    else {
        if(const auto p=first(definition.child,child_permutations))result.child=*p;
        result.named_child=definition.child_marker && definition.child_marker!=0xffffffffu;
    }
    return result;
}
ObjectAttachmentFrame source_object_attachment_frame(ObjectAttachmentFrame current,
    const Transform &parent_world,const Transform &child_marker,bool named_child){
    ObjectAttachmentFrame result;
    if(named_child){
        const auto current_matrix=camera_transform(current.position,current.forward,current.up);
        const auto child_local=compose_transform(inverse_transform(current_matrix),child_marker);
        const auto frame=compose_transform(parent_world,inverse_transform(child_local));
        result={frame.position,frame.basis[0],frame.basis[2]};
    }else{
        const auto child=inverse_transform(child_marker);
        const auto rotate=[&](Vec3 v){return Vec3{
            (child.basis[2].x*v.z+child.basis[1].x*v.y)+child.basis[0].x*v.x,
            (child.basis[2].y*v.z+child.basis[1].y*v.y)+child.basis[0].y*v.x,
            (child.basis[1].z*v.y+child.basis[2].z*v.z)+child.basis[0].z*v.x};};
        auto p=parent_world.position;if(child.scale!=1)p=scale(p,child.scale);
        result.position=add(rotate(p),child.position);
        result.position.z=((child.basis[1].z*p.y+child.basis[0].z*p.x)+child.basis[2].z*p.z)+child.position.z;
        result.forward=rotate(parent_world.basis[0]);result.up=rotate(parent_world.basis[2]);
    }
    return normalize_attachment_frame(result,current);
}
ObjectAttachmentFrame source_object_attachment_local_frame(ObjectAttachmentFrame world,
    const Transform &parent_node){
    const auto inverse=inverse_transform(parent_node);
    const auto rotate=[&](Vec3 v){return Vec3{
        (inverse.basis[2].x*v.z+inverse.basis[1].x*v.y)+inverse.basis[0].x*v.x,
        (inverse.basis[2].y*v.z+inverse.basis[1].y*v.y)+inverse.basis[0].y*v.x,
        (inverse.basis[2].z*v.z+inverse.basis[1].z*v.y)+inverse.basis[0].z*v.x};};
    auto position=world.position;if(inverse.scale!=1.f)position=scale(position,inverse.scale);
    return normalize_attachment_frame({add(rotate(position),inverse.position),rotate(world.forward),rotate(world.up)},world);
}
std::optional<AnimationClip> Cache::load_initialized_object_idle(std::uint32_t object)const {
    const Bytes data(storage_);
    const auto tag_root=[&](std::uint32_t datum,const char*group,std::size_t size){
        const auto tag=query_tag({.handle=TagHandle{datum}, .group=group ? std::optional<TagGroup>{TagGroup::from_string(group)} : std::nullopt, .require_address=true});
        if(!tag)throw FormatError("initialized object animation source tag missing");
        if(!group && (tag->group=="mode" || tag->group=="hlmt" || tag->group=="jmad"))
            throw FormatError("initialized animation requires an object definition");
        return metadata_offset(tag->address,size);
    };
    const auto object_root=tag_root(object,nullptr,0x3c);
    const auto model=data.u32(object_root+0x38);if(model==0xffffffffu)return {};
    const auto model_root=tag_root(model,"hlmt",0x7c);
    const auto graph=data.u32(model_root+0x14);if(graph==0xffffffffu)return {};
    const auto root=tag_root(graph,"jmad",0x5c);
    // B7B40 passes the hlmt datum to 1CB0D0, whose compatibility check
    // rejects controller allocation before the later 10A3F0 named override.
    if(std::int16_t(data.u16(root+0xc))!=std::bit_cast<std::int32_t>(data.u32(model_root+0x78)))return {};

    // 1DD560 uses signed StringID ordering and returns its first binary-search
    // match. It does not select a positive-weight preview alternative.
    const auto find=[&](TagBlockRange table,std::size_t stride,std::uint32_t name)->std::optional<std::size_t>{
        int low=0,high=int(table.count)-1;
        while(low<=high){const int index=(low+high)>>1;const auto at=table.offset+std::size_t(index)*stride;
            const auto candidate=data.u32(at);if(candidate==name)return at;
            if(std::bit_cast<std::int32_t>(name)<std::bit_cast<std::int32_t>(candidate))high=index-1;
            else low=index+1;
        }
        return {};
    };
    const auto inherited=metadata_block(root+0x4c, 32);
    const auto resolve_graph=[&](int inheritance){
        if(inheritance==-1)return graph;
        if(inheritance<0 || unsigned(inheritance)>=inherited.count)
            throw FormatError("initialized idle inherited graph outside source bank");
        const auto id=data.u32(inherited.offset+unsigned(inheritance)*32+4);
        if(id==0xffffffffu)throw FormatError("initialized idle references NONE inherited graph");
        (void)tag_root(id,"jmad",0x34);return id;
    };
    constexpr std::uint32_t idle=0x0400000cu,any=0x030000d9u,combat=0x06000086u,unarmed=0x07000083u;
    // The type6 SCEN constructor10A1B0 invokes 10A3F0's named override.
    // Weapons use their default/dual action producer instead. 1DD0B0 scans
    // the root then inherited graphs, stopping at the first NONE graph.
    const auto named=[&](std::uint32_t id,std::size_t at)->std::optional<AnimationClip>{
        const auto clips=metadata_block(at+0x2c, 0x6c);
        for(unsigned i=0;i<clips.count;++i)if(data.u32(clips.offset+i*0x6c)==idle)return load_animation(id,i);
        return {};
    };
    if(data.u16(object_root)==6){
        if(auto clip=named(graph,root))return clip;
        for(unsigned i=0;i<inherited.count;++i){
            const auto id=data.u32(inherited.offset+i*32+4);if(id==0xffffffffu)break;
            if(auto clip=named(id,tag_root(id,"jmad",0x34)))return clip;
        }
    }
    // 1CB0D0→1CC2F0: default resolves to combat/unarmed/any/idle. The
    // graph's type→class alias can replace unarmed before category fallback.
    auto weapon_class=unarmed;
    if(const auto alias=find(metadata_block(root+0x54, 8),8,any)){
        const auto value=data.u32(*alias+4);if(value!=0xffffffffu && value!=any)weapon_class=value;
    }
    const auto modes=metadata_block(root+0x34, 20);
    for(unsigned step=0;step<8;++step){
        if((step&1) || ((step&2) && weapon_class==any))continue;
        const auto mode=find(modes,20,step&4?any:combat);if(!mode)continue;
        const auto cls=find(metadata_block(*mode+4, 20),20,step&2?any:weapon_class);if(!cls)continue;
        const auto type=find(metadata_block(*cls+4, 0x34),0x34,any);if(!type)continue;
        const auto action=find(metadata_block(*type+4, 8),8,idle);if(!action)continue;
        const auto index=std::int16_t(data.u16(*action+6));if(index==-1)continue;
        if(index<0)throw FormatError("initialized idle animation index invalid");
        return load_animation(resolve_graph(std::int16_t(data.u16(*action+4))),unsigned(index));
    }
    return {};
}
WeaponAttachmentDefinition Cache::load_weapon_attachment(std::uint32_t unit,
    std::uint32_t weapon,unsigned hand)const {
    if(hand>1)throw FormatError("source weapon attachment hand outside slots");
    const Bytes data(storage_);
    const auto tag_root=[&](std::uint32_t id,const char*group,std::size_t size){
        const auto tag=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
        if(!tag)throw FormatError("weapon attachment source tag missing");
        return metadata_offset(tag->address,size);
    };
    const auto unit_root=tag_root(unit,"bipd",0x164),weapon_root=tag_root(weapon,"weap",0x288);
    const auto render_model=[&](std::size_t root){
        const auto model=data.u32(root+0x38);
        if(model==0xffffffffu)return std::optional<std::size_t>{};
        const auto model_root=tag_root(model,"hlmt",8);
        const auto render=data.u32(model_root+4);
        if(render==0xffffffffu)return std::optional<std::size_t>{};
        return std::optional(tag_root(render,"mode",0x60));
    };
    const auto unit_model=render_model(unit_root),weapon_model=render_model(weapon_root);
    const auto markers=[&](std::optional<std::size_t> model,std::uint32_t name){
        std::vector<ObjectAttachmentMarker> result;
        if(!model || !name || name==0xffffffffu)return result;
        const auto node_count=data.u32(*model+0x48),group_count=data.u32(*model+0x58);
        if(group_count>65536)throw FormatError("excessive attachment marker groups");
        if(!group_count)return result;
        const auto groups=metadata_offset(data.u32(*model+0x5c),std::size_t(group_count)*12);
        for(unsigned group=0;group<group_count;++group){
            const auto p=groups+group*12;if(data.u32(p)!=name)continue;
            const auto count=data.u32(p+4);if(count>65536)throw FormatError("excessive attachment markers");
            if(!count)break;
            const auto start=metadata_offset(data.u32(p+8),std::size_t(count)*36);
            for(unsigned i=0;i<count;++i){
                const auto q=start+i*36;
                const auto bytes=data.slice(q,3);
                ObjectAttachmentMarker marker;
                marker.region=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[0]));
                marker.permutation=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(bytes[1]));
                marker.node=std::to_integer<std::uint8_t>(bytes[2]);
                if(marker.node>=node_count)throw FormatError("attachment marker node outside source model");
                marker.local=source_marker_transform({data.f32(q+4),data.f32(q+8),data.f32(q+12)},
                    {data.f32(q+16),data.f32(q+20),data.f32(q+24),data.f32(q+28)});
                marker.authored_scale=data.f32(q+32);result.push_back(marker);
            }
            break;
        }
        return result;
    };
    WeaponAttachmentDefinition result;
    result.parent_marker=data.u32(unit_root+0x158+hand*4);
    result.preferred_child_marker=data.u32(unit_root+0x160);
    result.child_marker=data.u32(weapon_root+0x284);
    result.parent=markers(unit_model,result.parent_marker);
    result.preferred_child=markers(weapon_model,result.preferred_child_marker);
    result.child=markers(weapon_model,result.child_marker);
    return result;
}
ShadowCasterDefinition Cache::load_object_shadow_definition(std::uint32_t object)const {
    const auto tag=query_tag({.handle=TagHandle{object}});
    if(!tag)throw FormatError("shadow caster object tag missing");
    const Bytes data(storage_);const auto root=metadata_offset(tag->address,tag->size);
    data.require(root,0x3c);
    ShadowCasterDefinition result;result.name=tag->name;
    result.bounding_radius=data.f32(root+4);
    result.bounding_offset={data.f32(root+8),data.f32(root+12),data.f32(root+16)};
    result.mode=data.u16(root+0x18);result.model=data.u32(root+0x38);
    if(result.model!=0xffffffffu){
        const auto model=query_tag({.handle=TagHandle{result.model}, .group=tag_groups::model});
        if(!model)throw FormatError("shadow caster model tag missing");
        const auto at=metadata_offset(model->address,model->size);data.require(at,0x4e);
        result.model_present=true;result.fade_end=data.f32(at+0x28);result.fade_start=data.f32(at+0x2c);
        for(unsigned i=0;i<5;++i)result.render_cutoffs[i]=data.f32(at+0x34+i*4);
        result.render_lod_count=data.u16(at+0x4c);
    }
    return result;
}
std::uint8_t source_shadow_distance_fade_byte(const ShadowCasterDefinition &definition,float draw_metric,float current_radius,bool force_highest_lod){
    if(force_highest_lod)return 255;
    if(!definition.model_present)return 0;
    const auto index=4-int(definition.render_lod_count);
    const float cutoff=index>0?definition.render_cutoffs[unsigned(std::min(index,4))]:3.4028234663852886e38f;
    const float width=current_radius*7.f;
    float fade=1;
    if(draw_metric>=cutoff+width)fade=0;
    else if(draw_metric>cutoff){
        fade=std::clamp(1.f-(draw_metric-cutoff)/width,0.f,1.f);
        if(fade<=0.0949999988079071f)fade=0;
    }
    // Original __ftol2 truncates its x87 255 multiplication toward zero.
    return std::uint8_t(static_cast<int>(double(fade)*255.0)&255);
}
float source_shadow_distance_fade(const ShadowCasterDefinition &definition,float draw_metric,float current_radius,bool force_highest_lod){
    return float(source_shadow_distance_fade_byte(definition,draw_metric,current_radius,force_highest_lod))*0.003921568859368563f;
}
SourceShadowRootFade source_shadow_root_fade(SourceShadowRootFadeInput input){
    bool eligible=!input.no_shadow && (input.definition_mode==2 ||
        (input.definition_mode==0 && (input.object_type==0 || input.object_type==1)));
    float fade=eligible?float(input.model_fade)*0.003921568859368563f:0.f;
    eligible=eligible && fade>0;
    if(eligible && input.object_type==0){
        if(input.dying){fade=std::clamp(1.f-(input.seconds_since_death-2.f),0.f,1.f);eligible=fade>0;}
        // The first camouflage factor is skipped for invalid source values.
        if(input.camouflage>=0 && input.camouflage<=1)fade=(1.f-input.camouflage)*fade;
    }
    if(input.header_kind<2){
        const auto camo=std::clamp(input.camouflage,0.f,1.f);
        fade=(1.f-camo)*fade;if(fade<=0)eligible=false;
    }
    return {eligible,fade};
}
float source_shadow_coverage(Vec3 center,float radius,ShaderVector plane,float pixel_scale){
    float depth=(plane[2]*center.z+plane[0]*center.x)+plane[1]*center.y;
    depth+=plane[3];
    if(depth<0)depth=0.f-depth;
    if(depth<=0.10000000149011612f)depth=0.10000000149011612f;
    return ((pixel_scale/depth)*radius)*2.f;
}
std::vector<SourceShadowCandidate> source_shadow_select(SourceShadowSelectionCache &cache,
    std::span<const SourceShadowCandidate> candidates,std::int32_t frame,unsigned preferred,
    float acceleration,bool cinematic_bypass){
    static_assert(sizeof(SourceShadowCandidate)==20 && sizeof(SourceShadowSelectionEntry)==36);
    if(cache.count>cache.entries.size() || candidates.size()>128)
        throw FormatError("original light selection bank exceeds source bounds");
    auto& entries=cache.entries;
    // 4A7C0 retains the matching record's opacity/slope and its creation frame.
    std::uint32_t matched=0;
    for(unsigned i=0;i<cache.count;++i)for(unsigned j=0;j<candidates.size();++j)
        if(entries[i].candidate.identity==candidates[j].identity){
            entries[i].candidate=candidates[j];entries[i].last_seen_frame=frame;
            matched|=1u<<(j&31);break;
        }
    for(unsigned i=0;i<cache.count;){
        if(entries[i].last_seen_frame!=frame)entries[i]=entries[--cache.count];
        else ++i;
    }
    for(unsigned i=0;i<candidates.size();++i){
        const auto bit=1u<<(i&31);
        if(cache.count<32 && !(matched&bit)){
            entries[cache.count++]={candidates[i],0,candidates[i].requested_fade,frame,frame};
            matched|=bit;
        }
    }
    const auto flag=[](const auto&e,unsigned byte){return ((e.candidate.flags>>(byte*8))&255)!=0;};
    if(cache.count && !cinematic_bypass){
        const unsigned budget=std::min(preferred,cache.count);
        int last=int(cache.count)-1;
        unsigned priority=0;bool special=false;
        // The first source partition repeatedly swaps into slot0 and excludes
        // the last slot from this special flags0+1 case.
        for(int i=0;i<=last;++i)if(i<last && flag(entries[i],0) && flag(entries[i],1)){
            std::swap(entries[0],entries[i]);++priority;special=true;
        }
        if(!special)for(int i=int(priority);i<=last;++i)if(flag(entries[i],0)){
            std::swap(entries[priority],entries[i]);++priority;break;
        }
        if(preferred<cache.count)for(int i=int(priority);i<=last;){
            if(flag(entries[i],2) && entries[i].created_frame==frame){
                entries[i].fade=0;std::swap(entries[i],entries[last--]);
            }else ++i;
        }
        // 4B160 uses repeated adjacent exchanges with the same starting index.
        // Preserve the order of equal values and the source swap sequence.
        const auto sort=[&](unsigned mode){
            for(int pass=int(priority);pass<=last;++pass)
                for(int j=int(priority)+1;j<=last;++j){
                    bool exchange=mode==1?entries[j-1].candidate.owner<entries[j].candidate.owner:
                        entries[j-1].candidate.coverage<entries[j].candidate.coverage;
                    if(exchange)std::swap(entries[j-1],entries[j]);
                }
        };
        sort(1);
        for(int i=int(priority);i<=last;++i){
            if(i>0 && entries[i].candidate.owner!=-1 &&
                entries[i].candidate.owner==entries[i-1].candidate.owner && !flag(entries[i],3)){
                entries[i].fade=0;entries[i].slope=0.f-acceleration;
                std::swap(entries[i],entries[last--]);
            }
        }
        sort(2);
        if(preferred<cache.count)for(int i=int(cache.count)-1;i>=int(budget);--i){
            auto&e=entries[i];
            if(flag(e,3) || e.fade<=0)e.slope=0;
            else{e.slope-=acceleration;e.fade=std::clamp(e.fade+e.slope,0.f,e.candidate.requested_fade);}
        }
        for(unsigned i=0;i<budget;++i){
            auto&e=entries[i];
            if(e.fade>=e.candidate.requested_fade)e.slope=0;
            else{e.slope+=acceleration;e.fade=std::clamp(e.fade+e.slope,0.f,e.candidate.requested_fade);}
        }
    }
    std::vector<SourceShadowCandidate> output;output.reserve(cache.count);
    for(unsigned i=0;i<cache.count;++i){
        output.push_back(entries[i].candidate);output.back().requested_fade=entries[i].fade;
    }
    return output;
}
std::array<float,29> SourceShadowFrame::source_values()const {
    return {center.x,center.y,center.z,direction.x,direction.y,direction.z,
      up.x,up.y,up.z,left.x,left.y,left.z,virtual_eye.x,virtual_eye.y,virtual_eye.z,
      volume_center.x,volume_center.y,volume_center.z,
      volume_half_extent.x,volume_half_extent.y,volume_half_extent.z,
      transverse_radius,axial_radius,width,near_offset,far_offset,virtual_distance,cone_angle,opacity};
}
std::optional<SourceShadowFrame> source_shadow_frame(ShadowCasterSphere root,
    std::span<const ShadowCasterSphere> hierarchy,Vec3 shadow_direction,float cached_opacity,float runtime_fade){
    const auto opacity=cached_opacity*runtime_fade;
    if(cached_opacity<=0 || opacity<=0.09f)return {};
    auto transverse=std::max(root.radius,0.0002f),axial=transverse;
    for(const auto &sphere:hierarchy){
        const auto delta=subtract(sphere.center,root.center);
        // Source 31F50 consumes the incident direction before 32350 normalizes it.
        const auto along=(shadow_direction.z*delta.z+shadow_direction.y*delta.y)+shadow_direction.x*delta.x;
        const auto projected=scale(shadow_direction,along);
        transverse=std::max(transverse,sphere.radius+magnitude(subtract(delta,projected)));
        axial=std::max(axial,sphere.radius+magnitude(projected));
    }
    SourceShadowFrame result;result.center=root.center;result.direction=normalized(shadow_direction);
    const auto horizontal=std::max(std::abs(result.direction.x),std::abs(result.direction.y));
    const auto end=axial*2+horizontal;
    result.near_offset=end-axial;result.far_offset=(end+axial)-axial;
    result.transverse_radius=transverse;result.axial_radius=axial;result.width=transverse*2;
    const Vec3 axis=std::abs(result.direction.y)>=0.9f?Vec3{-1,0,0}:Vec3{0,1,0};
    result.up=normalized(cross(result.direction,axis));result.left=cross(result.up,result.direction);
    result.cone_angle=0.0055555556900799274f; // Authored fixed definition 47FDA0 +0x28.
    const auto half_tangent=float(std::tan(double(result.cone_angle)*0.5));
    result.virtual_distance=(result.width/half_tangent)*0.5f;
    result.virtual_eye=add(scale(result.direction,-result.virtual_distance),result.center);
    const auto half_depth=result.far_offset*0.5f;
    // Preserve 32350's operation order even when duplicated centers cancel.
    result.volume_center={
      (result.center.x-(result.direction.x*-half_depth+result.center.x))+result.center.x,
      result.center.y+(result.center.y-(result.direction.y*-half_depth+result.center.y)),
      (result.center.z-(result.direction.z*-half_depth+result.center.z))+result.center.z};
    result.volume_half_extent={half_depth,result.width*0.5f,result.width*0.5f};
    result.opacity=opacity;return result;
}
}
