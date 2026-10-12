#pragma once
#include <memory>
#include <string>

namespace halo2 {
// Private preflight relay connection. It echoes server challenges only; it
// never asserts latency, a peer identity, a rating or bandwidth. The owner
// destroys it when rotating/revoking the memory-only matchmaking session.
class OnlineSearchQos {
public:
  OnlineSearchQos(std::string origin, std::string bearer, bool allow_loopback_http);
  ~OnlineSearchQos();
  OnlineSearchQos(const OnlineSearchQos &) = delete;
  OnlineSearchQos &operator=(const OnlineSearchQos &) = delete;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
