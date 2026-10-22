#include "halo2/frontend_appearance.hpp"
#include "halo2/source_object_colors.hpp"

namespace halo2 {
ObjectColors frontend_appearance_colors(const FrontendAppearance &appearance,
                                        std::span<const Vec3> palette) {
  ObjectColors colors;
  for (unsigned i = 0; i < colors.size(); ++i) {
    const int index = appearance.colors[i];
    colors[i] = index >= 0 && std::size_t(index) < palette.size()
                    ? palette[std::size_t(index)] : Vec3{1, 1, 1};
  }
  return colors;
}
Vec3 frontend_appearance_name_color(const FrontendAppearance &appearance,
                                     std::span<const Vec3> palette) {
  return source_profile_name_color(frontend_appearance_colors(appearance, palette)[0]);
}
void preview_frontend_appearance(FrontendAppearance &appearance, unsigned screen,
                                 unsigned color_slot, int focus, bool exiting) {
  if (exiting)
    return;
  if (screen == 50) {
    if (color_slot >= appearance.colors.size())
      throw FormatError("frontend appearance color slot is outside the source bank");
    if (focus >= -1 && focus <= 17)
      appearance.colors[color_slot] = std::int8_t(focus);
  } else if (screen == 52)
    appearance.species = std::int8_t(focus);
  else if (screen == 49 && focus != -1)
    appearance.foreground = std::uint8_t(focus);
  else if (screen == 233 && focus != -1)
    appearance.background = std::uint8_t(focus);
}
void apply_frontend_appearance(std::span<Material> materials,
                               const FrontendAppearance &appearance,
                               std::span<const Vec3> palette) {
  const auto colors = frontend_appearance_colors(appearance, palette);
  for (auto &material : materials)
    apply_object_colors(material, colors);
}
} // namespace halo2
