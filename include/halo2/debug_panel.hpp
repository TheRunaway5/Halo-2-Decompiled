#pragma once
#include "halo2/debug_game.hpp"
#include "halo2/display_settings.hpp"
#include <SDL.h>
#include <memory>
#include <optional>
struct ImGuiContext;
struct ImDrawData;
namespace halo2 {
// The optional F1 window emits requests; applications apply them between ticks.
class DebugPanel {
public:
    explicit DebugPanel(SDL_Window*);
    ~DebugPanel();
    DebugPanel(const DebugPanel&)=delete;
    DebugPanel& operator=(const DebugPanel&)=delete;
    bool process_event(const SDL_Event&);
    bool captures_game_input() const;
    bool visible() const;
    void set_visible(bool);
    const DebugGameSettings& game_settings() const;
    void game_settings(DebugGameSettings);
    const DisplaySettings& display_settings() const;
    void display_settings(DisplaySettings);
    // The desktop session outlives every renderer using its preferences.
    void share_display_settings(DisplaySettings&);
    void observe(DebugGameObservation);
    void status(std::string);
    std::optional<DebugGameRequest> take_request();
    // Renderer-owned graphics integration; no Vulkan handles leave Renderer.
    ImGuiContext* context() const;
    ImDrawData* build_frame();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
