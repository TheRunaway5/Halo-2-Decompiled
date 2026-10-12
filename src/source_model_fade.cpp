#include "halo2/source_model_fade.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
template<class T> const T &find(std::span<const T> values,std::uint32_t id,const char *kind) {
    const T *result=nullptr;
    for(const auto &value:values)if(value.identity==id) {
        if(result)throw FormatError(std::string("source model fade duplicate ")+kind);
        result=&value;
    }
    if(!result || id==none)throw FormatError(std::string("source model fade missing ")+kind);
    return *result;
}
template<class T> T field(const std::optional<T> &value,const char *name) {
    if(!value)throw FormatError(std::string("source model fade unresolved ")+name);
    return *value;
}
float finite(const std::optional<float> &value,const char *name) {
    const float result=field(value,name);
    if(!std::isfinite(result))throw FormatError(std::string("source model fade nonfinite ")+name);
    return result;
}
std::uint32_t model(std::span<const SourceModelFadeDefinition> definitions,
                    const SourceModelFadeOwner &owner) {
    return field(find(definitions,field(owner.definition,"object definition"),"definition").model,
                 "definition model");
}
std::uint8_t byte(float value) {
    // Original stores the SSE result to float, then x87 PC53 multiplies by
    // literal255 and unchanged322B0C truncates toward zero. Finite factors
    // remain in[0,1], so double is exact. Finite fields can still overflow an
    // intermediate subtraction; original322B0C yields AL0 for its NaN.
    return std::isfinite(value)?static_cast<std::uint8_t>(static_cast<unsigned>(double(value)*255.0)):0;
}
}
SourceModelFadeBytes source_model_fade(std::span<const SourceModelFadeOwner> owners,
    std::span<const SourceModelFadeDefinition> definitions,
    std::span<const SourceModelFadeModel> models,const SourceModelFadeInput &input) {
    const auto &current=find(owners,input.owner,"owner");
    const auto flags=field(current.flags,"current flags");
    const auto override=field(input.view_modifier,"4BA019");
    // This is before current definition/model lookup in the original routine.
    if(((flags>>19)&1u) || override)return {255,255};
    auto selected=model(definitions,current);
    if(selected==none)return {};
    const auto *root=&current;
    std::vector<std::uint32_t> visited;
    for(;;) {
        if(std::find(visited.begin(),visited.end(),root->identity)!=visited.end())
            throw FormatError("source model fade parent cycle");
        visited.push_back(root->identity);
        const auto parent=field(root->parent,"parent link");
        if(parent==none)break;
        root=&find(owners,parent,"parent owner");
    }
    if(root->identity!=input.owner) {
        const auto root_model=model(definitions,*root);
        if(root_model!=none)selected=root_model;
    }
    const auto &metadata=find(models,selected,"model metadata");
    const float metric=finite(input.draw_metric,"XMM1 metric");
    const float radius=finite(current.radius,"current radius");
    if(radius<0)throw FormatError("source model fade negative current radius");
    const auto count=field(metadata.render_lod_count,"model4C count");
    const auto index=4-int(count);
    const float cutoff=index>0?finite(metadata.render_cutoffs[unsigned(std::min(index,4))],"selected render cutoff"):
        std::numeric_limits<float>::max();
    const float width=radius*7.f;
    float first=1,second=1;
    if(metric>=cutoff+width)second=0;
    else if(metric>cutoff) {
        const float fraction=(metric-cutoff)/width;
        const float value=1.f-fraction;
        if(value<0)second=0;
        else if(value>1)second=1;
        else second=value<=0.0949999988079071f?0:value;
    }
    const float end=finite(metadata.fade_end,"model28 end");
    if(end>0) {
        if(metric>=end)first=0;
        else {
            const float start=finite(metadata.fade_start,"model2C start");
            if(metric>start) {
                const float numerator=metric-start;
                const float denominator=end-start;
                const float fraction=numerator/denominator;
                first=1.f-fraction;
            }
        }
    }
    return {byte(first),byte(second)};
}
}
