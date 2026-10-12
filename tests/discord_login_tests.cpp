#include "halo2/discord_login.hpp"
#include <curl/curl.h>
#include <openssl/evp.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace halo2;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
std::string parameter(const std::string &url, const std::string &name) {
  auto start = url.find(name + "=");
  check(start != std::string::npos, "Missing OAuth parameter");
  start += name.size() + 1;
  return url.substr(start, url.find('&', start) - start);
}
DiscordLoginSnapshot wait(DiscordLogin &login) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while (std::chrono::steady_clock::now() < deadline) {
    auto result = login.snapshot();
    if (result.status != DiscordLoginStatus::Waiting && result.status != DiscordLoginStatus::Verifying) return result;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  throw std::runtime_error("OAuth worker did not finish");
}
std::size_t discard(char *, std::size_t size, std::size_t count, void *) { return size * count; }
long callback(const std::string &query) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
  const auto url = "http://127.0.0.1:42003/discord/callback?" + query;
  curl_easy_setopt(client.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(client.get(), CURLOPT_NOPROXY, "*");
  curl_easy_setopt(client.get(), CURLOPT_TIMEOUT, 3L);
  curl_easy_setopt(client.get(), CURLOPT_WRITEFUNCTION, discard);
  check(curl_easy_perform(client.get()) == CURLE_OK, "Loopback callback failed");
  long status{};
  curl_easy_getinfo(client.get(), CURLINFO_RESPONSE_CODE, &status);
  return status;
}
std::string pkce(const std::string &value) {
  unsigned char digest[32]; unsigned count{};
  EVP_Digest(value.data(), value.size(), digest, &count, EVP_sha256(), nullptr);
  unsigned char encoded[45]{};
  EVP_EncodeBlock(encoded, digest, 32);
  std::string out(reinterpret_cast<char *>(encoded));
  while (out.back() == '=') out.pop_back();
  for (auto &c : out) { if (c == '+') c = '-'; if (c == '/') c = '_'; }
  return out;
}
}
int main() {
  try {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    DiscordLoginOptions options; options.callback_port = 42003;
    std::string url;
    std::vector<DiscordHttpRequest> requests;
    DiscordHttpResponse token{200, R"({"access_token":"test-token","token_type":"Bearer","scope":"identify","expires_in":3600})"};
    DiscordHttpResponse user{200, R"({"id":"123456789012345678","username":"discord_user","global_name":"Display Name"})"};
    DiscordLogin login([&](const std::string &value) { url = value; return true; }, options,
      [&](const DiscordHttpRequest &r, std::stop_token) {
        requests.push_back(r);
        return r.url.ends_with("/token") ? token : user;
      });
    check(!login.snapshot().identity && !login.authorization(), "Signed-out state has credentials");
    login.start();
    check(url.starts_with("https://discord.com/oauth2/authorize?"), "Authorization leaves Discord");
    check(parameter(url, "client_id") == "1558522073272426519" && parameter(url, "scope") == "identify", "Wrong application or excessive scopes");
    check(parameter(url, "redirect_uri") == "http%3A%2F%2F127.0.0.1%3A42003%2Fdiscord%2Fcallback", "Wrong loopback redirect");
    check(parameter(url, "code_challenge_method") == "S256", "PKCE is not S256");
    const auto first_state = parameter(url, "state");
    check(callback("code=evil&state=wrong") == 400 && !login.snapshot().identity && requests.empty(), "State mismatch admitted an identity");
    check(callback("code=evil&state=" + first_state + "&state=" + first_state) == 400, "Duplicate state accepted");
    check(callback("code=valid-code&state=" + first_state) == 200, "Valid callback rejected");
    const auto signed_in = wait(login);
    check(signed_in.identity && signed_in.identity->id == "123456789012345678" && signed_in.identity->username == "discord_user" && signed_in.identity->display_name == "Display Name", "Verified Discord identity lost");
    check(requests.size() == 2 && requests[0].url == "https://discord.com/api/v10/oauth2/token" && requests[1].url == "https://discord.com/api/v10/users/@me", "Identity did not come from fixed Discord endpoints");
    check(requests[0].form.find("client_secret") == std::string::npos && requests[1].bearer == "test-token", "Secret embedded or bearer lost");
    check(parameter(url, "code_challenge") == pkce(parameter(requests[0].form, "code_verifier")), "PKCE verifier does not match challenge");
    check(login.authorization() == "test-token", "Private queue handoff lost the existing Discord authorization");
    login.cancel();
    check(!login.snapshot().identity && !login.authorization(), "Sign-out retained identity or authorization");
    requests.clear(); login.start();
    check(parameter(url, "state") != first_state, "OAuth state reused");
    check(callback("code=old&state=" + first_state) == 400, "Previous attempt state accepted");
    check(callback("error=access_denied&state=" + parameter(url, "state")) == 200, "Denial callback rejected");
    check(wait(login).status == DiscordLoginStatus::Failed && requests.empty(), "Denied login called Discord or signed in");
    for (const auto &bad : {
        DiscordHttpResponse{401, "{}"},
        DiscordHttpResponse{200, "invalid json"},
        DiscordHttpResponse{200, R"({"access_token":"t","token_type":"Bearer","scope":"email","expires_in":3600})"},
        DiscordHttpResponse{200, R"({"access_token":"t","token_type":"Bearer","scope":"identify","expires_in":0})"}}) {
      token = bad; requests.clear(); login.start();
      callback("code=test&state=" + parameter(url, "state"));
      check(wait(login).status == DiscordLoginStatus::Failed && !login.snapshot().identity && requests.size() == 1, "Bad token response admitted identity");
    }
    token = {200, R"({"access_token":"test-token","token_type":"Bearer","scope":"identify","expires_in":3600})"};
    for (const auto &bad : {
        DiscordHttpResponse{401, "{}"},
        DiscordHttpResponse{200, R"({"id":"forged","username":"user"})"},
        DiscordHttpResponse{200, R"({"id":"123456789012345678","username":"user","bot":true})"},
        DiscordHttpResponse{200, R"({"id":"123456789012345678","username":"user\nforged"})"}}) {
      user = bad; login.start(); callback("code=test&state=" + parameter(url, "state"));
      check(wait(login).status == DiscordLoginStatus::Failed && !login.snapshot().identity, "Bad user response admitted identity");
    }
    login.start();
    const auto before = std::chrono::steady_clock::now(); login.cancel();
    check(std::chrono::steady_clock::now() - before < std::chrono::seconds(1), "Cancel blocked the UI");
    DiscordLogin browser_failure([](const auto &) { return false; }, options);
    browser_failure.start();
    check(browser_failure.snapshot().status == DiscordLoginStatus::Failed, "Browser failure silently waits");
    options.timeout = std::chrono::milliseconds(20);
    DiscordLogin timeout([](const auto &) { return true; }, options);
    timeout.start(); check(wait(timeout).status == DiscordLoginStatus::Failed, "Login timeout did not fail closed");
    timeout.cancel();
    token = {200, R"({"access_token":"t","token_type":"Bearer","scope":"identify","expires_in":1})"};
    user = {200, R"({"id":"123456789012345678","username":"user","global_name":null})"};
    login.start(); callback("code=test&state=" + parameter(url, "state"));
    check(wait(login).identity->display_name == "user", "Null global name has no username fallback");
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    check(!login.snapshot().identity && login.snapshot().status == DiscordLoginStatus::SignedOut, "Expired login still authenticates");
    login.cancel();
    token={200,R"({"access_token":"identity-token","token_type":"Bearer","scope":"identify","expires_in":3600})"};
    login.start();callback("code=identity&state="+parameter(url,"state"));check(wait(login).identity.has_value(),"Identity fixture failed");
    options.timeout=std::chrono::seconds(4);options.social_presence=true;
    DiscordLogin social([&](const auto &value){url=value;return true;},options,
      [&](const auto &r,std::stop_token){return r.url.ends_with("/token")?token:user;});
    social.start();
    check(parameter(url,"scope")=="identify%20openid%20sdk.social_layer_presence","Friends consent omitted or exceeded its scopes");
    callback("code=denied-scopes&state="+parameter(url,"state"));
    check(wait(social).status==DiscordLoginStatus::Failed && !social.authorization() && login.authorization()=="identity-token", "Identity-only token enabled friends or changed ONLINE login");
    social.cancel();
    token={200,R"({"access_token":"social-token","token_type":"Bearer","scope":"sdk.social_layer_presence identify openid","expires_in":3600})"};
    social.start();callback("code=with-scopes&state="+parameter(url,"state"));
    check(wait(social).identity.has_value() && social.authorization()=="social-token" && login.authorization()=="identity-token","Consented friends token lost or replaced ONLINE identity");
    social.cancel();check(login.authorization()=="identity-token","Disconnecting friends signed out ONLINE");
    options.social_communication=true;
    DiscordLogin invitations([&](const auto &value){url=value;return true;},options,
      [&](const auto &r,std::stop_token){return r.url.ends_with("/token")?token:user;});
    invitations.start();
    check(parameter(url,"scope")=="identify%20openid%20sdk.social_layer","Game invitations omitted communication scopes");
    callback("code=presence-only&state="+parameter(url,"state"));
    check(wait(invitations).status==DiscordLoginStatus::Failed && !invitations.social_authorization() &&
      login.authorization()=="identity-token","Presence-only grant enabled invitations or changed ONLINE identity");
    invitations.cancel();
    token={200,R"({"access_token":"invitations-token","token_type":"Bearer","scope":"sdk.social_layer identify openid","expires_in":3600})"};
    invitations.start();callback("code=communication&state="+parameter(url,"state"));
    check(wait(invitations).identity.has_value() && invitations.social_authorization()=="invitations-token" &&
      login.authorization()=="identity-token","Communication consent lost or replaced ONLINE identity");
    invitations.cancel();check(login.authorization()=="identity-token","Disconnecting invitations signed out ONLINE");
    std::cout << "PASS Discord PKCE, loopback callback, verified identity, denial, replay/state rejection, failures, cancellation and expiry\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
