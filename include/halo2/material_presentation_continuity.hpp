#pragma once
#include "halo2/cache.hpp"
#include <memory>
#include <span>

namespace halo2 {
// Exact local post-upload material values and original child-pass topology.
// Cache/image/shader catalogs and GPU residency stay with the same Mesh and
// renderer. Authored image/catalog resident identity is fixed. An explicit
// list of generated root diffuse slots may instead retain exact CPU image
// outputs of an external owner (for example MultiplayerHud's radar pages).
// Their dimensions/format/mip layout/byte extent stay fixed. This does not
// prepare or publish renderer image uploads/descriptors.
// All external material writers must announce each completed batch;
// keep the Mesh, renderer and exported pass/draw borrowers quiescent at commit.
class MaterialPresentationContinuity {
    struct Impl;
public:
    struct Limits {
        std::size_t nodes=262144,links=2097152,depth=128;
        // Native storage policy, not an original Xbox capacity.
        std::size_t checkpoint_bytes=256ull*1024*1024;
    };
    struct Stats {
        // logical_bytes measures the former expanded per-node value layout.
        // unique_bytes counts every node/root/link and distinct reachable
        // immutable value/program/constants/pass/image payload once, including
        // payload reused from older cuts; the unchanged cap applies to this.
        // new/shared count unique payload first created/reused in this capture.
        // Excludes allocator/control blocks/capacity, temporary capture indexes,
        // resident authored assets, externally owned immutable VertexProgramBank/
        // PRT payloads retained by identity, and multi-cut/preparation budgets.
        std::size_t roots{},nodes{},links{},logical_bytes{},new_value_bytes{},shared_value_bytes{},resident_schema_bytes{},generated_image_bytes{},unique_bytes{};
        // Complete exact const-pass/schema comparisons and same-walk reuse.
        // The latter is never carried across external mutation batches.
        std::size_t static_passes_checked{},static_passes_reused{};
        // Capture-only predecessor Values pass comparisons after this node's
        // fixed mutable values matched. Excludes interning and ready/applied.
        std::size_t mutable_passes_checked{},mutable_passes_reused{};
    };
    explicit MaterialPresentationContinuity(Mesh &);
    MaterialPresentationContinuity(Mesh &,Limits);
    MaterialPresentationContinuity(Mesh &,std::span<const unsigned> generated_diffuse_slots);
    MaterialPresentationContinuity(Mesh &,std::span<const unsigned> generated_diffuse_slots,Limits);
    ~MaterialPresentationContinuity();
    MaterialPresentationContinuity(const MaterialPresentationContinuity &)=delete;
    MaterialPresentationContinuity &operator=(const MaterialPresentationContinuity &)=delete;
    // For alias-owning callers that already enforce their own batch barrier.
    void mutating();
    class Mutation {
    public:
        ~Mutation();Mutation(Mutation &&)noexcept;Mutation &operator=(Mutation &&)noexcept;
    private:
        friend class MaterialPresentationContinuity;
        struct Data;std::unique_ptr<Data> data_;explicit Mutation(MaterialPresentationContinuity &);
    };
    // Capture/preparation from a callback inside this scope is rejected.
    Mutation begin_mutation();
    class Checkpoint {
    public:
        bool valid()const noexcept;
        Stats stats()const;
    private:
        friend class MaterialPresentationContinuity;
        struct Data;std::shared_ptr<const Data> data_;
    };
    // Rejection identifies the bounded capture stage, root/node ordinals,
    // accounted bytes and first immutable schema member/byte offset. It never
    // exposes resident addresses or weakens resource/storage admission.
    Checkpoint checkpoint()const;
    bool owns_checkpoint(const Checkpoint &)const noexcept;
    class PreparedRestore;
    class TargetGraph {
    public:
        TargetGraph()=default;
        bool ready()const noexcept;
        bool applied()const noexcept;
        // A scene proves its captured historical PRT link against the target,
        // without temporarily installing that graph into live Materials.
        bool matches_environment(const Mesh &,unsigned,const Material *,
            const std::shared_ptr<const VertexProgramBank> &)const noexcept;
    private:
        friend class MaterialPresentationContinuity;
        friend class PreparedRestore;
        struct Data;std::shared_ptr<Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
        TargetGraph target_graph()const;
    private:
        friend class MaterialPresentationContinuity;
        PreparedRestore();std::shared_ptr<TargetGraph::Data> data_;
    };
    PreparedRestore prepare_restore(const Checkpoint &)const;
    bool restore_ready(const PreparedRestore &)const noexcept;
    // Swap-only. Roots retain their addresses; original child objects regain
    // their captured values/aliases. Keep the consumed token until old graph,
    // pass-bank and draw borrowers may retire outside the publication lock.
    bool commit_restore(PreparedRestore &)noexcept;
private:
    std::unique_ptr<Impl> impl_;
};
}
