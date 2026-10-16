#include "halo2/debug_panel.hpp"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include <algorithm>
#include <deque>

namespace halo2 {
struct DebugPanel::Impl {
    SDL_Window* window{};
    ImGuiContext* context{};
    bool visible{}, relative_mouse{}, text_input_before{};
    DebugGameSettings settings;
    DisplaySettings display;
    DisplaySettings* shared_display=&display;
    bool pstencil_available{};
    DebugGameObservation observation;
    std::deque<DebugGameRequest> requests;
    std::string status;
    unsigned player{}, selected_spawn{}, selected_snapshot{};
    char search[128]{}, snapshot_name[65]{"Snapshot"};
    void request(DebugGameRequest::Kind kind,unsigned index=0,std::string name={}) {
        requests.push_back({kind,player,index,std::move(name)});
    }
};
DebugPanel::DebugPanel(SDL_Window* window):impl_(std::make_unique<Impl>()) {
    impl_->window=window;
    IMGUI_CHECKVERSION();
    impl_->context=ImGui::CreateContext();
    ImGui::SetCurrentContext(impl_->context);
    auto& io=ImGui::GetIO();
    io.IniFilename=nullptr; // Session-only controls; never write into the game directory.
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    if(!ImGui_ImplSDL2_InitForVulkan(window)) {
        ImGui::DestroyContext(impl_->context);
        throw std::runtime_error("cannot initialize optional debug panel");
    }
}
DebugPanel::~DebugPanel() {
    set_visible(false);
    ImGui::SetCurrentContext(impl_->context);
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(impl_->context);
}
ImGuiContext* DebugPanel::context() const{return impl_->context;}
bool DebugPanel::visible() const{return impl_->visible;}
bool DebugPanel::captures_game_input() const{return visible();}
void DebugPanel::set_visible(bool value) {
    if(value==impl_->visible)return;
    ImGui::SetCurrentContext(impl_->context);
    for(const auto button:{SDL_BUTTON_LEFT,SDL_BUTTON_RIGHT,SDL_BUTTON_MIDDLE,SDL_BUTTON_X1,SDL_BUTTON_X2}) {
        SDL_Event release{};release.type=SDL_MOUSEBUTTONUP;
        release.button.windowID=SDL_GetWindowID(impl_->window);
        release.button.button=static_cast<Uint8>(button);
        ImGui_ImplSDL2_ProcessEvent(&release);
    }
    ImGui::GetIO().ClearEventsQueue();
    ImGui::GetIO().ClearInputKeys();
    ImGui::GetIO().ClearInputMouse();
    impl_->visible=value;
    if(value) {
        impl_->text_input_before=SDL_IsTextInputActive();
        SDL_StartTextInput();
        impl_->relative_mouse=SDL_GetRelativeMouseMode();
        SDL_SetRelativeMouseMode(SDL_FALSE);
    } else {
        SDL_CaptureMouse(SDL_FALSE);
        if(!impl_->text_input_before)SDL_StopTextInput();
        if(impl_->relative_mouse)SDL_SetRelativeMouseMode(SDL_TRUE);
    }
}
bool DebugPanel::process_event(const SDL_Event& event) {
    ImGui::SetCurrentContext(impl_->context);
    // Ignore events belonging to another native window.
    const auto window_id=SDL_GetWindowID(impl_->window);
    if((event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) && event.key.windowID && event.key.windowID!=window_id)return false;
    if(event.type==SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym==SDLK_F1) {
        set_visible(!visible());return true;
    }
    if(visible() && event.type==SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym==SDLK_ESCAPE) {
        set_visible(false);return true;
    }
    if(!visible())return false;
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch(event.type) {
    case SDL_KEYDOWN: case SDL_KEYUP: case SDL_TEXTINPUT: case SDL_TEXTEDITING:
    case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL:
    case SDL_CONTROLLERAXISMOTION: case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP:
        return true;
    default:return false;
    }
}
const DebugGameSettings& DebugPanel::game_settings() const{return impl_->settings;}
void DebugPanel::game_settings(DebugGameSettings value){impl_->settings=std::move(value);}
const DisplaySettings& DebugPanel::display_settings() const{return *impl_->shared_display;}
void DebugPanel::display_settings(DisplaySettings value){*impl_->shared_display=value;}
void DebugPanel::share_display_settings(DisplaySettings& value){impl_->shared_display=&value;}
void DebugPanel::pstencil_available(bool value){impl_->pstencil_available=value;}
void DebugPanel::observe(DebugGameObservation value){impl_->observation=std::move(value);}
void DebugPanel::status(std::string value){impl_->status=std::move(value);}
std::optional<DebugGameRequest> DebugPanel::take_request() {
    if(impl_->requests.empty())return {};
    auto value=std::move(impl_->requests.front());impl_->requests.pop_front();return value;
}
ImDrawData* DebugPanel::build_frame() {
    ImGui::SetCurrentContext(impl_->context);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    auto& p=*impl_;
    const auto& o=p.observation;
    ImGui::SetNextWindowSize({590,610},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({30,30},ImGuiCond_FirstUseEver);
    bool open=p.visible;
    if(ImGui::Begin("Halo 2 Settings (F1)",&open)) {
        if(ImGui::BeginTabBar("settings")) {
            if(ImGui::BeginTabItem("Graphics")) {
                auto& display=*p.shared_display;
                const auto current=pstencil::graphics_mode_name(display.graphics_mode);
                if(ImGui::BeginCombo("Graphics Mode",current)) {
                    if(ImGui::Selectable("Original",display.graphics_mode==pstencil::GraphicsMode::Original))
                        display.graphics_mode=pstencil::GraphicsMode::Original;
                    ImGui::BeginDisabled(!p.pstencil_available);
                    if(ImGui::Selectable("pStencil",display.graphics_mode==pstencil::GraphicsMode::pStencil))
                        display.graphics_mode=pstencil::GraphicsMode::pStencil;
                    ImGui::EndDisabled();ImGui::EndCombo();
                }
                if(!p.pstencil_available)ImGui::TextWrapped("pStencil is unavailable on this graphics device.");
                ImGui::TextWrapped("Changes apply immediately. Video Settings saves your preferred graphics mode.");
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Debug")) {
                ImGui::Text("%s | Tick %llu",o.mode.c_str(),static_cast<unsigned long long>(o.tick));
                ImGui::TextWrapped("Optional native workbench controls. Changes last for this session.");
                if(!o.limitation.empty())ImGui::TextWrapped("%s",o.limitation.c_str());
                ImGui::Separator();
                if(!o.players.empty()) {
                    if(std::none_of(o.players.begin(),o.players.end(),[&](const auto& v){return v.slot==p.player;}))p.player=o.players.front().slot;
                    const auto current=std::find_if(o.players.begin(),o.players.end(),[&](const auto& v){return v.slot==p.player;});
                    if(ImGui::BeginCombo("Local player",current->name.c_str())) {
                        for(const auto& v:o.players)if(ImGui::Selectable(v.name.c_str(),v.slot==p.player))p.player=v.slot;
                        ImGui::EndCombo();
                    }
                    ImGui::Text("Health %.3f  Shield %.3f  Ammo %d / %d",current->health,current->shield,current->loaded,current->reserve);
                    ImGui::Text("Position %.3f, %.3f, %.3f%s",current->position.x,current->position.y,current->position.z,current->dead?" (dead)":"");
                }
                ImGui::BeginDisabled(!o.available || o.players.empty());
                auto& settings=p.settings.players.at(p.player);
                ImGui::Checkbox("Invulnerable health and shields",&settings.invulnerable);
                ImGui::Checkbox("Unlimited ammunition",&settings.unlimited_ammo);
                ImGui::Checkbox("Noclip (jump up, crouch down)",&settings.noclip);
                ImGui::Checkbox("Maximum outgoing damage",&settings.maximum_damage);
                ImGui::BeginDisabled(!o.ai_available);
                ImGui::Checkbox("Enemies ignore this player",&settings.enemies_ignore);
                ImGui::EndDisabled();
                if(ImGui::Button("Restore health and shields"))p.request(DebugGameRequest::Heal);
                ImGui::SameLine();
                if(ImGui::Button("Reset all cheats"))p.settings={};
                if(ImGui::CollapsingHeader("Teleport to an authored spawn")) {
                    ImGui::InputText("Search",p.search,sizeof(p.search));
                    if(ImGui::BeginListBox("##spawns",{-1,125})) {
                        for(unsigned i=0;i<o.teleports.size();++i) {
                            const auto& v=o.teleports[i];
                            if(*p.search && v.name.find(p.search)==std::string::npos)continue;
                            if(ImGui::Selectable(v.name.c_str(),p.selected_spawn==i))p.selected_spawn=i;
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::BeginDisabled(p.selected_spawn>=o.teleports.size());
                    if(ImGui::Button("Teleport"))p.request(DebugGameRequest::Teleport,p.selected_spawn);
                    ImGui::EndDisabled();
                }
                ImGui::EndDisabled();
                if(ImGui::CollapsingHeader("Checkpoints and session snapshots")) {
                    ImGui::BeginDisabled(!o.snapshots_available);
                    if(ImGui::Button("Save checkpoint"))p.request(DebugGameRequest::SaveCheckpoint);
                    ImGui::SameLine();ImGui::BeginDisabled(!o.checkpoint_available);
                    if(ImGui::Button("Revert checkpoint"))p.request(DebugGameRequest::RevertCheckpoint);
                    ImGui::EndDisabled();
                    ImGui::InputText("Snapshot name",p.snapshot_name,sizeof(p.snapshot_name));
                    if(ImGui::Button("Save snapshot"))p.request(DebugGameRequest::SaveSnapshot,0,p.snapshot_name);
                    if(ImGui::BeginListBox("##snapshots",{-1,90})) {
                        for(unsigned i=0;i<o.snapshots.size();++i)if(ImGui::Selectable(o.snapshots[i].c_str(),p.selected_snapshot==i))p.selected_snapshot=i;
                        ImGui::EndListBox();
                    }
                    ImGui::BeginDisabled(p.selected_snapshot>=o.snapshots.size());
                    if(ImGui::Button("Load snapshot"))p.request(DebugGameRequest::LoadSnapshot,0,o.snapshots.at(p.selected_snapshot));
                    ImGui::SameLine();
                    if(ImGui::Button("Delete snapshot"))ImGui::OpenPopup("Delete session snapshot?");
                    if(ImGui::BeginPopupModal("Delete session snapshot?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::TextUnformatted("Remove this snapshot from memory?");
                        if(ImGui::Button("Delete")) {p.request(DebugGameRequest::DeleteSnapshot,0,o.snapshots.at(p.selected_snapshot));ImGui::CloseCurrentPopup();}
                        ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();
                        ImGui::EndPopup();
                    }
                    ImGui::EndDisabled();
                    ImGui::TextWrapped("Snapshots use the campaign checkpoint state and last until this session ends. Multiplayer snapshot/roster editing needs a complete match-state owner.");
                    ImGui::EndDisabled();
                }
                if(!p.status.empty())ImGui::TextWrapped("%s",p.status.c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    set_visible(open);
    ImGui::Render();
    return ImGui::GetDrawData();
}
} // namespace halo2
