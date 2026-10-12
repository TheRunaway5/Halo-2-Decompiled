#include "halo2/frontend_session.hpp"
#include "halo2/frontend_audio.hpp"
#include "halo2/native_vista_widget_managers.hpp"
#include "halo2/native_vista_widget_cache.hpp"
#include "halo2/native_vista_widget_memory.hpp"
#include <utility>

namespace halo2 {
struct FrontendSession::Impl {
  explicit Impl(FrontendContent value) : content(std::move(value)) {
    if(content.vista)vista_managers=std::make_unique<NativeVistaWidgetManagers>();
  }
  // State dies first, then the registry, then its borrowed cache/managers.
  FrontendContent content;
  std::unique_ptr<NativeVistaWidgetManagers> vista_managers;
  std::unique_ptr<NativeVistaWidgetCache> vista_cache;
  std::unique_ptr<NativeVistaWidgetMemory> vista_memory;
  std::unique_ptr<MenuState> state;
};

FrontendSession::FrontendSession() : ui_clock_previous_(std::chrono::steady_clock::now()) {}
FrontendSession::~FrontendSession() = default;
FrontendAudioState &FrontendSession::audio_state() {
  if(!audio_)audio_=std::make_unique<FrontendAudioState>();
  return *audio_;
}
void FrontendSession::world_audio_update() {
  if(audio_)audio_->world_update(ui_clock());
  if(discord_social) {
    discord_social->presence(has_state()?state().profile_name():std::string{},true);
    discord_social->pump();
  }
}
bool FrontendSession::has_content() const noexcept { return bool(impl_); }
FrontendContent &FrontendSession::retain_content(FrontendContent content) {
  if (impl_)
    throw FormatError("Frontend content already owned; enter an explicit new screen factory");
  impl_ = std::make_unique<Impl>(std::move(content));
  if(impl_->vista_managers)impl_->vista_managers->write(0xd71900,ui_clock_ms_,4);
  return impl_->content;
}
const FrontendContent &FrontendSession::content() const {
  if (!impl_) throw FormatError("Frontend owner has no content");
  return impl_->content;
}
FrontendContent &FrontendSession::content() {
  if (!impl_) throw FormatError("Frontend owner has no content");
  return impl_->content;
}
NativeVistaWidgetManagers &FrontendSession::vista_widget_managers() {
  if(!impl_||!impl_->content.vista)throw FormatError("Frontend has no retained Vista UI manager owner");
  if(!impl_->vista_managers) {
    impl_->vista_managers=std::make_unique<NativeVistaWidgetManagers>();
    impl_->vista_managers->write(0xd71900,ui_clock_ms_,4);
  }
  return *impl_->vista_managers;
}
const NativeVistaWidgetManagers &FrontendSession::vista_widget_managers() const {
  if(!impl_||!impl_->vista_managers)throw FormatError("Frontend has no retained Vista UI manager owner");
  return *impl_->vista_managers;
}
NativeVistaWidgetCache &FrontendSession::retain_vista_widget_cache(std::uint32_t base) {
  if(!impl_||!impl_->content.vista||!impl_->content.vista->vista_widget_cache)
    throw FormatError("Frontend has no original Vista UI cache asset bank");
  if(impl_->vista_cache) {
    if(impl_->vista_cache->source_base()!=base)
      throw FormatError("Retained Vista UI cache cannot change source32 mapping");
  } else {
    auto& memory=vista_widget_memory();
    auto cache=std::make_unique<NativeVistaWidgetCache>(
        impl_->content.vista->vista_widget_cache,base);
    // Validate the mapping against actual registered owners before publishing
    // a cache lifetime. Failed overlap leaves both existing authorities intact.
    memory.bind_cache(*cache);
    impl_->vista_cache=std::move(cache);
  }
  return *impl_->vista_cache;
}
NativeVistaWidgetCache &FrontendSession::vista_widget_cache() {
  if(!impl_||!impl_->vista_cache)
    throw FormatError("Frontend has no admitted Vista UI cache mapping");
  return *impl_->vista_cache;
}
const NativeVistaWidgetCache &FrontendSession::vista_widget_cache() const {
  if(!impl_||!impl_->vista_cache)
    throw FormatError("Frontend has no admitted Vista UI cache mapping");
  return *impl_->vista_cache;
}
NativeVistaWidgetMemory &FrontendSession::vista_widget_memory() {
  auto& managers=vista_widget_managers();
  if(!impl_->vista_memory) {
    auto memory=std::make_unique<NativeVistaWidgetMemory>();
    memory->bind_managers(managers);
    if(impl_->vista_cache)memory->bind_cache(*impl_->vista_cache);
    impl_->vista_memory=std::move(memory);
  }
  return *impl_->vista_memory;
}
const NativeVistaWidgetMemory &FrontendSession::vista_widget_memory() const {
  if(!impl_||!impl_->vista_memory)
    throw FormatError("Frontend has no retained Vista UI memory registry");
  return *impl_->vista_memory;
}
MenuState &FrontendSession::create_state(const std::filesystem::path &preferences) {
  if (!impl_) throw FormatError("Frontend state requires owned content");
  if (impl_->state)
    throw FormatError("Frontend owner already has its menu state");
  if(impl_->content.vista)(void)vista_widget_managers();
  impl_->state = std::make_unique<MenuState>(impl_->content, preferences);
  return *impl_->state;
}
bool FrontendSession::has_state() const noexcept { return impl_ && bool(impl_->state); }
MenuState &FrontendSession::state() {
  if (!has_state()) throw FormatError("Frontend owner has no menu state");
  return *impl_->state;
}
const MenuState &FrontendSession::state() const {
  if (!has_state()) throw FormatError("Frontend owner has no menu state");
  return *impl_->state;
}
std::uint32_t FrontendSession::ui_clock() {
  const auto now=std::chrono::steady_clock::now();
  const auto delta=std::chrono::duration<float>(now-ui_clock_previous_).count();
  ui_clock_previous_=now;
  //60CA7D multiplies its binary32 delta by1000 and truncates before adding
  // the low DWORD. It has no clamp. Native frames can be absent during a world
  // or resource load, so this observation consumes the full real elapsed gap.
  const auto elapsed=std::uint32_t(std::uint64_t(double(delta)*1000.0));
  if(impl_&&impl_->vista_managers) {
    // Once the source UI bank exists it owns the accumulator. Source callbacks
    // and the native frame adapter must observe the same cell, not a shadow.
    auto& bank=*impl_->vista_managers;
    const auto advanced=bank.read(0xd71900,4)+elapsed;
    bank.write(0xd71900,advanced,4);
    return advanced;
  }
  ui_clock_ms_+=elapsed;
  return ui_clock_ms_;
}
MenuState &FrontendSession::enter(FrontendOptions::Entry entry,
    std::shared_ptr<const VistaPcrScreenOwner> report) {
  if(!has_state() || entry==FrontendOptions::Entry::initial)
    throw FormatError("Retained frontend entry requires an explicit new screen factory");
  if(entry==FrontendOptions::Entry::postgame_report) {
    if(!report)throw FormatError("Postgame entry requires the actual retained report owner");
    state().ui_clock(ui_clock());
    state().open_vista_pcr(16,std::move(report));
  } else if(entry==FrontendOptions::Entry::fresh_lobby) {
    if(report)throw FormatError("Lobby entry cannot consume a report owner");
    state().ui_clock(ui_clock());
    state().open_vista_lobby();
  } else if(entry==FrontendOptions::Entry::online_return) {
    if(report)throw FormatError("ONLINE return cannot consume a report owner");
    state().return_online();
  } else throw FormatError("Unsupported retained frontend factory");
  return state();
}
void MenuState::clear_runtime_adapters() noexcept {
  if (online_voice_) online_voice_->leave();
  // Join the audio worker before this invocation's SDL/window owner can die.
  online_voice_.reset(); voice_={};
  if (online_account_) online_account_->sign_out();
  account_ = {}; account_name_.clear();
  if (online_matchmaking_) online_matchmaking_->sign_out();
  matchmaking_active_ = quickmatch_pending_ = false;
  pc_display_adapter_ = nullptr;
  pc_video_adapter_ = nullptr;
  pc_video_query_ = nullptr;
  multiplayer_map_validator_ = nullptr;
}
} // namespace halo2
