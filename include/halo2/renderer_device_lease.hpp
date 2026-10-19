#pragma once
#include "halo2/native_runtime_address.hpp"
#include <memory>
namespace halo2 {
class Renderer;
// Only a live Renderer can issue this lease. A use retains its actual device
// implementation through a source request. The SDL window must outlive active
// uses, just as it outlives Renderer; retirement rejects every new use.
class RendererDeviceLease {
    struct State;
public:
    class Use {
    public:
        Use(Use&&) noexcept=default;
        Use& operator=(Use&&) noexcept=default;
        NativeRuntimeAddress native_address() const;
    private:
        friend class RendererDeviceLease;
        Use(std::shared_ptr<void>,NativeRuntimeAddress);
        std::shared_ptr<void> owner_;
        NativeRuntimeAddress address_{};
    };
    RendererDeviceLease()=default;
    // Nonblocking eligibility snapshot only; retirement can follow this check.
    // acquire() serializes admission against retirement, and its Use pins the
    // actual device allocation for the complete admitted operation.
    bool valid() const noexcept;
    Use acquire() const;
private:
    friend class Renderer;
    static RendererDeviceLease create(NativeRuntimeAddress,std::shared_ptr<void>);
    void retire();
    std::shared_ptr<State> state_;
};
}
