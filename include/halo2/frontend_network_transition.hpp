#pragma once
#include "halo2/cache.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <vector>

namespace halo2 {
struct FrontendContent;
// TU1.5 0x19481B status strings shown by the network transition dialog.
// The native loader maps its world preparation to network_loading_map,
// first-frame/network setup to network_joining and a departure to leaving.
enum class FrontendNetworkTransitionStatus : std::uint8_t { LoadingMap, SettingUpGame, LeavingGame };

// Original screen231 network_transition_progress ("PLEASE WAIT"). The same
// frontend pane owner paints its authored bitmaps, animation and text from
// the selected menu content; only the materials it actually uses are kept,
// so the loading bridge can present it after the frontend has departed.
// TU1.5 0x229C50 draws this UI over a black clear, without the shell
// background; 0x258605 replaces pane text0 with the status every frame.
class FrontendNetworkTransitionPainter {
public:
  FrontendNetworkTransitionPainter(const FrontendContent &,const std::filesystem::path &fonts,
      const FrontendUiAssetPack &ui_assets={},const std::filesystem::path &sampled_fonts={});
  ~FrontendNetworkTransitionPainter();
  FrontendNetworkTransitionPainter(const FrontendNetworkTransitionPainter&)=delete;
  FrontendNetworkTransitionPainter& operator=(const FrontendNetworkTransitionPainter&)=delete;
  // Prepared RGBA owners, each named "network-transition/...".
  std::span<const Material> materials() const;
  // Seconds/clock since the screen opened; vertices address materials().
  std::vector<Vertex> draw(FrontendNetworkTransitionStatus,float seconds,std::uint32_t ui_clock_ms);
private:
  struct State;
  std::unique_ptr<State> state_;
};
}
