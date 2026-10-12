#include "halo2/source_geometry_lod.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
float source_geometry_lod_distance(Vec3 center,Vec3 eye,float scale) {
    const double x=double(center.x)-double(eye.x);
    const double y=double(center.y)-double(eye.y);
    const double z=double(center.z)-double(eye.z);
    return float(std::sqrt((z*z+y*y)+x*x)*double(scale));
}
bool source_geometry_lod_view_modifier(bool camera_58,bool camera_68,
    bool scenario_present,std::int16_t scenario_type) {
    return camera_58 || camera_68 || (scenario_present && scenario_type==2);
}
std::int16_t Cache::load_source_geometry_scenario_type() const {
    const Bytes data(storage_);
    const auto primary=data.u32(data.u32(16)+12);
    const auto tag=query_tag({.handle=TagHandle{primary}, .group=tag_groups::scenario, .require_address=true});
    if(!tag || tag->size<0x12)
        throw FormatError("source geometry primary scenario type is unavailable");
    return std::int16_t(data.u16(metadata_offset(tag->address,0x12)+0x10));
}
SourceGeometryLodChoice source_geometry_lod_choice(const SourceGeometryLodDefinition &definition,
    float draw_metric,std::int8_t previous_lod) {
    constexpr float maximum=.9999f;
    std::array<float,6> thresholds{10,8,6,4,2,0};float first=0;
    const auto& bank=definition.model_bank;
    if(definition.model_present && bank[3]>0.f && bank[3]>=bank[4] &&
        bank[4]>=bank[5] && bank[5]>=bank[6] && bank[6]>=bank[7] && bank[8]==0.f) {
        std::copy_n(bank.begin()+3,6,thresholds.begin());first=bank[0];
    }
    int lod=0;
    // COMISS's unordered result does not take the source's JA exit.
    while(lod<6 && !(draw_metric>thresholds[lod]) && thresholds[lod]!=0.f)++lod;
    float fraction=maximum;
    if(lod<6 && (lod!=0 || first>thresholds[0])) {
        const float upper=lod?thresholds[lod-1]:first;
        fraction=(upper-draw_metric)/(upper-thresholds[lod]);
        if(fraction<0.f)fraction=0.f;else if(fraction>maximum)fraction=maximum;
    }
    if(previous_lod!=-1){
        const int difference=int(previous_lod)-lod;
        if(difference==1 && .8f>fraction)++lod;
        else if(difference==-1 && fraction>.2f)--lod;
    }
    return {lod,fraction};
}
SourceGeometryLodPolicy source_geometry_lod_policy(unsigned players) {
    if(players==2 || players==3)return {3,.8f,.5f,.3f,{0,1,0,0,0,0}};
    if(players==4)return {2,.7f,.25f,.25f,{0,1,0,0,0,0}};
    return {4,1.f,1.f,.85f,{1,0,1,1,1,1}};
}
int source_geometry_lod_cap(SourceGeometryLodChoice choice,const SourceGeometryLodPolicy &policy,
    std::uint32_t flags,bool prt) {
    if(flags&0x80000u)return 5;
    return std::clamp(choice.lod,0,prt?4:policy.ordinary_maximum);
}
}
