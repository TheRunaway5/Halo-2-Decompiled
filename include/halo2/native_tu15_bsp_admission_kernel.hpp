#pragma once
#include <cstdint>

namespace halo2::native_bsp_admission_detail {
struct RecordFlags {
    bool fog{},negative{},positive{},primary{},secondary{},contained{};
};
// The raw owner carries the negative-side mask through a signed full word
// before its eventual 16-bit record publication. Preserve that semantic value.
inline std::uint32_t pack(RecordFlags bits){
    return(bits.fog?0x400u:0u)|(bits.negative?0xffff8000u:0u)|
        (bits.positive?0x4000u:0u)|(bits.primary?0x1000u:0u)|
        (bits.secondary?0x2000u:0u)|(bits.contained?0x800u:0u);
}
// Full 136EA0 cluster/instance admission policy. The bank view owns lazy
// acquisitions, mutable stream counters, caller storage and genuine child
// handoffs. Keeping cursors inside that view preserves authored store aliases
// without taking an eager copy of the BSP or its parallel queue streams.
template<class View>auto collect(View&view){
    if(view.begin()){
        do{
            view.cluster();
            view.mark_cluster();
            if(view.cluster_has_geometry()){
                view.cluster_sides();
                if(!view.cluster_allowed())continue;
                view.cluster_fog();
                if(view.cluster_room())view.append_cluster();
            }
            if(view.begin_instances()){
                do{
                    if(view.instance_seen())view.merge_instance();
                    else if(view.instance_has_geometry()){
                        if(view.instance_visible()||view.force_instance()){
                            view.instance_sides();
                            view.instance_fog();
                            view.instance_flags();
                            if(view.contained())view.none_association();
                            else view.allocate_association();
                            if(view.instance_room())view.append_instance();
                        }
                    }
                }while(view.next_instance());
            }
        }while(view.next_cluster());
    }
    return view.finish();
}
} // namespace halo2::native_bsp_admission_detail
