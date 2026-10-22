#include "halo2/source_object_light_rays.hpp"
#include <bit>
#include <utility>
namespace halo2 {
SourceObjectLightRaysResult source_object_light_rays(const SourceObjectLightRaysInput& in,
    const SourceObjectLightRayContact& contact,const SourceObjectLightRaySample& sample,
    ObjectLightmapSample& output) {
    SourceObjectLightRaysResult out{2,in.caller_contact,{}};
    if(!in.bank_valid)return out;
    constexpr Vec3 one[]{ {0,0,-10} };
    constexpr Vec3 five[]{ {-10,0,0},{10,0,0},{0,-10,0},{0,10,0},{0,0,-40} };
    std::span<const Vec3> rays;
    if(in.flags&1)rays=five;
    else if(in.flags&4) {
        if(!in.custom_delta)throw FormatError("source object light custom ray missing");
        rays=std::span<const Vec3>(&*in.custom_delta,1);
    }else rays=one;
    if(!contact)throw FormatError("source object light contact service missing");
    constexpr float offset=std::bit_cast<float>(0x3a83126fu);
    for(std::uint32_t i=0;i<rays.size() && out.status==2;++i) {
        const auto delta=rays[i];
        const Vec3 start{in.origin.x-delta.x*offset,in.origin.y-delta.y*offset,
            in.origin.z-delta.z*offset};
        SourceObjectLightRayStep step{{i,in.excluded,in.initial_query,start,delta},false,{},out.contact};
        step.contacted=contact(step.request,out.contact);step.contact=out.contact;
        if(step.contacted) {
            if(!sample)throw FormatError("source object light sample service missing");
            out.status=sample(out.contact,in.flags,in.initial_query,output);
            step.sample_status=out.status;
        }
        out.steps.push_back(std::move(step));
    }
    return out;
}
}
