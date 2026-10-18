#pragma once
#include "halo2/simulation.hpp"
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <thread>
namespace halo2 {
namespace source_script_sound_detail {
struct Admission {
  std::function<void(const SoundEvent &)> submit;
  bool draining{};
};
inline thread_local std::map<const Simulation *,std::shared_ptr<Admission>> admissions;
}
// The caller owns the host audio adapter and its fault diagnostics. This
// binding changes delivery phase only; it is never serialized with Simulation.
// It must remain on the Simulation executor and may not outlive its owner.
class SourceScriptSoundAdmissionBindingTu15 {
  const Simulation *owner_;
  std::shared_ptr<source_script_sound_detail::Admission> admission_;
  std::thread::id thread_=std::this_thread::get_id();
public:
  SourceScriptSoundAdmissionBindingTu15(const Simulation &owner,
      std::function<void(const SoundEvent &)> submit):owner_(&owner),
      admission_(std::make_shared<source_script_sound_detail::Admission>(
          source_script_sound_detail::Admission{std::move(submit)})) {
    if(!admission_->submit ||
        !source_script_sound_detail::admissions.emplace(owner_,admission_).second)
      throw std::runtime_error("Script sound admission requires one live host callback per Simulation");
  }
  ~SourceScriptSoundAdmissionBindingTu15() {
    if(thread_!=std::this_thread::get_id())std::terminate();
    const auto found=source_script_sound_detail::admissions.find(owner_);
    if(found==source_script_sound_detail::admissions.end() || found->second!=admission_)
      std::terminate();
    source_script_sound_detail::admissions.erase(found);
  }
  SourceScriptSoundAdmissionBindingTu15(const SourceScriptSoundAdmissionBindingTu15 &)=delete;
  SourceScriptSoundAdmissionBindingTu15 &operator=(const SourceScriptSoundAdmissionBindingTu15 &)=delete;
};
// Called only after the script/restore state has been published. Appending to
// the existing queue first preserves older Reset/stop/spatial/NONE ordering.
// A callback can append a later event or restore that queue; consume one value
// at a time and do not hold an iterator/reference across host delivery.
inline void source_admit_queued_script_sounds_tu15(const Simulation &owner,
    std::vector<SoundEvent> &pending) {
  const auto found=source_script_sound_detail::admissions.find(&owner);
  if(found==source_script_sound_detail::admissions.end())return;
  const auto admission=found->second;
  if(admission->draining)return;
  struct Draining {
    source_script_sound_detail::Admission &state;
    explicit Draining(source_script_sound_detail::Admission &s):state(s){state.draining=true;}
    ~Draining(){state.draining=false;}
  } draining(*admission);
  while(!pending.empty()) {
    // Keep the in-flight callback alive, but do not deliver another event
    // after its binding was removed or replaced by host lifecycle work.
    const auto live=source_script_sound_detail::admissions.find(&owner);
    if(live==source_script_sound_detail::admissions.end() || live->second!=admission)return;
    const auto event=pending.front();pending.erase(pending.begin());
    admission->submit(event);
  }
}
} // namespace halo2

// This registry is separate from Admission and its original thread-local map.
// It adds query services without changing the event-delivery binding layout.
#include <cstdint>
#include <mutex>
#include <optional>
namespace halo2 {
struct SourceScriptSoundClockQueriesTu15 {
  std::function<std::int32_t(std::uint32_t)> stored_time;
  std::function<std::int32_t(std::uint32_t,std::int32_t)> explicit_time;
  std::function<std::int32_t(std::uint32_t)> language_time;
};
namespace source_script_sound_query_detail {
struct Services {
  SourceScriptSoundClockQueriesTu15 callbacks;
  std::thread::id thread=std::this_thread::get_id();
};
inline std::mutex mutex;
inline std::map<const Simulation*,std::shared_ptr<Services>> bindings;
// Keep the executor identity in flight even if the callback unbinds itself.
// Replacing that binding cannot evade the query's reentry boundary.
inline std::map<const Simulation*,std::thread::id> active;
template<class Member,class... Args>
std::optional<std::int32_t> query(const Simulation& owner,Member member,Args... args) {
  std::unique_lock lock(mutex);
  if(active.contains(&owner))
    throw std::runtime_error("Script sound query cannot reenter its Simulation executor");
  const auto found=bindings.find(&owner);
  if(found==bindings.end())return std::nullopt;
  if(found->second->thread!=std::this_thread::get_id())
    throw std::runtime_error("Script sound query requires its creating thread");
  active.emplace(&owner,std::this_thread::get_id());
  struct Active {
    const Simulation* owner;
    ~Active(){std::lock_guard lock(mutex);active.erase(owner);}
  } guard{&owner};
  // These locals are destroyed before guard, including callable destructors.
  // No registry mutex is held across any user callable operation.
  const auto services=found->second;
  lock.unlock();
  const auto callback=services->callbacks.*member;
  return callback(args...);
}
}
// All three original HS signatures share one creating-thread service owner.
// The caller retains its actual World/clock through these callbacks. A bound
// error propagates; only an absent binding permits the development fallback.
// As with Simulation itself, teardown must be quiescent on its executor.
class SourceScriptSoundClockQueryBindingTu15 final {
  const Simulation* owner_;
  std::shared_ptr<source_script_sound_query_detail::Services> services_;
public:
  SourceScriptSoundClockQueryBindingTu15(const Simulation& owner,
      SourceScriptSoundClockQueriesTu15 callbacks):owner_(&owner),
      services_(std::make_shared<source_script_sound_query_detail::Services>(
          source_script_sound_query_detail::Services{std::move(callbacks)})) {
    if(!services_->callbacks.stored_time || !services_->callbacks.explicit_time ||
        !services_->callbacks.language_time)
      throw std::runtime_error("Script sound queries require all three original signatures");
    std::lock_guard lock(source_script_sound_query_detail::mutex);
    if(source_script_sound_query_detail::active.contains(owner_) ||
        !source_script_sound_query_detail::bindings.emplace(owner_,services_).second)
      throw std::runtime_error("Script sound queries require one quiescent binding per Simulation");
  }
  ~SourceScriptSoundClockQueryBindingTu15() {
    if(services_->thread!=std::this_thread::get_id())std::terminate();
    std::lock_guard lock(source_script_sound_query_detail::mutex);
    const auto found=source_script_sound_query_detail::bindings.find(owner_);
    if(found==source_script_sound_query_detail::bindings.end() || found->second!=services_)
      std::terminate();
    source_script_sound_query_detail::bindings.erase(found);
  }
  SourceScriptSoundClockQueryBindingTu15(const SourceScriptSoundClockQueryBindingTu15&)=delete;
  SourceScriptSoundClockQueryBindingTu15&operator=(const SourceScriptSoundClockQueryBindingTu15&)=delete;
};
inline std::optional<std::int32_t> source_query_script_sound_stored_time_tu15(
    const Simulation& owner,std::uint32_t sound) {
  return source_script_sound_query_detail::query(owner,
      &SourceScriptSoundClockQueriesTu15::stored_time,sound);
}
inline std::optional<std::int32_t> source_query_script_sound_explicit_time_tu15(
    const Simulation& owner,std::uint32_t sound,std::int32_t ticks_at30) {
  return source_script_sound_query_detail::query(owner,
      &SourceScriptSoundClockQueriesTu15::explicit_time,sound,ticks_at30);
}
inline std::optional<std::int32_t> source_query_script_sound_language_time_tu15(
    const Simulation& owner,std::uint32_t sound) {
  return source_script_sound_query_detail::query(owner,
      &SourceScriptSoundClockQueriesTu15::language_time,sound);
}
} // namespace halo2
