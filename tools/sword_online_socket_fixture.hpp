#pragma once
#include "halo2/multiplayer_datagrams.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace halo2 {
struct UiVariant;
namespace test {
// Test-only pair of actual OnlineMatchSession endpoints. One owns its real
// ICE/DTLS/SCTP sockets in this process; the other owns them in an exec'd child.
// A bounded private IPC bridge lets the existing native LanClient codec use
// that child endpoint. Gameplay packets still traverse the direct sockets.
class SwordOnlineSocketPair {
public:
  static bool requested() noexcept;
  SwordOnlineSocketPair();
  ~SwordOnlineSocketPair();
  SwordOnlineSocketPair(const SwordOnlineSocketPair&)=delete;
  SwordOnlineSocketPair& operator=(const SwordOnlineSocketPair&)=delete;
  unsigned host_slot() const;
  unsigned other_slot() const;
  const std::vector<OnlineAdmission>& roster() const;
  std::shared_ptr<MultiplayerDatagrams> endpoint(unsigned slot) const;
  // Both genuine sessions publish the same actual fingerprint/source variant
  // and wait for the coordinator's ordinary Running transition.
  void loaded(std::uint64_t fingerprint,const UiVariant&);
  void health() const;
  // ready() may run ordinary receive()/broadcast() calls on their owning
  // native executor. This pumps no world and manufactures no admission.
  void wait_for_io(const std::function<bool()>& ready,const char *stage,
      std::chrono::milliseconds timeout=std::chrono::seconds(10));
  // Bounded diagnostics only: no bearer, room identity, SDP or fingerprints.
  void write_report(const std::filesystem::path&) const;
private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

// Call once at the executable entry point, before source-world construction.
// Returns -1 for an ordinary capture invocation, otherwise the child exit code.
int sword_online_socket_worker_main(int argc,char **argv);
} // namespace test
} // namespace halo2
