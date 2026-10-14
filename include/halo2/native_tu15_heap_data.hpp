#pragma once
#include "halo2/source_runtime_memory.hpp"
#include <array>

namespace halo2 {
// Recovered RtlCreateHeap parameters consumed by actual2DE35F..2DE386.
// These are immutable engine operands, not executable/header byte backing.
struct NativeTu15HeapDefaults {
    std::uint32_t segment_reserve,segment_commit;
    std::uint32_t decommit_total_free_threshold,decommit_free_block_threshold;
};
inline constexpr NativeTu15HeapDefaults native_tu15_heap_defaults{0x100000,0x2000,0x10000,0x1000};

// Only the named data needed by the complete heap constructor under the
// recovered native startup caller (including its aligned parameter record).
// Every region has a native owner and typed initializers. Import identities remain
// opaque source IDs for the existing native platform dispatch. No program
// bytes, executable sections, mutable snapshots or file I/O enter this factory.
// The caller still owns the stack, FS exception chain and real VM/lock services.
SourceRuntimeMemory native_tu15_process_heap_memory();

// Actual2DBA60..2DBA8C caller configuration: HEAP_GROWABLE, native reservation
// and commit extents, and caller-owned RTL_HEAP_PARAMETERS storage.
std::array<std::uint32_t,6> native_tu15_process_heap_arguments(std::uint32_t parameters);
}
