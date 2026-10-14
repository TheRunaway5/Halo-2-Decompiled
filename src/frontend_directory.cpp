#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>

namespace halo2 {
namespace {
const OnlineServer *detail(const OnlineDirectorySnapshot &snapshot, const std::string &id) {
  const auto found = std::find_if(snapshot.servers.begin(), snapshot.servers.end(), [&](const auto &server) { return server.id == id; });
  return found == snapshot.servers.end() ? nullptr : &*found;
}
constexpr std::array<const char *, 8> fields{"Server name", "Map", "Game type", "Players", "Region", "Server type", "Version", "Address"};
std::string field(const OnlineServer &server, unsigned index) {
  switch (index) {
  case 0: return server.name;
  case 1: return server.map;
  case 2: return server.gametype;
  case 3: return std::to_string(server.players) + " / " + std::to_string(server.max_players);
  case 4: return server.region.empty() ? "Not specified" : server.region;
  case 5: return server.server_type == "dedicated" ? "Dedicated" : "Player hosted";
  case 6: return server.game_version;
  case 7: return (server.host.find(':') != std::string::npos ? "[" + server.host + "]" : server.host) + ":" + std::to_string(server.port);
  default: return {};
  }
}
} // namespace
void MenuState::set_online_directory(std::shared_ptr<OnlineDirectory> directory) {
  if (online_directory_) online_directory_->cancel();
  online_directory_ = std::move(directory);
  online_directory_active_ = false;
  online_servers_ = {};
}
void MenuState::refresh_online_directory() {
  if (!online_identity()) return;
  online_directory_active_ = true;
  online_refresh_at_ = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  if (online_directory_) online_directory_->refresh();
  update_online_directory();
}
void MenuState::update_online_directory() {
  const bool browsing = online_identity() && std::any_of(stack_.begin(), stack_.end(), [](const auto &f) { return (f.screen == 219 || f.screen == 237) && f.context == online_server_browser_context; });
  if (!browsing) {
    if (online_directory_active_ && online_directory_) online_directory_->cancel();
    online_directory_active_ = false;
    online_servers_ = {};
    online_detail_id_.clear();
    return;
  }
  if (online_directory_active_ && online_directory_ && std::chrono::steady_clock::now() >= online_refresh_at_) {
    online_refresh_at_ = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    online_directory_->refresh();
  }
  OnlineDirectorySnapshot next;
  if (online_directory_) {
    auto changed = online_directory_->snapshot_after(online_servers_.revision);
    if (!changed) return; // Avoid copying up to 1000 listings every rendered frame.
    next = std::move(*changed);
  }
  if (next.message.empty()) next.message = "No master server configured.";
  if (next.revision == online_servers_.revision && next.status == online_servers_.status && next.message == online_servers_.message) return;
  std::string selected;
  for (const auto &f : stack_)
    if (f.screen == 219 && f.context == online_server_browser_context && f.selected < online_servers_.servers.size()) selected = online_servers_.servers[f.selected].id;
  online_servers_ = std::move(next);
  for (auto &f : stack_) {
    if ((f.screen != 219 && f.screen != 237) || f.context != online_server_browser_context) continue;
    // Preserve the original widget allocation and authored timing on updates.
    f.entries.clear();
    populate_online_directory(f);
    if (f.screen == 219 && !selected.empty()) {
      const auto found = std::find_if(online_servers_.servers.begin(), online_servers_.servers.end(), [&](const auto &server) { return server.id == selected; });
      if (found != online_servers_.servers.end()) f.selected = unsigned(found - online_servers_.servers.begin());
    }
    f.selected = std::min(f.selected, unsigned(f.entries.size() - 1));
  }
  ++revision;
}
void MenuState::populate_online_directory(MenuFrame &frame) const {
  frame.title = frame.screen == 219 ? "ONLINE SERVERS" : "SERVER DETAILS";
  frame.description.clear();
  if (frame.screen == 219) {
    for (const auto &server : online_servers_.servers) frame.entries.push_back({server.name, 237, int(online_server_browser_context)});
    if (frame.entries.empty()) frame.entries.push_back({"Refresh", -13, 0});
  } else {
    if (detail(online_servers_, online_detail_id_))
      for (const auto *label : fields) frame.entries.push_back({label, -1, 0});
    else frame.entries.push_back({"Back", -5, 0});
  }
}
std::string MenuState::online_directory_text(const MenuFrame &frame, unsigned index) const {
  if (frame.screen == 237) {
    const auto *server = detail(online_servers_, online_detail_id_);
    if (index == 0) return "Server details\n\nJoining is not available yet.";
    if (index == 1) return server && frame.selected < fields.size() ? fields[frame.selected] : "Server unavailable";
    if (index == 2) return server ? field(*server, frame.selected) : "This server is no longer listed. Go back and refresh.";
    return {};
  }
  const auto *server = frame.selected < online_servers_.servers.size() ? &online_servers_.servers[frame.selected] : nullptr;
  if (index == 0) return server ? field(*server, 3) + " players\n\nMap: " + server->map + "\n\n" + server->gametype
                              : "Public community servers";
  if (index == 1) return online_servers_.status == OnlineDirectoryStatus::Loading ? "Finding servers..."
      : std::to_string(online_servers_.servers.size()) + " servers";
  if (index == 2) {
    if (!server) return online_servers_.message.empty() ? "No master server configured." : online_servers_.message;
    return server->name + "\nRegion: " + field(*server, 4) + "\n\nJoining is not available yet. Select for details.";
  }
  // PC13's focused summary is a single30-unit line. Its empty-list field
  // occupies a separate authored slot above the rows.
  if (index == 3) return server ? server->map + " | " + server->gametype + " | " + field(*server, 3) + " players" : "Public community servers";
  if (index == 4) return server ? std::string{} : online_directory_text(frame, 1);
  return {};
}
std::string MenuState::online_directory_column(const MenuFrame &frame, unsigned row, unsigned column) const {
  if (frame.screen != 219 || frame.context != online_server_browser_context) return {};
  if (row >= online_servers_.servers.size())
    return column == 6 && row < frame.entries.size() ? frame.entries[row].label : std::string{};
  const auto &server = online_servers_.servers[row];
  switch (column) {
  case 0: return server.name;
  case 1: return server.map;
  case 2: return server.gametype;
  case 4: return field(server, 3);
  // Discovery does not establish a variant, QoS or gameplay admission.
  // "Listed" describes only the directory record that owns this row.
  case 5: return "Listed";
  default: return {};
  }
}
} // namespace halo2
