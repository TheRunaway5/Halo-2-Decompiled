#include "halo2/source_network_session_startup_tu15.hpp"
#include <array>
#include <algorithm>
#include <cstring>
namespace halo2 {
namespace {
std::array<std::byte,4> encode(std::uint32_t value) {
    std::array<std::byte,4> out{};for(unsigned i=0;i<4;++i)out[i]=std::byte(value>>(8*i));return out;
}
void word(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value,const char* name) {
    m.mount(p,encode(value),SourceRuntimeMemory::Access::ReadOnly,name);
}
}
void mount_native_tu15_network_session_data(SourceRuntimeMemory& m) {
    // Exact constructor-owned array/record extents, recovered from each
    // source clear, field access and fixed-size session stride. Every record
    // starts cold, not from a captured or already-constructed game bank.
    // Two authored mutable defaults are installed explicitly below.
    struct Bank {std::uint32_t address,bytes;const char* name;};
    for(auto b:{Bank{0x4da868,0x17d8,"network configuration constructor destination"},
        Bank{0x536c38,0x588,"network endpoint manager constructor destination"},
        Bank{0x535f00,0xd34,"network simulation manager and constructor records"},
        Bank{0x5371c0,0x5a0,"network message registration catalog"},
        Bank{0x537760,0x660,"network gateway constructor destination"},
        Bank{0x537dc0,0x18,"network session bridge constructor destination"},
        Bank{0x537dd8,0x51a0,"network channels constructor destination"},
        Bank{0x53cf78,0x10,"network simulation bridge constructor destination"},
        Bank{0x53cf88,0x78b8*3,"three network session constructor destinations"},
        Bank{0x515f30,28,"network session object bindings"},
        Bank{0x515f50,0x70f8,"network session persisted state constructor destination"},
        Bank{0x51d048,1,"network gateway cold initialized flag"},
        Bank{0x51d050,12,"network session active bridge and timestamp"},
        Bank{0x51d060,0x268,"network startup session selection state"},
        Bank{0x4e5774,0x74,"network simulation session bindings"},
        Bank{0x4839c0,0x2584,"network online task callback records"},
        Bank{0x483888,0x130,"two network simulation channel constructor records"},
        Bank{0x483880,1,"network simulation channels initialized flag"},
        Bank{0x4da088,0x7dc,"network simulation session constructor destination"},
        Bank{0x4dc5d0,0x8e4c,"network persisted player configuration buffer"},
        Bank{0x4dc540,0x90,"network player configuration constructor state"},
        Bank{0x4dc05c,4,"network online task pool binding"},
        Bank{0x4dc3e8,1,"network session cold initialized flag"},
        Bank{0x4dc0e8,0x300,"network cleared handles and thirty-two request records"},
        Bank{0x4d6098,0x144,"network saved session startup state"},
        Bank{0x534588,1,"network saved session initialized flag"},
        Bank{0x53458c,0x40,"network saved session sixteen-slot bank"},
        Bank{0x5345d0,0x1704,"network saved session sixteen constructor records"},
        Bank{0x535cd4,1,"network saved session auxiliary initialized flag"},
        Bank{0x535cd8,0x214,"network saved session polling constructor destination"},
        Bank{0x48380c,1,"network authored configuration initialization flags"},
        Bank{0x515f2f,1,"network authored Ethernet classifier result"}})
        m.mount(b.address,std::vector<std::byte>(b.bytes),SourceRuntimeMemory::Access::ReadWrite,b.name);
    m.write(0x485f40,encode(0xffffffffu)); // Authored online-task cursor default.
    m.write(0x483924,encode(0x45c8e4u)); // Authored second simulation-channel name identity.
    word(m,0x483808,0x4736ac,"network authored configuration interface binding");
    // The reached indirect initializer must execute actual673B0/67400. Other
    // members remain opaque source identities until their consumers run.
    constexpr std::array<std::uint32_t,5> descriptor{0,0x45c7d0,0x29,0,0x673b0};
    std::array<std::byte,20> bytes{};
    for(unsigned i=0;i<descriptor.size();++i)std::memcpy(bytes.data()+4*i,encode(descriptor[i]).data(),4);
    m.mount(0x4736ac,bytes,SourceRuntimeMemory::Access::ReadOnly,"network authored configuration interface fields");
    constexpr std::array<std::array<std::uint8_t,6>,2> ethernet{{{0,0x0d,0x3a,0x5d,0xd1,0xf1},{0,0x50,0xf2,0x10,0x52,0x20}}};
    m.mount(0x44a050,std::as_bytes(std::span(ethernet)),SourceRuntimeMemory::Access::ReadOnly,"network authored Ethernet classifier operands");
    constexpr std::array<char,16> task_name{"online tasks"};
    m.mount(0x45c854,std::as_bytes(std::span(task_name)),SourceRuntimeMemory::Access::ReadOnly,"network online task pool authored name and word padding");
    constexpr std::array<char,36> profile_name{"z:\\player_configuration_cache.dat"};
    m.mount(0x45c9d4,std::as_bytes(std::span(profile_name)),SourceRuntimeMemory::Access::ReadOnly,"network private player cache logical filename and word padding");
    struct Scalar {std::uint32_t address,bytes,value;};
    constexpr Scalar parameters[]{
#include "native_tu15_network_session_parameters.inc"
    };
    for(auto p:parameters) {
        const auto raw=encode(p.value);const auto value=std::span(raw).first(p.bytes);
        bool present=false;
        try {const auto old=m.view(p.address,p.bytes);present=true;
            if(!m.is_read_only(p.address,p.bytes) || !std::ranges::equal(old,value))
                throw std::invalid_argument("Network constructor operand conflicts with existing immutable owner");
        }catch(const std::out_of_range&){}
        if(!present)m.mount(p.address,value,SourceRuntimeMemory::Access::ReadOnly,"network constructor typed scalar operand");
    }
}
}
