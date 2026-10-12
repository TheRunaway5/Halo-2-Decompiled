#pragma once
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/multiplayer.hpp"
#include <memory>

namespace halo2 {
// Native active/finished-Slayer adapter, including explicit teams. Current
// ownership and the source respawn countdown also drive automatic display.
// The original roster, palette, text and geometry leaves use native session
// and input bindings. Special watched-player observers have no native owner.
class MultiplayerScoreboard {
public:
  MultiplayerScoreboard(const Cache &,const std::filesystem::path &fonts,Mesh &,
      const UiVariant &,const std::filesystem::path &ui_assets = {});
  bool available() const {return bool(painter_);}
  void step(unsigned view,bool held);
  void step(unsigned view,bool held,const MultiplayerState&,unsigned player_slot);
  struct Draw {
    bool normal_hud=true;
    FullScoreboardLayout layout;
    FullScoreboardPaintResult paint;
  };
  Draw draw(const MultiplayerState &,std::span<const unsigned> local_ids,
      unsigned view,unsigned width,unsigned height) const;
private:
  std::shared_ptr<const OriginalData> original_;
  FullScoreboardAssets assets_;
  FullScoreboardConstants layout_constants_;
  FullScoreboardOptions options_;
  RetailFont font_;
  std::unique_ptr<FullScoreboardPainter> painter_;
  FullScoreboardHeldState held_;
  std::int16_t font_horizontal_offset_{};
  Mesh *mesh_{};
  std::array<std::array<unsigned,multiplayer_capacity>,4> emblem_materials_{},dead_materials_{};
};
} // namespace halo2
