// Actual native thread/mutex/stop-token boundary, modern account API only.
#include "halo2/online_account.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <curl/curl.h>
#include <exception>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <stdexcept>
#include <thread>
using namespace halo2;
using Json = nlohmann::json;
using namespace std::chrono_literals;
#ifdef HALO2_ACCOUNT_CURL_OBSERVER
namespace {
std::mutex curl_barrier_mutex;
std::condition_variable curl_barrier_cv;
bool curl_observe{}, curl_entered{}, curl_released{};
thread_local std::exception_ptr entropy_exception;
} // namespace
extern "C" CURLcode __real_curl_easy_perform(CURL *);
extern "C" int __real_RAND_bytes(unsigned char *, int);
extern "C" int __wrap_RAND_bytes(unsigned char *bytes, int count) {
  if (entropy_exception)
    std::rethrow_exception(entropy_exception);
  return __real_RAND_bytes(bytes, count);
}
extern "C" CURLcode __wrap_curl_easy_perform(CURL *curl) {
  const auto result = __real_curl_easy_perform(curl);
  long status{};
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  std::unique_lock lock(curl_barrier_mutex);
  if (curl_observe && result == CURLE_OK && status == 201) {
    curl_entered = true;
    curl_barrier_cv.notify_all();
    if (!curl_barrier_cv.wait_for(lock, 5s, [] { return curl_released; }))
      return CURLE_ABORTED_BY_CALLBACK;
  }
  return result;
}
#endif
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class F> void eventually(F f) {
  const auto end = std::chrono::steady_clock::now() + 5s;
  while (!f() && std::chrono::steady_clock::now() < end)
    std::this_thread::sleep_for(1ms);
  check(f(), "Native account client fixture timed out");
}
std::size_t discard(char *, std::size_t size, std::size_t count, void *) {
  return size * count;
}
std::shared_ptr<DiscordLogin> authenticated_login() {
  auto browser = std::make_shared<std::pair<std::mutex, std::string>>();
  DiscordLoginOptions options;
  options.callback_port = 42108;
  auto login = std::make_shared<DiscordLogin>(
      [browser](const std::string &url) {
        std::lock_guard lock(browser->first);
        browser->second = url;
        return true;
      },
      options,
      [](const DiscordHttpRequest &r, std::stop_token) -> DiscordHttpResponse {
        return r.url.ends_with("/token")
                   ? DiscordHttpResponse{200,
                                         R"({"access_token":"native-fixture-oauth","token_type":"Bearer","scope":"identify","expires_in":3600})"}
                   : DiscordHttpResponse{
                         200,
                         R"({"id":"123456789012345678","username":"fixture","global_name":"Fixture"})"};
      });
  login->start();
  std::string opened;
  eventually([&] {
    std::lock_guard lock(browser->first);
    opened = browser->second;
    return !opened.empty();
  });
  const auto at = opened.find("state=") + 6;
  const auto state = opened.substr(at, opened.find('&', at) - at);
  const auto url =
      "http://127.0.0.1:42108/discord/callback?code=test&state=" + state;
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(),
                                                           curl_easy_cleanup);
  check(bool(curl), "Curl fixture initialization failed");
  curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_PROXY, "");
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, 2000L);
  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, discard);
  check(curl_easy_perform(curl.get()) == CURLE_OK,
        "Discord fixture callback failed");
  eventually([&] { return login->authorization().has_value(); });
  return login;
}
Json late_grant(const std::shared_ptr<DiscordLogin> &login, bool reconnect,
                bool before) {
  std::mutex mutex;
  std::condition_variable cv;
  bool entered = false, released = false;
  std::atomic<unsigned> deletes{}, posts{};
  std::atomic<bool> stopped{}, cleanup_cancelled{};
  OnlineAccountOptions options;
  options.endpoint = "https://account.example";
  options.poll_interval = 10ms;
  {
    OnlineAccount client(
        login, options,
        [&](const OnlineMatchmakingRequest &r,
            std::stop_token stop) -> OnlineMatchmakingResponse {
          if (r.method == "POST" && r.url.ends_with("/session")) {
            const auto number = ++posts;
            check(r.bearer == "native-fixture-oauth",
                  "Wrong identity in session exchange");
            if (number == 1) {
              std::unique_lock lock(mutex);
              entered = true;
              cv.notify_all();
              check(cv.wait_for(lock, 5s, [&] { return released; }),
                    "Late response barrier timed out");
              stopped = stop.stop_requested();
            }
            return {201, Json{{"token", number == 1 ? "late-owned-grant"
                                                    : "new-owned-grant"},
                              {"expires_in", 900}}
                             .dump()};
          }
          if (r.method == "DELETE") {
            if (r.bearer == "late-owned-grant")
              ++deletes;
            cleanup_cancelled =
                cleanup_cancelled.load() || stop.stop_requested();
            return {200, "{}"};
          }
          if (r.url.find("/v1/clans") != std::string::npos)
            return {200, R"({"clan":null,"invitations":[]})"};
          if (r.url.find("/v1/friends") != std::string::npos)
            return {200, R"({"friend_tag":"Fixture#12345678","friends":[]})"};
          return {200, R"({"party":null,"invitations":[]})"};
        });
    client.connect("Pilot");
    {
      std::unique_lock lock(mutex);
      check(cv.wait_for(lock, 5s, [&] { return entered; }),
            "Session exchange did not start");
    }
    if (reconnect)
      client.connect("Replacement");
    else
      client.sign_out();
    {
      std::lock_guard lock(mutex);
      released = true;
    }
    cv.notify_all();
    if (reconnect)
      eventually([&] { return client.snapshot().authenticated; });
    else
      check(client.snapshot().message == "Signed out.",
            "Sign-out snapshot was lost");
    // Joining the real native worker proves final cleanup has finished.
  }
  check(stopped, "Request did not receive the original cancellation token");
  check(!cleanup_cancelled, "Cleanup reused the cancelled request token");
  check(deletes == (before ? 0U : 1U),
        before ? "Preserved C++ did not reproduce the orphan"
               : "Complete late grant was orphaned");
  return Json{{"trigger", reconnect ? "reconnect" : "signout"},
              {"late_grant_deletes", deletes.load()},
              {"request_cancelled", stopped.load()},
              {"cleanup_cancelled", cleanup_cancelled.load()}};
}
Json rejected_lease(const std::shared_ptr<DiscordLogin> &login, bool before) {
  std::atomic<unsigned> posts{}, rejected_reads{}, rejected_deletes{};
  OnlineAccountOptions options;
  options.endpoint = "https://account.example";
  options.poll_interval = 100ms;
  {
    OnlineAccount client(
        login, options,
        [&](const OnlineMatchmakingRequest &r,
            std::stop_token) -> OnlineMatchmakingResponse {
          if (r.method == "POST" && r.url.ends_with("/session")) {
            const auto number = ++posts;
            return {201, Json{{"token", number == 1 ? "first-owned-grant"
                                                    : "rejected-owned-grant"},
                              {"expires_in", number == 1 ? 900 : 0}}
                             .dump()};
          }
          if (r.method == "DELETE") {
            if (r.bearer == "rejected-owned-grant")
              ++rejected_deletes;
            return {200, "{}"};
          }
          if (r.bearer == "rejected-owned-grant")
            ++rejected_reads;
          if (r.url.find("/v1/clans") != std::string::npos)
            return {200, R"({"clan":null,"invitations":[]})"};
          return {200, R"({"friend_tag":"Fixture#12345678","friends":[]})"};
        });
    client.connect("Pilot");
    eventually([&] { return client.snapshot().authenticated; });
    client.connect("Renamed");
    eventually([&] { return posts >= 3 || rejected_reads > 0; });
    if (!before)
      check(!client.snapshot().authenticated,
            "Rejected lease became authenticated");
  }
  if (before)
    check(posts == 2 && rejected_reads > 0,
          "Preserved C++ did not reproduce rejected-grant reuse");
  else
    check(rejected_reads == 0 && rejected_deletes == posts - 1,
          "Rejected grants were reused or not retired");
  return Json{{"trigger", "replacement lease expires_in=0"},
              {"session_exchanges", posts.load()},
              {"rejected_grant_reads", rejected_reads.load()},
              {"rejected_grant_deletes", rejected_deletes.load()}};
}
#ifdef HALO2_ACCOUNT_CURL_OBSERVER
struct EntropyError : std::logic_error {
  using std::logic_error::logic_error;
};
Json native_exception(const std::shared_ptr<DiscordLogin> &login) {
  OnlineAccountOptions options;
  options.endpoint = "https://account.example";
  options.poll_interval = 100ms;
  OnlineAccount client(
      login, options,
      [](const OnlineMatchmakingRequest &r,
         std::stop_token) -> OnlineMatchmakingResponse {
        if (r.method == "POST" && r.url.ends_with("/session"))
          return {201, R"({"token":"exception-owned-grant","expires_in":900})"};
        if (r.method == "DELETE")
          return {200, "{}"};
        if (r.url.find("/v1/clans") != std::string::npos)
          return {200, R"({"clan":null,"invitations":[]})"};
        return {200, R"({"friend_tag":"Fixture#12345678","friends":[]})"};
      });
  client.connect("Pilot");
  eventually([&] { return client.snapshot().authenticated; });
  const void *identity = nullptr;
  try {
    throw EntropyError("Native entropy refusal sentinel");
  } catch (const EntropyError &e) {
    entropy_exception = std::current_exception();
    identity = &e;
  }
  bool matched = false;
  try {
    client.command({"create", "Clan", 0, false});
  } catch (const EntropyError &e) {
    matched = &e == identity &&
              std::string(e.what()) == "Native entropy refusal sentinel";
  } catch (...) {
    entropy_exception = nullptr;
    throw;
  }
  entropy_exception = nullptr;
  check(matched,
        "Native exception type, text or object identity was lost across Rust");
  check(!client.snapshot().busy, "Failed entropy partially queued a command");
  return Json{{"trigger", "native entropy exception"},
              {"native_type_text_and_object_identity", true},
              {"partial_command", false}};
}
Json curl_late_grant(const std::shared_ptr<DiscordLogin> &login) {
  const auto *endpoint = std::getenv("HALO2_ACCOUNT_CLIENT_HTTP_ORIGIN");
  check(endpoint, "Private loopback origin is required");
  OnlineAccountOptions options;
  options.endpoint = endpoint;
  options.allow_loopback_http = true;
  options.poll_interval = 10ms;
  {
    std::lock_guard lock(curl_barrier_mutex);
    curl_observe = true;
    curl_entered = false;
    curl_released = false;
  }
  {
    OnlineAccount client(login, options);
    client.connect("Curl Pilot");
    {
      std::unique_lock lock(curl_barrier_mutex);
      check(curl_barrier_cv.wait_for(lock, 5s, [] { return curl_entered; }),
            "Real Curl exchange did not complete");
    }
    client.sign_out();
    {
      std::lock_guard lock(curl_barrier_mutex);
      curl_released = true;
    }
    curl_barrier_cv.notify_all();
    check(client.snapshot().message == "Signed out.",
          "Real Curl sign-out snapshot was lost");
  }
  {
    std::lock_guard lock(curl_barrier_mutex);
    curl_observe = false;
  }
  return Json{{"transport", "real native Curl"},
              {"trigger", "signout after complete 201"},
              {"authenticated", false}};
}
#endif
} // namespace
int main(int argc, char **argv) {
  try {
    bool before = false, http = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--expect-before")
        before = true;
      else if (arg == "--http")
        http = true;
      else
        throw std::runtime_error("Unknown native account fixture option");
    }
    const auto login = authenticated_login();
    Json cases = Json::array();
    if (http) {
#ifdef HALO2_ACCOUNT_CURL_OBSERVER
      cases.push_back(curl_late_grant(login));
#else
      throw std::runtime_error("Curl observer was not compiled");
#endif
    } else {
      cases.push_back(late_grant(login, false, before));
      cases.push_back(late_grant(login, true, before));
      cases.push_back(rejected_lease(login, before));
#ifdef HALO2_ACCOUNT_CURL_OBSERVER
      cases.push_back(native_exception(login));
#endif
    }
    std::cout << Json{{"passed", true},
                      {"reference_before", before},
                      {"cases", cases}}
                     .dump()
              << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
