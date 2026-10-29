#include "halo2/source_input_collection_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <stdexcept>

namespace halo2 {
namespace {
struct InputData {std::uint32_t address;std::span<const std::byte> bytes;bool writable;};
#include "native_tu15_input_collection_data.inc"
}
void mount_native_tu15_input_collection_data(SourceRuntimeMemory&memory) {
    struct Region {std::uint32_t address;std::size_t bytes;};
    std::vector<Region> regions;
    memory.visit_committed([&](auto address,auto bytes){regions.push_back({address,bytes.size()});});
    for(const auto&data:input_data) {
        const auto end=std::uint64_t(data.address)+data.bytes.size();
        std::uint64_t at=data.address;
        while(at<end) {
            const auto existing=std::find_if(regions.begin(),regions.end(),[&](const auto&r){
                return r.address<=at&&at<std::uint64_t(r.address)+r.bytes;});
            if(existing!=regions.end()) {
                const auto n=std::size_t(std::min(end,std::uint64_t(existing->address)+existing->bytes)-at);
                if(data.writable)memory.validate_mutable_view(std::uint32_t(at),n);
                else if(!memory.is_read_only(std::uint32_t(at),n)||
                    !std::ranges::equal(memory.view(std::uint32_t(at),n),data.bytes.subspan(std::size_t(at-data.address),n)))
                    throw std::invalid_argument("Native collector operand conflicts with its existing immutable owner");
                at+=n;continue;
            }
            auto stop=end;
            for(const auto&r:regions)if(at<r.address)stop=std::min(stop,std::uint64_t(r.address));
            const auto n=std::size_t(stop-at);
            memory.mount(std::uint32_t(at),data.bytes.subspan(std::size_t(at-data.address),n),
                data.writable?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,
                data.writable?"native TU1.5 local input/profile cold constructor destination":
                              "native TU1.5 authenticated local input/profile operand");
            at=stop;
        }
    }
}
}
