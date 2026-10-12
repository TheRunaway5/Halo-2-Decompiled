#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
namespace halo2 {
struct VistaBrowserCreateServices {
  std::function<void(bool)> disconnect; //5AD6AF(1), before local birth.
  std::function<void()> clear_report_counts; //40B8B9, actual86EEE8/86EEF0.
  std::function<bool()> birth_local; //5AD8FC, no fabricated result.
  std::function<void(std::uint32_t)> configure_selected; //617138(2).
};
//619D1C's actual Create row in PC13 (619F5D sets149D=0), including complete
//616345 local branch. The observer, not this callback, requests a new lobby.
bool vista_network_browser_create_local(const VistaBrowserCreateServices&);
//5B1D1B ->5B1C39/5AD87C ->5C8977. The bounded native owner supplies the
//same current session for both original resolver calls. Local states5..8 only;
//remote3/4 transport is rejected before any bank mutation.
bool vista_network_browser_preflight_local(std::span<std::byte> session_bank,
    bool current_session,std::uint32_t value);
} //namespace halo2
