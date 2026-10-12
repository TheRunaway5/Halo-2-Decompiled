#include "halo2/source_bitmap_residency.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
std::vector<SourceBitmapResidencyRequest> source_bitmap_residency_requests(
    std::array<SourceBitmapStageResidency,4>&stages,const SourceBitmapResidencyView&view){
    if(view.enabled_stages&0xf0 || std::isnan(view.coverage_adjustment))
        throw FormatError("source bitmap residency requires a proved four-stage coverage context");
    std::vector<SourceBitmapResidencyRequest> requests;
    for(unsigned index=0;index<stages.size();++index){
        auto&s=stages[index];
        if(!(view.enabled_stages&(1u<<index)) || s.datum==0xffffffffu)continue;
        if(!std::isfinite(s.frame) || !std::isfinite(s.scalar) || !std::isfinite(s.minimum_request) ||
           s.minimum_request<0 || s.minimum_request>3 || std::floor(s.minimum_request)!=s.minimum_request)
            throw FormatError("source bitmap residency stage state requires literal finite producer fields");
        std::int32_t image=0;
        if(!(std::abs(s.frame)<.0001f)){
            if(!s.image_count || s.image_count>65536)
                throw FormatError("source bitmap residency frame requires its actual bitmap count");
            const float wrapped=float(std::fmod(double(s.frame),double(s.image_count)));
            image=std::int32_t(std::floor(wrapped));
        }
        float fraction=0;
        if(!(s.flags&1u) && !view.view_modifier){
            const float sum=s.scalar+view.coverage_adjustment;
            fraction=sum-.5f;
            if(fraction<0)fraction=0;
            else if(fraction>4)fraction=4;
        }
        float bucket=std::floor(fraction);
        if(bucket>2)bucket=2;
        if(bucket<s.minimum_request){
            requests.push_back({index,s.datum,std::bit_cast<std::int16_t>(std::uint16_t(image)),fraction});
            s.minimum_request=bucket;
        }
    }
    return requests;
}
SourceBitmapResidentPeek source_bitmap_resident_peek(SourceBitmapResidentState&s,
    float request,const SourceBitmapResidentContext&context){
    if(context.request_flags!=3u && context.request_flags!=0u)
        throw FormatError("source bitmap peek requires literal12310 flags3 or D15E0 flags0");
    //Both entrypoints check this cache before12CCF0's bias/software controls.
    if(s.cached_until>context.clock && s.cached_texture)return {s.cached_texture,false};
    std::uint32_t candidate=0;
    if(context.bias_enabled){
        const float bias=float(s.bias)*.01f;
        const float adjusted=bias+request;
        request=adjusted+context.global_bias;
    }
    bool software=false;
    if((s.flags&2u) && context.software_images){
        if(!s.software_category)
            throw FormatError("source software bitmap cache requires its actual category");
        const auto category=*s.software_category;
        software=(category>=1 && category<=11) || category==17 || category==18;
    }
    if(!(s.flags&2u))candidate=s.cached_texture;
    else if(software){
        if(context.software_cache_enabled && (s.software_control!=0 || s.software_pending_size>0)){
            if(s.software_control<1)
                throw FormatError("source software bitmap requires original12C820 header construction");
            if(!s.software_texture)
                throw FormatError("source software bitmap inline header lacks its actual resource");
            candidate=s.software_texture;
        }
    }
    else if(request<s.cutoff){
        const unsigned partition=request>=2.f?2u:(request>=1.f?1u:0u);
        auto&p=s.partitions[partition];
        if(p.handle!=0xffffffffu){
            p.last_use=context.use_clock;
            if(p.ready){
                if(!p.texture_token)throw FormatError("ready source bitmap partition lacks its resident texture token");
                candidate=p.texture_token;
                if(partition==0){
                    s.cached_until=std::bit_cast<std::int32_t>(std::uint32_t(context.clock)+2u);
                    s.cached_texture=candidate;
                }
            }
        }
    }
    const auto token=s.cached_until>=context.clock && s.cached_texture?s.cached_texture:candidate;
    return {token,token==0};
}
} // namespace halo2
