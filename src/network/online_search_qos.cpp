#include "halo2/online_search_qos.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;
void require(bool value) { if (!value) throw std::runtime_error("Search connection unavailable"); }
int progress(void *context, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<std::stop_token *>(context)->stop_requested() ? 1 : 0;
}
std::string pinned_url(const std::string &origin, bool local) {
  require(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
  require(origin.size() <= 2048 && origin.find_first_of("@?#") == std::string::npos &&
          std::none_of(origin.begin(),origin.end(),[](unsigned char c){return c <= 32 || c == 127 || c == '\\';}));
  std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> url(curl_url(), curl_url_cleanup);
  require(url && curl_url_set(url.get(), CURLUPART_URL, origin.c_str(), 0) == CURLUE_OK);
  auto part = [&](CURLUPart key) {
    char *raw{};
    const auto result = curl_url_get(url.get(), key, &raw, 0);
    std::unique_ptr<char, decltype(&curl_free)> owner(raw, curl_free);
    return result == CURLUE_OK && raw ? std::string(raw) : std::string{};
  };
  const auto scheme = part(CURLUPART_SCHEME), host = part(CURLUPART_HOST);
  require(!host.empty() && part(CURLUPART_PATH) == "/" && part(CURLUPART_USER).empty() &&
          part(CURLUPART_PASSWORD).empty() && part(CURLUPART_OPTIONS).empty());
  require(scheme == "https" || (local && scheme == "http" && (host == "127.0.0.1" || host == "[::1]")));
  auto value = origin;
  if (value.ends_with('/')) value.pop_back();
  return std::string(scheme == "https" ? "wss" : "ws") + value.substr(value.find(':')) + "/v1/matchmaking/qos";
}
}
struct OnlineSearchQos::Impl {
  std::jthread worker;
  Impl(std::string origin, std::string bearer, bool local) {
    const auto url = pinned_url(origin, local);
    require(bearer.size() >= 32 && bearer.size() <= 128 &&
            bearer.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_") == std::string::npos);
    worker = std::jthread([this, url, bearer = std::move(bearer)](std::stop_token stop) {
      while (!stop.stop_requested()) {
        try { run(url, bearer, stop); } catch (...) {}
        // A dropped probe connection must recover while the same queue remains
        // active. The backend invalidates old measurements on disconnect.
        const auto retry = Clock::now()+std::chrono::seconds(1);
        while (!stop.stop_requested() && Clock::now() < retry)
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  }
  ~Impl() { worker.request_stop(); if (worker.joinable()) worker.join(); }
  void run(const std::string &url, const std::string &bearer, std::stop_token stop) {
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
    require(bool(client));
    const auto authorization = "Authorization: Bearer "+bearer;
    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(curl_slist_append(nullptr, authorization.c_str()), curl_slist_free_all);
    curl_easy_setopt(client.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(client.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(client.get(), CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS_STR, "wss,ws");
    curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(client.get(), CURLOPT_PROXY, "");
    curl_easy_setopt(client.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(client.get(), CURLOPT_TIMEOUT_MS, 3000L);
    curl_easy_setopt(client.get(), CURLOPT_CONNECTTIMEOUT_MS, 2000L);
    curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(client.get(), CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(client.get(), CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(client.get(), CURLOPT_XFERINFOFUNCTION, progress);
    curl_easy_setopt(client.get(), CURLOPT_XFERINFODATA, &stop);
#ifdef CURLWS_NOAUTOPONG
    curl_easy_setopt(client.get(), CURLOPT_WS_OPTIONS, CURLWS_NOAUTOPONG);
#endif
    require(curl_easy_perform(client.get()) == CURLE_OK);
    auto send = [&](const char *bytes, std::size_t size, unsigned flags) {
      std::size_t offset{};
      const auto deadline = Clock::now()+std::chrono::milliseconds(500);
      for (;;) {
        std::size_t sent{};
        const auto result = curl_ws_send(client.get(), bytes+offset, size-offset, &sent, 0, flags);
        require(result == CURLE_OK || result == CURLE_AGAIN);
        offset += sent;
        if(offset == size)break;
        require(!stop.stop_requested() && Clock::now() < deadline);
        if (offset < size) std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
    };
    auto last = Clock::now();
    std::string assembled;
    while (!stop.stop_requested()) {
      char chunk[256]; std::size_t count{}; const curl_ws_frame *meta{};
      const auto result = curl_ws_recv(client.get(), chunk, sizeof(chunk), &count, &meta);
      if (result == CURLE_AGAIN) {
        require(Clock::now()-last < std::chrono::seconds(20));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        continue;
      }
      require(result == CURLE_OK && meta && !(meta->flags & CURLWS_CLOSE));
      last = Clock::now();
      if (meta->flags & CURLWS_PING) { require(count <= 125 && !meta->bytesleft); send(chunk, count, CURLWS_PONG); continue; }
      if (meta->flags & CURLWS_PONG) continue;
      require((meta->flags & CURLWS_TEXT) && assembled.size()+count <= 256);
      assembled.append(chunk, count);
      if (meta->bytesleft || (meta->flags & CURLWS_CONT)) continue;
      bool duplicate = false;
      auto data = Json::parse(assembled, [&](int, Json::parse_event_t event, Json &) {
        // Only one key is allowed, so any second key, including a duplicate,
        // is rejected before nlohmann's object replacement can hide it.
        if (event == Json::parse_event_t::key) { if (duplicate) throw std::runtime_error("Invalid search challenge"); duplicate = true; }
        return true;
      });
      assembled.clear();
      require(data.is_object() && data.size() == 1 && (data.contains("ping") || data.contains("probe")));
      const auto &nonce = data.begin().value();
      require(nonce.is_string());
      const auto value = nonce.get<std::string>();
      require(value.size() == 32 && value.find_first_not_of("0123456789abcdef") == std::string::npos);
      const auto reply = Json{{data.contains("ping") ? "pong" : "echo", value}}.dump();
      send(reply.data(), reply.size(), CURLWS_TEXT);
    }
  }
};
OnlineSearchQos::OnlineSearchQos(std::string origin, std::string bearer, bool local)
  : impl_(std::make_unique<Impl>(std::move(origin), std::move(bearer), local)) {}
OnlineSearchQos::~OnlineSearchQos() = default;
} // namespace halo2
