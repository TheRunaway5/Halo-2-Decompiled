#pragma once
#include "halo2/frontend.hpp"
#include "halo2/player_training.hpp"
namespace halo2 {
// Original13CB50/13C680: a single localized cinematic line, including its
// 1.5-second trailing fade. Kept in simulation time, so pausing freezes it.
bool campaign_subtitles_visible(unsigned preference, unsigned game_language,
                                unsigned map_language);
struct CampaignTextContent {
  UiStrings training, subtitles, title_strings;
  std::vector<CampaignTitleDefinition> titles;
  std::vector<TrainingEventDefinition> training_events;
  std::array<std::uint32_t, 4>
      panel{}; // right corner, edge, left corner, center.
  std::map<std::uint32_t, UiImage> images;
};
CampaignTextContent load_campaign_text(const Cache &,
                                       const std::filesystem::path &map);
class CampaignTextRenderer {
public:
  CampaignTextRenderer(CampaignTextContent, const std::filesystem::path &fonts,
                       const std::filesystem::path &vista_assets, Mesh &);
  std::vector<Vertex>
  draw(const PlayerTrainingState &, const CampaignSubtitle &,
       unsigned subtitles, unsigned width, unsigned height, float letterbox,
       const std::function<std::string(const std::string &)> &substitute = {},
       std::span<const CampaignTitle> titles = {}) const;

private:
  CampaignTextContent content_;
  RetailFont training_, subtitle_;
  std::map<unsigned, std::pair<RetailFont, unsigned>> title_fonts_;
  unsigned training_material_{}, subtitle_material_{};
  std::array<unsigned, 4> panel_materials_{};
};
} // namespace halo2
