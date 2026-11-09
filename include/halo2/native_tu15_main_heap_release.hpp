#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <map>
namespace halo2 {
struct NativeTu15MainHeapReleaseServices {
    // Current allocator virtual+C, RET4(data). This is a semantic lifetime
    // owner; no original executor, return image or allocator alias is used.
    std::function<void(std::uint32_t allocator,std::uint32_t owner,
        std::uint32_t data,NativeTu15PhysicsFrameFp&,unsigned)> release;
    // Retained ABI adapters enter the same native ledger through its actual
    // caller frame. Ordinary typed owners compose the DATA function directly.
    std::function<void(std::uint32_t allocator,std::uint32_t data,
        NativeTu15PhysicsFrameFp&,unsigned)> ledger_entry;
};
enum class NativeTu15MainHeapReleasePhase {owner_table,count_acquired,header_word,cursor_ready,
    candidate,search_advanced,matched,copy_begin,copy_word,copy_advanced,publication_inputs};
struct NativeTu15MainHeapReleasePhaseData {
    std::uint32_t table{},count{},base{},end{},cursor{},index{},offset{},data{},value{},field{};
    bool copying{};
};
struct NativeTu15MainHeapReleasePhases {
    std::function<void(NativeTu15MainHeapReleasePhase,const NativeTu15MainHeapReleasePhaseData&)> observe;
};
struct NativeTu15MainHeapReleaseStats {
    std::uint64_t wrapper_calls{},ledger_calls{},visited{},matched{},moved_rows{},
        completed{},unbound_services{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Actual232C80: source allocation-ledger removal, not a host free or pool
// alias. Signed current counts, backward search, lazy rereads and word-by-word
// overlapping compaction retain their original order and wrapping addresses.
void native_tu15_remove_main_heap_allocation(SourcePhysicsContactMemory&,
    std::uint32_t allocator,std::uint32_t data,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MainHeapReleaseStats* = nullptr);
void native_tu15_remove_main_heap_allocation_with_phases(SourcePhysicsContactMemory&,
    std::uint32_t allocator,const std::function<std::uint32_t()>&acquire_data,
    const NativeTu15MainHeapReleasePhases&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15MainHeapReleaseStats* = nullptr);
void native_tu15_release_main_heap_allocation_with_phases(SourcePhysicsContactMemory&,
    std::uint32_t allocator,const std::function<std::uint32_t()>&acquire_data,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15MainHeapReleaseServices&,const NativeTu15MainHeapReleasePhases&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MainHeapReleaseStats* = nullptr);
// Actual232D20. The bytes/tag formals are unused by the source wrapper.
// Source callers discard its volatile EAX; only real child DATA/FP effects
// are published. The known232C80 child is composed directly; other owners
// require the explicitly supplied typed service or are counted and refused.
void native_tu15_release_main_heap_allocation(SourcePhysicsContactMemory&,
    std::uint32_t allocator,std::uint32_t data,std::uint32_t bytes,std::uint32_t tag,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const NativeTu15MainHeapReleaseServices& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MainHeapReleaseStats* = nullptr);
} // namespace halo2
