#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <map>
namespace halo2 {
struct NativeTu15ModelConstraintRequest {std::uint32_t identity{};};
struct NativeTu15ModelConstraintScratch {NativeTu15ModelConstraintRequest request;};
struct NativeTu15ModelConstraintRequestFields {
    std::uint32_t first_datum{},first_slot{},second_datum{},second_slot{},constraint_class{};
    std::uint16_t first_node{},second_node{};
    std::uint8_t mode{};
    bool has_descriptor{};
};
// Deferred ordinary DATA reads preserve original vector/output alias order.
// A native caller may supply typed local values; no register/frame image enters.
class NativeTu15ModelConstraintRequestData {
public:
    virtual ~NativeTu15ModelConstraintRequestData()=default;
    virtual std::uint32_t position_word(unsigned)const=0;
    virtual std::uint32_t forward_word(unsigned)const=0;
    virtual std::uint32_t descriptor_word()const=0;
};
// Sole general20D8B0, including its null-descriptor two-word store branch.
void native_tu15_initialize_model_constraint_request(SourcePhysicsContactMemory&,
    NativeTu15ModelConstraintRequest,const NativeTu15ModelConstraintRequestFields&,
    const NativeTu15ModelConstraintRequestData&,
    const SourcePhysicsContactRecordObserver& = {});
struct NativeTu15ModelConstraintFrame {
    std::uint32_t scale{};
    std::array<std::uint32_t,3>forward{},side{},up{},position{};
};
static_assert(sizeof(NativeTu15ModelConstraintFrame)==52);
struct NativeTu15ModelConstraintServices {
    // Actual1DD710, including its literal zero admission flag.
    std::function<std::uint32_t(NativeTu15PhysicsComponent,NativeTu15ModelConstraintRequest,
        std::uint32_t flags,NativeTu15PhysicsFrameFp&)>find_existing;
    // Actual1DD7B0. Index FFFFFFFF is the genuine caller input here.
    std::function<bool(NativeTu15PhysicsComponent,std::uint32_t body_slot,
        std::uint32_t extent_bits,NativeTu15PhysicsFrameFp&)>allow_creation;
    // Actual22CEA0. The request is64 caller-owned DATA bytes with the original
    // sparse field layout; unknown padding is never a supplied CPU result.
    std::function<std::uint32_t(NativeTu15ModelConstraintRequest,std::uint32_t flags,
        NativeTu15PhysicsFrameFp&)>create;
};
struct NativeTu15ModelConstraintUpdateStats {
    std::uint64_t calls{},flag_skips{},inactive_skips{},empty_banks{},rows{},
        active_queries{},active_body_queries{},definition_queries{},body_slot_queries{},
        requests{},existing{},creation_refusals{},created{},appends{},list_allocations{},
        published{},unbound_services{},unknown_definition_kinds{},body_frame_calls{},
        frame_compositions{},reference_distance_calls{},reference_distance_points{};
    std::map<std::uint32_t,std::uint64_t>dependent_owners;
};
// Actual1D9EC0 and315C90. Preserve source signed counts and live bank/count reads.
bool native_tu15_component_has_active_body(const SourcePhysicsContactMemory&,
    NativeTu15PhysicsComponent,NativeTu15ModelConstraintUpdateStats* = nullptr);
// Actual1D8B90/2E4650/2E44D0. Return ordinary authored frame fields; there is no
// guest volatile-register state in this native interface.
NativeTu15ModelConstraintFrame native_tu15_model_constraint_body_frame(
    const SourcePhysicsContactMemory&,NativeTu15PhysicsComponent,std::uint32_t body_slot,
    NativeTu15PhysicsFrameFp&,NativeTu15ModelConstraintUpdateStats* = nullptr);
// Actual230F60, admitted original jump-table modes0..3. The result is scalar
// bits and only the genuine SIMD status is published.
std::uint32_t native_tu15_model_constraint_reference_distance(
    const SourcePhysicsContactMemory&,std::uint32_t position,std::uint32_t mode,
    NativeTu15PhysicsFrameFp&,NativeTu15ModelConstraintUpdateStats* = nullptr);
// Actual22D460 with native1F3460,DE8C0,20D8B0 and20D940/16E6A0 dependencies.
// Every unconverted positive-loop owner is an explicit typed service. Once
// selected this parent never reexecutes or falls back to original22D460.
void native_tu15_update_body_constraints(SourcePhysicsContactMemory&,std::uint32_t body_datum,
    NativeTu15ModelConstraintScratch,NativeTu15PhysicsFrameFp&,
    const NativeTu15ModelConstraintServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ModelConstraintUpdateStats* = nullptr);
} // namespace halo2
