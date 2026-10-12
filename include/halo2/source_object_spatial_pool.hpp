#pragma once
#include "halo2/source_object_location.hpp"
#include "halo2/source_object_primitive.hpp"
#include <array>
#include <map>

namespace halo2 {
// Actual16B840/16BA40 data arrays used by1CAC60:2048 records in each
// independent pool. Three words are owner references; eight are cluster entries.
struct SourceObjectSpatialDatumPool {
    static constexpr std::uint32_t capacity=2048;
    std::array<std::array<std::uint32_t,8>,capacity> records{};
    std::array<std::uint32_t,64> occupied{};
    std::uint32_t cursor{},highwater{},active_count{};
    std::uint16_t next_generation{};
    std::uint8_t record_words=8;
    bool debug_fill{};
    //16B7A0 uses the first two literal pool-name bytes, with bit15 set.
    // All other record bytes survive when debug-fill flag3 is clear.
    void activate(std::uint16_t name_prefix,bool debug=false);
    std::uint32_t allocate();
    // Caller supplies an actual allocated full token. The original ignores
    // generation on this operation; native rejects invalid index/empty slot.
    void release(std::uint32_t token);
};
struct SourceObjectSpatialPool {
    std::vector<std::uint32_t> heads;
    SourceObjectSpatialDatumPool entries,references;
    explicit SourceObjectSpatialPool(std::size_t clusters,bool collidable=true);
    void activate(bool debug=false);
    // Ordered membership is the actual14A5B0/14A550 output. Both allocations
    // run independently, including partial failure. The source object+60
    // reference head is owned by the live owner, not this pool.
    void insert(std::uint32_t full_owner,std::uint32_t &reference_head,
        const ObjectLightSpatialPrimitive&,std::span<const std::int16_t> memberships);
    void update(std::uint32_t full_owner,std::uint32_t reference_head,
        const ObjectLightSpatialPrimitive&);
    void erase(std::uint32_t full_owner,std::uint32_t &reference_head);
    // Literal B8940/B89B0 order. Every full owner is retained, including an
    // orphan entry after reference allocation failed. No inferred registry.
    std::vector<ObjectLightSpatialRecord> cluster(std::int16_t) const;
private:
    std::uint16_t reference_name_prefix_{};
};

struct SourceObjectRegistrationOwner {
    std::uint32_t full_handle=0xffffffffu;
    std::uint16_t pool_rank{}; // Actual zero-based iterator rank supplied by the owner.
    std::uint32_t parent=0xffffffffu,child_head=0xffffffffu,sibling=0xffffffffu;
    std::uint8_t parent_node{}; // Actual object+18; constructor0 and detachedFF differ.
    SourceObjectPrimitiveInput primitive;
    ObjectLightSpatialSphere rendering_sphere;
    Vec3 root_position;
    SourceObjectLocation location;
    std::int16_t header_cluster=-1;
    std::uint32_t reference_head=0xffffffffu;
};
enum class SourceObjectRegistrationEventKind {
    birth,register_supplied,register_default,primitive_update,
    bound_refresh,unregister,free,attach_link,detach_link
};
struct SourceObjectRegistrationEvent {
    std::uint64_t ordinal{};
    SourceObjectRegistrationEventKind kind{};
    std::uint32_t full_handle=0xffffffffu;
};
struct SourceObjectRegistrationEffects {
    bool registered{},relinked{},old_none_to_valid{};
    // CompleteB8600's subsequent BEF30/B6D60/physics callbacks are outside
    // this prefix. This return asks the caller for those actual services.
    bool activation_services_required{};
};
// Event-driven native lifetime state. This does not discover/register owners
// from a sorted scene vector or invent original successful slot chronology.
// Caller emits actual ordered events; copy/assignment is a complete checkpoint.
class SourceObjectRegistrationJournal {
public:
    explicit SourceObjectRegistrationJournal(std::size_t clusters);
    void birth(std::uint64_t ordinal,const SourceObjectRegistrationOwner&);
    SourceObjectRegistrationEffects register_owner(std::uint64_t ordinal,
        std::uint32_t full_handle,const SourceObjectLocation &supplied,
        const ObjectLightSpatialBsp&);
    SourceObjectRegistrationEffects register_default(std::uint64_t ordinal,
        std::uint32_t full_handle,const BspAtmosphere&,std::int16_t selected_bsp,
        const ObjectLightSpatialBsp&);
    void primitive_update(std::uint64_t ordinal,std::uint32_t full_handle,
        const SourceObjectPrimitiveInput&);
    SourceObjectRegistrationEffects bound_refresh(std::uint64_t ordinal,
        std::uint32_t full_handle,ObjectLightSpatialSphere rendering,
        ObjectLightSpatialSphere next_primitive,Vec3 actual_root_position,
        const BspAtmosphere&,std::int16_t selected_bsp,const ObjectLightSpatialBsp&);
    void unregister(std::uint64_t ordinal,std::uint32_t full_handle);
    // Retail B950A..B9548 / TU1.1 BA3FA..BA438, after actual attachment
    // admission, deactivation/unregister and backend removal. Prepends the
    // child and resets its location only. Marker/local transforms, bounds,
    // dirty/activation and subsequent callbacks remain the caller's owners.
    // A registered child is rejected; its unregister is a separate event.
    void attach_link(std::uint64_t ordinal,std::uint32_t parent,
        std::uint32_t child,std::int16_t actual_parent_node,std::int16_t active_bsp);
    // Complete TU1.1 BA780 -> BF540 topology owner, before BA7D0's retained
    // parent-node transform/velocity/registration services. Full-datum splice;
    // no reordering of the remaining siblings and no inferred child recursion.
    // If absent from the parent's list, BF540 leaves child.sibling unchanged;
    // BA780 still clears parent/node/header80. Returns that actual splice outcome.
    bool detach_link(std::uint64_t ordinal,std::uint32_t child);
    // Original root destruction supplies its prior unregister/child callbacks.
    // Free refuses still-registered owners instead of guessing those callbacks.
    void free(std::uint64_t ordinal,std::uint32_t full_handle);
    const SourceObjectRegistrationOwner& owner(std::uint32_t full_handle) const;
    const SourceObjectSpatialPool& collidable() const {return collidable_;}
    const SourceObjectSpatialPool& noncollidable() const {return noncollidable_;}
    std::span<const SourceObjectRegistrationEvent> events() const {return events_;}
    std::vector<SourceObjectRegistrationOwner> actual_pool_order() const;
private:
    std::map<std::uint32_t,SourceObjectRegistrationOwner> owners_;
    SourceObjectSpatialPool collidable_,noncollidable_;
    std::vector<SourceObjectRegistrationEvent> events_;
    SourceObjectRegistrationOwner& mutable_owner(std::uint32_t);
    void event(std::uint64_t,SourceObjectRegistrationEventKind,std::uint32_t);
    void unlink(SourceObjectRegistrationOwner&);
    SourceObjectRegistrationEffects link(SourceObjectRegistrationOwner&,
        const SourceObjectLocation&,const ObjectLightSpatialBsp&);
};
}
