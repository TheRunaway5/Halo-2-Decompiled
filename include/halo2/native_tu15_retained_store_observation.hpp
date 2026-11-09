#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Optional typed retained-DATA observation contract. It never authorizes a
// read or a Source store. A private owner can suppress ONLY its bounded local
// preimages/events; ordinary reads still pass its definition/lifetime guard.
// Default public/numeric memory keeps every original observer event.
class NativeTu15RetainedStoreObservationDomain {
public:
    virtual ~NativeTu15RetainedStoreObservationDomain()=default;
    virtual bool observes_retained_store(std::uint32_t instruction,
        std::uint32_t address,std::size_t bytes) const=0;
    virtual void require_retained_extent(std::uint32_t,std::size_t) const {}
    // Optional same-domain bitwise DATA transfer. False means ordinary
    // storage: the public caller keeps its existing source copy algorithm.
    virtual bool copy_defined_data(std::uint32_t,std::uint32_t,std::uint32_t,
        std::size_t,const SourcePhysicsContactRecordObserver& = {}) {return false;}
    // Invalidating a private lifetime never touches its byte values. Returning
    // false declines the capability; invalidating retained DATA must refuse.
    virtual bool withhold_defined_data(std::uint32_t,std::size_t) {return false;}
    virtual bool data_is_defined(std::uint32_t,std::size_t) const {return true;}
};
inline bool native_tu15_observes_retained_store(const SourcePhysicsContactMemory&m,
    const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::size_t n){
    // Consult even with no observer, so an owned private range cannot conceal
    // an aliased retained heap publication in the read-only qualification pass.
    const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&m);
    const bool permitted=!domain||domain->observes_retained_store(pc,p,n);
    return bool(o)&&permitted;
}
} // namespace halo2
