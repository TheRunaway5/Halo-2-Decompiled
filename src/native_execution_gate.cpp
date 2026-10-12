#include "halo2/native_execution_gate.hpp"
#include <mutex>
#include <stdexcept>

namespace halo2 {
struct NativeExecutionGate::State { std::mutex mutex; };
namespace {
struct Entry {
  const void *state;
  std::unique_lock<std::mutex> lock;
  bool revoked=false;
};
thread_local Entry *current=nullptr;
struct CurrentEntry {
  explicit CurrentEntry(Entry &entry) { current=&entry; }
  ~CurrentEntry() { current=nullptr; }
};
} // namespace
NativeExecutionGate::NativeExecutionGate():state_(std::make_shared<State>()) {}
bool NativeExecutionGate::owned_by_current_thread() const noexcept {
  return current&&current->state==state_.get()&&current->lock.owns_lock()&&!current->revoked;
}
void NativeExecutionGate::require_owned() const {
  if(!owned_by_current_thread())
    throw std::logic_error("Native source memory access outside its execution lease");
}
void NativeExecutionGate::require_outside_entry() const {
  if(current)throw std::logic_error("Native worker teardown inside a source execution entry");
}
void NativeExecutionGate::execute(const std::function<void()> &entry) const {
  if(!entry)throw std::invalid_argument("Native source CPU entry unavailable");
  if(current)throw std::logic_error("Recursive native source execution entry");
  if(!state_)throw std::logic_error("Native source execution gate owner unavailable");
  // Retain the gate for the full entry even when its facade is released.
  const auto state=state_;
  Entry lease{state.get(),std::unique_lock(state->mutex)};
  CurrentEntry active(lease);
  entry();
  if(lease.revoked||!lease.lock.owns_lock())
    throw std::logic_error("Native source CPU continued after a failed suspension");
}
void NativeExecutionGate::suspend(const std::function<void()> &provider) const {
  require_owned();
  if(!provider)throw std::invalid_argument("Native wait/yield provider unavailable");
  auto &entry=*current;
  entry.lock.unlock();
  try {
    provider();
    entry.lock.lock();
  } catch(...) {
    entry.revoked=true;
    throw;
  }
}
} // namespace halo2
