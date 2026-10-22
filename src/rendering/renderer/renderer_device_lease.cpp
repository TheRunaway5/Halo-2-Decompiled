#include "halo2/renderer_device_lease.hpp"
#include <atomic>
#include <mutex>
#include <stdexcept>
namespace halo2 {
struct RendererDeviceLease::State {
    mutable std::mutex gate;
    const std::weak_ptr<void> owner;
    NativeRuntimeAddress address;
    std::atomic<bool> live{true};
    State(NativeRuntimeAddress address,std::shared_ptr<void> owner):owner(std::move(owner)),address(address){}
};
RendererDeviceLease::Use::Use(std::shared_ptr<void> owner,NativeRuntimeAddress address)
    :owner_(std::move(owner)),address_(address) {}
NativeRuntimeAddress RendererDeviceLease::Use::native_address() const {
    if(!owner_)throw std::logic_error("Native renderer device use has moved");
    return address_;
}
RendererDeviceLease RendererDeviceLease::create(NativeRuntimeAddress address,std::shared_ptr<void> owner) {
    if(!address.value || !owner)throw std::invalid_argument("Native renderer device owner is empty");
    RendererDeviceLease result;result.state_=std::make_shared<State>(address,std::move(owner));return result;
}
bool RendererDeviceLease::valid() const noexcept {
    if(!state_)return false;
    return state_->live.load(std::memory_order_acquire) && !state_->owner.expired();
}
RendererDeviceLease::Use RendererDeviceLease::acquire() const {
    if(!state_)throw std::runtime_error("Native renderer device owner is unavailable");
    std::lock_guard lock(state_->gate);const auto owner=state_->owner.lock();
    if(!state_->live.load(std::memory_order_acquire) || !owner)throw std::runtime_error("Native renderer device has retired");
    return Use(owner,state_->address);
}
void RendererDeviceLease::retire() {
    if(!state_)return;
    std::lock_guard lock(state_->gate);state_->live.store(false,std::memory_order_release);
}
}
