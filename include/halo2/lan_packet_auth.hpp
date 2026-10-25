#pragma once
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>
namespace halo2 {
// Socket adapter. Keys, authentication and replay policy are owned by Rust.
class LanPacketAuth {
public:
  using Key=std::array<unsigned char,32>;
  // Largest frame overhead (LNA1); established sessions use 29 bytes (LNA2).
  static constexpr std::size_t overhead=77;
  static constexpr std::size_t compact_overhead=29;
  struct Session {
    struct Impl;
    std::unique_ptr<Impl> inner;
    std::optional<Key> peer;
    Session();~Session();Session(Session &&)noexcept;Session &operator=(Session &&)noexcept;
  };
  LanPacketAuth();~LanPacketAuth();
  const Key &public_key()const;
  static bool packet(std::span<const std::byte>);
  void bind(Session &,const Key &)const;
  std::vector<std::byte> seal(std::span<const std::byte>,Session &,bool host)const;
  bool open(std::vector<std::byte> &,Session &,bool host)const;
private:
  struct Impl;
  std::unique_ptr<Impl> inner_;
};
}
