#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
struct OnlinePeerSignal { unsigned peer{}; std::string type, payload, mid; };
// Authenticated signaling is supplied by the room coordinator. ICE finds a
// direct UDP path; DTLS verifies the fingerprint in that signaling. No TURN,
// ICE-TCP or gameplay-through-WebSocket fallback is admitted by this owner.
class OnlinePeerTransport {
public:
  OnlinePeerTransport(unsigned self, std::vector<unsigned> connection_slots,
                      const std::vector<std::string> &stun_servers);
  ~OnlinePeerTransport();
  void signal(const OnlinePeerSignal &);
  std::vector<OnlinePeerSignal> signals();
  bool ready(unsigned peer) const;
  bool failed(unsigned peer) const;
  unsigned round_trip_ms(unsigned peer) const;
  bool send(std::span<const std::byte>, unsigned peer);
  bool receive(std::vector<std::byte> &, unsigned &peer);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
