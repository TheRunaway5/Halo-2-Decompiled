#pragma once
#include "halo2/native_tu15_solver_hull_query.hpp"

namespace halo2 {
// The same DATA view must outlive the selected30DE60 call and every consumer
// of its arithmetic work. Unprovided rows have no backing-byte read capability.
// A genuine later source-owned write defines only the bytes it actually writes.
class NativeTu15ConvexSimplexMemory final : public SourcePhysicsContactMemory {
    struct Region {
        std::uint32_t address;
        std::vector<bool> defined;
    };
    SourcePhysicsContactMemory& storage_;
    std::vector<Region> regions_;
    mutable std::uint64_t refused_reads_{};
    void check(std::uint32_t,std::size_t) const;
public:
    explicit NativeTu15ConvexSimplexMemory(SourcePhysicsContactMemory& storage)
        :storage_(storage) {}
    // Metadata only: never reads, initializes or serializes the backing bytes.
    void withhold(std::uint32_t,std::size_t);
    std::uint64_t refused_reads() const noexcept { return refused_reads_; }
    void read(std::uint32_t,std::span<std::byte>) const override;
    void read_vector(std::uint32_t,std::span<std::byte,16>) const override;
    void write(std::uint32_t,std::span<const std::byte>) override;
};

struct NativeTu15ConvexSimplexSingleInput {
    std::uint32_t work{};
    NativeTu15ContactComponent first,second;
    std::uint32_t relative_transform{};
};
struct NativeTu15ConvexSimplexSingleScratch {
    std::uint32_t simplex{},differences{},output_plane{};
    NativeTu15SolverHullQueryScratch hull;
};
struct NativeTu15ConvexSimplexSingleStats {
    std::uint64_t attempts{},completed{},refusals{},input_domain_refusals{},
        result_domain_refusals{};
    NativeTu15SolverHullQueryStats hull;
};
struct NativeTu15ConvexSimplexSingleServices {
    std::function<void(SourcePhysicsContactMemory&,NativeTu15ContactComponent,
        NativeTu15ContactVirtualOwner,std::uint32_t direction,
        std::uint32_t output,NativeTu15PhysicsFrameFp&)> support;
};

// Selected30DE60 domain: both incoming counts<3, native hull returns count1.
// Four Source copies of unprovided local rows are deliberately unavailable in
// this retained-DATA/FP contract. Raw private stores/beforebytes/address faults
// are not admitted. No SourcePC observer or generic child executor is accepted.
// Every support provider must consume the SAME guarded memory passed here.
std::uint32_t native_tu15_solve_single_vertex_convex_simplex(
    NativeTu15ConvexSimplexMemory&,NativeTu15ConvexSimplexSingleInput,
    NativeTu15ConvexSimplexSingleScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const NativeTu15ConvexSimplexSingleServices&,
    const std::function<std::uint64_t()>& timestamp_counter={},
    NativeTu15ConvexSimplexSingleStats* = nullptr);
} // namespace halo2
