#pragma once
#include <bit>
#include <cstdint>

namespace halo2::native_camera_instance_admission_detail {
enum class Read {count,rows,last_flags,mask,scenario,cluster_count,cluster_table,table_count,mapping_rows,mapping_entry};
struct Progress {
    std::uint32_t instance{},definition{},count{},rows{},last_flags{},mask{},scenario{},cluster_count{},table{},table_count{},mapping{},entry{};
};
// Definition availability, authored last-row exclusion and the optional
// scenario instance table. Storage is lazy and belongs to the caller view.
// This predicate never executes or supplies a ray-query result.
template<class Access>bool admit(Access&memory,Progress&d){
    d.instance=memory.instance();d.definition=memory.definition();
    d.count=memory.read(Read::count,d);
    if(std::bit_cast<std::int32_t>(d.count)<=0)return false;
    d.rows=memory.read(Read::rows,d);d.last_flags=memory.read(Read::last_flags,d);
    if(d.last_flags&0x10u)return false;
    d.mask=memory.read(Read::mask,d);if(!(d.mask&0x40000u))return true;
    d.scenario=memory.read(Read::scenario,d);d.cluster_count=memory.read(Read::cluster_count,d);
    if(std::bit_cast<std::int32_t>(d.cluster_count)<=0)return true;
    d.table=memory.read(Read::cluster_table,d);if(!d.table)return true;
    memory.index(d);if(std::bit_cast<std::int32_t>(d.instance)<0)return false;
    d.table_count=memory.read(Read::table_count,d);
    if(std::bit_cast<std::int32_t>(d.instance)>=std::bit_cast<std::int32_t>(d.table_count))return false;
    d.mapping=memory.read(Read::mapping_rows,d);d.entry=memory.read(Read::mapping_entry,d);
    return d.entry!=0xffffu;
}
} // namespace halo2::native_camera_instance_admission_detail
