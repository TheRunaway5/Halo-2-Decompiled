#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace halo2 {
// Supplied Project Cartographer xlive.dll SHA256
// 3829cf82e87e51f5eb4a40d241a23e22c7729612385f56fc9812549dee471d72.
// These are that DLL's banks, not Microsoft GFWL or a profile-name identity.
struct VistaOfflineIdentityState {
  std::array<std::byte,0x28> user_info; // 107E8648, including retained name.
  std::array<std::byte,0x2a8> machine;  // 107E8320, address starts at+4.
  std::string login_token;            // Native ownership of107947A4 payload.
};
struct VistaOfflineIdentityScratch {
  // Actual caller-owned destination bytes BEFORE XNetRandom XORs them.
  // Original offline stack arrays were not cleared by the supplied DLL.
  std::array<std::byte,6> enet;
  std::array<std::byte,20> online;
};
class VistaOfflineIdentityServices {
public:
  virtual ~VistaOfflineIdentityServices()=default;
  virtual void random_bytes(std::span<std::byte>)=0; // XNetRandom semantics.
  virtual std::int32_t crt_rand()=0;               // Actual signed EAX.
  virtual std::uint32_t interface_ipv4()=0;        //105812A0 result.
  virtual std::uint32_t inet_addr(std::string_view)=0; // Actual network order.
  //100C4EC0 owns the context at107E83B0 after the machine bank is cleared.
  // A binding must own this service; there is no successful no-op default.
  virtual void initialize_machine_context(std::span<std::byte>)=0;
  virtual void master_status_changed(std::uint32_t signin_state)=0;
};
// Normal supplied offline caller →10535FA0, onlineflag0. Services are required
// owners, not guessed success intents. Name and+8 flags retain source bytes.
// LAN/baseport are actual configuration outputs. The original fixed dummy
// token is retained as native string storage; no account service is contacted.
// Returns false for the actual zero-XUID validation branch, after random/rand
// calls but before any retained account/machine/token mutation.
bool vista_offline_identity_configure(VistaOfflineIdentityState&,
    VistaOfflineIdentityScratch&,std::uint32_t configured_lan_ipv4,
    std::uint16_t configured_base_port,VistaOfflineIdentityServices&);
// Actual XUserGetXUID tail105B8936 for an already-selected40-byte user bank.
// Zeroes output first; signin states1/2 copy XUID, otherwise returns4DD.
std::uint32_t vista_offline_identity_xuid(std::span<const std::byte> user_info,
    std::array<std::byte,8>& output);
// Actual XNetGetTitleXnAddr body10596024: zero destination, then copy36bytes
// and return2 iff machine+30 is nonzero; otherwise return0.
std::uint32_t vista_offline_identity_machine(std::span<const std::byte> machine,
    std::array<std::byte,36>& output);

//105A1F60 retains this state across calls. The initial actual QPC's8 little
// endian bytes seed its permutation; each output XORs existing caller bytes.
struct VistaXnetRandomState {
  std::array<std::uint8_t,256> permutation;
  std::uint8_t i{},j{};
  bool initialized{};
};
void vista_xnet_random(VistaXnetRandomState&,std::span<std::byte>,
    const std::function<std::uint64_t()>& performance_counter);
// Literal supplied CRT rand102B0193. The real thread's retained state is an
// explicit input; no seed/history is silently manufactured.
std::int32_t vista_crt_rand(std::uint32_t& retained_state) noexcept;
} // namespace halo2
