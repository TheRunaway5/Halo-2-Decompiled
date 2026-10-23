#include "halo2/steam_login.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <thread>
using namespace halo2;
using Json=nlohmann::json;
void check(bool v,const char *m){if(!v)throw std::runtime_error(m);}
std::string encode(const std::string &s){auto *p=curl_easy_escape(nullptr,s.c_str(),int(s.size()));std::string v(p);curl_free(p);return v;}
std::string url(const std::string &d){const std::string ns="http://specs.openid.net/auth/2.0";
  return "https://steamcommunity.com/openid/login?openid.ns="+encode(ns)+"&openid.mode=checkid_setup&openid.realm="+
    encode(std::string(default_online_origin)+"/")+"&openid.return_to="+encode(std::string(default_online_origin)+"/v1/auth/steam/callback/"+d)+
    "&openid.identity="+encode(ns+"/identifier_select")+"&openid.claimed_id="+encode(ns+"/identifier_select");}
DiscordLoginSnapshot wait(auto &login){for(unsigned i=0;i<800;++i){auto s=login.snapshot();
  if(s.status!=DiscordLoginStatus::Waiting&&s.status!=DiscordLoginStatus::Verifying)return s;
  std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error("Steam worker timeout");}
std::string hash(const std::string &s){unsigned char p[32],out[45]{};unsigned n{};
  EVP_Digest(s.data(),s.size(),p,&n,EVP_sha256(),nullptr);EVP_EncodeBlock(out,p,32);
  std::string v(reinterpret_cast<char*>(out));while(v.back()=='=')v.pop_back();
  std::replace(v.begin(),v.end(),'+','-');std::replace(v.begin(),v.end(),'/','_');return v;}
int main(){try{
  curl_global_init(CURL_GLOBAL_DEFAULT);
  std::string challenge,opened;unsigned polls{};
  const std::string device(43,'d'),bearer="h2s_"+std::string(43,'t');
  Json completed={{"state","signed_in"},{"token",bearer},{"expires_in",28800},
    {"identity",{{"id","76561197960287930"},{"username","Steam Persona"},{"display_name","Steam Persona"}}}};
  auto http=[&](const DiscordHttpRequest &r,std::stop_token){
    if(r.url==std::string(default_online_origin)+"/v1/account/link-discord") {
      check(r.bearer==bearer&&Json::parse(r.form)==Json{{"discord_token","private-discord-proof"}},"Link omitted one identity proof");
      return DiscordHttpResponse{200,R"({"state":"linked"})"};
    }
    check(r.bearer.empty(),"Native identity leaked a service credential into login setup");
    auto body=Json::parse(r.form);
    if(r.url==std::string(default_online_origin)+"/v1/auth/steam/start"){
      check(body.size()==1,"Unexpected initial proof fields");challenge=body.at("challenge");
      return DiscordHttpResponse{201,Json{{"device",device},{"url",url(device)},{"expires_in",300},{"interval",1}}.dump()};
    }
    check(r.url==std::string(default_online_origin)+"/v1/auth/steam/poll","Handoff sent to a different origin");
    check(body.size()==2&&body.at("device")==device&&hash(body.at("verifier").get<std::string>())==challenge,"S256 handoff proof lost");
    ++polls;return DiscordHttpResponse{200,completed.dump()};
  };
  auto login=DiscordLogin::steam([&](const auto &value){opened=value;return true;},http);
  check(login->uses_steam()&&!login->authorization()&&!login->social_authorization(),"Signed-out Steam credential or social grant");
  login->start();auto signedin=wait(*login);
  check(signedin.identity&&signedin.identity->display_name=="Steam Persona"&&opened==url(device)&&polls==1,"Verified persona/login lost");
  check(login->authorization()==bearer&&!login->social_authorization(),"Steam credential exposed as Discord social authorization");
  check(login->link_discord("private-discord-proof"),"Dual-proof account link did not start");
  for(unsigned i=0;i<800&&login->discord_link().pending;++i)std::this_thread::sleep_for(std::chrono::milliseconds(5));
  check(login->discord_link().linked&&login->authorization()==bearer&&login->snapshot().identity.has_value(),"Link changed ONLINE identity or lost result");
  login->cancel_discord_link();check(!login->discord_link().linked&&login->authorization()==bearer,"Social disconnect signed out of Steam");
  check(signedin.message.find("76561197960287930")==std::string::npos&&signedin.message.find(bearer)==std::string::npos,"Private identity in display message");
  login->cancel();check(!login->snapshot().identity&&!login->authorization(),"Sign-out retained identity");
  const auto first=challenge;login->start();check(wait(*login).identity.has_value()&&challenge!=first,"Login reused the private proof");login->cancel();
  for(const auto &bad: {Json{{"state","declined"}},Json{{"state","failed"}},Json{{"state","signed_in"},{"token","forged"}}}){
    completed=bad;login->start();check(wait(*login).status==DiscordLoginStatus::Failed&&!login->authorization(),"Bad response admitted identity");login->cancel();}
  SteamLogin redirected([](const auto&){throw std::runtime_error("Untrusted URL reached browser");return true;},{},
    [&](const auto &,std::stop_token){return DiscordHttpResponse{201,Json{{"device",device},{"url","https://evil.invalid/"},{"expires_in",300},{"interval",1}}.dump()};});
  redirected.start();check(wait(redirected).status==DiscordLoginStatus::Failed,"Untrusted browser URL accepted");
  SteamLoginOptions bad;bad.origin="http://example.com";
  SteamLogin invalid([](const auto&){return true;},bad,[](const auto&,std::stop_token){throw std::runtime_error("Invalid origin reached HTTP");return DiscordHttpResponse{};});
  invalid.start();check(wait(invalid).status==DiscordLoginStatus::Failed,"Insecure public origin accepted");
  login->start();const auto before=std::chrono::steady_clock::now();login->cancel();
  check(std::chrono::steady_clock::now()-before<std::chrono::seconds(1),"Cancellation blocked while waiting");
  // Re-enter ONLINE and restart under two local profiles without opening a
  // browser or reusing another profile's refresh credential.
  std::map<std::string,std::string> saved;
  SteamLoginOptions cache;
  cache.load=[&](const auto &base,const auto &scope,const auto &profile){check(base==default_online_origin && scope=="test-preferences","Saved identity lost its origin/namespace");return saved[profile];};
  cache.save=[&](const auto &,const auto &,const auto &profile,const auto &value){saved[profile]=value;};
  cache.erase=[&](const auto &,const auto &,const auto &profile){saved.erase(profile);};
  const std::string refresh_a="h2r_"+std::string(43,'a'),refresh_b="h2r_"+std::string(43,'b');
  saved["Spartan"]=refresh_a;saved["Elite"]=refresh_b;
  bool revoked{},unavailable{};unsigned resumed{};
  auto resume_http=[&](const DiscordHttpRequest &r,std::stop_token){
    check(r.url==std::string(default_online_origin)+"/v1/auth/steam/resume" || r.url==std::string(default_online_origin)+"/v1/auth/steam/forget","Restore opened browser authorization");
    check(r.bearer==refresh_a || r.bearer==refresh_b,"Restore sent another profile's credential");
    if(r.url.ends_with("/forget")){revoked=true;return DiscordHttpResponse{200,R"({"state":"signed_out"})"};}
    ++resumed;if(unavailable)return DiscordHttpResponse{503,"{}"};
    auto response=Json{{"token",bearer},{"expires_in",28800},{"identity",{{"id",r.bearer==refresh_a?"76561197960287930":"76561197960287931"},{"username","Same Persona"},{"display_name","Same Persona"}}}};
    return DiscordHttpResponse{200,response.dump()};
  };
  {
    SteamLogin remembered([](const auto&){throw std::runtime_error("Restore opened a browser");return false;},cache,resume_http);
    remembered.bind_profile("test-preferences","Spartan");remembered.restore();
    check(wait(remembered).identity->id=="76561197960287930","Profile A restored a different identity");
    remembered.cancel();remembered.restore();check(wait(remembered).identity.has_value(),"Returning ONLINE lost remembered sign-in");
    remembered.bind_profile("test-preferences","Elite");check(!remembered.authorization(),"Profile switch retained previous authorization");
    remembered.restore();check(wait(remembered).identity->id=="76561197960287931","Identical personas mixed local profile identities");
  }
  SteamLogin restarted({},cache,resume_http);restarted.bind_profile("test-preferences","Spartan");restarted.restore();
  check(wait(restarted).identity.has_value(),"Restart did not restore remembered profile");
  restarted.cancel();unavailable=true;restarted.restore();check(wait(restarted).status==DiscordLoginStatus::Failed && saved["Spartan"]==refresh_a,"Transient outage erased remembered sign-in");
  unavailable=false;restarted.sign_out();restarted.cancel();check(revoked && !saved.contains("Spartan") && saved["Elite"]==refresh_b,"Explicit sign-out did not revoke just the selected profile");
  std::cout<<"Steam identity proof, fixed endpoints, privacy, rejection, cancellation and sign-out passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
