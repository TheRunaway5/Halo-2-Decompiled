#include "halo2/native_tu15_contrail_draw_data.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {namespace {
struct Table {
    std::uint32_t address;
    std::array<std::uint32_t,11> words;
};
// Exact static XBE reads retained by oracle_contrail_state_tu15.py in
// contrail_state_tu15_original.json (SHA256 e926ad8121e9e0f030b3a147edbd503b
// 7b0eef4c2ecb909ad2d915b3853e42aa; original TU1.5 21b207e7...09c4fd).
// Table order follows SourceContrailScene's actual blend_operands[0..2].
constexpr std::array<Table,3> tables{{
    {0x45c438u,{0x302u,0x306u,0x306u,1u,1u,1u,1u,1u,0x8001u,0x8002u,0xffffffffu}},
    {0x45c408u,{0x303u,0u,0x300u,1u,1u,1u,1u,0x303u,0x8002u,0x8001u,0xffffffffu}},
    {0x45c3d8u,{0x8006u,0x8006u,0x8006u,0x8006u,0x800bu,0x8007u,0x8008u,
                0x8006u,0x8006u,0x8006u,0xffffffffu}}
}};
// Separate original42CE0 geometry dispatch table, not blend state. Whole
// original430BE bounds the selector to0..4 before430C7 reads these words.
// contrail_geometry_dispatch_tu15_original.json retains the independent
// authenticated20-byte read from TU1.5 21b207e7...09c4fd.
constexpr std::uint32_t geometry_address=0x436f0u;
constexpr std::array<std::uint32_t,5> geometry_words{
    0x430ceu,0x431c8u,0x431c8u,0x436dfu,0x432bdu};
std::array<std::byte,20> encode_geometry() {
    std::array<std::byte,20> bytes{};
    for(unsigned word=0;word<geometry_words.size();++word)
        for(unsigned byte=0;byte<4;++byte)
            bytes[word*4u+byte]=std::byte(geometry_words[word]>>(byte*8u));
    return bytes;
}
std::array<std::byte,44> encode(const Table& table) {
    std::array<std::byte,44> bytes{};
    for(unsigned word=0;word<table.words.size();++word)
        for(unsigned byte=0;byte<4;++byte)
            bytes[word*4u+byte]=std::byte(table.words[word]>>(byte*8u));
    return bytes;
}
}
void mount_native_tu15_contrail_draw_data(SourceRuntimeMemory& memory) {
    // Follow the cold-data owner preflight used by renderer startup. Inspect
    // reservations as well as committed bytes; a hole is not a free address.
    // Reject all foreign overlap before publishing any blend or geometry bank.
    const auto before=memory.checkpoint();
    for(const auto& region:before.regions) {
        for(const auto& table:tables)
            if(std::uint64_t(region.address)<std::uint64_t(table.address)+44u &&
               std::uint64_t(table.address)<std::uint64_t(region.address)+region.bytes.size())
                throw std::invalid_argument("Native TU1.5 contrail blend data conflicts with an existing owner");
        if(std::uint64_t(region.address)<std::uint64_t(geometry_address)+20u &&
           std::uint64_t(geometry_address)<std::uint64_t(region.address)+region.bytes.size())
            throw std::invalid_argument("Native TU1.5 contrail geometry data conflicts with an existing owner");
    }
    for(const auto& table:tables)
        memory.mount(table.address,encode(table),SourceRuntimeMemory::Access::ReadOnly,
            "native TU1.5 authored eleven-word contrail blend table");
    memory.mount(geometry_address,encode_geometry(),SourceRuntimeMemory::Access::ReadOnly,
        "native TU1.5 authored five-word contrail geometry dispatch table");
}
}
