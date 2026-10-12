#include "halo2/source_object_colors.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
Vec3 hsv(Vec3 rgb){
    const auto maximum=rgb.x>std::max(rgb.y,rgb.z)?rgb.x:std::max(rgb.y,rgb.z);
    const auto minimum=rgb.x<=std::min(rgb.y,rgb.z)?rgb.x:std::min(rgb.y,rgb.z);
    const float delta=maximum-minimum,saturation=maximum==0.f?0.f:delta/maximum;
    float hue=0;
    if(saturation!=0.f){
        if(rgb.x==maximum)hue=(rgb.y-rgb.z)/delta;
        else if(rgb.y==maximum)hue=(rgb.z-rgb.x)/delta+2.f;
        else hue=(rgb.x-rgb.y)/delta+4.f;
        hue*=0.1666666716337204f;
        if(hue<0.f)hue+=1.f;
    }
    return {hue,saturation,maximum};
}
Vec3 rgb(Vec3 hsv){
    if(hsv.y==0.f)return {hsv.z,hsv.z,hsv.z};
    const float six=hsv.x*6.f;
    const int sector=int(std::floor(six));
    const float fraction=six-float(sector);
    const float descending=(1.f-hsv.y*fraction)*hsv.z;
    const float bottom=(1.f-hsv.y)*hsv.z;
    const float ascending=(1.f-(1.f-fraction)*hsv.y)*hsv.z;
    switch(sector){
    case 0:return {hsv.z,ascending,bottom};
    case 1:return {descending,hsv.z,bottom};
    case 2:return {bottom,hsv.z,ascending};
    case 3:return {bottom,descending,hsv.z};
    case 4:return {ascending,bottom,hsv.z};
    case 5:return {hsv.z,bottom,descending};
    default:throw FormatError("source object color HSV sector escapes original table");
    }
}
float clamp(float v){return v>=0.f?(v<=1.f?v:1.f):0.f;}
Vec3 clamp(Vec3 v){return {clamp(v.x),clamp(v.y),clamp(v.z)};}
Vec3 interpolate(const SourceObjectColorPermutation& permutation,float amount){
    auto lower=hsv(permutation.lower),upper=hsv(permutation.upper);
    if(std::abs(double(lower.x)-double(upper.x))>.5){
        if(upper.x>lower.x)lower.x+=1.f;
        else upper.x+=1.f;
    }
    const float inverse=1.f-amount;
    Vec3 blend{lower.x*inverse+upper.x*amount,lower.y*inverse+upper.y*amount,
               lower.z*inverse+upper.z*amount};
    if(blend.x>1.f)blend.x-=1.f;
    return rgb(blend);
}
}
Vec3 source_profile_name_color(Vec3 primary) {
    auto value=hsv(primary);
    value.y=std::bit_cast<float>(0x3e555555u); //45E19C.
    value.z=std::bit_cast<float>(0x3f4aaaabu); //45E198.
    return rgb(value);
}
ObjectColors source_object_initial_colors(const SourceObjectColorDefinition& definition,
    Vec3 position,std::uint32_t variant,std::uint32_t override_mask,const ObjectColors& overrides){
    if(definition.channels.size()>4)throw FormatError("source object color bank exceeds material channels");
    ObjectColors result{{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}};
    for(unsigned channel=0;channel<definition.channels.size();++channel){
        const auto& record=definition.channels[channel];
        if(override_mask&(1u<<channel)){result[channel]=clamp(overrides[channel]);continue;}
        if(record.permutations.empty())continue;
        // The retail x87 hash retains double precision until its final float
        // store. Its expression order is Z, X, channel, Y.
        double hash=double(position.z)*double(744.1241455078125f)+
                    double(position.x)*double(315.89312744140625f);
        hash+=double(channel)*double(431.1289367675781f);
        hash+=double(position.y)*double(587.1294555664062f);
        const float choice=float(std::fmod(std::abs(hash),1.0));
        const float blend=float(std::fmod(std::abs(double(channel)*double(0.7121099829673767f)+double(position.y)),1.0));
        float total=0;std::uint32_t admitted=0;unsigned last=0;
        for(unsigned i=0;i<record.permutations.size();++i){
            const auto& p=record.permutations[i];
            if(!p.variant_name || p.variant_name==variant){total+=p.weight;admitted|=1u<<(i&31u);last=i;}
        }
        if(!admitted || total<=0.f)continue;
        float cumulative=0;const float threshold=choice*total;
        for(unsigned i=0;i<record.permutations.size();++i)if(admitted&(1u<<(i&31u))){
            const auto& p=record.permutations[i];cumulative+=p.weight;
            if(i==last || cumulative>=threshold){result[channel]=clamp(interpolate(p,blend));break;}
        }
    }
    return result;
}
SourceObjectColorDefinition Cache::load_source_object_color_definition(std::uint32_t datum)const{
    const auto tag=query_tag({.handle=TagHandle{datum}, .require_address=true});
    if(!tag || tag->group=="mode" || tag->group=="hlmt" || tag->size<0xb4)
        throw FormatError("source object color definition missing");
    const Bytes data(storage_);const auto root=metadata_offset(tag->address,0xb4);
    const auto count=data.u32(root+0xac);
    if(count>4)throw FormatError("source object color channel count exceeds material bank");
    SourceObjectColorDefinition result;
    if(!count)return result;
    const auto channels=metadata_offset(data.u32(root+0xb0),std::size_t(count)*16);
    for(unsigned i=0;i<count;++i){
        const auto at=channels+i*16;SourceObjectColorChannel channel;
        const auto permutations=data.u32(at);channel.function_count=data.u32(at+8);
        if(permutations>65536)throw FormatError("source object color permutation count exceeds limit");
        if(permutations){
            const auto start=metadata_offset(data.u32(at+4),std::size_t(permutations)*32);
            for(unsigned j=0;j<permutations;++j){const auto p=start+j*32;
                channel.permutations.push_back({data.f32(p),{data.f32(p+4),data.f32(p+8),data.f32(p+12)},
                    {data.f32(p+16),data.f32(p+20),data.f32(p+24)},data.u32(p+28)});}
        }
        result.channels.push_back(std::move(channel));
    }
    return result;
}
}
