#include "halo2/steam_login.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;
void require(bool ok) { if(!ok)throw std::runtime_error("Steam returned an invalid sign-in response."); }
bool proof(const std::string &v) {
  return v.size()==43 && std::all_of(v.begin(),v.end(),[](unsigned char c) {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-';
  });
}
std::string b64(const unsigned char *p,unsigned n) {
  std::string s(4*((n+2)/3),'\0');EVP_EncodeBlock(reinterpret_cast<unsigned char*>(s.data()),p,int(n));
  while(!s.empty()&&s.back()=='=')s.pop_back();
  std::replace(s.begin(),s.end(),'+','-');std::replace(s.begin(),s.end(),'/','_');return s;
}
std::string random_proof() {
  std::array<unsigned char,32> p{};
  if(RAND_bytes(p.data(),int(p.size()))!=1)throw std::runtime_error("Could not prepare Steam sign-in security.");
  return b64(p.data(),unsigned(p.size()));
}
std::string digest(const std::string &v) {
  std::array<unsigned char,EVP_MAX_MD_SIZE> p{};unsigned n{};
  if(EVP_Digest(v.data(),v.size(),p.data(),&n,EVP_sha256(),nullptr)!=1)
    throw std::runtime_error("Could not prepare Steam sign-in security.");
  return b64(p.data(),n);
}
std::string encode(const std::string &s) {
  constexpr char hex[]="0123456789ABCDEF";std::string out;
  for(unsigned char c:s) {
    if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')out+=char(c);
    else {out+='%';out+=hex[c>>4];out+=hex[c&15];}
  }return out;
}
std::string origin(std::string value,bool local) {
  while(value.ends_with('/'))value.pop_back();
  std::unique_ptr<CURLU,decltype(&curl_url_cleanup)> url(curl_url(),curl_url_cleanup);
  require(url && curl_url_set(url.get(),CURLUPART_URL,value.c_str(),0)==CURLUE_OK);
  auto part=[&](CURLUPart p) {char *s=nullptr;const auto status=curl_url_get(url.get(),p,&s,0);
    std::unique_ptr<char,decltype(&curl_free)> owner(s,curl_free);return status==CURLUE_OK?std::string(s):std::string{};};
  const auto scheme=part(CURLUPART_SCHEME),host=part(CURLUPART_HOST);
  require(!host.empty()&&(scheme=="https"||(local&&scheme=="http"&&(host=="127.0.0.1"||host=="[::1]"))));
  require(part(CURLUPART_USER).empty()&&part(CURLUPART_PASSWORD).empty()&&part(CURLUPART_QUERY).empty()&&
          part(CURLUPART_FRAGMENT).empty()&&(part(CURLUPART_PATH).empty()||part(CURLUPART_PATH)=="/"));
  return value;
}
std::size_t receive(char *p,std::size_t size,std::size_t count,void *context) {
  auto &s=*static_cast<std::string*>(context);const auto n=size*count;
  if(n>65536||s.size()>65536-n)return 0;
  s.append(p,n);return n;
}
int progress(void *p,curl_off_t,curl_off_t,curl_off_t,curl_off_t) {return static_cast<std::stop_token*>(p)->stop_requested()?1:0;}
DiscordHttpResponse transport(const DiscordHttpRequest &r,std::stop_token stop) {
  static const bool ready=curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK;require(ready);
  std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> c(curl_easy_init(),curl_easy_cleanup);require(bool(c));
  DiscordHttpResponse out;
  curl_easy_setopt(c.get(),CURLOPT_URL,r.url.c_str());
  curl_easy_setopt(c.get(),CURLOPT_PROTOCOLS_STR,r.url.starts_with("http://")?"http":"https");
  curl_easy_setopt(c.get(),CURLOPT_FOLLOWLOCATION,0L);
  curl_easy_setopt(c.get(),CURLOPT_PROXY,"");
  curl_easy_setopt(c.get(),CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(c.get(),CURLOPT_SSL_VERIFYHOST,2L);
  curl_easy_setopt(c.get(),CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c.get(),CURLOPT_CONNECTTIMEOUT,8L);
  curl_easy_setopt(c.get(),CURLOPT_TIMEOUT,15L);curl_easy_setopt(c.get(),CURLOPT_USERAGENT,"Halo2Native/0.1 SteamIdentity");
  curl_easy_setopt(c.get(),CURLOPT_POSTFIELDS,r.form.c_str());
  curl_easy_setopt(c.get(),CURLOPT_POSTFIELDSIZE,long(r.form.size()));
  auto *raw=curl_slist_append(nullptr,"Content-Type: application/json");
  if(!r.bearer.empty())raw=curl_slist_append(raw,("Authorization: Bearer "+r.bearer).c_str());
  std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> headers(raw,curl_slist_free_all);
  curl_easy_setopt(c.get(),CURLOPT_HTTPHEADER,headers.get());
  curl_easy_setopt(c.get(),CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(c.get(),CURLOPT_WRITEDATA,&out.body);
  curl_easy_setopt(c.get(),CURLOPT_NOPROGRESS,0L);curl_easy_setopt(c.get(),CURLOPT_XFERINFOFUNCTION,progress);
  curl_easy_setopt(c.get(),CURLOPT_XFERINFODATA,&stop);
  if(curl_easy_perform(c.get())!=CURLE_OK)throw std::runtime_error("Could not reach Steam sign-in securely. Please try again.");
  curl_easy_getinfo(c.get(),CURLINFO_RESPONSE_CODE,&out.status);return out;
}
std::string login_url(const std::string &base,const std::string &device) {
  const std::string ns="http://specs.openid.net/auth/2.0",select=ns+"/identifier_select";
  return "https://steamcommunity.com/openid/login?openid.ns="+encode(ns)+"&openid.mode=checkid_setup&openid.realm="+
    encode(base+"/")+"&openid.return_to="+encode(base+"/v1/auth/steam/callback/"+device)+
    "&openid.identity="+encode(select)+"&openid.claimed_id="+encode(select);
}
bool name(const std::string &v) {
  return !v.empty()&&v.size()<=128&&std::none_of(v.begin(),v.end(),[](unsigned char c){return c<32||c==127;});
}
} // namespace
SteamLoginOptions steam_login_environment() {
  SteamLoginOptions o;
  if(const auto *v=std::getenv("H2_ACCOUNT_URL"))o.origin=v;
  if(const auto *v=std::getenv("H2_ACCOUNT_ALLOW_LOOPBACK_HTTP"))o.allow_loopback_http=std::string(v)=="1";
  return o;
}
struct SteamLogin::Impl {
  DiscordLogin::OpenBrowser browser;SteamLoginOptions options;DiscordLogin::Http http;
  mutable std::mutex mutex;DiscordLoginSnapshot state;std::string token;Clock::time_point expires{};
  DiscordLinkSnapshot link;
  std::string profile_namespace,profile_name;
  std::jthread worker;
  void publish(DiscordLoginSnapshot s,std::string t={},Clock::time_point e={}) {
    std::lock_guard lock(mutex);state=std::move(s);token=std::move(t);expires=e;
  }
  bool cached() const { return !profile_namespace.empty() && !profile_name.empty(); }
  void admit(const Json &result,std::string bearer) {
    const auto seconds=result.at("expires_in").get<int>();const auto &user=result.at("identity");
    DiscordIdentity identity{user.at("id").get<std::string>(),user.at("username").get<std::string>(),user.at("display_name").get<std::string>()};
    require(bearer.starts_with("h2s_")&&proof(bearer.substr(4))&&seconds>0&&seconds<=28800&&
      identity.id.size()==17&&identity.id.front()!='0'&&std::all_of(identity.id.begin(),identity.id.end(),[](char c){return c>='0'&&c<='9';})&&
      name(identity.username)&&name(identity.display_name));
    publish({DiscordLoginStatus::SignedIn,identity,"Signed in as "+identity.display_name},std::move(bearer),Clock::now()+std::chrono::seconds(seconds));
  }
  void run(std::stop_token stop,bool browser_allowed) {
    try {
      const auto base=origin(options.origin,options.allow_loopback_http);
      require(options.timeout.count()>0);
      if(cached()) {
        std::string saved;
        try {saved=options.load(base,profile_namespace,profile_name);}catch(...){}
        if(!saved.empty()) {
          require(saved.starts_with("h2r_")&&proof(saved.substr(4)));
          const auto resumed=http({base+"/v1/auth/steam/resume","{}",saved},stop);
          if(stop.stop_requested())return;
          if(resumed.status==200) {
            const auto result=Json::parse(resumed.body);
            admit(result,result.at("token").get<std::string>());return;
          }
          if(resumed.status!=401)throw std::runtime_error("Could not restore your saved Steam sign-in. Please try again.");
          try {options.erase(base,profile_namespace,profile_name);}catch(...){}
        }
      }
      if(!browser_allowed){publish({});return;}
      const auto verifier=random_proof();
      std::array<unsigned char,6> confirmation_bytes{};
      require(RAND_bytes(confirmation_bytes.data(),int(confirmation_bytes.size()))==1);
      std::string confirmation;constexpr char hex[]="0123456789ABCDEF";
      for(auto byte:confirmation_bytes){confirmation+=hex[byte>>4];confirmation+=hex[byte&15];}
      const auto started=http({base+"/v1/auth/steam/start",Json{{"challenge",digest(verifier)},
          {"confirmation",digest(confirmation)}}.dump(),{}},stop);
      if(stop.stop_requested())return;
      if(started.status!=201)throw std::runtime_error("Steam sign-in is unavailable. Please try again.");
      const auto body=Json::parse(started.body);const auto device=body.at("device").get<std::string>();
      const auto lifetime=body.at("expires_in").get<int>(),interval=body.at("interval").get<int>();
      require(proof(device)&&lifetime>0&&lifetime<=300&&interval>=1&&interval<=5);
      const auto url=login_url(base,device);require(body.at("url").get<std::string>()==url);
      const auto deadline=Clock::now()+std::min(options.timeout,std::chrono::milliseconds(lifetime*1000));
      if(!browser||!browser(url))throw std::runtime_error("Could not open your browser for Steam sign-in.");
      const auto confirm_message="Enter this code in your browser after Steam sign-in: "+confirmation+". Press Back to cancel.";
      publish({DiscordLoginStatus::Waiting,{},confirm_message});
      std::mutex wait_mutex;std::condition_variable_any wake;std::unique_lock wait_lock(wait_mutex);
      while(!stop.stop_requested()&&Clock::now()<deadline) {
        wake.wait_for(wait_lock,stop,std::chrono::seconds(interval),[]{return false;});
        if(stop.stop_requested())return;
        const auto response=http({base+"/v1/auth/steam/poll",Json{{"device",device},{"verifier",verifier}}.dump(),{}},stop);
        if(stop.stop_requested())return;
        if(response.status!=200)throw std::runtime_error("Steam sign-in expired or was rejected. Please try again.");
        const auto result=Json::parse(response.body);const auto status=result.at("state").get<std::string>();
        if(status=="pending"||status=="verifying"||status=="awaiting_confirmation") {
          if(status=="awaiting_confirmation")publish({DiscordLoginStatus::Waiting,{},confirm_message});
          if(status=="verifying")publish({DiscordLoginStatus::Verifying,{},"Verifying your Steam identity..."});
          continue;
        }
        if(status=="declined")throw std::runtime_error("Steam sign-in was declined. Try again when ready.");
        if(status=="failed")throw std::runtime_error("Steam could not complete sign-in. Please try again.");
        require(status=="signed_in");
        // Validate before persisting anything. Older services may omit refresh.
        admit(result,result.at("token").get<std::string>());
        if(cached() && result.contains("refresh_token")) {
          const auto refresh=result.at("refresh_token").get<std::string>();
          require(refresh.starts_with("h2r_")&&proof(refresh.substr(4)));
          try {options.save(base,profile_namespace,profile_name,refresh);}
          catch(...) {std::lock_guard lock(mutex);state.message+=". Secure storage is unavailable; sign-in was not remembered.";}
        }
        return;
      }
      if(!stop.stop_requested())throw std::runtime_error("Steam sign-in timed out. Please try again.");
    }catch(const Json::exception &) {
      if(!stop.stop_requested())publish({DiscordLoginStatus::Failed,{},"Steam returned an invalid sign-in response. Please try again."});
    }catch(const std::exception &e) {if(!stop.stop_requested())publish({DiscordLoginStatus::Failed,{},e.what()});}
  }
};
SteamLogin::SteamLogin(DiscordLogin::OpenBrowser browser,SteamLoginOptions options,DiscordLogin::Http http):impl_(std::make_unique<Impl>()) {
  impl_->browser=std::move(browser);impl_->options=std::move(options);impl_->http=http?std::move(http):transport;
  auto &o=impl_->options;
  if(!o.load)o.load=[](const auto &base,const auto &scope,const auto &profile){return std::string(online_protocol_bridge::steam_saved_login(base,scope,profile));};
  if(!o.save)o.save=[](const auto &base,const auto &scope,const auto &profile,const auto &token){online_protocol_bridge::steam_save_login(base,scope,profile,token);};
  if(!o.erase)o.erase=[](const auto &base,const auto &scope,const auto &profile){online_protocol_bridge::steam_forget_login(base,scope,profile);};
}
SteamLogin::~SteamLogin(){cancel();}
void SteamLogin::start(){cancel();impl_->publish({DiscordLoginStatus::Verifying,{},"Preparing Steam sign-in..."});
  impl_->worker=std::jthread([p=impl_.get()](std::stop_token stop){p->run(stop,true);});}
void SteamLogin::bind_profile(std::string scope,std::string profile) {
  if(impl_->profile_namespace==scope && impl_->profile_name==profile)return;
  cancel();impl_->profile_namespace=std::move(scope);impl_->profile_name=std::move(profile);
}
void SteamLogin::restore(){
  const auto status=snapshot().status;
  if(!impl_->cached() || status==DiscordLoginStatus::SignedIn || status==DiscordLoginStatus::Waiting || status==DiscordLoginStatus::Verifying)return;
  cancel();impl_->publish({DiscordLoginStatus::Verifying,{},"Restoring Steam sign-in..."});
  impl_->worker=std::jthread([p=impl_.get()](std::stop_token stop){p->run(stop,false);});
}
void SteamLogin::sign_out(){
  cancel();
  impl_->publish({DiscordLoginStatus::Verifying,{},"Removing saved Steam sign-in..."});
  impl_->worker=std::jthread([p=impl_.get()](std::stop_token stop){
    try {
      if(!p->cached()){p->publish({});return;}
      const auto base=origin(p->options.origin,p->options.allow_loopback_http);
      const auto saved=p->options.load(base,p->profile_namespace,p->profile_name);
      if(!saved.empty()) {
        const auto response=p->http({base+"/v1/auth/steam/forget","{}",saved},stop);
        if(stop.stop_requested())return;
        require(response.status>=0 && response.status<=65535 &&
          online_protocol_bridge::steam_revocation_accepted(std::uint16_t(response.status),response.body));
      }
      p->options.erase(base,p->profile_namespace,p->profile_name);
      p->publish({});
    }catch(...) {if(!stop.stop_requested())p->publish({DiscordLoginStatus::Failed,{},"Could not remove saved sign-in. Please try Sign Out again."});}
  });
}
void SteamLogin::cancel(){if(impl_->worker.joinable()){impl_->worker.request_stop();impl_->worker.join();}impl_->publish({});
  std::lock_guard lock(impl_->mutex);impl_->link={};}
DiscordLoginSnapshot SteamLogin::snapshot()const {
  std::lock_guard lock(impl_->mutex);
  if(impl_->state.status==DiscordLoginStatus::SignedIn&&Clock::now()>=impl_->expires)
    return {DiscordLoginStatus::SignedOut,{},"Your Steam sign-in expired. Please sign in again."};
  return impl_->state;
}
std::optional<SteamLogin::Credential> SteamLogin::credential()const {
  std::lock_guard lock(impl_->mutex);
  if(impl_->state.status!=DiscordLoginStatus::SignedIn||!impl_->state.identity||Clock::now()>=impl_->expires)return {};
  return Credential{impl_->token,"steam:"+impl_->state.identity->id};
}
std::optional<std::string> SteamLogin::authorization()const {const auto c=credential();return c?std::optional(c->bearer):std::nullopt;}
bool SteamLogin::link_discord(std::string social) {
  const auto c=credential();
  if(!c||social.empty()||social.size()>2048||social.find_first_of("\r\n")!=std::string::npos)return false;
  {std::lock_guard lock(impl_->mutex);if(impl_->link.pending)return false;}
  if(impl_->worker.joinable())impl_->worker.join(); // Completed sign-in/link only.
  {std::lock_guard lock(impl_->mutex);impl_->link={true,false,"Linking Discord to your Steam account..."};}
  impl_->worker=std::jthread([p=impl_.get(),bearer=c->bearer,social=std::move(social)](std::stop_token stop){
    DiscordLinkSnapshot result{false,false,"Could not link Discord. Return to ONLINE and try again."};
    try {
      const auto base=origin(p->options.origin,p->options.allow_loopback_http);
      const auto r=p->http({base+"/v1/account/link-discord",Json{{"discord_token",social}}.dump(),bearer},stop);
      if(r.status==200&&Json::parse(r.body)==Json{{"state","linked"}})result={false,true,"Discord linked."};
      else if(r.status==409)result.message="Account link needs attention. Leave matchmaking first; accounts with separate ONLINE history cannot be combined automatically.";
    }catch(...){}
    std::lock_guard lock(p->mutex);
    if(!stop.stop_requested()&&p->token==bearer&&Clock::now()<p->expires)p->link=std::move(result);
    else p->link={};
  });return true;
}
DiscordLinkSnapshot SteamLogin::discord_link()const {std::lock_guard lock(impl_->mutex);return impl_->link;}
void SteamLogin::cancel_discord_link(){
  {std::lock_guard lock(impl_->mutex);if(!impl_->link.pending){impl_->link={};return;}}
  impl_->worker.request_stop();if(impl_->worker.joinable())impl_->worker.join();
  std::lock_guard lock(impl_->mutex);impl_->link={};
}
} // namespace halo2
