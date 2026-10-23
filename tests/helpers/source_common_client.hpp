#pragma once
#include "halo2/multiplayer_runtime_authority.hpp"
#include "halo2/multiplayer_receipt_continuity.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include <exception>
#include <optional>
#include <utility>

namespace halo2::test {
// Actual executor-local gate over one named losing alternative. The witness
// is weak and genuine; this control never manufactures an owner or native ACK.
struct CommonClientRoleRetirement {
    using Retirement=MultiplayerRuntimeAuthority::OuterRetirement;
    Retirement::Reason reason;
    AuthorityTransferContext context;
    AuthorityCutSeal cut;
    SourceNativePublication::Boundary witness;
    bool allow{},unexpected{};
    unsigned refused{},retired{};
    static bool retire(void *opaque,const Retirement &request)noexcept {
        auto &self=*static_cast<CommonClientRoleRetirement *>(opaque);
        if(request.reason!=self.reason||request.context!=self.context||request.seal!=self.cut||!self.witness.current()){
            self.unexpected=true;return false;
        }
        if(!self.allow){++self.refused;return false;}
        ++self.retired;return true;
    }
};

// Headless CPU routing/receipt subtransaction on the actual surviving display
// world. No renderer, image upload, device input or playing Audio is claimed.
class CommonClientPresentation {
    std::shared_ptr<const Cache> cache_;
    Mesh mesh_;
    SourceWorldParticleDrawsTu15 particles_;
    SourceBspEffectLights models_;
    MultiplayerObjectRuntime *particle_{},*model_{};
    SourceForegroundWorldHandoff foreground_;
    SourceForegroundWorldHandoff::Prepared binding_;
    MultiplayerReceiptContinuity receipt_;
    MultiplayerReceiptContinuity::Prepared prepared_;
    ValidatedAuthorityCut cut_;
public:
    unsigned calls{},published{};
    bool allow{};
    CommonClientPresentation(MultiplayerRuntimeAuthority &runtime,
        const EffectPresentationHistory::RuntimeCheckpoint &checkpoint,
        MultiplayerRuntimeReceiptValues &values,MultiplayerState &current,MultiplayerState &previous,
        MultiplayerSnapshotOrigin &origin,EffectPresentationContinuity &effects,NativeEffectSounds &sounds,
        MultiplayerPoseContinuity &poses)
        :cache_(runtime.world().cache),particles_(*cache_,runtime.world().content.objects),
         models_(*cache_,mesh_,0,std::span<const std::uint32_t>{}),
         foreground_(cache_,mesh_,particles_,models_,mesh_.materials.size(),
            SourceForegroundWorldHandoff::Policy{false,false},particle_,model_),
         binding_(foreground_.prepare_client(runtime.world(),checkpoint)),
         receipt_({values,current,previous,origin},effects,sounds,poses),
         prepared_(receipt_.prepare(checkpoint,binding_.muzzle_collision(),binding_.world_collision())),cut_(checkpoint.cut()){}
    bool ready()const noexcept {return foreground_.ready(binding_)&&receipt_.ready(prepared_,cut_);}
    static bool publish(void *opaque)noexcept {
        auto &self=*static_cast<CommonClientPresentation *>(opaque);++self.calls;
        if(!self.allow||!self.ready())return false;
        if(!self.foreground_.commit(self.binding_)||!self.receipt_.commit(std::move(self.prepared_),self.cut_))std::terminate();
        ++self.published;return true;
    }
};
}
