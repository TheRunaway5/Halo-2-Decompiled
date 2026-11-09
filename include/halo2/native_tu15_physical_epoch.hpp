#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
enum class NativeTu15PhysicalEpochPhase {begin,array,enabled,iteration_begin,index_advance,limit,bitmap,
    bit_address,bit_test,occupied,record_size,record_base,record_address,object,timestamp,adjusted,
    finish,counter,tick_begin,pool,epoch,epoch_advance,rebase_call};
struct NativeTu15PhysicalEpochPhaseData {
    std::uint32_t pool{},array{},index{0xffffffffu},previous{},limit{},bitmap{},mask{},value{},
        stride{},records{},record{},object{},age{},policy{0x7fffffffu},field{};
    bool enabled{},scan{};
};
struct NativeTu15PhysicalEpochPhases {
    std::function<void(NativeTu15PhysicalEpochPhase,const NativeTu15PhysicalEpochPhaseData&)> observe;
    // Publish the owning data-access site before a retained memory service
    // can throw. This is diagnostic/exception state, not instruction delivery.
    std::function<void(std::uint32_t)> before_memory;
};
struct NativeTu15PhysicalEpochResume {std::uint32_t pool{},policy{0x7fffffffu};};
struct NativeTu15PhysicalEpochServices {
    std::function<void(NativeTu15PhysicsFrameFp&)> reset_objects;
    // Retained callers transport the real child's preserved pool/policy
    // values too, including any caller-frame aliases of its saved words.
    std::function<NativeTu15PhysicalEpochResume(std::uint32_t,NativeTu15PhysicsFrameFp&)> rebase_pages;
};
struct NativeTu15PhysicalEpochStats {
    std::uint64_t ticks{},object_resets{},page_rebases{},objects_cleared{},pages_rebased{};
};
void native_tu15_reset_physical_object_epochs(SourcePhysicsContactMemory&,
    const NativeTu15PhysicalEpochPhases& = {},const SourcePhysicsContactRecordObserver& = {},
    NativeTu15PhysicalEpochStats* = nullptr);
void native_tu15_rebase_physical_page_epochs(SourcePhysicsContactMemory&,std::uint32_t pool,
    const NativeTu15PhysicalEpochPhases& = {},const SourcePhysicsContactRecordObserver& = {},
    NativeTu15PhysicalEpochStats* = nullptr);
void native_tu15_advance_physical_epochs(SourcePhysicsContactMemory&,NativeTu15PhysicsFrameFp&,
    const NativeTu15PhysicalEpochServices& = {},const NativeTu15PhysicalEpochPhases& = {},
    const SourcePhysicsContactRecordObserver& = {},NativeTu15PhysicalEpochStats* = nullptr);
} // namespace halo2
