#pragma once
#include <functional>
#include <memory>

namespace halo2 {
// Explicit host serialization policy for cooperating source CPU owners. This
// is not original Xbox atomic/interleaving, TLS or scheduler emulation.
// Copies retain the same gate. Entry leases are thread-bound and nonrecursive.
class NativeExecutionGate {
public:
  NativeExecutionGate();
  // One complete CPU entry. Raw-memory callbacks must require_owned(), and
  // their spans must remain private/ephemeral within this entry. Do not return
  // or publish raw spans, run a nested entry or join a worker inside an entry.
  void execute(const std::function<void()> &entry) const;
  // Required around real blocking waits/yield, before attempting acquisition.
  // Reacquires only after normal provider return. A failed/cancelled provider
  // revokes this entry and propagates; no source post-wait access is admitted.
  void suspend(const std::function<void()> &provider) const;
  void require_owned() const;
  // Teardown/join must be outside any entry, including a suspended lease.
  void require_outside_entry() const;
  bool owned_by_current_thread() const noexcept;
private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
