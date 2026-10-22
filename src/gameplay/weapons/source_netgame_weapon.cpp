#include "halo2/source_netgame_weapon.hpp"
#include "halo2/source_euler.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
std::uint32_t word(float value){return std::bit_cast<std::uint32_t>(value);}
std::int16_t short_wrap(std::int32_t value){
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
}
SourceWorldWeaponDefinition Cache::load_source_world_weapon_definition(std::uint32_t datum)const{
    const Bytes bytes(cache_bytes());
    const auto tag=[&](std::uint32_t id,const char *group,std::size_t size){
        const auto found=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
        if(!found)throw FormatError("source world weapon tag missing");
        return metadata_offset(found->address,size);
    };
    const auto root=tag(datum,"weap",0x2d4);
    if(bytes.u16(root)!=2)throw FormatError("source world weapon object type mismatch");
    SourceWorldWeaponDefinition result;
    result.definition=datum;
    result.name=query_tag({.handle=TagHandle{datum}})->name;
    result.object_flags=bytes.u16(root+2);result.model=bytes.u32(root+0x38);
    result.item_flags=bytes.u32(root+0xbc);
    result.multiplayer_scale=bytes.f32(root+0xc4);result.campaign_scale=bytes.f32(root+0xc8);
    result.owner_function_count=bytes.u32(root+0x104);
    result.barrel_count=std::bit_cast<std::int32_t>(bytes.u32(root+0x2d0));
    result.functions=load_object_functions(datum);
    const auto magazines=bytes.u32(root+0x2c0);
    if(magazines>4096)throw FormatError("source world weapon magazine count exceeds metadata bounds");
    if(magazines){const auto first=metadata_offset(bytes.u32(root+0x2c4),std::size_t(magazines)*0x5c);
        for(unsigned i=0;i<magazines;++i){const auto p=first+i*0x5c;
            result.magazines.push_back({std::bit_cast<std::int16_t>(bytes.u16(p+6)),
                std::bit_cast<std::int16_t>(bytes.u16(p+0xa)),std::bit_cast<std::int16_t>(bytes.u16(p+0xc))});}}
    const auto widgets=bytes.u32(root+0x94);
    if(widgets>4096)throw FormatError("source world weapon widget count exceeds metadata bounds");
    if(widgets){const auto first=metadata_offset(bytes.u32(root+0x98),std::size_t(widgets)*0x18);
        for(unsigned i=0;i<widgets;++i)if(bytes.u32(first+i*0x18+4)!=0xffffffffu)
            result.widget_groups.push_back(bytes.u32(first+i*0x18));}
    if(!std::isfinite(result.multiplayer_scale)||!std::isfinite(result.campaign_scale))
        throw FormatError("source world weapon authored scale is not finite");
    if(result.model!=0xffffffffu){
        const auto model=tag(result.model,"hlmt",0x18);
        result.render_model=bytes.u32(model+4);result.collision_model=bytes.u32(model+0xc);
        result.animation_graph=bytes.u32(model+0x14);
        result.default_variant=bytes.u32(root+0x30);
    }
    return result;
}
float source_world_weapon_scale(const SourceWorldWeaponDefinition&d,int game_type,std::uint32_t flags){
    if(!std::isfinite(d.multiplayer_scale)||!std::isfinite(d.campaign_scale))
        throw FormatError("source world weapon scale requires finite authored inputs");
    if(flags&1u)return 1;
    const float scale=game_type==2?d.multiplayer_scale:d.campaign_scale;
    if(!(scale>0))return 1;
    //10DAD0 loads literal45DBBC=.5 and45DBD4=3 into XMM3
    //before the ordered comparisons and retains them outside the range.
    if(scale<.5f)return .5f;
    if(scale>3.f)return 3.f;
    return scale;
}
SourceNetgameWeaponBirth source_netgame_weapon_birth(const NetgameEquipment&row,
    const SourceWorldWeaponDefinition&definition,int game_type,int weapon_option){
    if(row.classification!=0 || weapon_option!=0 || row.collection==0xffffffffu ||
       row.collection_group!=0x69746d63u || row.items.size()!=1 ||
       row.items[0].group!=0x77656170u || row.items[0].definition!=definition.definition ||
       definition.definition==0xffffffffu || !std::isfinite(row.items[0].weight) ||
       row.items[0].weight<=0 || !finite(row.position)||!finite(row.rotation))
        throw FormatError("source netgame weapon requires singleton stock weapon inputs");
    const auto basis=source_euler_basis(row.rotation);
    SourceNetgameWeaponBirth out;
    out.definition=definition.definition;out.variant=row.items[0].variant;
    out.position=row.position;out.forward=basis.forward;out.up=basis.up;
    out.object_scale=source_world_weapon_scale(definition,game_type,0);
    out.levitated=bool(row.flags&1u);
    auto&d=out.descriptor;
    d[0]=out.definition;d[1]=0xffffffffu;d[2]=0xffffffffu;d[3]=out.variant;
    d[4]=0xffffffffu;d[22]=word(1);d[23]=d[24]=d[25]=0xffffffffu;
    d[26]=d[27]=0xffffffffu;d[28]=d[45]=0xffffu;
    d[7]=word(out.position.x);d[8]=word(out.position.y);d[9]=word(out.position.z);
    d[10]=word(out.forward.x);d[11]=word(out.forward.y);d[12]=word(out.forward.z);
    d[13]=word(out.up.x);d[14]=word(out.up.y);d[15]=word(out.up.z);
    return out;
}
int source_netgame_equipment_selector(const NetgameEquipment&row,bool available,int engine){
    for(unsigned i=0;i<row.modes.size();++i){
        const int selector=row.modes[i];
        if(!available){if(selector==0)return int(i);continue;}
        bool admitted=selector==engine;
        if((engine==1||engine==9)&&(selector==1||selector==9))admitted=true;
        if(selector==12)admitted=true;
        else if((selector==13||selector==14)&&engine!=1&&engine!=9)admitted=true;
        if(admitted)return int(i);
    }
    return -1;
}
bool source_netgame_weapon_step(NetgameWeaponState&state,const NetgameEquipment&row,unsigned ordinal,
    const SourceNetgameWeaponStepInput&input,const std::optional<SourceNetgameWeaponBound>&bound){
    if(ordinal>=100 || input.tick_rate<=0 || row.classification!=0)
        throw FormatError("source netgame weapon timer requires source row/clock/classification");
    if(input.playback_mode || int(ordinal)%input.tick_rate!=input.tick%input.tick_rate ||
       source_netgame_equipment_selector(row,input.engine_available,input.original_engine)==-1)return false;
    if((state.object==0xffffffffu)!=!bound || (bound&&bound->handle!=state.object))
        throw FormatError("source netgame weapon bound item does not match actual row handle");
    if(bound&&!finite(bound->position))throw FormatError("source netgame weapon sphere is not finite");
    if(state.timer==0){
        bool start=state.object==0xffffffffu;
        if(bound&&row.timer_start==0){
            const float z=bound->position.z-row.position.z,y=bound->position.y-row.position.y,
                        x=bound->position.x-row.position.x;
            const float zz=z*z,yy=y*y,xx=x*x;
            start=(bound->item_flags&3u)!=0 || (zz+yy)+xx>1.f;
        }else if(bound&&row.timer_start==1)start=bool(bound->control10a&4u);
        if(start){
            std::int32_t delay=row.respawn_seconds!=0?row.respawn_seconds:
                (row.collection!=0xffffffffu && (row.collection_group==0x69746d63u ||
                 row.collection_group==0x76656863u) && row.collection_delay!=0?row.collection_delay:30);
            if(input.timer_option==1)delay=-1;
            else if(input.timer_option==2)delay=delay>>1;
            else if(input.timer_option==3)delay=std::int32_t(std::uint32_t(delay)<<1);
            state.timer=short_wrap(delay);
        }
    }
    if(state.timer<=0)return false;
    state.timer=short_wrap(std::int32_t(state.timer)-1);
    return state.timer==0;
}
void source_netgame_weapon_reset(std::span<NetgameWeaponState>states,bool playback,
    std::uint32_t flags,std::int16_t round,const std::function<void(std::uint32_t)>&destroy){
    if(states.size()>100)throw FormatError("source netgame row capacity exceeded");
    if(playback||(!(flags&8u)&&round!=0))return;
    for(auto&state:states){
        if(state.object!=0xffffffffu){
            if(!destroy)throw FormatError("source netgame reset requires actual destruction service");
            destroy(state.object);state.object=0xffffffffu;
        }
        state.timer=1;state.empty_counter=0;
    }
}
void source_netgame_weapon_allocate(NetgameWeaponState&state,unsigned row,const SourceNetgameWeaponBirth&birth,
    const std::function<void(std::uint32_t)>&unbind,
    const std::function<std::uint32_t(const SourceNetgameWeaponBirth&,unsigned)>&allocate){
    if(row>=100 || !allocate)throw FormatError("source netgame allocation requires actual row/service");
    if(state.object!=0xffffffffu){
        if(!unbind)throw FormatError("source netgame replacement requires actual unbind service");
        unbind(state.object);state.object=0xffffffffu;
    }
    const auto handle=allocate(birth,row);
    state.object=handle;state.timer=state.empty_counter=0;
}
bool source_levitated_weapon_update(const SourceWorldWeaponDefinition&definition,SourceLevitatedWeaponUpdate&state){
    if(!(state.control_c1&1u) || state.parent!=0xffffffffu || state.remaining_ticks>0 ||
       (state.item_flags&0x70u) || (definition.item_flags&1u) ||
       state.ignored_object!=0xffffffffu || state.item_owner!=0xffffffffu ||
       (state.object_owner_c8!=0xffffffffu && definition.owner_function_count))
        throw FormatError("source world weapon update requires unported contact/spin/expiry/alignment services");
    //10B850 sets the valid collision model flag before10B360. This is the
    //only write in the bounded, active levitated source branch.
    if((state.object_flags&0x100u) && state.collision_model!=-1)
        state.object_flags|=0x20000000u;
    return false;
}
float source_unattached_weapon_camouflage(const SourceLevitatedWeaponUpdate &state){
    if(state.parent!=0xffffffffu)
        throw FormatError("source weapon camouflage requires its actual parent chain");
    return 0.f;
}
SourceNetgameWeaponInitialState source_netgame_weapon_initial_state(
    const SourceWorldWeaponDefinition&definition,const SourceNetgameWeaponBirth&birth){
    if(!birth.levitated || birth.definition!=definition.definition ||
       definition.animation_graph!=0xffffffffu || (definition.item_flags&1u))
        throw FormatError("source netgame constructor needs unported controller/alignment/ground services");
    for(auto group:definition.widget_groups)
        if(group!=0x65666665u && group!=0x6c736e64u && group!=0x6c696768u)
            throw FormatError("source netgame constructor widget service is unresolved");
    SourceNetgameWeaponInitialState out;
    //10B2C0 marks all weapon objects;15AAD0 freezes the unowned levitated
    //item after B7B40. Remaining fields retain zeroed/source NONE storage.
    out.item.object_flags=0x1000u|(definition.collision_model!=0xffffffffu?0x200u:0u);
    out.item.control_c1=1;
    for(const auto&magazine:definition.magazines)
        out.magazines.push_back(source_weapon_magazine_initial_state(magazine));
    return out;
}
} // namespace halo2
