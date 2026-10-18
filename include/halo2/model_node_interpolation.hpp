#pragma once
#include "halo2/model.hpp"
#include <optional>
#include <span>

namespace halo2 {
// Render-only history of completed model outputs, including deferred grip/IK
// callbacks. The caller owns lifetime cuts; sampling never evaluates a channel.
class ModelNodeInterpolation {
public:
    void capture(std::uint64_t tick,std::span<const Transform> nodes,
        std::optional<Transform> reference_frame={});
    // A render collector can run less often than the simulation. Its two
    // completed endpoints retain their actual tick spacing; the caller clears
    // history at lifetime/visibility/timeline discontinuities.
    void capture_render(std::uint64_t tick,std::span<const Transform> nodes,
        std::optional<Transform> reference_frame={});
    std::vector<Transform> sample_render(const RenderModel &,float fraction,
        std::optional<Transform> reference_frame={})const;
    std::vector<Transform> sample(const RenderModel &,float fraction,
        std::optional<Transform> reference_frame={})const;
    bool paired()const{return paired_;}
private:
    void capture(std::uint64_t,std::span<const Transform>,std::optional<Transform>,bool render);
    std::uint64_t tick_{},previous_tick_{};
    bool initialized_{},paired_{};
    std::vector<Transform> previous_,current_;
    std::optional<Transform> previous_frame_,current_frame_;
};
// Recover parent-local transforms from completed WORLD outputs, interpolate
// those locals, then rebuild the hierarchy. Endpoints retain their exact words.
std::vector<Transform> interpolate_model_nodes(const RenderModel &,
    std::span<const Transform> previous,std::span<const Transform> current,float fraction);
}
