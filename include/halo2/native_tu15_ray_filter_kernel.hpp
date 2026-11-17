#pragma once
#include <cstdint>
namespace halo2::native_ray_filter_detail {
inline bool same_nonzero_group(std::uint32_t first,std::uint32_t second){
    return ((first^second)&0xffff0000u)==0u&&(first&0xffff0000u)!=0u;
}
// Native ray/collection group policy. The genuine collection owns acquisition
// of its filter info; the view retains later lazy ray/mask/result accesses.
template<class View>void filter(View&view){
    view.begin();const auto second=view.collection_info();const auto first=view.ray_info();
    view.groups(first,second);
    if(same_nonzero_group(first,second)){view.reject();return;}
    const auto mask=view.mask(first,second);const auto bit=1u<<(second&31u);
    view.accept(mask,bit);
}
} // namespace halo2::native_ray_filter_detail
