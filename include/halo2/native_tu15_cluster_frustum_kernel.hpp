#pragma once
#include <cstdint>

namespace halo2::native_cluster_frustum_detail {
struct Range {std::int32_t index{},count{};};
// Aggregate the authored frustum ranges in group order. Visibility is sticky;
// containment stops the current range, while subsequent groups still acquire
// their own counts. The view owns lazy data and genuine classifier calls.
template<class View>auto aggregate(View&view){
    if(view.begin()>0){
        view.open_groups();
        do{
            auto range=view.range();
            while(range.index<range.count){
                if(view.contained())break;
                const auto visibility=view.classify(range);
                if(visibility)view.accept(visibility==2u);
                range=view.next(range);
            }
        }while(view.next_group());
        view.close_groups();
    }
    return view.finish();
}
} // namespace halo2::native_cluster_frustum_detail
