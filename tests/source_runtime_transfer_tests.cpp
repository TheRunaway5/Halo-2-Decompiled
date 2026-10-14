#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
std::byte original_byte(std::uint32_t index) {
    auto value = index * 0x9e3779b9u;
    value ^= value >> 16;
    value *= 0x85ebca6bu;
    value ^= value >> 13;
    return std::byte(value >> 24);
}
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void transfer(std::string_view operation) {
    // One actual 40 MiB bank fits the simulation budget. Transferring an
    // overlapping extent inside it must preserve memmove byte semantics.
    constexpr std::uint32_t base = 0x10000000u, extent = 40u * 1024u * 1024u;
    halo2::SourceRuntimeMemory memory;
    memory.reserve(base, extent, 4096, "actual source transfer bank");
    std::array<std::byte,4096> page;
    for (std::uint32_t offset = 0; offset < extent; offset += page.size()) {
        for (std::uint32_t i = 0; i < page.size(); ++i)
            page[i] = original_byte(offset+i);
        memory.commit(base+offset, page);
    }
    std::cout << "Ready: actual 40 MiB random bank committed before " << operation << '\n';
    if (operation == "write") memory.write(base+16, memory.view(base, extent-16));
    else memory.read(base, memory.mutable_view(base+16, extent-16));
    const auto bytes = memory.view(base, extent);
    for (std::uint32_t i = 0; i < extent; ++i)
        check(bytes[i] == original_byte(i < 16 ? i : i-16),
              "Overlapping transfer changed an original source byte");
    std::cout << "PASS " << operation << " actual 40 MiB overlapping transfer\n";
}
void boundaries() {
    halo2::SourceRuntimeMemory memory;
    using Access = halo2::SourceRuntimeMemory::Access;
    std::array<std::byte,128> first, second;
    for (unsigned i=0;i<128;++i) {first[i]=std::byte(i);second[i]=std::byte(i+128);}
    memory.mount(0x1000,first,Access::ReadWrite,"first adjacent owner");
    memory.mount(0x1080,second,Access::ReadWrite,"second adjacent owner");
    memory.write(0x1078,memory.view(0x1040,64));
    for (unsigned i=0;i<8;++i)
        check(memory.view(0x1078,8)[i]==std::byte(64+i),"Cross-owner write lost first bytes");
    for (unsigned i=0;i<56;++i)
        check(memory.view(0x1080,56)[i]==std::byte(72+i),"Cross-owner write failed to stage its aliased input");
    memory.read(0x1078,memory.mutable_view(0x1040,64));
    for (unsigned i=0;i<64;++i)
        check(memory.view(0x1040,64)[i]==std::byte(64+i),"Cross-owner read changed its aliased input");
    memory.mount(0x1100,first,Access::ReadOnly,"authored read-only owner");
    const auto unchanged=memory.view(0x10fc,4);
    const std::array before{unchanged[0],unchanged[1],unchanged[2],unchanged[3]};
    bool rejected=false;
    try {memory.write(0x10fc,first);}catch(const std::exception&){rejected=true;}
    check(rejected && std::equal(before.begin(),before.end(),unchanged.begin()),
          "Rejected transfer published bytes before validating the entire destination");
    // Empty token transfers previously required no region owner.
    memory.read(0xffffffffu,{});memory.write(0xffffffffu,{});
    std::cout << "PASS cross-owner staging, full rejection and empty transfer policy\n";
}
}
int main(int argc,char **argv) try {
    std::cout << std::unitbuf;
    check(argc==2,"Select read, write or boundaries transfer case");
    const std::string_view operation=argv[1];
    if (operation=="boundaries")boundaries();
    else {check(operation=="read" || operation=="write","Unknown transfer case");transfer(operation);}
    return 0;
}catch(const std::exception&error){std::cerr << error.what() << '\n';return 1;}
