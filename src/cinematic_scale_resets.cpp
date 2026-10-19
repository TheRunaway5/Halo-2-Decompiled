#include "halo2/model_scale_reset.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
std::vector<ModelScaleReset> model_terminal_scale_resets(const RenderModel &model,const AnimationClip &clip) {
    std::vector<ModelScaleReset> result;
    if(clip.nodes.size()!=clip.channels.size() || clip.nodes.size()!=clip.components.size())
        throw FormatError("Scale reset metadata lost its authored channel bank");
    if(clip.frame_count<3)return result;
    for(unsigned node=0;node<clip.nodes.size();++node) {
        if(!(clip.components[node]&4u))continue;
        const auto &track=clip.channels[node][2];
        if(track.times.size()!=track.keys.size())throw FormatError("Scale reset key/time count differs");
        if(track.keys.size()<3 || track.times.back()<clip.frame_count-1)continue;
        for(unsigned k=0;k<track.times.size();++k)
            if(!std::isfinite(track.keys[k][0]) || (k && track.times[k]<=track.times[k-1]))
                throw FormatError("Scale reset metadata has invalid authored keys");
        const float hidden=track.keys.back()[0];
        if(hidden<0)continue;
        unsigned first=unsigned(track.keys.size()-1);
        while(first && track.keys[first-1][0]==hidden)--first;
        if(!first || first==track.keys.size()-1 || track.times[first]-track.times[first-1]!=1)continue;
        float low=track.keys.front()[0],high=low;
        for(unsigned k=1;k<first;++k){low=std::min(low,track.keys[k][0]);high=std::max(high,track.keys[k][0]);}
        if(!(hidden<low) || track.keys[first-1][0]!=high)continue;
        const auto bone=std::find_if(model.bones.begin(),model.bones.end(),[&](const auto &b){return b.name==clip.nodes[node];});
        if(bone!=model.bones.end())result.push_back({unsigned(bone-model.bones.begin()),low,high,hidden});
    }
    return result;
}
bool model_scale_reset(const ModelScaleReset &reset,float before,float after) {
    return after==reset.hidden && before>=reset.visible_min && before<=reset.visible_max;
}
}
