#include "halo2/source_native_publication.hpp"
#include "halo2/multiplayer_world.hpp"
#include <atomic>
#include <exception>

namespace halo2 {
namespace {
bool same_cache_owner(const std::shared_ptr<const Cache> &a,
    const std::shared_ptr<const Cache> &b)noexcept {
    return a && b && a.get()==b.get() && !a.owner_before(b) && !b.owner_before(a);
}
void need_publication(bool value) {
    if(!value)throw FormatError("Native publication requires its exact retained current target");
}
}
struct SourceNativePublication::Owner {
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<MultiplayerPreparedWorld> target;
    const std::shared_ptr<const Cache> cache;
    MultiplayerObjectRuntime *const objects;
    PhysicsLiveWorldTu15 *const native;
    std::atomic<bool> alive{true},held{false};
    Owner(std::shared_ptr<MultiplayerPreparedWorld> target,std::shared_ptr<const Cache> cache,
        MultiplayerObjectRuntime &objects,PhysicsLiveWorldTu15 &native)
        :target(std::move(target)),cache(std::move(cache)),objects(&objects),native(&native){}
};
struct SourceNativePublication::State {
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<Owner> owner;
    std::atomic<bool> eligible{true};
    std::unique_ptr<MultiplayerObjectRuntime::NativePublicationHold> native;
    explicit State(std::shared_ptr<Owner> owner):owner(std::move(owner)) {
        need_publication(this->owner && std::this_thread::get_id()==this->owner->executor);
        auto &o=*this->owner;
        need_publication(o.alive.load(std::memory_order_acquire) &&
            o.target && same_cache_owner(o.target->cache,o.cache) &&
            o.target->source_objects.get()==o.objects);
        bool expected=false;
        need_publication(o.held.compare_exchange_strong(expected,true,std::memory_order_acq_rel));
        try {native=o.objects->hold_native_publication();}
        catch(...){o.held.store(false,std::memory_order_release);throw;}
    }
    ~State() {
        if(std::this_thread::get_id()!=executor)std::terminate();
        // Future timing holders must release their timing mutex before this
        // last reference. Release both actual native gates and registry guards
        // before the retained target/cache may run any physical retirement.
        native.reset();
        owner->held.store(false,std::memory_order_release);
    }
};
SourceNativePublication::SourceNativePublication(std::shared_ptr<State> state):state_(std::move(state)){}
SourceNativePublication::~SourceNativePublication() {
    if(std::this_thread::get_id()!=executor_)std::terminate();
    if(state_)state_->eligible.store(false,std::memory_order_release);
    state_.reset();
}
std::shared_ptr<SourceNativePublication::Owner> SourceNativePublication::make_owner(
    std::shared_ptr<MultiplayerPreparedWorld> target) {
    need_publication(target && target->cache && target->source_objects);
    auto &objects=*target->source_objects;
    need_publication(objects.authority_executor_thread());
    objects.require_authority_executor(true);
    const auto cache=target->cache;
    auto &native=objects.tu15_world();
    // This factory is private to the actual takeover constructor, before that
    // private target or any mutable ownership slot has escaped to its caller.
    return std::make_shared<Owner>(std::move(target),cache,objects,native);
}
SourceNativePublication SourceNativePublication::acquire(const std::shared_ptr<Owner> &owner) {
    need_publication(owner && std::this_thread::get_id()==owner->executor);
    return SourceNativePublication(std::make_shared<State>(owner));
}
void SourceNativePublication::close_owner(const std::shared_ptr<Owner> &owner)noexcept {
    if(!owner)return;
    if(std::this_thread::get_id()!=owner->executor)std::terminate();
    owner->alive.store(false,std::memory_order_release);
}
bool SourceNativePublication::owner_available(const std::shared_ptr<Owner> &owner)noexcept {
    if(!owner || std::this_thread::get_id()!=owner->executor)return false;
    return owner->alive.load(std::memory_order_acquire) && !owner->held.load(std::memory_order_acquire);
}
bool SourceNativePublication::retained_current(const std::shared_ptr<State> &state)noexcept {
    if(!state || std::this_thread::get_id()!=state->executor)return false;
    const auto &o=*state->owner;
    return state->eligible.load(std::memory_order_acquire) && o.alive.load(std::memory_order_acquire) &&
        o.held.load(std::memory_order_acquire) && o.target &&
        same_cache_owner(o.target->cache,o.cache) && o.target->source_objects.get()==o.objects &&
        state->native && state->native->matches(*o.objects);
}
bool SourceNativePublication::retained_matches(const std::shared_ptr<State> &state,
    const PhysicsLiveWorldTu15 &native,const std::shared_ptr<const Cache> &cache)noexcept {
    return retained_current(state) && state->owner->native==&native && same_cache_owner(state->owner->cache,cache);
}
void SourceNativePublication::require_provider(const std::shared_ptr<State> &state,
    const MccSoundVariationProvider &provider,const std::shared_ptr<const Cache> &cache) {
    need_publication(retained_current(state) && same_cache_owner(state->owner->cache,cache));
    // Private native-owner admission, before timing/SDL: never reopen a public
    // World operation through gates which this actual scope already retains.
    state->owner->native->require_publication_provider(provider,*cache);
}
bool SourceNativePublication::current()const noexcept {
    if(std::this_thread::get_id()!=executor_)return false;
    return retained_current(state_);
}
bool SourceNativePublication::matches(const PhysicsLiveWorldTu15 &native,
    const std::shared_ptr<const Cache> &cache)const noexcept {
    if(std::this_thread::get_id()!=executor_)return false;
    return retained_matches(state_,native,cache);
}
}
