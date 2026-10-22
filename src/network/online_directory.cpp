#include "halo2/online_directory.hpp"
#include "halo2/online_service.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
constexpr std::size_t max_body = 4 * 1024 * 1024;
using Json = nlohmann::json;
void require(bool value) { if (!value) throw std::runtime_error("Invalid directory response"); }
void initialize_curl() {
  static const auto result = curl_global_init(CURL_GLOBAL_DEFAULT);
  require(result == CURLE_OK);
}
std::string origin(const std::string &value, bool local_http) {
  require(!value.empty() && value.size() <= 2048 &&
          std::none_of(value.begin(), value.end(), [](unsigned char c) { return c <= 32 || c == 127 || c == '\\'; }));
  initialize_curl();
  const std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url(curl_url(), curl_url_cleanup);
  require(url && curl_url_set(url.get(), CURLUPART_URL, value.c_str(), 0) == CURLUE_OK);
  auto part = [&](CURLUPart key) {
    char *raw = nullptr;
    const auto result = curl_url_get(url.get(), key, &raw, 0);
    const std::unique_ptr<char, decltype(&curl_free)> allocated(raw, curl_free);
    return result == CURLUE_OK && raw ? std::string(raw) : std::string{};
  };
  const auto scheme = part(CURLUPART_SCHEME), host = part(CURLUPART_HOST);
  require(!host.empty() && part(CURLUPART_USER).empty() && part(CURLUPART_PASSWORD).empty() &&
          part(CURLUPART_OPTIONS).empty() && part(CURLUPART_QUERY).empty() && part(CURLUPART_FRAGMENT).empty());
  // Reject even empty userinfo/query/fragment syntax; accept an optional final slash.
  require(value.find('@') == std::string::npos && value.find('?') == std::string::npos && value.find('#') == std::string::npos);
  require(part(CURLUPART_PATH) == "/");
  require(scheme == "https" || (scheme == "http" && local_http && (host == "127.0.0.1" || host == "[::1]")));
  char *normalized = nullptr;
  require(curl_url_get(url.get(), CURLUPART_URL, &normalized, 0) == CURLUE_OK);
  const std::unique_ptr<char, decltype(&curl_free)> allocated(normalized, curl_free);
  std::string result(normalized);
  if (result.ends_with('/')) result.pop_back();
  return result;
}
struct Transfer { std::string body; std::stop_token stop; };
std::size_t receive(char *data, std::size_t size, std::size_t count, void *context) {
  auto &transfer = *static_cast<Transfer *>(context);
  if (transfer.stop.stop_requested() || (count && size > std::numeric_limits<std::size_t>::max() / count)) return 0;
  const auto bytes = size * count;
  if (bytes > max_body - transfer.body.size()) return 0;
  try { transfer.body.append(data, bytes); } catch (...) { return 0; }
  return bytes;
}
int progress(void *context, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<Transfer *>(context)->stop.stop_requested() ? 1 : 0;
}
OnlineDirectoryResponse get(const std::string &url, std::stop_token stop) {
  initialize_curl();
  const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
  require(bool(client));
  Transfer transfer{{}, stop};
  curl_easy_setopt(client.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS_STR, "https,http");
  curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(client.get(), CURLOPT_PROXY, "");
  curl_easy_setopt(client.get(), CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(client.get(), CURLOPT_CONNECTTIMEOUT_MS, 2000L);
  curl_easy_setopt(client.get(), CURLOPT_TIMEOUT_MS, 3000L);
  curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(client.get(), CURLOPT_ACCEPT_ENCODING, "identity");
  curl_easy_setopt(client.get(), CURLOPT_HTTP_CONTENT_DECODING, 0L);
  curl_easy_setopt(client.get(), CURLOPT_WRITEFUNCTION, receive);
  curl_easy_setopt(client.get(), CURLOPT_WRITEDATA, &transfer);
  curl_easy_setopt(client.get(), CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(client.get(), CURLOPT_XFERINFOFUNCTION, progress);
  curl_easy_setopt(client.get(), CURLOPT_XFERINFODATA, &transfer);
  require(curl_easy_perform(client.get()) == CURLE_OK && !stop.stop_requested());
  long status = 0;
  curl_easy_getinfo(client.get(), CURLINFO_RESPONSE_CODE, &status);
  return {status, std::move(transfer.body)};
}
std::int64_t integer(const Json &object, const char *key, std::int64_t low, std::int64_t high) {
  const auto &value = object.at(key);
  require(value.is_number_integer());
  if (value.is_number_unsigned()) require(value.get<std::uint64_t>() <= std::uint64_t(high));
  const auto number = value.get<std::int64_t>();
  require(number >= low && number <= high);
  return number;
}
std::string text(const Json &object, const char *key, std::size_t maximum = 64) {
  const auto &value = object.at(key);
  require(value.is_string());
  auto result = value.get<std::string>();
  require(!result.empty() && result.size() <= maximum &&
          std::none_of(result.begin(), result.end(), [](unsigned char c) { return c < 32 || c == 127; }));
  return result;
}
std::string uuid(const Json &object, const char *key) {
  auto result = text(object, key);
  require(result.size() == 36);
  for (unsigned i = 0; i < result.size(); ++i)
    require(i == 8 || i == 13 || i == 18 || i == 23 ? result[i] == '-'
        : (result[i] >= '0' && result[i] <= '9') || (result[i] >= 'a' && result[i] <= 'f'));
  return result;
}
std::vector<OnlineServer> parse(const std::string &body, std::int64_t now) {
  require(body.size() <= max_body);
  std::vector<std::set<std::string>> keys;
  const auto root = Json::parse(body, [&](int depth, Json::parse_event_t event, Json &value) {
    require(depth <= 16);
    if (event == Json::parse_event_t::object_start) keys.emplace_back();
    if (event == Json::parse_event_t::key) require(!keys.empty() && keys.back().insert(value.get<std::string>()).second);
    if (event == Json::parse_event_t::object_end) keys.pop_back();
    return true;
  });
  require(root.is_object() && root.size() == 1 && root.contains("servers"));
  const auto &rows = root.at("servers");
  require(rows.is_array() && rows.size() <= 1000);
  std::vector<OnlineServer> result;
  std::set<std::string> ids;
  for (const auto &row : rows) {
    require(row.is_object() && row.size() == (row.contains("region") ? 18 : 17));
    OnlineServer server;
    server.id = uuid(row, "server_id");
    require(ids.insert(server.id).second);
    uuid(row, "owner_id"); uuid(row, "origin_master_id"); // Validate, then discard.
    const auto registered = integer(row, "registered_at", 0, 9007199254740991LL);
    const auto heartbeat = integer(row, "last_heartbeat", registered, 9007199254740991LL);
    server.expires_at = integer(row, "expires_at", heartbeat, heartbeat + 180);
    require(heartbeat <= now + 30 && row.at("reachability") == "unverified" && row.at("visibility") == "public");
    server.name = text(row, "name"); server.host = text(row, "host", 253);
    require(std::all_of(server.host.begin(), server.host.end(), [](unsigned char c) {
      return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == ':';
    }));
    server.map = text(row, "map"); server.gametype = text(row, "gametype");
    server.game_version = text(row, "game_version"); server.server_type = text(row, "server_type");
    require(server.server_type == "dedicated" || server.server_type == "player-hosted");
    server.port = unsigned(integer(row, "port", 1, 65535));
    server.max_players = unsigned(integer(row, "max_players", 1, 16));
    server.players = unsigned(integer(row, "players", 0, server.max_players));
    if (row.contains("region")) {
      server.region = text(row, "region", 16);
      require(server.region.size() >= 2 && std::all_of(server.region.begin(), server.region.end(), [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
      }));
    }
    if (server.expires_at > now) result.push_back(std::move(server));
  }
  std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return a.id < b.id; });
  return result;
}
} // namespace
OnlineDirectoryOptions online_directory_environment() {
  OnlineDirectoryOptions result;
  if (const char *http = std::getenv("H2_DIRECTORY_ALLOW_LOOPBACK_HTTP")) result.allow_loopback_http = std::string(http) == "1";
  if (const char *urls = std::getenv("H2_MASTER_URLS")) {
    const std::string input(urls);
    std::size_t start = input.find_first_not_of(", \t\r\n");
    while (start != std::string::npos) {
      const auto end = input.find_first_of(", \t\r\n", start);
      result.masters.push_back(input.substr(start, end - start));
      if (end == std::string::npos) break;
      start = input.find_first_not_of(", \t\r\n", end);
    }
  } else result.masters.emplace_back(default_online_origin);
  return result;
}
struct OnlineDirectory::Impl {
  Http http;
  Now now;
  std::vector<std::string> masters;
  mutable std::mutex mutex;
  std::condition_variable_any wake;
  mutable OnlineDirectorySnapshot state;
  bool pending{}, configured{};
  std::uint64_t generation{};
  std::stop_source request_stop;
  std::jthread worker;
  Impl(OnlineDirectoryOptions options, Http transport, Now clock) : http(std::move(transport)), now(std::move(clock)) {
    if (!http) http = get;
    if (!now) now = [] { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); };
    state.revision = 1;
    state.message = "No master server configured.";
    if (options.masters.empty()) return;
    try {
      require(options.masters.size() <= 8);
      for (const auto &url : options.masters) {
        const auto normalized = origin(url, options.allow_loopback_http);
        if (std::find(masters.begin(), masters.end(), normalized) == masters.end()) masters.push_back(normalized);
      }
    } catch (...) { state.status = OnlineDirectoryStatus::Failed; state.message = "Master server configuration is invalid."; return; }
    configured = true;
    state.status = OnlineDirectoryStatus::Idle;
    state.message = "Refresh to discover servers.";
    worker = std::jthread([this](std::stop_token stop) {
      while (!stop.stop_requested()) {
        std::unique_lock lock(mutex);
        if (!wake.wait(lock, stop, [&] { return pending; })) break;
        pending = false;
        const auto version = generation;
        request_stop = std::stop_source{};
        auto source = request_stop;
        const auto request = request_stop.get_token();
        lock.unlock();
        std::stop_callback shutdown(stop, [source]() mutable { source.request_stop(); });
        bool success = false;
        std::vector<OnlineServer> servers;
        for (const auto &master : masters) {
          if (request.stop_requested()) break;
          try {
            const auto response = http(master + "/v1/servers", request);
            require(response.status == 200);
            servers = parse(response.body, now());
            success = true;
            break; // An empty valid snapshot is also authoritative.
          } catch (...) { /* Try the next explicitly configured master. */ }
        }
        lock.lock();
        if (version != generation || stop.stop_requested() || request.stop_requested()) continue;
        state.status = success ? OnlineDirectoryStatus::Ready : OnlineDirectoryStatus::Failed;
        state.servers = std::move(servers);
        state.message = success ? (state.servers.empty() ? "No public servers available." : "Public servers available.")
                                : "Cannot reach a master server. Press X to retry.";
        ++state.revision;
      }
    });
  }
  ~Impl() {
    worker.request_stop();
    { std::lock_guard lock(mutex); request_stop.request_stop(); }
    wake.notify_all();
    if (worker.joinable()) worker.join();
  }
  void expire() const {
    const auto time = now();
    const auto removed = std::erase_if(state.servers, [&](const auto &server) { return server.expires_at <= time; });
    if (removed) {
      if (state.servers.empty() && state.status == OnlineDirectoryStatus::Ready) state.message = "No public servers available.";
      ++state.revision;
    }
  }
};
OnlineDirectory::OnlineDirectory(OnlineDirectoryOptions options, Http http, Now now)
    : impl_(std::make_unique<Impl>(std::move(options), std::move(http), std::move(now))) {}
OnlineDirectory::~OnlineDirectory() = default;
void OnlineDirectory::refresh() {
  std::lock_guard lock(impl_->mutex);
  if (!impl_->configured || impl_->state.status == OnlineDirectoryStatus::Loading) return;
  ++impl_->generation;
  impl_->pending = true;
  impl_->state.status = OnlineDirectoryStatus::Loading;
  impl_->state.message = "Finding public servers...";
  ++impl_->state.revision;
  impl_->wake.notify_all();
}
void OnlineDirectory::cancel() {
  std::lock_guard lock(impl_->mutex);
  impl_->request_stop.request_stop();
  ++impl_->generation;
  impl_->pending = false;
  impl_->state.servers.clear();
  if (impl_->configured) { impl_->state.status = OnlineDirectoryStatus::Idle; impl_->state.message = "Refresh to discover servers."; }
  ++impl_->state.revision;
}
OnlineDirectorySnapshot OnlineDirectory::snapshot() const {
  std::lock_guard lock(impl_->mutex);
  impl_->expire();
  return impl_->state;
}
std::optional<OnlineDirectorySnapshot> OnlineDirectory::snapshot_after(std::uint64_t revision) const {
  std::lock_guard lock(impl_->mutex);
  impl_->expire();
  if (impl_->state.revision == revision) return std::nullopt;
  return impl_->state;
}
} // namespace halo2
