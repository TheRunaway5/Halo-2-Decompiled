#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace halo2 {
struct OnlinePeerSignal { unsigned peer{}; std::string type, payload, mid; };
struct OnlineEosPeer { unsigned slot{}; std::string key; };
struct OnlineEosOptions {
  std::string socket;
  std::vector<OnlineEosPeer> peers;
  bool force_relay{};
  bool sessions{};
  std::string playlist,revision,map,variant;
};
// Authenticated signaling is supplied by the room coordinator. ICE finds a
// direct UDP path; DTLS verifies the fingerprint in that signaling. The EOS
// constructor uses Rust admission/framing and Epic's direct/relay P2P paths.
class OnlinePeerTransport {
public:
  OnlinePeerTransport(unsigned self, std::vector<unsigned> connection_slots,
                      const std::vector<std::string> &stun_servers);
  OnlinePeerTransport(unsigned self, const OnlineEosOptions &);
  ~OnlinePeerTransport();
  void signal(const OnlinePeerSignal &);
  std::vector<OnlinePeerSignal> signals();
  bool ready(unsigned peer) const;
  bool failed(unsigned peer) const;
  bool relayed(unsigned peer) const;
  unsigned round_trip_ms(unsigned peer) const;
  bool send(std::span<const std::byte>, unsigned peer);
  bool receive(std::vector<std::byte> &, unsigned &peer);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
