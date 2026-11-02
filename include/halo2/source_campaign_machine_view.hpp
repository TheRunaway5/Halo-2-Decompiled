#pragma once
#include "halo2/renderer.hpp"
#include <atomic>
#include <memory>
#include <thread>

namespace halo2 {
class SourceMachineScene;
struct SourceCampaignMachineDrawInputs {
    ObjectColors colors;
    ShaderInput input;
};
namespace source_campaign_machine_detail {
struct FrameAuthority {
    std::atomic<std::uint64_t> generation{};
    std::atomic<bool> alive{true};
    const std::thread::id executor=std::this_thread::get_id();
};
}
// Completed immutable selected draws, independent of the scene's legacy four
// scratch view slots. Ordinary and texture-camera views may both be view0.
// Source depth ordinals, posed vertices and copied live PRT banks stay owned
// until the enclosing frame retires; no controller or lighting query runs.
class SourceCampaignMachineViewPacket {
public:
    std::vector<unsigned> ordinals;
    std::vector<ModelObjectDraw> objects;
    void require_current()const {
        if(!authority_ || authority_->executor!=std::this_thread::get_id() ||
           !authority_->alive.load(std::memory_order_acquire) ||
           authority_->generation.load(std::memory_order_acquire)!=generation_)
            throw FormatError("Campaign machine packet belongs to a retired frame/resource owner");
    }
    SourceCampaignMachineViewPacket(const SourceCampaignMachineViewPacket&)=delete;
    SourceCampaignMachineViewPacket&operator=(const SourceCampaignMachineViewPacket&)=delete;
private:
    friend class SourceMachineScene;
    SourceCampaignMachineViewPacket()=default;
    struct Section {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> source_vertices;
        std::vector<SourcePrtMaterialBank> prt;
        ObjectColors colors;
        ShaderInput input;
        std::optional<ObjectLightingState> lighting;
        SourceModelDepthSelection depth;
    };
    std::vector<Section> sections_;
    std::shared_ptr<const void> resources_;
    std::shared_ptr<const source_campaign_machine_detail::FrameAuthority> authority_;
    std::uint64_t generation_{};
};
}
