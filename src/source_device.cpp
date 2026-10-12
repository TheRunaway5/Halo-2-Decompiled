#include "halo2/source_device.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <tuple>

namespace halo2 {
BonePose source_device_overlay(BonePose base,BonePose delta,unsigned components){
    if(components&~7u)throw FormatError("invalid source device overlay mask");
    if(components&1){
        const auto a=base.rotation,d=delta.rotation;
        // Preserve 2818E0's multiply/add/subtract order, with no final
        // quaternion normalization until the source hierarchy conversion.
        base.rotation={((a[3]*d[0]+d[3]*a[0])+d[2]*a[1])-a[2]*d[1],
                       ((a[3]*d[1]+d[3]*a[1])+d[0]*a[2])-a[0]*d[2],
                       ((a[3]*d[2]+d[3]*a[2])+d[1]*a[0])-a[1]*d[0],
                       ((a[3]*d[3]+-(d[0]*a[0]))+-(d[1]*a[1]))-a[2]*d[2]};
    }
    if(components&2)base.translation={base.translation.x+delta.translation.x,
        base.translation.y+delta.translation.y,base.translation.z+delta.translation.z};
    if(components&4)base.scale=base.scale*delta.scale;
    return base;
}
float source_device_channel_frame(unsigned count,float position){
    if(!count || count>32767 || !std::isfinite(position))throw FormatError("invalid source device channel phase");
    constexpr float epsilon=0.00009999999747378752f; // DAT45DBDC.
    const auto end=float(count-1)+epsilon;
    return std::min(std::clamp(end*position,0.f,end),float(count)-epsilon);
}
SourceMachineGearStep source_machine_gear_step(const SourceMachineDefinition&d,float position,float power,float dt){
    if(d.type!=2 || !std::isfinite(position) || !std::isfinite(power) || !std::isfinite(dt) || dt<0 ||
       !std::isfinite(d.powered_position_rate) || !std::isfinite(d.unpowered_position_rate))
        throw FormatError("invalid source gear tick");
    SourceMachineGearStep result;result.position=position;
    // 11541A..115445 uses x87 at the source's 53-bit precision, then
    // stores its rate to float before the SSE tick multiply/add.
    result.rate=float(double(d.powered_position_rate)*double(power)+
        (1.0-double(power))*double(d.unpowered_position_rate));
    if(std::abs(result.rate)<=0.00009999999747378752f)return result;
    result.changed=true;result.position=dt*result.rate+position;
    result.wrapped=1.f<=result.position;
    if(result.wrapped)result.position=result.position-1.f;
    return result;
}
std::uint32_t source_machine_placement_flags(std::uint32_t v){
    return (v&1)|((v&0x20)>>4)|((v&2)<<1)|((v&0x10)>>1)|((v&4)<<2)|((v&8)<<2);
}
std::uint32_t source_machine_object_flags(std::uint32_t existing,const SourceMachineDefinition&d){
    existing|=0x800u;
    return d.machine_flags&4?existing|0x3000u:existing&~0x3000u;
}
std::vector<BonePose> source_machine_local_pose(const RenderModel&model,const SourceMachineDefinition&d,
    float position,float power,float base_frame){
    if(!std::isfinite(base_frame))throw FormatError("invalid source machine primary phase");
    std::vector<BonePose> locals;for(const auto&bone:model.bones)locals.push_back(bone.bind);
    const auto apply=[&](const AnimationClip&clip,float frame,bool overlay){
        if(clip.nodes.size()!=clip.components.size())throw FormatError("invalid source device channel nodes");
        for(unsigned i=0;i<model.bones.size();++i){
            const auto node=std::find(clip.nodes.begin(),clip.nodes.end(),model.bones[i].name);
            if(node==clip.nodes.end())throw FormatError("device graph lacks model node");
            const auto index=std::size_t(node-clip.nodes.begin());
            const auto sampled=sample_animation(clip,index,frame,overlay?BonePose{}:locals[i]);
            locals[i]=overlay?source_device_overlay(locals[i],sampled,clip.components[index]):sampled;
        }
    };
    if(d.initialized_idle)apply(*d.initialized_idle,base_frame,false);
    const auto channel=[&](const std::optional<SourceDeviceChannel>&c,float value){
        if(!c)return;
        if(c->animation_type>1)throw FormatError("unsupported source device animation type");
        apply(c->clip,source_device_channel_frame(c->clip.frame_count,value),c->animation_type==1);
    };
    channel(d.position,position);channel(d.power,power);return locals;
}

SourceMachineDefinition Cache::load_source_machine_definition(std::uint32_t datum)const {
    const auto tag=query_tag({.handle=TagHandle{datum}});
    if(!tag || tag->group!="mach")throw FormatError("source machine tag missing");
    return load_source_device_pose_definition(datum);
}
SourceMachineDefinition Cache::load_source_device_pose_definition(std::uint32_t datum)const {
    const Bytes data(storage_);
    const auto tag_root=[&](std::uint32_t id,const char*group,std::size_t size){
        const auto t=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
        if(!t)throw FormatError("source machine tag missing");
        return metadata_offset(t->address,size);
    };
    const auto tag=query_tag({.handle=TagHandle{datum}});
    if(!tag || (tag->group!="mach" && tag->group!="ctrl"))
        throw FormatError("source device pose requires mach or ctrl");
    const auto p=tag_root(datum,tag->group.c_str(),0x120);
    SourceMachineDefinition result;
    result.name=query_tag({.handle=TagHandle{datum}})->name;
    if(data.u16(p)!=(tag->group=="mach"?7:8))throw FormatError("source device object type mismatch");
    result.model=data.u32(p+0x38);
    if(tag->group=="mach") {result.type=data.u16(p+0x11c);result.machine_flags=data.u16(p+0x11e);}
    result.object_flags=data.u16(p+2);
    result.powered_position_rate=data.f32(p+0xc8);result.unpowered_position_rate=data.f32(p+0xd0);
    result.initialized_idle=load_initialized_object_idle(datum);
    if(result.model==0xffffffffu)return result;
    const auto model=tag_root(result.model,"hlmt",0x7c);const auto graph=data.u32(model+0x14);
    result.render_model=data.u32(model+4);
    if(graph==0xffffffffu)return result;
    const auto root=tag_root(graph,"jmad",0x5c);
    if(std::int16_t(data.u16(root+0xc))!=std::bit_cast<std::int32_t>(data.u32(model+0x78)))return result;

    const auto find=[&](TagBlockRange table,std::size_t stride,std::uint32_t name)->std::optional<std::size_t>{
        int lo=0,hi=int(table.count)-1;
        while(lo<=hi){const int i=(lo+hi)>>1;const auto at=table.offset+std::size_t(i)*stride;const auto candidate=data.u32(at);
            if(candidate==name)return at;
            if(std::bit_cast<std::int32_t>(name)<std::bit_cast<std::int32_t>(candidate))hi=i-1;else lo=i+1;
        }return {};
    };
    const auto inherited=metadata_block(root+0x4c, 32);
    const auto inherited_graph=[&](int index){
        if(index==-1)return graph;
        if(index<0 || unsigned(index)>=inherited.count)throw FormatError("device inherited graph index invalid");
        const auto id=data.u32(inherited.offset+unsigned(index)*32+4);
        if(id==0xffffffffu)throw FormatError("device channel references NONE graph");
        return id;
    };
    const auto make_channel=[&](std::uint32_t id,unsigned animation){
        const auto g=tag_root(id,"jmad",0x34);const auto clips=metadata_block(g+0x2c, 0x6c);
        if(animation>=clips.count)throw FormatError("device channel animation index invalid");
        SourceDeviceChannel channel;channel.graph=id;channel.animation=std::uint16_t(animation);
        channel.animation_type=std::to_integer<std::uint8_t>(data.slice(clips.offset+animation*0x6c+0x10,1)[0]);
        channel.clip=load_animation(id,animation);return channel;
    };
    const auto selected=[&](std::uint32_t name)->std::optional<SourceDeviceChannel>{
        // 1CB4C0→1DD0B0 searches the root and inherited named clip banks.
        const auto named=[&](std::uint32_t id)->std::optional<SourceDeviceChannel>{
            const auto g=tag_root(id,"jmad",0x34);const auto clips=metadata_block(g+0x2c, 0x6c);
            for(unsigned i=0;i<clips.count;++i)if(data.u32(clips.offset+i*0x6c)==name)return make_channel(id,i);
            return {};
        };
        if(auto c=named(graph))return c;
        for(unsigned i=0;i<inherited.count;++i){const auto id=data.u32(inherited.offset+i*32+4);
            if(id==0xffffffffu)break;
            if(auto c=named(id))return c;}
        // 1DBC80: exact initialized controller categories, with the default
        // type→class alias and ordered 2963F0 fallback. Device fallback
        // searches the type's overlay bank +C, unlike default action +4.
        constexpr std::uint32_t any=0x030000d9u,combat=0x06000086u,unarmed=0x07000083u;
        auto cls=unarmed;
        if(const auto alias=find(metadata_block(root+0x54, 8),8,any)){
            const auto value=data.u32(*alias+4);if(value!=0xffffffffu && value!=any)cls=value;
        }
        const auto modes=metadata_block(root+0x34, 20);
        for(unsigned step=0;step<8;++step){
            if((step&1) || ((step&2) && cls==any))continue;
            const auto mode=find(modes,20,step&4?any:combat);if(!mode)continue;
            const auto wc=find(metadata_block(*mode+4, 20),20,step&2?any:cls);if(!wc)continue;
            const auto type=find(metadata_block(*wc+4, 0x34),0x34,any);if(!type)continue;
            const auto item=find(metadata_block(*type+0xc, 8),8,name);if(!item)continue;
            const auto animation=std::int16_t(data.u16(*item+6));if(animation==-1)continue;
            if(animation<0)throw FormatError("negative device animation selection");
            return make_channel(inherited_graph(std::int16_t(data.u16(*item+4))),unsigned(animation));
        }return {};
    };
    result.position=selected(0x08000080u);result.power=selected(0x05000081u);return result;
}
SourceMachinePlacement Cache::load_source_machine_placement(unsigned ordinal)const {
    const Bytes data(storage_);
    const auto tag=query_tag({.group=tag_groups::scenario, .require_address=true});
    if(!tag)throw FormatError("source machine scenario missing");
    const auto s=metadata_offset(tag->address,0xb0);const auto count=data.u32(s+0xa8);
    if(ordinal>=count || count>65536)throw FormatError("source machine placement outside scenario");
    const auto p=metadata_offset(data.u32(s+0xac),std::size_t(count)*0x48)+ordinal*0x48;
    SourceMachinePlacement result;
    result.power_group=std::int16_t(data.u16(p+0x34));result.position_group=std::int16_t(data.u16(p+0x36));
    result.device_flags=data.u32(p+0x38);result.machine_flags=data.u32(p+0x3c);
    const auto groups=data.u32(s+0xa0);if(groups>65536)throw FormatError("excessive device groups");
    const auto group_start=groups?metadata_offset(data.u32(s+0xa4),std::size_t(groups)*40):0;
    const auto initial=[&](int group,bool power)->std::pair<float,std::uint16_t>{
        if(group==-1)return {power?(result.device_flags&2?0.f:1.f):(result.device_flags&1?1.f:0.f),
            std::uint16_t(power?4:(((result.device_flags&4)|0x10)>>2))};
        if(group<0 || unsigned(group)>=groups)throw FormatError("source device named group index invalid");
        const auto at=group_start+unsigned(group)*40;
        return {data.f32(at+32),std::uint16_t(data.u32(at+36)&1)};
    };
    std::tie(result.initial_power,result.power_group_flags)=initial(result.power_group,true);
    std::tie(result.initial_position,result.position_group_flags)=initial(result.position_group,false);
    return result;
}
}
