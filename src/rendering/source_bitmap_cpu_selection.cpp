#include "halo2/source_bitmap_cpu_selection.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
bool software_category(const SourceBitmapResidentState &state) {
    if(!state.software_category)
        throw FormatError("source CPU bitmap selection requires the actual category");
    const auto category=*state.software_category;
    return (category>=1 && category<=11) || category==17 || category==18;
}
struct InlineImage {
    std::uint32_t token{};
    bool construction{};
};
InlineImage inline_image(const SourceBitmapResidentState &state,
    const SourceBitmapResidentContext &context) {
    if(!context.software_cache_enabled ||
       (!state.software_control && state.software_pending_size<=0))return {};
    if(state.software_control<=0)return {0,true};
    if(!state.software_texture)
        throw FormatError("source CPU bitmap inline image lacks its actual resource");
    return {state.software_texture,false};
}
float adjusted_request(const SourceBitmapResidentState &state,float request,
    const SourceBitmapResidentContext &context) {
    if(context.bias_enabled){
        const float bias=float(state.bias)*.01f;
        const float sum=bias+request;
        request=sum+context.global_bias;
    }
    if(std::isnan(request))
        throw FormatError("source CPU bitmap selection has an unproved unordered request");
    return request;
}
std::uint32_t touch_partition(SourceBitmapResidentState &state,unsigned index,
    std::int32_t use_clock) {
    auto &selected=state.partitions[index];
    if(selected.handle==0xffffffffu)return 0;
    // Original12CCF0/12C990 address the pool by the low16 bits, without a salt
    // comparison. Alias inputs must represent the same actual image/LRU row.
    for(const auto &other:state.partitions){
        if(other.handle!=0xffffffffu &&
           std::uint16_t(other.handle)==std::uint16_t(selected.handle) &&
           (other.texture_token!=selected.texture_token || other.ready!=selected.ready ||
            other.last_use!=selected.last_use))
            throw FormatError("source CPU bitmap aliases disagree about their current pool row");
    }
    for(auto &other:state.partitions){
        if(other.handle!=0xffffffffu &&
           std::uint16_t(other.handle)==std::uint16_t(selected.handle))other.last_use=use_clock;
    }
    if(selected.ready && !selected.texture_token)
        throw FormatError("source CPU bitmap ready row lacks its actual image token");
    return selected.ready?selected.texture_token:0;
}
unsigned resource_partition(const SourceBitmapCpuState &state,float adjusted) {
    unsigned selected=0;
    constexpr std::array<float,3> thresholds{0,1,2};
    for(unsigned index=0;index<state.resources.size();++index){
        const auto &resource=state.resources[index];
        if(resource.location==0xffffffffu || !resource.size ||
           (index && resource.size<=0x400))continue;
        if(thresholds[index]>adjusted)break;
        selected=index;
    }
    return selected;
}
} // namespace

SourceBitmapCpuSelection source_bitmap_cpu_select(SourceBitmapCpuState &state,
    float raw_request,SourceBitmapCpuContext &context) {
    if(context.cache.request_flags!=0)
        throw FormatError("source D15E0 selection requires its literal request flags0");
    auto &resident=state.resident;
    SourceBitmapCpuSelection result;
    // D15E0's future-deadline shortcut does not read the stale request/bank.
    if(resident.cached_until>context.cache.clock && resident.cached_texture){
        result.texture_token=resident.cached_texture;return result;
    }
    if(std::isnan(raw_request))
        throw FormatError("source CPU bitmap selection has an unproved unordered request");
    const float adjusted=adjusted_request(resident,raw_request,context.cache);
    std::uint32_t candidate=0;
    if(!(resident.flags&2u))candidate=resident.cached_texture;
    else if(context.cache.software_images && software_category(resident)){
        const auto image=inline_image(resident,context.cache);
        if(image.construction){result.boundary=SourceBitmapCpuBoundary::InlineHeaderConstruction;return result;}
        candidate=image.token;
    }else{
        if(std::isnan(resident.cutoff))
            throw FormatError("source CPU bitmap selection has an unproved unordered cutoff");
        if(adjusted<resident.cutoff){
            const unsigned partition=adjusted>=2?2u:(adjusted>=1?1u:0u);
            candidate=touch_partition(resident,partition,context.cache.use_clock);
            if(candidate && partition==0){
                resident.cached_until=std::bit_cast<std::int32_t>(std::uint32_t(context.cache.clock)+2u);
                resident.cached_texture=candidate;
            }
        }
    }
    result.texture_token=resident.cached_until>=context.cache.clock && resident.cached_texture?
        resident.cached_texture:candidate;
    if(result.texture_token)return result;

    result.request_entered=true; // Complete12CE00 with D15E0's flags0.
    float requested=adjusted_request(resident,raw_request,context.cache);
    if(context.force_zero)requested=0;
    if(context.force_one)requested=1;
    if(!(resident.flags&2u))return result;
    const auto image=inline_image(resident,context.cache);
    if(image.construction){result.boundary=SourceBitmapCpuBoundary::InlineHeaderConstruction;return result;}
    candidate=image.token;

    bool wait=false;
    if(context.hardware_mode>0 &&
       (context.hardware_category==0 ||
        (context.hardware_category==1 && resident.software_category && *resident.software_category==3))){
        wait=context.hardware_mode==1;
    }else if(candidate && !(state.flags_low&0x80u))wait=false;
    if(context.hardware_mode>0 && context.hardware_category==1 && !resident.software_category)
        throw FormatError("source CPU bitmap forced category lacks its actual byte");

    const bool software=context.cache.software_images && software_category(resident);
    if((!software && raw_request<resident.cutoff) || !candidate){
        result.requested_partition=resource_partition(state,requested);
        // Literal CL=0 selects12CABD: ready lookup only, irrespective of
        // request mode, pressure, wait byte or authored resource eligibility.
        const auto ready=touch_partition(resident,result.requested_partition,context.cache.use_clock);
        if(ready){result.texture_token=ready;return result;}
        for(unsigned index=0;index<resident.partitions.size();++index){
            const auto token=touch_partition(resident,index,context.cache.use_clock);
            if(token){candidate=token;wait=false;}
        }
        if(wait && !candidate){
            const unsigned type=state.type==0?0u:(state.type==1?1u:2u);
            result.texture_token=context.default_textures[type];
            context.fallback_marked=true;return result;
        }
    }
    result.texture_token=candidate;return result;
}
} // namespace halo2
