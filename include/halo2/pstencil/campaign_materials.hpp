#pragma once
#include "halo2/renderer.hpp"
#include <memory>

namespace halo2::pstencil {
struct CampaignMaterialConsumers {
    std::span<const Vertex> world,debug,foreground,background,hud;
    std::span<const ModelView> views;
};
// A requested campaign upload is completed before rendering, once its actual
// consumers are known, in either graphics mode. The full-bank diagnostic
// retains immediate updates. Shared child-pass owners keep their root order.
class CampaignMaterialUpdates {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    CampaignMaterialUpdates();
    ~CampaignMaterialUpdates();
    void request(Renderer&,std::span<const Material>);
    void flush(Renderer&,std::span<const Material>,const CampaignMaterialConsumers&,std::uint64_t frame);
};
}
