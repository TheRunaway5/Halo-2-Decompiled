#include "halo2/source_decorator_decal_draw.hpp"
#include "halo2/source_decorator_decal_material.hpp"
#include <bit>
#include <cstring>
#include <map>
#include <set>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    return Bytes(memory.view(address,4)).u32(0);
}
unsigned group_index(const SourceDecoratorDecalCommandTu15& command){return (command.words[3]>>8)&0x3fffffu;}
unsigned cell_ordinal(const SourceDecoratorDecalCommandTu15& command){return command.words[3]&255u;}
void validate_command(const SourceDecoratorDecalBspTu15& bsp,const SourceDecoratorDecalFrameTu15& frame,
    const SourceDecoratorDecalCommandTu15& command) {
    if(frame.bsp!=bsp.bsp || frame.definition!=0 || bsp.definitions.size()!=1 ||
       command.words[0]!=0x4df90u || command.words[1]!=0x4ebc0u ||
       command.ordinal>=frame.command_header[1] || cell_ordinal(command)>=frame.cells.size())
        throw FormatError("Decorator command lost its actual BSP, callback or admitted-cell owner");
    const auto& definition=bsp.definitions.front();const auto group=group_index(command);
    const auto cell=Bytes(frame.cells[cell_ordinal(command)]).u32(0);
    if(group>=definition.groups.size() || cell>=definition.cells.size())
        throw FormatError("Decorator command escapes its actual cell or group bank");
    const auto& authored=definition.groups[group];const auto& owner=definition.cells[cell];
    if(authored.authored!=command.group || !authored.selection ||
       authored.selection->shader!=command.words[2] ||
       (authored.selection->fade_distance&3u)!=(command.words[3]>>30) ||
       owner.group_first<0 || owner.group_count<0 || group<unsigned(owner.group_first) ||
       group-unsigned(owner.group_first)>=unsigned(owner.group_count))
        throw FormatError("Decorator command differs from its authored raw group, shader or cell range");
    if(authored.cache_block<0 || unsigned(authored.cache_block)>=definition.caches.size() ||
       authored.vertex_first<0 || authored.vertex_count<0)
        throw FormatError("Decorator command has no complete actual cache vertex range");
    const auto& cache=definition.caches[authored.cache_block];
    if(unsigned(authored.vertex_first)>cache.vertices.size() ||
       unsigned(authored.vertex_count)>cache.vertices.size()-unsigned(authored.vertex_first))
        throw FormatError("Decorator vertex range escapes the actual immutable cache resource");
}
}
SourceDecoratorDecalFrameTu15 source_decorator_decal_frame_tu15(
    const SourceRuntimeMemory& memory,const SourceDecoratorDecalBspTu15& bsp,
    const std::array<ShaderVector,192>& camera_constants,
    ShaderVector viewport,ShaderVector clip_range,ShaderVector depth_parameters) {
    const auto current=word(memory,0x4ecc18u),table=word(memory,0x4f0414u);
    const auto tag=table+(bsp.bsp&65535u)*16u;
    if(!current || !table || word(memory,tag)!=0x73627370u ||
       word(memory,tag+4u)!=bsp.bsp || word(memory,tag+8u)!=current)
        throw FormatError("Decorator readback lost its current full salted source BSP identity");
    SourceDecoratorDecalFrameTu15 frame;frame.bsp=bsp.bsp;frame.camera_constants=camera_constants;
    frame.viewport=viewport;frame.clip_range=clip_range;frame.depth_parameters=depth_parameters;
    std::memcpy(frame.command_header.data(),memory.view(0x4ce268u,24).data(),24);
    if(frame.command_header[0]!=2560 || frame.command_header[1]>frame.command_header[0] ||
       !frame.command_header[5])
        throw FormatError("Decorator readback escaped the original ordinary command bank");
    const auto count=word(memory,0x515f18u);
    if(count>256)throw FormatError("Decorator readback escaped the actual admitted-cell capacity");
    frame.cells.resize(count);
    if(count)std::memcpy(frame.cells.data(),memory.view(0x4d1f20u,std::size_t(count)*16u).data(),std::size_t(count)*16u);
    const auto definitions=word(memory,current+0x234u),definition=word(memory,current+0x238u);
    if(definitions!=bsp.definitions.size() || definitions>1 || (definitions && !definition))
        throw FormatError("Decorator source definition bank differs from its actual immutable BSP");
    for(unsigned ordinal=0;ordinal<frame.command_header[1];++ordinal) {
        SourceDecoratorDecalCommandTu15 command;command.ordinal=ordinal;
        const auto address=std::uint64_t(frame.command_header[5])+ordinal*24u;
        if(address>0xffffffffu)throw FormatError("Decorator source command address overflows");
        std::memcpy(command.words.data(),memory.view(std::uint32_t(address),24).data(),24);
        if(command.words[0]!=0x4df90u && command.words[1]!=0x4ebc0u)continue;
        if(!definition || group_index(command)>=word(memory,definition+0x18u))
            throw FormatError("Decorator command has no actual source group definition");
        const auto group=std::uint64_t(word(memory,definition+0x1cu))+group_index(command)*24u;
        if(group>0xffffffffu)throw FormatError("Decorator source group address overflows");
        std::memcpy(command.group.data(),memory.view(std::uint32_t(group),24).data(),24);
        const auto fade=(command.words[3]>>30)*4u;
        command.fade={std::bit_cast<float>(word(memory,0x53455cu+fade)),
            std::bit_cast<float>(word(memory,0x534568u+fade)),0.f,
            std::bit_cast<float>(word(memory,0x46a0d0u))};
        validate_command(bsp,frame,command);frame.commands.push_back(command);
    }
    return frame;
}
struct SourceDecoratorDecalSceneTu15::Impl {
    const Cache& cache;
    std::shared_ptr<const SourceDecoratorDecalBspTu15> assets;
    std::map<std::uint32_t,std::shared_ptr<const Material>> materials;
    Impl(const Cache& owner,std::size_t bsp):cache(owner),
        assets(std::make_shared<const SourceDecoratorDecalBspTu15>(owner.load_source_decorator_decals_tu15(bsp))) {}
};
SourceDecoratorDecalSceneTu15::SourceDecoratorDecalSceneTu15(const Cache& cache,std::size_t bsp)
    :impl_(std::make_unique<Impl>(cache,bsp)) {}
SourceDecoratorDecalSceneTu15::~SourceDecoratorDecalSceneTu15()=default;
const SourceDecoratorDecalBspTu15& SourceDecoratorDecalSceneTu15::assets()const{return *impl_->assets;}
std::vector<SourceDecoratorDecalDrawTu15> SourceDecoratorDecalSceneTu15::draw(const SourceDecoratorDecalFrameTu15& frame) {
    if(frame.bsp!=impl_->assets->bsp || frame.definition!=0 || frame.cells.size()>256 ||
       frame.command_header[0]!=2560 || frame.command_header[1]>2560 ||
       frame.commands.size()>frame.command_header[1])
        throw FormatError("Decorator frame lost its complete source view and command-bank bounds");
    std::vector<SourceDecoratorDecalDrawTu15> result;result.reserve(frame.commands.size());
    std::set<unsigned> ordinals;
    for(const auto& command:frame.commands) {
        if(!ordinals.insert(command.ordinal).second)
            throw FormatError("Decorator frame repeats an actual source command ordinal");
        validate_command(*impl_->assets,frame,command);
        const auto group=group_index(command);const auto& definition=impl_->assets->definitions.front();
        const auto& authored=definition.groups[group];
        if(authored.selection->type!=1)
            throw FormatError("Projected decorator draw requires its actual runtime-generated resource owner");
        if((command.words[5]>>8&255u)!=0 || !(command.words[4]&(1u<<2)) ||
           authored.vertex_count%4)
            throw FormatError("Floating decorator draw differs from its original LOD0/layer2 QUADS owner");
        auto& material=impl_->materials[command.words[2]];
        if(!material)material=std::make_shared<const Material>(
            source_decorator_decal_material_tu15(impl_->cache,command.words[2]));
        SourceDecoratorDecalDrawTu15 draw;draw.source=command;draw.bsp=frame.bsp;
        draw.group=group;draw.cell=Bytes(frame.cells[cell_ordinal(command)]).u32(0);
        draw.first=unsigned(authored.vertex_first);draw.count=unsigned(authored.vertex_count);
        draw.cache={impl_->assets,&definition.caches[authored.cache_block]};draw.material=material;
        draw.constants=frame.camera_constants;draw.constants[53]=command.fade;
        draw.viewport=frame.viewport;draw.clip_range=frame.clip_range;draw.depth_parameters=frame.depth_parameters;
        result.push_back(std::move(draw));
    }
    return result;
}
}
