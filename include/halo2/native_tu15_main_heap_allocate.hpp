#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <map>

namespace halo2 {
// These are caller-owned values, not registers. Retained ABI frames implement
// the same interface over their actual scratch/argument cells, so a ledger or
// child store overlapping a caller word is observed by the next lazy load.
enum class NativeTu15MainHeapAllocationSlot : unsigned {
    result,index,row_cursor,candidate_offset,candidate_bytes,memory_class,bytes
};
class NativeTu15MainHeapCandidateSink {
public:
    virtual ~NativeTu15MainHeapCandidateSink()=default;
    virtual void candidate(unsigned word,std::uint32_t value,std::uint32_t publication)=0;
};
class NativeTu15MainHeapAllocationFrame:public NativeTu15MainHeapCandidateSink {
public:
    virtual std::uint32_t load(NativeTu15MainHeapAllocationSlot)const=0;
    virtual void store(NativeTu15MainHeapAllocationSlot,std::uint32_t,std::uint32_t publication)=0;
    void candidate(unsigned word,std::uint32_t value,std::uint32_t publication)override {
        store(word?NativeTu15MainHeapAllocationSlot::candidate_bytes:
            NativeTu15MainHeapAllocationSlot::candidate_offset,value,publication);
    }
};
class NativeTu15MainHeapAllocationValues final:public NativeTu15MainHeapAllocationFrame {
    std::array<std::uint32_t,7> values_{};
public:
    NativeTu15MainHeapAllocationValues(std::uint32_t bytes,std::uint32_t memory_class) {
        values_[unsigned(NativeTu15MainHeapAllocationSlot::bytes)]=bytes;
        values_[unsigned(NativeTu15MainHeapAllocationSlot::memory_class)]=memory_class;
    }
    std::uint32_t load(NativeTu15MainHeapAllocationSlot slot)const override{return values_.at(unsigned(slot));}
    void store(NativeTu15MainHeapAllocationSlot slot,std::uint32_t value,std::uint32_t)override{values_.at(unsigned(slot))=value;}
};
enum class NativeTu15MainHeapAllocationPhase {
    begin,count,tail_header,tail_cursor,tail_record,capacity_end,capacity,
    search_begin,gap_inputs,gap_call,gap_return,search_advance,
    insert_count,insert_move_begin,insert_header,insert_word,insert_advance,
    insert_publish_header,insert_candidate,result_check,
    tail_inputs,tail_capacity,memory_class,category,branch,aligned_cursor,
    tail_result_base,tail_result,tail_insert_inputs,tail_insert_count,tail_insert_word,
    gap_begin,gap_allocator,gap_capacity,gap_class,gap_category,gap_branch,
    gap_aligned,gap_candidate,gap_base,gap_result,gap_returning,returning
};
struct NativeTu15MainHeapAllocationPhaseData {
    std::uint32_t allocator{},count{},base{},end{},cursor{},bound{},bytes{},
        memory_class{},category{},target{},index{},row{},offset{},length{},
        value{},field{},remaining{},result{},previous_count{},previous_index{},
        previous_offset{},previous_row{},shift_offset{};
    bool aligned{},capacity_checked{},capacity_exhausted{};
};
struct NativeTu15MainHeapAllocationPhases {
    std::function<void(NativeTu15MainHeapAllocationPhase,
        const NativeTu15MainHeapAllocationPhaseData&)> observe;
};
struct NativeTu15MainHeapAlignmentResidual {
    bool required{};
    std::uint32_t instruction{},owner{};
};
struct NativeTu15MainHeapGapRequest {std::uint32_t cursor{},bytes{},limit{};};
struct NativeTu15MainHeapGapArguments {
    std::function<std::uint32_t()> allocator;
    std::function<std::uint32_t()> memory_class;
};
struct NativeTu15MainHeapGapResult {
    std::uint32_t allocation{},cursor{},bytes{},limit{},category{},target{};
    NativeTu15MainHeapAlignmentResidual residual;
};
struct NativeTu15MainHeapAllocationResult {
    std::uint32_t allocation{},cursor{},bytes{};
    NativeTu15MainHeapAlignmentResidual residual;
};
struct NativeTu15MainHeapAllocationServices {
    // Retained callers transport the real child CALL frame and invoke the
    // native gap DATA function below with delayed child argument getters.
    // Ordinary value callers compose that same native child directly.
    std::function<NativeTu15MainHeapGapResult(const NativeTu15MainHeapGapRequest&,
        std::uint32_t allocator,std::uint32_t memory_class,NativeTu15MainHeapCandidateSink&)> probe_gap;
};
struct NativeTu15MainHeapAllocationStats {
    std::uint64_t allocations{},gap_probes{},visited_rows{},shifted_rows{},
        gap_successes{},tail_successes{},exhausted{},alignment_requests{};
    std::map<std::uint32_t,std::uint64_t> unbound_alignment_owners;
};
// Actual232D30. Alignment comes from the mounted source category/branch DATA
// at232D7B/232D70. Null or unknown branch identities are explicit residuals;
// neither invalid classes nor their actual read faults are clamped or guessed.
NativeTu15MainHeapGapResult native_tu15_probe_main_heap_gap(
    SourcePhysicsContactMemory&,const NativeTu15MainHeapGapRequest&,
    const NativeTu15MainHeapGapArguments&,NativeTu15MainHeapCandidateSink&,
    const NativeTu15MainHeapAllocationPhases& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MainHeapAllocationStats* = nullptr);
// Actual232AD0. Signed32 comparisons, wrapping source addresses, backward
// ledger traversal, overlap-safe word moves and every lazy header/frame read
// retain their source transaction order. No resource owner is fabricated.
NativeTu15MainHeapAllocationResult native_tu15_allocate_main_heap_with_phases(
    SourcePhysicsContactMemory&,std::uint32_t allocator,NativeTu15MainHeapAllocationFrame&,
    const NativeTu15MainHeapAllocationServices& = {},
    const NativeTu15MainHeapAllocationPhases& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MainHeapAllocationStats* = nullptr);
std::uint32_t native_tu15_allocate_main_heap(SourcePhysicsContactMemory&,
    std::uint32_t allocator,std::uint32_t bytes,std::uint32_t memory_class,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15MainHeapAllocationStats* = nullptr);
} // namespace halo2
