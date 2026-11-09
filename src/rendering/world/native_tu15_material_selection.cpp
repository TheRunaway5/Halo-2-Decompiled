#include "halo2/native_tu15_material_selection.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
struct Inputs {
    const SourcePhysicsContactMemory& memory;
    const NativeTu15MaterialSelectionArgs& args;
    std::uint32_t read(std::uint32_t at,unsigned width=4) const {
        const auto end=std::uint64_t(at)+width;
        const auto overlaps=[&](std::uint32_t start,unsigned bytes){return at<std::uint64_t(start)+bytes&&start<end;};
        if(end>0x100000000ull||overlaps(args.stack+0x18u,8)||overlaps(args.descriptor,24))
            throw std::invalid_argument("Native4CBE0 metadata aliases its output");
        std::array<std::byte,4> raw{};memory.read(at,std::span(raw).first(width));
        std::uint32_t result=0;
        for(unsigned i=0;i<width;++i)result|=std::to_integer<std::uint32_t>(raw[i])<<(8u*i);
        return result;
    }
    std::uint32_t definition(std::uint32_t table,std::uint32_t datum) const {
        return read(table+16u*(datum&65535u)+8u);
    }
};
int shader_class(const Inputs& in,std::uint32_t layer,std::uint32_t flags,std::uint32_t model,
    std::uint32_t shader_definition,std::uint32_t pass_range) {
    if(!(pass_range&0xfe00u))return -1;
    if(layer==3u) {
        const auto structure=in.read(0x4ecc14u);
        if(!structure||std::bit_cast<std::int32_t>(in.read(structure+0x80u))<=0)return 3;
        const auto bsp=in.read(0x4ecc18u);
        if(!bsp)return 3;
        const auto lightmap=in.read(structure+0x84u);
        return in.read(lightmap+0x1cu)!=none&&in.read(lightmap+4u)==in.read(bsp+8u)?-1:3;
    }
    if(layer==10u) {
        if(flags&0x400u)return -1;
        if(model==none)return 2;
        return (in.read(0x4c6834u)&0x30u)==0x10u?0:1;
    }
    if(layer<11u||layer>13u)return 0;
    const bool specular=in.read(shader_definition+0x3eu,2)!=0;
    const auto features=in.read(0x4c6834u);
    if(layer==13u)return (features&3u)==3u&&specular?0:-1;
    if(layer==12u) {
        if(features&1u)return 1;
        if(in.read(in.read(0x4920e4u),1)&0x20u)return 4;
        if((features&2u)&&specular)return 0;
        return features&4u?2:3;
    }
    if(features&1u)return -1;
    if(in.read(in.read(0x4920e4u),1)&0x20u)return 2;
    if(!(features&4u))return (features&2u)&&specular?2:-1;
    if(flags&0x400u)return 2;
    if(features&0x40u)return 6;
    if(features&0x10u)return features&0x20u?3:4;
    return (features>>3u)&1u;
}
struct Geometry {
    std::uint32_t metadata,parts_owner;
};
Geometry geometry(const Inputs& in,std::uint32_t table,std::uint32_t flags,std::uint32_t model,std::uint32_t packed) {
    const auto section=(packed>>9u)&511u;
    if(model!=none) {
        const auto row=in.read(in.definition(table,model)+0x28u)+0x5cu*section;
        return {row+4u,row+0x34u};
    }
    const auto bsp=in.read(0x4ecc18u);
    if(!(flags&0x1000u)) {
        const auto row=in.read(bsp+0xa0u)+0xb0u*section;
        return {row,row+0x50u};
    }
    const auto instance=in.read(bsp+0x144u)+0x58u*((packed>>18u)&2047u);
    const auto definition=std::bit_cast<std::int16_t>(std::uint16_t(in.read(instance+0x34u,2)));
    const auto row=in.read(bsp+0x13cu)+0xc8u*std::uint32_t(definition);
    return {row,row+0x50u};
}
}

NativeTu15MaterialSelection native_tu15_material_selection_4cbe0(
    const SourcePhysicsContactMemory& memory,const NativeTu15MaterialSelectionArgs& a) {
    if(a.layer==18u||a.layer==19u||(a.shader&0x8000u)||
        a.stack>0xffffffdfu||a.descriptor>0xffffffe7u)
        throw std::invalid_argument("Native4CBE0 requires an ordinary selector and frame");
    const auto shifted_token=std::uint32_t(std::bit_cast<std::int32_t>(a.token)>>8);
    const auto index=std::uint16_t(shifted_token);
    if(index>=850u)throw std::invalid_argument("Native4CBE0 unit escapes the retained draw bank");
    const Inputs in{memory,a};
    const auto unit=0x4c6958u+32u*index;
    const auto flags=in.read(unit),model=in.read(unit+4u);
    const auto table=in.read(0x4f0414u),shader=in.definition(table,a.shader);
    if(in.read(table+16u*(a.shader&65535u))!=0x73686164u)
        throw std::invalid_argument("Native4CBE0 requires the ordinary shad owner");
    const auto template_definition=in.definition(table,in.read(in.read(shader+0x24u)));
    const auto post=in.read(template_definition+0x5cu);
    const auto lod_row=in.read(post+4u)+10u*a.lod;
    const auto layer_row=in.read(post+0xcu)+2u*((in.read(lod_row,2)&511u)+a.layer);
    const auto pass_range=in.read(layer_row,2);
    auto classification=shader_class(in,a.layer,flags,model,shader,pass_range);
    if(a.layer==3u) {
        const auto lighting=flags>>29u;
        classification=lighting<=4u?int(lighting):3;
    } else if(a.layer==16u) {
        const bool gate=in.read(0x4c6824u,1)&&!in.read(0x492295u,1)&&!in.read(0x492296u,1);
        if(!(flags&0x80u))classification=-1;
        else if(flags&0x100u)classification=gate?-1:1;
        else classification=gate?2:0;
    }

    NativeTu15MaterialSelection out;
    const auto store=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        out.stores[out.count++]={pc,address,value,width};
    };
    store(0x4cc1fu,a.stack+0x18u,shifted_token);
    store(0x4ce06u,a.descriptor+4u,std::uint8_t(classification),1);
    if(classification<0) {
        out.class_rejected=true;
        return out;
    }
    const auto packed=in.read(unit+0xcu),group=packed>>29u;
    store(group>1u?0x4ce1eu:group?0x4ce40u:0x4ce63u,a.descriptor+5u,group>1u?2u:1u,1);
    if(group)store(group>1u?0x4ce3bu:0x4ce5eu,a.descriptor+0xcu,((packed>>9u)&511u)|((model&65535u)<<11u));
    store(0x4ce6eu,a.stack+0x1cu,(flags>>12u)&1u,1);
    const auto g=geometry(in,table,flags,model,packed);
    store(0x4ce7cu,a.stack+0x18u,g.metadata);
    const auto part=in.read(in.read(g.parts_owner)+4u)+72u*(a.token&255u);
    store(0x4cf14u,a.stack+0x1cu,part);
    const auto pass=(pass_range&511u)+a.pass;
    const auto pass_datum=in.read(in.read(post+0x14u)+10u*pass+4u);
    const auto pass_post=in.read(in.definition(table,pass_datum)+0x20u);
    const auto implementation=in.read(pass_post+4u)+0x132u*std::uint32_t(classification);
    store(0x4cfc6u,a.descriptor+0x10u,implementation);
    const auto part_type=in.read(part,2);
    const auto selector=std::bit_cast<std::int8_t>(std::uint8_t(in.read(g.metadata+((part_type==4u||part_type==5u)?0x11u:0x10u),1)));
    const auto vertex_tag=in.read(implementation+0x100u);
    auto vertex=in.read(g.metadata+0x14u,2);
    if(vertex>=3u) {
        if(vertex!=3u||vertex_tag==none)throw std::invalid_argument("Native4CBE0 vertex class has an unresolved owner");
        const auto candidate=std::uint32_t(int(selector)+2);
        vertex=candidate<in.read(in.definition(table,vertex_tag)+4u)?candidate:0;
    }
    store(0x4cfffu,a.descriptor+8u,std::uint16_t(vertex),2);
    store(0x4d006u,a.descriptor+0x14u,in.read(unit+0x10u));
    if(vertex_tag!=none) {
        const auto definition=in.definition(table,vertex_tag);
        if(std::uint16_t(vertex)<in.read(definition+4u)) {
            const auto entry=in.read(definition+8u)+0x1cu*std::uint16_t(vertex);
            out.accepted=(in.read(entry+0xcu)>>4u)!=0;
        }
    }
    if(a.layer==5u)out.accepted=(flags&0x2000u)!=0;
    return out;
}
}
