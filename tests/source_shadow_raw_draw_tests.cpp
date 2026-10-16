#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/source_shadow_plan.hpp"
#include "halo2/source_shadow_raw_plan.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class T>T read(std::ifstream &file) {
    T value;require(bool(file.read(reinterpret_cast<char*>(&value),sizeof(value))),"raw shadow fixture truncated");
    return value;
}
unsigned rejected;
template<class F>void rejects(F fn) {
    try{fn();}catch(const FormatError&){++rejected;return;}
    throw std::runtime_error("unsupported raw shadow join was admitted");
}
}
int main(int argc,char **argv) {try {
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    const auto map=argc>1?std::filesystem::path(argv[1]):root/"assets/disc/maps/lockout.map";
    if(!std::filesystem::exists(map))return 77;
    Cache cache(map);
    // Provenance and hashes are retained beside the3188-byte fixture. It
    // contains actual v21 native descriptors/commands/units, mask arena,
    // current model palettes and instance transform. The60 uniform words
    // are independently computed by unchanged OG Xbox TU1.5 instructions.
    std::ifstream file(root/"tests/fixtures/shadow_raw_tu15_native_v21.bin",std::ios::binary);
    require(bool(file),"raw shadow fixture missing");
    const auto header=read<std::array<std::uint32_t,7>>(file);
    require(header[0]==0x53524853 && header[1]==1 && header[2]==18,"raw shadow fixture version changed");
    SourceShadowUniforms uniforms;
    uniforms.projection=read<std::array<ShaderVector,6>>(file);
    uniforms.constants=read<std::array<ShaderVector,9>>(file);
    uniforms.opacity=std::bit_cast<float>(header[6]);
    SourceBspRangeArena arena;require(header[4]<=arena.words.size(),"raw shadow mask arena exceeds capacity");
    arena.used=std::uint16_t(header[4]);
    require(bool(file.read(reinterpret_cast<char*>(arena.words.data()),header[4]*4)),"raw shadow mask arena truncated");
    std::deque<SourceShadowMaterial> completed;
    std::vector<SourceShadowRawDraw> draws;
    unsigned generate=0,apply=0,matrices=0,ranges=0;
    for(unsigned record=0;record<header[2];++record) {
        const auto info=read<std::array<std::uint32_t,5>>(file);
        SourceShadowRawCommandTu15 proof;
        proof.phase=info[0]?SourceShadowPhase::Apply:SourceShadowPhase::Generate;
        proof.root_owner=header[5];proof.bank=info[0]?0x4ce280:0x4ce298;
        proof.category=info[0]?1:2;proof.layer=info[0]?11:10;
        proof.sorted_ordinal=info[1];proof.unit_ordinal=info[2];
        proof.descriptor=read<std::array<std::uint32_t,6>>(file);
        proof.command=read<std::array<std::uint32_t,6>>(file);
        proof.unit=read<std::array<std::uint32_t,8>>(file);
        std::vector<std::array<float,12>> palette(info[4]);
        if(!palette.empty())require(bool(file.read(reinterpret_cast<char*>(palette.data()),palette.size()*48)),"raw shadow current palette truncated");
        matrices+=unsigned(palette.size());
        const auto &c=proof.command;const auto &u=proof.unit;
        const bool model=u[1]!=0xffffffffu,instance=!model && (u[0]&0x1000u);
        std::shared_ptr<const MenuWorldSourceSection> geometry;
        SourceBspRangeGeometry range_geometry;
        if(model) {
            geometry=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,u[1],u[3]>>9&511u));
            range_geometry=source_model_range_geometry(*geometry);
            proof.geometry_owner={MenuWorldSourceOwnerKind::dynamic_object,u[1],info[3]&65535u,info[3]};
        } else {
            const auto kind=instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            const auto section=instance?load_menu_world_source_bsp_owner(cache,0,u[3]>>18&2047u).section:(u[3]>>9&511u);
            geometry=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,kind,section));
            range_geometry=source_bsp_range_geometry(*geometry);
            proof.geometry_owner={instance?MenuWorldSourceOwnerKind::bsp_instance:MenuWorldSourceOwnerKind::bsp_cluster,
                geometry->owner_datum(),instance?(u[3]>>18&2047u):section,{}};
        }
        auto authored=cache.load_material(c[2]);authored.selected_shader_lod=(c[5]>>8)&255u;
        auto materials=prepare_shadow_material(cache,authored,proof.phase,model,false,uniforms.opacity);
        require((c[5]&255u)<materials.size(),"actual native shadow pass is not authored");
        completed.push_back(std::move(materials[c[5]&255u]));
        const auto &material=completed.back();
        const auto pass=load_menu_world_source_pass(cache,c[2],{(c[5]>>8)&255u,proof.layer,c[5]&255u,info[0]?4u:0u});
        require(pass.implementation_address()==proof.descriptor[4],"actual native/cache implementation namespaces differ");
        MenuWorldSourceConstants previous;
        for(unsigned i=0;i<previous.size();++i)for(unsigned component=0;component<4;++component)
            previous[i][component]=std::bit_cast<float>(0x3f000000u+i*4+component);
        previous[37]={std::bit_cast<float>(0x7fa12345u),-0.0f,17,19};
        const auto constants=source_shadow_raw_constants(previous,material,uniforms);
        require(!std::memcmp(previous.data(),constants.data(),18*sizeof(ShaderVector)) &&
            !std::memcmp(previous.data()+37,constants.data()+37,3*sizeof(ShaderVector)) &&
            !std::memcmp(previous.data()+49,constants.data()+49,143*sizeof(ShaderVector)),
            "raw shadow changed unowned camera/projection-gap/caller constants");
        const auto selected=source_effect_light_part_ranges(SourceCullDrawUnit{u},range_geometry,c[3]&255u,arena,header[3]);
        // These are explicit CPU admission controls. The captured game bank
        // owns no SDK surface/clip state; this test makes no GPU-caller claim.
        MenuWorldRasterPacket raster{};raster.surface_scale={1,1};
        raster.depth_factor_offset=info[0]?std::array<float,2>{0,0}:std::array<float,2>{2,550};
        raster.regions.fill({0,0,961,541});
        const auto surface=info[0]?std::array<float,2>{960,540}:std::array<float,2>{128,128};
        const auto surface_clip=info[0]?std::array<std::uint32_t,4>{0,0,960,540}:
            std::array<std::uint32_t,4>{0,0,128,128};
        for(const auto range:selected.ranges) {
            if(!range.count)continue;
            auto source_draw=model || instance?menu_world_source_draw(geometry,c[3]&255u,selected.topology,palette,range):
                menu_world_source_cluster_draw(geometry,c[3]&255u,selected.topology,range);
            auto binding=menu_world_source_binding_class(cache,proof.geometry_owner,std::move(source_draw),
                pass.vertex_shader(),proof.descriptor[2]&65535u,constants);
            const auto raw=source_shadow_raw_draw(record,material,pass,std::move(binding),proof,uniforms,
                {0,true,surface_clip},raster,{0,16777215,0.01f,1024},{1,1},surface);
            validate_source_shadow_raw_draw(raw,proof.phase,uniforms);
            require(raw.source.draw().object_rows==palette,"raw shadow discarded actual current palette");
            rejects([&]{auto bad=raw;bad.command.descriptor[4]^=4;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.command.descriptor[2]^=1;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.command.command[2]^=1;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.command.geometry_owner.ordinal^=1;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.command.unit[4]^=4;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.controls.depth_clamp=false;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.raster.source_stencil_mode=1;validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=raw;bad.surface_size[0]=std::numeric_limits<float>::quiet_NaN();validate_source_shadow_raw_draw(bad,proof.phase,uniforms);});
            rejects([&]{auto bad=uniforms;bad.projection[0][0]+=1;validate_source_shadow_raw_draw(raw,proof.phase,bad);});
            rejects([&]{validate_source_shadow_raw_draw(raw,info[0]?SourceShadowPhase::Generate:SourceShadowPhase::Apply,uniforms);});
            if(raw.source.declaration().class_index)rejects([&]{
                auto packed=material;packed.material.vertex_declaration=raw.source.declaration();
                packed.material.passes[packed.material.forward_pass_index].vertex_declaration=raw.source.declaration();
                validate_source_shadow_plan_material(packed,proof.phase);
            });
            draws.push_back(raw);++ranges;
        }
        if(info[0])++apply;else ++generate;
    }
    require(file.peek()==std::ifstream::traits_type::eof(),"raw shadow fixture has trailing bytes");
    require(generate==6 && apply==12 && matrices==21 && ranges>=18,"raw shadow fixture scope changed");
    rejects([&]{validate_source_shadow_raw_material(completed.front(),static_cast<SourceShadowPhase>(2));});
    rejects([&]{validate_source_shadow_raw_draw(draws.front(),static_cast<SourceShadowPhase>(2),uniforms);});
    std::vector<SourceShadowRawDraw> casters,receivers;
    for(const auto &draw:draws)(draw.command.phase==SourceShadowPhase::Generate?casters:receivers).push_back(draw);
    for(const auto *sample:{&casters.front(),&receivers.front()}) {
        rejects([&]{auto bad=*sample;bad.command.unit[0]|=0x800u;
            validate_source_shadow_raw_draw(bad,bad.command.phase,uniforms);});
        rejects([&]{auto bad=*sample;bad.command.unit[6]&=~0x00ff0000u;
            validate_source_shadow_raw_draw(bad,bad.command.phase,uniforms);});
        rejects([&]{auto bad=*sample;bad.controls.surface_clip[2]=0;
            validate_source_shadow_raw_draw(bad,bad.command.phase,uniforms);});
        rejects([&]{auto bad=*sample;bad.controls.surface_clip[0]=0xffffffffu;
            validate_source_shadow_raw_draw(bad,bad.command.phase,uniforms);});
    }
    std::vector<SourceShadowRawPlanMaterial> resident;
    for(unsigned n=0;n<completed.size();++n)resident.push_back({n,&completed[n]});
    SourceShadowRawPlanRoot shadow_root;
    shadow_root.root_owner=header[5];shadow_root.uniforms=uniforms;
    shadow_root.command_masks={0x13c0e,0x13c0a}; // Actual v21 bank header[2].
    shadow_root.casters=casters;shadow_root.receivers=receivers;
    SourceShadowRawPlan plan;
    std::copy_n(draws.front().source.constants().begin(),12,plan.camera.begin());
    plan.roots={&shadow_root,1};
    const auto stats=validate_source_shadow_raw_plan(plan,resident);
    require(stats.roots==1 && stats.caster_draws==casters.size() &&
        stats.receiver_draws==receivers.size() && stats.indices>0,
        "raw shadow plan lost actual descriptor/range counts");
    rejects([&]{auto bad=plan;bad.camera[0][0]+=1;validate_source_shadow_raw_plan(bad,resident);});
    rejects([&]{auto bad=shadow_root;bad.root_owner^=1;auto p=plan;p.roots={&bad,1};validate_source_shadow_raw_plan(p,resident);});
    rejects([&]{auto bad=resident;bad.erase(bad.begin());validate_source_shadow_raw_plan(plan,bad);});
    rejects([&]{auto bad=resident;bad.push_back(bad.front());validate_source_shadow_raw_plan(plan,bad);});
    rejects([&]{auto bad=resident;bad.front().authored=&completed[1];validate_source_shadow_raw_plan(plan,bad);});
    const auto receiver_slot=receivers.front().material_slot;
    const auto receiver_resident=std::find_if(resident.begin(),resident.end(),
        [&](const auto &entry){return entry.material_slot==receiver_slot;});
    require(receiver_resident!=resident.end() && receiver_resident->authored->external_targets[0]==10 &&
        receiver_resident->authored->external_targets[1]==0 &&
        !receiver_resident->authored->material.detail.pixels.empty(),
        "raw shadow image control lacks actual resident receiver stage1");
    const auto receiver_index=std::size_t(receiver_resident-resident.begin());
    rejects([&]{auto altered=*receiver_resident->authored;++altered.material.detail.width;
        auto bad=resident;bad[receiver_index].authored=&altered;validate_source_shadow_raw_plan(plan,bad);});
    auto external_placeholder=*receiver_resident->authored;++external_placeholder.material.diffuse.width;
    auto external_residency=resident;external_residency[receiver_index].authored=&external_placeholder;
    require(validate_source_shadow_raw_plan(plan,external_residency).indices==stats.indices,
        "raw shadow external target10 incorrectly requires its unused resident placeholder");
    rejects([&]{auto reordered=casters;std::swap(reordered.front(),reordered.back());
        auto bad=shadow_root;bad.casters=reordered;auto p=plan;p.roots={&bad,1};validate_source_shadow_raw_plan(p,resident);});
    rejects([&]{auto repeated=casters;repeated[1].command.sorted_ordinal=repeated[0].command.sorted_ordinal;
        auto bad=shadow_root;bad.casters=repeated;auto p=plan;p.roots={&bad,1};validate_source_shadow_raw_plan(p,resident);});
    rejects([&]{auto bad=shadow_root;bad.command_masks[0]=0;
        auto p=plan;p.roots={&bad,1};validate_source_shadow_raw_plan(p,resident);});
    auto empty=shadow_root;empty.casters={};empty.receivers={};
    auto empty_plan=plan;empty_plan.roots={&empty,1};
    require(validate_source_shadow_raw_plan(empty_plan,resident).roots==1,
        "active source command masks lost original empty clear/filter/application chronology");
    empty.command_masks={};
    require(validate_source_shadow_raw_plan(empty_plan,resident).roots==0,
        "inactive original command masks admitted a shadow phase");
    // Dynamic completed pixel/vertex constants are draw data, not a new
    // immutable residency key. The literal selected pass remains unchanged.
    auto changed_resident=completed.front();
    changed_resident.material.forward_program.words[43]^=1;
    changed_resident.material.program_constants[1][0][0]+=1;
    auto dynamic_residency=resident;dynamic_residency.front().authored=&changed_resident;
    require(validate_source_shadow_raw_plan(plan,dynamic_residency).indices==stats.indices,
        "raw shadow residency conflated dynamic constants with static pass identity");
    std::cout<<generate<<" actual caster / "<<apply<<" receiver descriptors joined to literal metadata; "
        <<matrices<<" captured palette matrices; "<<ranges<<" original ordered ranges; "<<rejected
        <<" rejected controls; raw plan camera/residency/order/empty-phase checks passed. "
        <<"CPU admission only; declared raster/clip/surface controls.\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
