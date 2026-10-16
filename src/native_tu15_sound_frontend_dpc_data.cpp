#include "halo2/native_tu15_sound_frontend_dpc_data.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace halo2 {
namespace {
struct Literal {std::uint32_t address;std::span<const std::byte> bytes;};
bool present(const SourceRuntimeMemory& m,const Literal& row) {
    std::size_t covered=0;
    m.visit_committed([&](auto first,auto bytes){
        const auto begin=std::max<std::uint64_t>(first,row.address);
        const auto end=std::min<std::uint64_t>(std::uint64_t(first)+bytes.size(),std::uint64_t(row.address)+row.bytes.size());
        if(begin<end)covered+=std::size_t(end-begin);
    });
    if(!covered)return false;
    if(covered!=row.bytes.size())throw std::invalid_argument("Native DPC immutable operand is only partially owned");
    for(std::size_t i=0;i<row.bytes.size();++i) {
        const auto p=row.address+std::uint32_t(i);
        if(!m.is_read_only(p,1)||m.view(p,1)[0]!=row.bytes[i])
            throw std::invalid_argument("Native DPC existing immutable operand differs in access or bytes");
    }
    return true;
}
}
void mount_native_tu15_sound_frontend_dpc_data(SourceRuntimeMemory& m) {
    constexpr std::array raw0{std::byte{0x83},std::byte{0x75},std::byte{0x38},std::byte{0x00},std::byte{0xec},std::byte{0x3d},std::byte{0x38},std::byte{0x00}};
    constexpr std::array raw1{std::byte{0x98},std::byte{0x91},std::byte{0x38},std::byte{0x00},std::byte{0xec},std::byte{0x3d},std::byte{0x38},std::byte{0x00}};
    constexpr std::array raw2{std::byte{0xaf},std::byte{0xc5},std::byte{0x38},std::byte{0x00},std::byte{0xec},std::byte{0x3d},std::byte{0x38},std::byte{0x00},std::byte{0x0a},std::byte{0x3e},std::byte{0x38},std::byte{0x00},std::byte{0xf9},std::byte{0x3d},std::byte{0x38},std::byte{0x00},std::byte{0xe0},std::byte{0xa9},std::byte{0x38},std::byte{0x00},std::byte{0x2b},std::byte{0xaa},std::byte{0x38},std::byte{0x00},std::byte{0x1e},std::byte{0xb4},std::byte{0x32},std::byte{0x00},std::byte{0x1e},std::byte{0xb4},std::byte{0x32},std::byte{0x00},std::byte{0x1e},std::byte{0xb4},std::byte{0x32},std::byte{0x00},std::byte{0x1e},std::byte{0xb4},std::byte{0x32},std::byte{0x00},std::byte{0x2c},std::byte{0xd2},std::byte{0x38},std::byte{0x00},std::byte{0xec},std::byte{0x3d},std::byte{0x38},std::byte{0x00},std::byte{0x0a},std::byte{0x3e},std::byte{0x38},std::byte{0x00},std::byte{0xf9},std::byte{0x3d},std::byte{0x38},std::byte{0x00},std::byte{0xe0},std::byte{0xa9},std::byte{0x38},std::byte{0x00},std::byte{0xc3},std::byte{0xd0},std::byte{0x38},std::byte{0x00},std::byte{0xd7},std::byte{0xca},std::byte{0x38},std::byte{0x00},std::byte{0xb9},std::byte{0xcb},std::byte{0x38},std::byte{0x00},std::byte{0xe8},std::byte{0xcb},std::byte{0x38},std::byte{0x00},std::byte{0xdd},std::byte{0xd1},std::byte{0x38},std::byte{0x00},std::byte{0x00},std::byte{0x80},std::byte{0xbb},std::byte{0x46},std::byte{0x96},std::byte{0x96},std::byte{0x16},std::byte{0x3f},std::byte{0xcd},std::byte{0xcc},std::byte{0x4c},std::byte{0x3f},std::byte{0xcd},std::byte{0xcc},std::byte{0xcc},std::byte{0x49}};
    constexpr std::array raw3{std::byte{0x9c},std::byte{0xf9},std::byte{0x38},std::byte{0x00},std::byte{0x9c},std::byte{0xf9},std::byte{0x38},std::byte{0x00},std::byte{0x94},std::byte{0xf9},std::byte{0x38},std::byte{0x00},std::byte{0x8c},std::byte{0xf9},std::byte{0x38},std::byte{0x00}};
    constexpr std::array raw4{std::byte{0x02},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x3c},std::byte{0xfa},std::byte{0x38},std::byte{0x00}};
    constexpr std::array raw5{std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x01},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00}};
    constexpr std::array raw6{std::byte{0x99},std::byte{0x00},std::byte{0x00},std::byte{0x80}};
    constexpr std::array raw7{std::byte{0x97},std::byte{0x00},std::byte{0x00},std::byte{0x80}};
    const std::array rows{Literal{0x42122c,raw0},Literal{0x4212b0,raw1},Literal{0x421304,raw2},Literal{0x38f84c,raw3},Literal{0x38f99c,raw4},Literal{0x38fa3c,raw5},Literal{0x41b7f4,raw6},Literal{0x41b7b8,raw7}};
    std::array<bool,rows.size()> owned{};
    for(std::size_t i=0;i<rows.size();++i)owned[i]=present(m,rows[i]);
    std::array<std::uint32_t,rows.size()> added{};std::size_t count=0;
    try {
        for(std::size_t i=0;i<rows.size();++i)if(!owned[i]) {
            m.mount(rows[i].address,rows[i].bytes,SourceRuntimeMemory::Access::ReadOnly,"Authenticated TU1.5 immutable PCM voice/DPC operand");
            added[count++]=rows[i].address;
        }
    }catch(...) {
        // This quiescent cold assembly owns only these newly added rows.
        // Existing immutable backing is never released or replaced.
        while(count)m.release(added[--count]);
        throw;
    }
}
}
