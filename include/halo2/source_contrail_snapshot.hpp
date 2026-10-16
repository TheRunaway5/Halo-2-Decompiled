#pragma once
#include "halo2/physics_contact_record.hpp"
#include <memory>
#include <vector>
namespace halo2 {
class Cache;class SourceRuntimeMemory;
// Actual retained TU1.5 render inputs. Addresses identify original data; no
// code, generated trajectory, geometry result or physics override is carried.
struct SourceContrailSlice {
    std::uint32_t address{};
    std::vector<std::byte> bytes;
    bool operator==(const SourceContrailSlice &)const=default;
};
struct SourceContrailSnapshot {
    bool authoritative{};
    std::vector<SourceContrailSlice> slices;
    bool operator==(const SourceContrailSnapshot &)const=default;
};
void validate_source_contrail_snapshot(const SourceContrailSnapshot &);
SourceContrailSnapshot capture_source_contrail_snapshot(const SourceRuntimeMemory &);
// Read-only sparse live inputs over the immutable owned original data image.
// The original geometry callback keeps its separate caller-owned scratch.
class SourceContrailSnapshotMemory final:public SourcePhysicsContactMemory {
public:
    SourceContrailSnapshotMemory(const SourceContrailSnapshot &,const SourceRuntimeMemory &original_data);
    void read(std::uint32_t,std::span<std::byte>)const override;
    void write(std::uint32_t,std::span<const std::byte>)override;
private:
    const SourceContrailSnapshot &snapshot_;const SourceRuntimeMemory &original_data_;
};
}
