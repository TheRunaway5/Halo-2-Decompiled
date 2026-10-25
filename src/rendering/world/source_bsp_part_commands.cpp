#include "halo2/source_bsp_part_commands.hpp"
#include <bit>
#include <cstring>
#include <algorithm>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
std::uint16_t half(const std::array<std::byte,72>& p,unsigned offset) {
    std::uint16_t v;std::memcpy(&v,p.data()+offset,2);return v;
}
std::int16_t signed_half(const std::array<std::byte,72>& p,unsigned offset) {
    return std::bit_cast<std::int16_t>(half(p,offset));
}
float scalar(const std::array<std::byte,72>& p,unsigned offset) {
    float v;std::memcpy(&v,p.data()+offset,4);return v;
}
std::uint32_t layer(const SourceBspCommandShader& s,std::int32_t lod) {
    if(lod<0 || unsigned(lod)>=s.layers.size())
        throw FormatError("source BSP part command consumes absent template LOD");
    return s.layers[unsigned(lod)];
}
const SourceBspCommandShader& shader(const SourceBspPartCommandWorld& w,std::uint32_t id) {
    if(!w.shader)throw FormatError("source BSP part command shader service missing");
    return w.shader(id);
}
bool part_visible(const std::array<std::byte,72>& part,const SourceCullDrawUnit& unit,
    const SourceBspRangeArena& arena,std::uint32_t base) {
    if(!(unit.words[0]&0x40u))return true;
    const int first=signed_half(part,10),end=first+signed_half(part,12);
    for(int bit=first;bit<end;++bit) {
        const auto address=std::int64_t(unit.words[5])+std::int64_t(bit>>5)*4;
        const auto offset=address-base;
        if(!base || offset<0 || offset%4 || offset>=2048)
            throw FormatError("source BSP part command mask escapes current arena");
        if(arena.words[unsigned(offset/4)]&(1u<<(unsigned(bit)&31)))return true;
    }
    return false;
}
void enqueue(SourceBspPartCommandState& state,const SourceBspPartCommandContext& c,
    const SourceBspCommandShader& s,std::uint32_t shader_id,float metric,
    std::int32_t category,std::uint32_t token) {
    state.calls.push_back(0x40e30);
    if(std::int16_t(category)==-1)return;
    state.calls.push_back(0x1bf50);
    const auto choice=source_shader_lod_choice(s.lod.thresholds,s.lod.bias,metric,c.lod_view,c.view_modifier);
    if(!choice.accepted)return;
    state.calls.push_back(0x3b9d0);
    if(category<0 || unsigned(category)>=state.banks.size())
        throw FormatError("source BSP part command category outside current bank");
    auto& bank=state.banks[unsigned(category)];
    if(std::bit_cast<std::int32_t>(bank.header[1])>=std::bit_cast<std::int32_t>(bank.header[0]))return;
    const auto index=bank.header[1];
    if(index>=bank.storage.size())throw FormatError("source BSP part command output storage missing");
    const auto layers=layer(s,choice.index);
    ++bank.header[1];auto& command=bank.storage[index].words;
    command[0]=c.callback_geometry;command[1]=c.callback_light;command[2]=shader_id;
    command[3]=token;command[4]=layers; //44AC0 supplies maskNONE and add0.
    command[5]=(command[5]&0xffff0000u)|(std::uint32_t(std::uint8_t(choice.index))<<8);
    bank.header[2]|=layers;
}
}
void source_bsp_material_visibility_initialize(std::span<std::byte> bytes) {
    if(bytes.size()!=0x4204)
        throw FormatError("source material visibility allocation must contain4204 bytes");
    std::fill_n(bytes.begin(),0x4201,std::byte{255});
}
std::uint32_t source_part_commands_begin(SourceBspPartCommandState& state,
    std::uint32_t mode,std::uint32_t selector) {
    state.calls.push_back(0x40e90);
    const auto reset=[](auto& bank) {
        bank.header[1]=bank.header[2]=bank.header[3]=0;
    };
    if(!mode) {reset(state.banks[0]);state.main_object_counts={};}
    else {reset(state.banks[1]);reset(state.banks[2]);}
    return !selector?0xffffffffu:selector==2?0x10u:0x20u;
}
bool source_bsp_material_visible(std::span<const std::byte> bytes,std::int16_t bsp,
    std::int32_t instance,std::int32_t index) {
    if(index==-1)return true;
    const std::int64_t bank=instance==-1?std::int64_t(bsp)*32+1:
        std::int64_t(bsp)*1024+0x201+instance;
    const auto offset=bank+std::int64_t(index>>5)*4;
    if(offset<0 || std::uint64_t(offset)+4>bytes.size())
        throw FormatError("source BSP material visibility escapes current allocation");
    std::uint32_t word;std::memcpy(&word,bytes.data()+offset,4);
    return (word&(1u<<(unsigned(index)&31)))!=0;
}
void source_bsp_append_part_commands(SourceBspPartCommandState& state,
    std::span<const SourceCullDrawUnit> units,const SourceBspRangeArena& arena,
    const SourceBspPartCommandWorld& w,const SourceBspPartCommandContext& c) {
    //44AC0 visits [0,main_prefix) in main mode0 and [main_prefix,draw_count)
    // in every secondary mode, with the same per-unit services.
    const unsigned begin=c.mode?c.main_prefix&65535u:0u,end=(c.mode?c.draw_count:c.main_prefix)&65535u;
    if(end>units.size() || end>850 || c.coverage.size()<end || c.override_slots.size()<end)
        throw FormatError("source BSP part commands current draw bank incomplete");
    state.calls.push_back(0x44ac0);
    for(unsigned ordinal=begin;ordinal<end;++ordinal) {
        const auto& unit=units[ordinal];const auto& u=unit.words;
        if(u[0]&0x400u) {
            if(state.special_count<0)throw FormatError("source BSP special-unit count precedes bank");
            if(state.special_count<32)state.special_units[unsigned(state.special_count++)]=unit;
            continue;
        }
        if(!(u[0]&c.flag_mask&0x1fffffu) || (u[3]>>4&31u)!=c.group ||
            !(u[0]&1u) || !(u[6]&0x00ff0000u))continue;
        const auto slot=c.override_slots[ordinal];std::uint32_t override=none;
        if(slot!=255) {
            if(slot>=c.overrides.size())throw FormatError("source BSP shader override bank missing");
            override=c.overrides[slot];
        }
        state.calls.push_back(0x449e0);
        const auto instance=u[3]>>18&2047u;
        const SourceBspCommandGeometry* geometry=nullptr;
        const SourceModelCommandGeometry* model=nullptr;
        if(u[1]!=none){
            if(!w.model)throw FormatError("source part commands consume absent model geometry/material/palette owner");
            model=&w.model(unit);geometry=&model->geometry;
        }else if(u[0]&0x1000u) {
            if(instance>=w.instance_definitions.size())throw FormatError("source BSP part command instance missing");
            const auto definition=w.instance_definitions[instance];
            if(definition<0 || unsigned(definition)>=w.definitions.size())throw FormatError("source BSP part command definition missing");
            geometry=&w.definitions[unsigned(definition)];
        } else {
            const auto cluster=u[3]>>9&511u;
            if(cluster>=w.clusters.size())throw FormatError("source BSP part command cluster missing");
            geometry=&w.clusters[cluster];
        }
        if(!geometry->resident)continue;
        for(unsigned part_index=0;part_index<geometry->parts.size();++part_index) {
            state.calls.push_back(0x45ce0);const auto& part=geometry->parts[part_index];
            const auto material_index=signed_half(part,4);
            const auto materials=model?std::span<const SourceBspCommandMaterial>(model->materials):w.materials;
            if(material_index<0 || unsigned(material_index)>=materials.size())
                throw FormatError("source BSP part command material missing");
            const auto& material=materials[unsigned(material_index)];
            if(model){
                if((u[3]>>9&511u)==255u)continue;
                state.calls.push_back(0x1d6a0);
                //45CE0 reads the current selected cache matrix even when
                //secondary41040 only consumes its shader and LOD scale.
                (void)source_model_palette_part_center(model->palette,u[3]&15u,model->rigid_node,model->model_flags);
            }else{
                state.calls.push_back(0x184000);
                if(!source_bsp_material_visible(w.material_visibility,w.bsp_selector,
                    instance==2047?-1:std::int32_t(instance),material.visibility_index==255?-1:material.visibility_index))continue;
            }
            state.calls.push_back(0x460d0);
            if(!part_visible(part,unit,arena,c.mask_arena_address))continue;
            if((override==none?material.shader:override)==none)continue;
            state.calls.push_back(0x41040);
            const auto type=half(part,0);
            if(!type || type==5 || material.shader==none)continue;
            const auto shader_id=override==none?material.shader:override;
            const auto& selected=shader(w,shader_id);
            const auto authored=scalar(part,44);const float scale=authored>0?authored:1.f;
            const auto metric=source_shader_part_metric(c.coverage[ordinal],scale,c.player_multiplier);
            state.calls.push_back(0x1bf50);
            const auto choice=source_shader_lod_choice(selected.lod.thresholds,selected.lod.bias,metric,c.lod_view,c.view_modifier);
            const auto layers=layer(selected,choice.index);
            if(override!=none) {
                const auto original_layers=layer(shader(w,material.shader),choice.index);
                if((original_layers&0x8080u) && !(original_layers&10u) && (layer(selected,0)&0x200000u))continue;
            }
            if(!(layers&0xffff7fffu) && c.group==31)continue;
            const bool category1=(u[0]&2u)!=0;
            const bool category2=(u[0]&4u)!=0 && c.mode!=3;
            const auto token=((std::uint32_t(c.coverage[ordinal])<<16)|ordinal)<<8|part_index;
            //41030 at412D9: the extra category-2 emission is skipped in mode0.
            if(category1 && category2 && c.mode)enqueue(state,c,selected,shader_id,metric,2,token);
            //40DD0(ECX=mode,flag2,flag4): mode0 always selects bank0; every
            // other mode selects bank1 for flag2, bank2 for flag4, else none.
            enqueue(state,c,selected,shader_id,metric,c.mode?(category1?1:category2?2:-1):0,token);
        }
    }
}
void source_bsp_build_part_commands(SourceBspPartCommandState& state,
    std::span<const SourceCullDrawUnit> units,const SourceBspRangeArena& arena,
    const SourceBspPartCommandWorld& world,const SourceBspPartCommandContext& context,
    std::uint32_t selector,
    const std::function<void(SourceBspPartCommandState&,std::uint32_t)>& objects) {
    if(!context.mode || !objects)
        throw FormatError("source secondary command frame lacks its current object queue owner");
    auto current=context;
    current.flag_mask=source_part_commands_begin(state,current.mode,selector);
    current.group=31; //40F09 supplies literal1F, independent of the caller's record.
    state.calls.push_back(0x41c80);
    objects(state,current.mode);
    source_bsp_append_part_commands(state,units,arena,world,current);
}
}
