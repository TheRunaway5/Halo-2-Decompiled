#include "halo2/online_directory.hpp"
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
using Json = nlohmann::json;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<class F> void eventually(F predicate) {
  const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while (!predicate() && std::chrono::steady_clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(2));
  check(predicate(), "Directory worker did not finish in time");
}
OnlineDirectorySnapshot fetch(OnlineDirectory &client) {
  client.refresh();
  eventually([&] { return client.snapshot().status != OnlineDirectoryStatus::Loading; });
  return client.snapshot();
}
Json listing() {
  return {{"server_id", "00000000-0000-0000-0000-000000000001"}, {"owner_id", "00000000-0000-0000-0000-000000000002"},
    {"origin_master_id", "00000000-0000-0000-0000-000000000003"}, {"registered_at", 1000}, {"last_heartbeat", 1005}, {"expires_at", 1090},
    {"reachability", "unverified"}, {"name", "Community Server"}, {"host", "203.0.113.10"}, {"port", 2002}, {"map", "lockout"},
    {"gametype", "Slayer"}, {"players", 3}, {"max_players", 16}, {"game_version", "simulator-v1"}, {"visibility", "public"},
    {"server_type", "dedicated"}, {"region", "US-EAST"}};
}
}
int main(int argc, char **argv) {
  try {
    if (argc > 2 && std::string(argv[1]) == "--probe") {
      OnlineDirectoryOptions options; options.allow_loopback_http = true;
      for (int i = 2; i < argc; ++i) options.masters.emplace_back(argv[i]);
      OnlineDirectory directory(options);
      const auto result = fetch(directory);
      check(result.status == OnlineDirectoryStatus::Ready, "Live discovery failed");
      Json rows = Json::array();
      for (const auto &server : result.servers) rows.push_back({{"name", server.name}, {"map", server.map}, {"players", server.players}, {"gametype", server.gametype}});
      std::cout << rows.dump() << '\n';
      return 0;
    }
    const OnlineDirectoryOptions options{{"https://primary.example/", "https://backup.example"}, false};
    const auto clock = [] { return 1010; };
    const auto body = Json{{"servers", Json::array({listing()})}}.dump();
    std::atomic<unsigned> requests{};
    OnlineDirectory client(options, [&](const auto &url, auto) -> OnlineDirectoryResponse {
      ++requests;
      if (url == "https://primary.example/v1/servers") return {503, "Unavailable"};
      check(url == "https://backup.example/v1/servers", "Request escaped configured discovery route");
      return {200, body};
    }, clock);
    auto result = fetch(client);
    check(result.status == OnlineDirectoryStatus::Ready && result.servers.size() == 1 && requests == 2, "Failover or decoding failed");
    check(result.servers[0].name == "Community Server" && result.servers[0].map == "lockout" && result.servers[0].players == 3 &&
          result.servers[0].port == 2002 && result.servers[0].region == "US-EAST", "Advertised fields changed");
    requests = 0;
    OnlineDirectory empty(options, [&](const auto &, auto) -> OnlineDirectoryResponse { ++requests; return {200, R"({"servers":[]})"}; }, clock);
    check(fetch(empty).servers.empty() && requests == 1, "Empty snapshot triggered failover");
    OnlineDirectory unconfigured;
    check(fetch(unconfigured).status == OnlineDirectoryStatus::Unconfigured, "Missing configuration started networking");
    for (const auto *url : {"http://remote.example", "https://master.example/api", "https://user:password@master.example", "https://master.example?x", "https://master.example#x", "file:///tmp/master", "https://master.example:99999"}) {
      OnlineDirectory bad({{url}, true}, [&](const auto &, auto) -> OnlineDirectoryResponse { throw std::runtime_error("Must not fetch"); });
      check(fetch(bad).status == OnlineDirectoryStatus::Failed, "Unsafe master configuration accepted");
    }
    OnlineDirectory plain({{"http://127.0.0.1:18081"}, false});
    check(fetch(plain).status == OnlineDirectoryStatus::Failed, "Loopback HTTP did not require opt-in");
    OnlineDirectory local({{"http://127.0.0.1:18081"}, true}, [&](const auto &, auto) -> OnlineDirectoryResponse { return {200, body}; }, clock);
    check(fetch(local).servers.size() == 1, "Explicit loopback development configuration failed");
    std::atomic<std::int64_t> now{1010};
    OnlineDirectory expiring(options, [&](const auto &, auto) -> OnlineDirectoryResponse { return {200, body}; }, [&] { return now.load(); });
    const auto before = fetch(expiring);
    check(!expiring.snapshot_after(before.revision), "Unchanged directory snapshot copied its listing bank");
    now = 1090;
    check(expiring.snapshot_after(before.revision).has_value(), "Expiry did not publish a directory change");
    const auto after = expiring.snapshot();
    check(after.servers.empty() && after.revision > before.revision, "Cached listing outlived its lease");
    for (const auto &[key, value] : std::vector<std::pair<std::string, Json>>{{"players", true}, {"players", 17}, {"port", 0}, {"max_players", 17},
         {"server_id", "invalid"}, {"visibility", "private"}, {"name", "Line\nbreak"}, {"name", std::string(65, 'x')},
         {"expires_at", 2000}, {"last_heartbeat", 2000}, {"region", "lowercase"}, {"server_type", "unknown"}}) {
      auto invalid = listing(); invalid[key] = value;
      const auto raw = Json{{"servers", Json::array({invalid})}}.dump();
      OnlineDirectory bad(options, [&](const auto &, auto) -> OnlineDirectoryResponse { return {200, raw}; }, clock);
      check(fetch(bad).status == OnlineDirectoryStatus::Failed, "Invalid directory metadata accepted");
    }
    for (const auto &raw : {std::string(R"({"servers":[],"servers":[]})"), std::string("{bad json"),
         Json{{"servers", Json::array({listing(), listing()})}}.dump(), std::string(4 * 1024 * 1024 + 1, ' ')}) {
      OnlineDirectory bad(options, [&](const auto &, auto) -> OnlineDirectoryResponse { return {200, raw}; }, clock);
      check(fetch(bad).status == OnlineDirectoryStatus::Failed, "Malformed/oversized/duplicate response accepted");
    }
    std::atomic<unsigned> calls{};
    OnlineDirectory cancelled(options, [&](const auto &, std::stop_token stop) -> OnlineDirectoryResponse {
      ++calls;
      while (!stop.stop_requested()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      return {200, body}; // A late successful reply must not undo cancellation.
    }, clock);
    const auto start = std::chrono::steady_clock::now();
    cancelled.refresh();
    check(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100), "Refresh blocked the UI");
    eventually([&] { return calls == 1; });
    cancelled.refresh();
    check(calls == 1, "Repeated refresh multiplied active workers");
    cancelled.cancel();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    check(cancelled.snapshot().status == OnlineDirectoryStatus::Idle && cancelled.snapshot().servers.empty(), "Cancelled reply leaked into state");
    std::atomic<bool> fail{};
    OnlineDirectory recovery(options, [&](const auto &, auto) -> OnlineDirectoryResponse { return {fail ? 503 : 200, body}; }, clock);
    check(fetch(recovery).servers.size() == 1, "Initial recovery fixture failed");
    fail = true;
    check(fetch(recovery).status == OnlineDirectoryStatus::Failed && recovery.snapshot().servers.empty(), "Failed refresh retained stale listings");
    fail = false;
    check(fetch(recovery).servers.size() == 1, "Retry failed to recover");
    std::cout << "PASS asynchronous discovery, failover, configuration, bounded parsing, expiry, cancellation and recovery\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
