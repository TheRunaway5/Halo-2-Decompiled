#pragma once

#include "halo2/source_bsp_admission.hpp"
#include <utility>

namespace halo2::source_object_render_queue_detail {

template<class Classify>
void promote(std::span<SourceBspAdmissionRecord> queue, Classify&& classify) {
    // Finish all lookups before swapping rows. A missing source field leaves
    // the caller's queue in its original order.
    std::vector<bool> selected;
    selected.reserve(queue.size());
    for (const auto& row : queue) selected.push_back(classify(row));

    std::size_t first = 0;
    for (std::size_t current = 0; current < queue.size(); ++current) {
        if (!selected[current]) continue;
        if (first != current) std::swap(queue[first], queue[current]);
        ++first;
    }
}

} // namespace halo2::source_object_render_queue_detail
