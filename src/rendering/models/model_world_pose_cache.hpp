#pragma once
#include "halo2/model.hpp"
#include "halo2/model_skin_submission.hpp"
#include "halo2/transform.hpp"
#include <cfenv>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace halo2 {
// The model's source vertices, skin bindings and hierarchy stay immutable and
// outlive this cache. Always sample current WORLD nodes before calling update.
// Returned storage stays valid until this cache is updated or destroyed.
class ModelWorldPoseCache {
    const RenderModel *model_{};
    std::vector<Transform> nodes_;
    std::vector<Vertex> vertices_;
    std::vector<std::array<float,12>> palette_;
    bool gpu_{},deferred_{};
    int rounding_{};
    unsigned mxcsr_{};
    struct Effects {
        unsigned mxcsr{};
        std::uint16_t control{},status{};
        int error{};
        bool normal{},available{};
    };
    Effects effects_{};
    bool effects_ready_{};
    static Effects effects() {
        Effects out;
        const auto *scalar=std::getenv("HALO2_REFERENCE_SCALAR_SKINNING");
        const auto *verify=std::getenv("HALO2_VERIFY_SIMD_SKINNING");
        out.normal=!(scalar && *scalar) && !(verify && *verify);
        out.error=errno;
#if defined(__x86_64__) && !defined(_MSC_VER)
        out.mxcsr=_mm_getcsr();
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.control),"=m"(out.status)::"memory");
        out.available=true;
#endif
        return out;
    }
    static bool eligible(const Effects &before,const Effects &after) {
        return before.available && after.available && before.normal && after.normal &&
            (before.mxcsr&0x1f80u)==0x1f80u && (before.control&0x3fu)==0x3fu &&
            (before.mxcsr&~0x3fu)==(after.mxcsr&~0x3fu) &&
            before.control==after.control && before.status==after.status && before.error==after.error;
    }
    bool contains_effects(const Effects &now)const {
        // This x64 skin closure uses SSE arithmetic. Only borrow when its
        // recorded sticky effects are already present; never replay exceptions.
        return effects_ready_ && now.available && now.normal &&
            (now.mxcsr&~0x3fu)==(effects_.mxcsr&~0x3fu) &&
            (now.mxcsr&effects_.mxcsr&0x3fu)==(effects_.mxcsr&0x3fu) &&
            now.control==effects_.control && now.status==effects_.status && now.error==effects_.error;
    }
public:
    struct Result {std::span<const Vertex> vertices;bool reused{};};
    struct DeferredResult {ModelSkinWorldPoseView pose;bool reused{};};
    Result update(const RenderModel &model,std::span<const Transform> nodes) {
        const auto result=update(model,nodes,false,false);
        return {result.pose.vertices,result.reused};
    }
    // Selected geometry previously computed a fresh skin. Preserve its normal
    // arithmetic/errno effects or rebuild; diagnostic modes always compute.
    Result update_preserving_effects(const RenderModel &model,std::span<const Transform> nodes) {
        const auto result=update(model,nodes,true,false);
        return {result.pose.vertices,result.reused};
    }
    // This mode owns the same original cache policy, but a cold admitted fill
    // retains local records and palette rows until the renderer deforms them.
    DeferredResult update_deferred(const RenderModel &model,std::span<const Transform> nodes,
                                  bool preserve_effects=false) {
        return update(model,nodes,preserve_effects,true);
    }
private:
    DeferredResult view(bool reused)const {return {{vertices_,palette_,gpu_},reused};}
    DeferredResult update(const RenderModel &model,std::span<const Transform> nodes,
                          bool preserve_effects,bool deferred) {
        const auto rounding=std::fegetround();unsigned mxcsr=0;
#if defined(__SSE2__) || defined(_M_X64)
        // Ignore sticky flags; preserve rounding, exception masks, DAZ and FTZ.
        mxcsr=_mm_getcsr()&0xffc0u;
#endif
        const bool same=model_==&model && deferred_==deferred && rounding_==rounding && mxcsr_==mxcsr && nodes_.size()==nodes.size() &&
            (nodes.empty() || !std::memcmp(nodes_.data(),nodes.data(),nodes.size_bytes()));
        if(same && !preserve_effects && !deferred)return view(true);
        const auto before=effects();
        if(same && !preserve_effects && before.normal)return view(true);
        if(same && preserve_effects && contains_effects(before))return view(true);
        // Publish only a complete successful skin; failed inputs leave the
        // preceding immutable result and its key together.
        ModelSkinWorldPose pose;
        if(deferred)pose=model_skin_world_pose(model,nodes,true);
        else pose.vertices=skin_model_world_nodes(model,nodes);
        std::vector<Transform> retained(nodes.begin(),nodes.end());
        vertices_=std::move(pose.vertices);palette_=std::move(pose.palette);
        gpu_=pose.gpu;nodes_=std::move(retained);
        const auto after=effects();
        model_=&model;deferred_=deferred;rounding_=rounding;mxcsr_=mxcsr;
        effects_=after;effects_ready_=eligible(before,after);
        return view(false);
    }
};
}
