#include "halo2/native_tu15_frustum_draw_cull.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include <bit>

namespace halo2 {
namespace {
std::span<std::uint32_t> mask_for(SourceBspRangeArena& arena,std::uint32_t base,
    const SourceCullDrawUnit& unit,std::size_t count) {
    const auto address=unit.words[5];
    if(!base||address<base||((address-base)&3u))
        throw FormatError("Source frustum cull lost its actual mask backing");
    const auto offset=std::size_t(address-base)/4u;
    if(offset>arena.words.size()||count>arena.words.size()-offset)
        throw FormatError("Source frustum cull mask escapes its shared arena");
    return std::span(arena.words).subspan(offset,count);
}
void linear_entry(SourceCullDrawUnit& unit,SourceCullLinearGeometry& geometry,
    SourceBspRangeArena& arena,std::uint32_t mask_base,
    const SourcePortalVisibilitySet& visibility,unsigned entry,const NativeTu15FrustumBoundObserver& observe) {
    //45372 composes the complete current WORLD/inverse-bind bank before
    // testing the signed subpart count or inspecting any existing mask bit.
    // Unlike45560, this branch has no NONE-owner or FFFF-bound shortcut.
    if(unit.words[1]==0xffffffffu)
        throw FormatError("Source linear frustum cull requires its original model context");
    if(!geometry.owner_pose&&geometry.resolve_owner_pose)
        geometry.owner_pose=geometry.resolve_owner_pose();
    if(!geometry.owner_pose||geometry.owner_pose->owner==0xffffffffu)
        throw FormatError("Source linear frustum cull lost its current owner node bank");
    const auto& pose=*geometry.owner_pose;
    if(pose.nodes.size()>255u)
        throw FormatError("Source linear frustum node count exceeds its matrix bank");
    if(pose.inverse_bind.size()<pose.nodes.size())
        throw FormatError("Source linear frustum cull inverse-bind prefix is incomplete");
    std::vector<Transform> nodes;nodes.reserve(pose.nodes.size());
    for(unsigned i=0;i<pose.nodes.size();++i)
        nodes.push_back(compose_transform(pose.nodes[i],pose.inverse_bind[i]));
    if(geometry.subpart_count<=0)return;
    const auto count=std::size_t(geometry.subpart_count);
    if(count>geometry.subpart_bounds.size())
        throw FormatError("Source linear frustum cull subpart prefix is incomplete");
    auto mask=mask_for(arena,mask_base,unit,(count+31u)/32u);
    const auto frustums=visibility.sphere_bank();
    for(std::size_t i=0;i<count;++i) {
        const auto bit=1u<<(i&31u);auto& word=mask[i>>5u];
        if(word&bit)continue;
        const auto index=geometry.subpart_bounds[i];
        if(index>=geometry.bounds.size())
            throw FormatError("Source linear frustum cull consumed an unavailable literal bound");
        const auto& bound=geometry.bounds[index];
        if(bound.node>=nodes.size())
            throw FormatError("Source linear frustum cull consumed an unconstructed node matrix");
        const auto& transform=nodes[bound.node];auto point=bound.center;
        if(transform.scale!=1.f)
            point={point.x*transform.scale,point.y*transform.scale,point.z*transform.scale};
        //4547E..45519: Z product plus Y, then X, then translation. The
        // bound radius is passed unchanged, including nonunit WORLD scales.
        point={((transform.basis[2].x*point.z+transform.basis[1].x*point.y)+transform.basis[0].x*point.x)+transform.position.x,
            ((transform.basis[2].y*point.z+transform.basis[1].y*point.y)+transform.basis[0].y*point.x)+transform.position.y,
            ((transform.basis[2].z*point.z+transform.basis[1].z*point.y)+transform.basis[0].z*point.x)+transform.position.z};
        if(observe)observe({std::uint32_t(i),point,bound.radius,bound.node});
        if(!source_cluster_frustum_visibility(frustums,entry,point,bound.radius).visible)continue;
        unit.words[0]|=1u;word|=bit;
    }
}
}
void native_tu15_frustum_draw_cull_entry_44e80(SourceCullDrawUnit& unit,SourceCullDrawBanks& bank,
    SourceBspRangeArena& arena,std::uint32_t mask_base,std::uint32_t pass_mode,
    const SourcePortalVisibilitySet& visibility,unsigned entry,const NativeTu15FrustumBoundObserver& observe) {
    if(entry>=visibility.clusters.size())
        throw FormatError("Source frustum cull constructed cluster entry is unavailable");
    if(!bank.geometry_resolved||!bank.geometry)
        throw FormatError("Source frustum cull resident geometry is unavailable");
    auto& geometry=*bank.geometry;
    if(geometry.accelerator_count>0) {
        if(!geometry.accelerator||geometry.accelerator->accelerator_count!=geometry.accelerator_count||
            geometry.accelerator->subpart_count!=geometry.subpart_count)
            throw FormatError("Source frustum cull accelerator geometry is incomplete");
        source_bsp_frustum_cull_entry(unit,{geometry.accelerator,true,bank.transform},arena,
            {0,0,pass_mode,0,mask_base},visibility,entry);
    }else {
        auto next_unit=unit;auto next_arena=arena;
        linear_entry(next_unit,geometry,next_arena,mask_base,visibility,entry,observe);
        unit=next_unit;arena=next_arena;
    }
}
std::vector<NativeTu15FrustumDrawCall> native_tu15_frustum_draw_cull_44710(
    std::span<SourceCullDrawUnit> units,SourceBspRangeArena& arena,std::uint32_t mask_base,
    const SourceDrawUnitCullContext& context,const SourcePortalVisibilitySet& visibility,
    std::span<const std::array<std::uint32_t,4>> associations,
    const std::function<SourceCullDrawBanks(unsigned,const SourceCullDrawUnit&)>& resolve) {
    const unsigned first=context.mode?context.main_prefix&65535u:0u;
    const unsigned end=(context.mode?context.draw_count:context.main_prefix)&65535u;
    std::vector<NativeTu15FrustumDrawCall> calls;
    for(unsigned i=first;i<end;++i) {
        if(i>=32768u||i>=units.size())throw FormatError("Source frustum cull draw prefix is unavailable");
        auto& unit=units[i];const auto flags=unit.words[0];
        if((flags&1u)||!(flags&0x40u))continue;
        if(context.query_mode!=0)throw FormatError("Native44710 frustum owner requires query mode0");
        std::optional<SourceCullDrawBanks> bank;
        const auto call=[&](unsigned entry) {
            if(!bank) {
                if(!resolve)throw FormatError("Source frustum cull geometry owner is unavailable");
                bank=resolve(i,unit);
            }
            calls.push_back({std::uint16_t(i),std::uint16_t(entry),context.mode?context.main_prefix+(i-first):i});
            native_tu15_frustum_draw_cull_entry_44e80(unit,*bank,arena,mask_base,context.mode,visibility,entry);
        };
        if(flags&8u) {
            const auto association=std::bit_cast<std::int16_t>(std::uint16_t(unit.words[6]));
            if(association<0||std::size_t(association)>=associations.size())
                throw FormatError("Source frustum cull association bank is unavailable");
            for(unsigned word=0;word<4;++word) {
                auto bits=associations[association][word];
                while(bits) {
                    const unsigned bit=std::countr_zero(bits);bits&=bits-1u;call(word*32u+bit);
                }
            }
        }else call(std::uint16_t(unit.words[6]));
    }
    return calls;
}
}
