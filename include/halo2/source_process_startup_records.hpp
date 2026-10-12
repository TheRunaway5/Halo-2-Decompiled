#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <functional>

namespace halo2 {
struct SourceProcessStartupRecordStore {
    std::uint32_t address;
    std::vector<std::byte> before, written;
};
using SourceProcessStartupRecordObserver = std::function<void(const SourceProcessStartupRecordStore &)>;
// Pinned primary compatibility initializer expressions, not recovered Xbox
// kernel implementations. These write only the declared fields of existing
// 28-byte KDPC / 40-byte KTIMER storage. They do not schedule or dispatch work.
void source_initialize_process_dpc(SourceRuntimeMemory &, std::uint32_t record,
    std::uint32_t routine, std::uint32_t context,
    const SourceProcessStartupRecordObserver & = {});
void source_initialize_process_timer(SourceRuntimeMemory &, std::uint32_t record,
    std::uint32_t type, const SourceProcessStartupRecordObserver & = {});
} // namespace halo2
