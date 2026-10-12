#include "halo2/discord_login.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;
#ifdef _WIN32
using Socket = SOCKET;
constexpr auto invalid_socket = INVALID_SOCKET;
void close_socket(Socket fd) { closesocket(fd); }
#else
using Socket = int;
constexpr auto invalid_socket = -1;
void close_socket(Socket fd) { close(fd); }
#endif
struct SocketOwner {
  Socket fd = invalid_socket;
  explicit SocketOwner(Socket value) : fd(value) {}
  ~SocketOwner() { if (fd != invalid_socket) close_socket(fd); }
};
bool readable(Socket fd) {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(fd, &set);
  timeval timeout{0, 100000};
  return select(int(fd + 1), &set, nullptr, nullptr, &timeout) > 0;
}
std::string encode(const std::string &text) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string out;
  for (unsigned char c : text) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') out += char(c);
    else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
  }
  return out;
}
std::string decode(std::string text) {
  std::string out;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '%') {
      if (i + 2 >= text.size()) throw std::runtime_error("Invalid callback encoding.");
      unsigned value{};
      const auto result = std::from_chars(text.data() + i + 1, text.data() + i + 3, value, 16);
      if (result.ec != std::errc{} || result.ptr != text.data() + i + 3 || value < 32 || value == 127)
        throw std::runtime_error("Invalid callback encoding.");
      out += char(value); i += 2;
    } else out += text[i] == '+' ? ' ' : text[i];
  }
  return out;
}
std::map<std::string, std::string> query(const std::string &text) {
  std::map<std::string, std::string> values;
  for (std::size_t start = 0; start < text.size();) {
    auto end = text.find('&', start);
    if (end == std::string::npos) end = text.size();
    const auto part = text.substr(start, end - start);
    const auto split = part.find('=');
    if (split == std::string::npos || !values.emplace(decode(part.substr(0, split)), decode(part.substr(split + 1))).second)
      throw std::runtime_error("Invalid callback parameters.");
    start = end + 1;
  }
  return values;
}
std::string base64url(const unsigned char *data, unsigned size) {
  std::string out(4 * ((size + 2) / 3), '\0');
  EVP_EncodeBlock(reinterpret_cast<unsigned char *>(out.data()), data, int(size));
  while (!out.empty() && out.back() == '=') out.pop_back();
  std::replace(out.begin(), out.end(), '+', '-');
  std::replace(out.begin(), out.end(), '/', '_');
  return out;
}
std::string random_value() {
  std::array<unsigned char, 32> bytes{};
  if (RAND_bytes(bytes.data(), int(bytes.size())) != 1) throw std::runtime_error("Could not generate Discord login security values.");
  return base64url(bytes.data(), unsigned(bytes.size()));
}
std::string challenge(const std::string &verifier) {
  std::array<unsigned char, EVP_MAX_MD_SIZE> bytes{};
  unsigned size{};
  if (EVP_Digest(verifier.data(), verifier.size(), bytes.data(), &size, EVP_sha256(), nullptr) != 1)
    throw std::runtime_error("Could not prepare Discord login security values.");
  return base64url(bytes.data(), size);
}
bool snowflake(const std::string &value) {
  std::uint64_t id{};
  const auto result = std::from_chars(value.data(), value.data() + value.size(), id);
  return !value.empty() && value.front() != '0' && result.ec == std::errc{} && result.ptr == value.data() + value.size() && id != 0;
}
std::size_t receive_body(char *data, std::size_t size, std::size_t count, void *context) {
  auto &body = *static_cast<std::string *>(context);
  const auto length = size * count;
  if (length > 65536 || body.size() > 65536 - length) return 0;
  body.append(data, length);
  return length;
}
int transfer_progress(void *context, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<std::stop_token *>(context)->stop_requested() ? 1 : 0;
}
DiscordHttpResponse https(const DiscordHttpRequest &request, std::stop_token stop) {
  static const bool initialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
  if (!initialized) throw std::runtime_error("Could not initialize Discord HTTPS.");
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
  if (!curl) throw std::runtime_error("Could not initialize Discord HTTPS.");
  DiscordHttpResponse response;
  curl_easy_setopt(curl.get(), CURLOPT_URL, request.url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS_STR, "https");
  curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 25L);
  curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "Halo2Native/0.1 DiscordIdentity");
  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, receive_body);
  curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response.body);
  curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, transfer_progress);
  curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &stop);
  if (!request.form.empty()) curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, request.form.c_str());
  curl_slist *raw_headers = nullptr;
  if (!request.form.empty()) raw_headers = curl_slist_append(raw_headers, "Content-Type: application/x-www-form-urlencoded");
  if (!request.bearer.empty()) raw_headers = curl_slist_append(raw_headers, ("Authorization: Bearer " + request.bearer).c_str());
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(raw_headers, curl_slist_free_all);
  curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
  if (curl_easy_perform(curl.get()) != CURLE_OK)
    throw std::runtime_error("Could not reach Discord securely. Check your connection and try again.");
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response.status);
  return response;
}
void reply(Socket fd, bool accepted) {
  const std::string body = accepted
      ? "Discord authorization received. Return to Halo 2 to finish signing in."
      : "This callback was not accepted. Return to Halo 2 and try signing in again.";
  const auto response = "HTTP/1.1 " + std::string(accepted ? "200 OK" : "400 Bad Request") +
      "\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\n"
      "Referrer-Policy: no-referrer\r\nConnection: close\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
#ifdef MSG_NOSIGNAL
  send(fd, response.data(), int(response.size()), MSG_NOSIGNAL);
#else
  send(fd, response.data(), int(response.size()), 0);
#endif
}
} // namespace
struct DiscordLogin::Impl {
  OpenBrowser browser;
  DiscordLoginOptions options;
  Http http;
  mutable std::mutex mutex;
  DiscordLoginSnapshot state;
  std::string authorization;
  Clock::time_point expires{};
  std::jthread worker;
  void publish(DiscordLoginSnapshot value, Clock::time_point deadline = {}, std::string bearer = {}) {
    std::lock_guard lock(mutex);
    state = std::move(value); expires = deadline; authorization = std::move(bearer);
  }
  void run(std::stop_token stop, const std::shared_ptr<SocketOwner> &listener,
           const std::string &state_value, const std::string &verifier, const std::string &redirect) {
    try {
      const auto deadline = Clock::now() + options.timeout;
      std::string code;
      while (!stop.stop_requested() && Clock::now() < deadline) {
        if (!readable(listener->fd)) continue;
        SocketOwner peer(accept(listener->fd, nullptr, nullptr));
        if (peer.fd == invalid_socket) continue;
        std::string request;
        const auto peer_deadline = std::min(deadline, Clock::now() + std::chrono::seconds(2));
        while (!stop.stop_requested() && Clock::now() < peer_deadline && request.size() < 8192 && request.find("\r\n\r\n") == std::string::npos) {
          if (!readable(peer.fd)) continue;
          std::array<char, 1024> bytes{};
          const auto n = recv(peer.fd, bytes.data(), int(bytes.size()), 0);
          if (n <= 0) break;
          request.append(bytes.data(), std::size_t(n));
        }
        if (stop.stop_requested()) return;
        try {
          const auto end = request.find(" HTTP/1.");
          if (!request.starts_with("GET /discord/callback?") || end == std::string::npos || request.find("\r\n\r\n") == std::string::npos)
            throw std::runtime_error("Invalid callback.");
          const auto values = query(request.substr(22, end - 22));
          const auto found = values.find("state");
          if (found == values.end() || found->second != state_value) throw std::runtime_error("Invalid callback state.");
          if (values.contains("error")) {
            reply(peer.fd, true);
            publish({DiscordLoginStatus::Failed, {}, "Discord sign-in was declined. Try again when ready."});
            return;
          }
          const auto c = values.find("code");
          if (c == values.end() || c->second.empty() || c->second.size() > 2048) throw std::runtime_error("Missing code.");
          code = c->second;
          reply(peer.fd, true);
          break;
        } catch (const std::exception &) { reply(peer.fd, false); }
      }
      if (stop.stop_requested()) return;
      if (code.empty()) throw std::runtime_error("Discord sign-in timed out. Please try again.");
      // Release the callback port before contacting Discord.
      close_socket(listener->fd); listener->fd = invalid_socket;
      publish({DiscordLoginStatus::Verifying, {}, "Verifying your Discord identity..."});
      const auto token_response = http({"https://discord.com/api/v10/oauth2/token",
          "grant_type=authorization_code&client_id=" + encode(options.client_id) +
          "&code=" + encode(code) + "&redirect_uri=" + encode(redirect) + "&code_verifier=" + encode(verifier), {}}, stop);
      if (stop.stop_requested()) return;
      if (token_response.status != 200) throw std::runtime_error("Discord could not complete sign-in. Please try again.");
      const auto token = Json::parse(token_response.body);
      const auto access = token.at("access_token").get<std::string>();
      const auto lifetime = token.at("expires_in").get<std::int64_t>();
      const auto scopes = token.at("scope").get<std::string>();
      const auto scope_list = " " + scopes + " ";
      if (token.at("token_type").get<std::string>() != "Bearer" || access.empty() || access.size() > 4096 ||
          access.find_first_of("\r\n") != std::string::npos || lifetime <= 0 || lifetime > 31536000 || scope_list.find(" identify ") == std::string::npos)
        throw std::runtime_error("Discord returned an invalid identity authorization.");
      const bool communication=scope_list.find(" sdk.social_layer ")!=std::string::npos;
      if((options.social_presence || options.social_communication) &&
         (scope_list.find(" openid ")==std::string::npos ||
          (options.social_communication ? !communication :
           !communication && scope_list.find(" sdk.social_layer_presence ")==std::string::npos)))
        throw std::runtime_error("Discord Friends permission was not granted. Your ONLINE identity is unchanged.");
      const auto token_deadline = Clock::now() + std::chrono::seconds(lifetime);
      const auto user_response = http({"https://discord.com/api/v10/users/@me", {}, access}, stop);
      if (stop.stop_requested()) return;
      if (user_response.status != 200) throw std::runtime_error("Discord could not verify your identity. Please sign in again.");
      const auto user = Json::parse(user_response.body);
      DiscordIdentity identity{user.at("id").get<std::string>(), user.at("username").get<std::string>(), {}};
      if (user.contains("global_name") && !user.at("global_name").is_null()) identity.display_name = user.at("global_name").get<std::string>();
      if (!snowflake(identity.id) || identity.username.empty() || identity.username.size() > 128 || identity.display_name.size() > 128 ||
          (user.contains("bot") && user.at("bot").get<bool>())) throw std::runtime_error("Discord returned an invalid user identity.");
      for (const auto *name : {&identity.username, &identity.display_name})
        if (std::any_of(name->begin(), name->end(), [](unsigned char c) { return c < 32 || c == 127; }))
          throw std::runtime_error("Discord returned an invalid user name.");
      if (identity.display_name.empty()) identity.display_name = identity.username;
      publish({DiscordLoginStatus::SignedIn, identity, "Signed in as " + identity.username}, token_deadline, access);
    } catch (const Json::exception &) {
      if (!stop.stop_requested()) publish({DiscordLoginStatus::Failed, {}, "Discord returned an invalid response. Please try again."});
    } catch (const std::exception &e) {
      if (!stop.stop_requested()) publish({DiscordLoginStatus::Failed, {}, e.what()});
    }
  }
};
DiscordLogin::DiscordLogin(OpenBrowser browser, DiscordLoginOptions options, Http http) : impl_(std::make_unique<Impl>()) {
  impl_->browser = std::move(browser); impl_->options = std::move(options); impl_->http = http ? std::move(http) : https;
}
DiscordLogin::~DiscordLogin() { cancel(); }
void DiscordLogin::cancel() {
  if (impl_->worker.joinable()) { impl_->worker.request_stop(); impl_->worker.join(); }
  impl_->publish({});
}
void DiscordLogin::start() {
  cancel();
  try {
    if (!snowflake(impl_->options.client_id) || !impl_->options.callback_port || impl_->options.callback_port > 65535 || impl_->options.timeout.count() <= 0)
      throw std::runtime_error("Discord login configuration is invalid.");
#ifdef _WIN32
    static const bool started = [] { WSADATA data; return WSAStartup(MAKEWORD(2, 2), &data) == 0; }();
    if (!started) throw std::runtime_error("Could not initialize Discord login networking.");
#endif
    auto listener = std::make_shared<SocketOwner>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (listener->fd == invalid_socket) throw std::runtime_error("Could not open the Discord login callback.");
#ifndef _WIN32
    const int reuse = 1;
    setsockopt(listener->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#else
    const int exclusive = 1;
    setsockopt(listener->fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char *>(&exclusive), sizeof(exclusive));
#endif
    sockaddr_in address{};
    address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(impl_->options.callback_port);
    if (bind(listener->fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) || listen(listener->fd, 4))
      throw std::runtime_error("Discord login callback port is busy. Close the other login and try again.");
    const auto state = random_value(), verifier = random_value();
    const auto redirect = "http://127.0.0.1:" + std::to_string(impl_->options.callback_port) + "/discord/callback";
    const auto url = "https://discord.com/oauth2/authorize?response_type=code&client_id=" + encode(impl_->options.client_id) +
        "&scope=" + encode(impl_->options.social_communication ? "identify openid sdk.social_layer" :
                          impl_->options.social_presence ? "identify openid sdk.social_layer_presence" : "identify") +
        "&redirect_uri=" + encode(redirect) + "&state=" + encode(state) +
        "&code_challenge_method=S256&code_challenge=" + encode(challenge(verifier));
    if (!impl_->browser || !impl_->browser(url)) throw std::runtime_error("Could not open your browser for Discord sign-in. Please try again.");
    impl_->publish({DiscordLoginStatus::Waiting, {}, "Complete Discord sign-in in your browser. Press Back to cancel."});
    impl_->worker = std::jthread([p = impl_.get(), listener, state, verifier, redirect](std::stop_token stop) { p->run(stop, listener, state, verifier, redirect); });
  } catch (const std::exception &e) { impl_->publish({DiscordLoginStatus::Failed, {}, e.what()}); }
}
DiscordLoginSnapshot DiscordLogin::snapshot() const {
  std::lock_guard lock(impl_->mutex);
  if (impl_->state.status == DiscordLoginStatus::SignedIn && Clock::now() >= impl_->expires)
    return {DiscordLoginStatus::SignedOut, {}, "Your Discord sign-in expired. Please sign in again."};
  return impl_->state;
}
std::optional<std::string> DiscordLogin::authorization() const {
  std::lock_guard lock(impl_->mutex);
  if (impl_->state.status != DiscordLoginStatus::SignedIn || Clock::now() >= impl_->expires)
    return std::nullopt;
  return impl_->authorization;
}
std::optional<std::string> DiscordLogin::social_authorization() const {
  return impl_->options.social_presence || impl_->options.social_communication?authorization():std::nullopt;
}
std::optional<DiscordLogin::ServiceIdentity> DiscordLogin::service_identity() const {
  std::lock_guard lock(impl_->mutex);
  if(impl_->state.status!=DiscordLoginStatus::SignedIn || !impl_->state.identity || Clock::now()>=impl_->expires)
    return {};
  return ServiceIdentity{impl_->authorization,impl_->state.identity->id};
}
} // namespace halo2
