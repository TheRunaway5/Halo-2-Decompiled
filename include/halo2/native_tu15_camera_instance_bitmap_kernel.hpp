#pragma once
#include <bit>
#include <cstdint>
namespace halo2::native_camera_instance_bitmap_detail {
struct Progress {std::uint32_t instance{},cluster{},base{},offset{},address{};bool per_instance{};};
// Cluster collision bitmaps have a compact sentinel row and a per-instance
// row. Signed cluster halves and address arithmetic retain their source bits.
template<class Access>std::uint32_t locate(Access&memory,Progress&d){
    d.instance=memory.instance();d.per_instance=d.instance!=0xffffffffu;
    d.cluster=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(memory.cluster(d)))));
    d.base=memory.base(d);d.offset=d.cluster<<(d.per_instance?10u:5u);
    d.address=d.base+d.offset+(d.per_instance?d.instance+0x201u:1u);
    return d.address;
}
template<class Memory>struct Values {
    const Memory&memory;std::uint32_t instance_value;
    std::uint32_t instance()const{return instance_value;}
    std::uint32_t cluster(const Progress&)const{return memory.word(0x474c24u,2);}
    std::uint32_t base(const Progress&)const{return memory.word(0x4f9b50u);}
};
} // namespace halo2::native_camera_instance_bitmap_detail
