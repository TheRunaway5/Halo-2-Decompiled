#include "halo2/discord_voice_blocks.hpp"
#include "halo2/online_service.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace halo2 {
namespace {
using Json=nlohmann::json;
void require(bool ok){if(!ok)throw std::runtime_error("Voice preference sync failed");}
std::string origin(const std::string &value,bool local) {
  require(!value.empty() && value.size()<=2048 && value.find_first_of("@?#\\") == std::string::npos &&
      std::none_of(value.begin(),value.end(),[](unsigned char c){return c<=32 || c==127;}));
  static const auto initialized=curl_global_init(CURL_GLOBAL_DEFAULT);require(initialized==CURLE_OK);
  std::unique_ptr<CURLU,decltype(&curl_url_cleanup)> url(curl_url(),curl_url_cleanup);
  require(url && curl_url_set(url.get(),CURLUPART_URL,value.c_str(),0)==CURLUE_OK);
  auto part=[&](CURLUPart key){char *raw=nullptr;const auto result=curl_url_get(url.get(),key,&raw,0);
    std::unique_ptr<char,decltype(&curl_free)> owner(raw,curl_free);return result==CURLUE_OK && raw?std::string(raw):std::string{};};
  const auto scheme=part(CURLUPART_SCHEME),host=part(CURLUPART_HOST);
  require(!host.empty() && part(CURLUPART_USER).empty() && part(CURLUPART_PASSWORD).empty() &&
      part(CURLUPART_OPTIONS).empty() && part(CURLUPART_PATH)=="/" &&
      (scheme=="https" || (local && scheme=="http" && (host=="127.0.0.1" || host=="[::1]"))));
  auto result=value;if(result.ends_with('/'))result.pop_back();return result;
}
struct Transfer {std::string body;std::stop_token stop;};
std::size_t receive(char *bytes,std::size_t size,std::size_t count,void *context) {
  auto &transfer=*static_cast<Transfer *>(context);
  if(transfer.stop.stop_requested() || size>1024 || count>1024 || size*count>1024-transfer.body.size())return 0;
  try {transfer.body.append(bytes,size*count);}catch(...){return 0;}return size*count;
}
int progress(void *context,curl_off_t,curl_off_t,curl_off_t,curl_off_t) {
  return static_cast<Transfer *>(context)->stop.stop_requested()?1:0;
}
DiscordVoiceBlockResponse transport(const DiscordVoiceBlockRequest &request,std::stop_token stop) {
  std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> client(curl_easy_init(),curl_easy_cleanup);require(bool(client));
  auto *raw=curl_slist_append(nullptr,("Authorization: Bearer "+request.bearer).c_str());
  raw=curl_slist_append(raw,"Content-Type: application/json");
  std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> headers(raw,curl_slist_free_all);
  Transfer transfer{{},stop};
  curl_easy_setopt(client.get(),CURLOPT_URL,request.url.c_str());curl_easy_setopt(client.get(),CURLOPT_HTTPHEADER,headers.get());
  curl_easy_setopt(client.get(),CURLOPT_CUSTOMREQUEST,"PUT");curl_easy_setopt(client.get(),CURLOPT_POSTFIELDS,request.body.c_str());
  curl_easy_setopt(client.get(),CURLOPT_POSTFIELDSIZE,long(request.body.size()));
  curl_easy_setopt(client.get(),CURLOPT_PROTOCOLS_STR,"https,http");curl_easy_setopt(client.get(),CURLOPT_FOLLOWLOCATION,0L);
  curl_easy_setopt(client.get(),CURLOPT_PROXY,"");curl_easy_setopt(client.get(),CURLOPT_NOSIGNAL,1L);
  curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYHOST,2L);
  curl_easy_setopt(client.get(),CURLOPT_CONNECTTIMEOUT_MS,2000L);curl_easy_setopt(client.get(),CURLOPT_TIMEOUT_MS,3000L);
  curl_easy_setopt(client.get(),CURLOPT_ACCEPT_ENCODING,"identity");curl_easy_setopt(client.get(),CURLOPT_HTTP_CONTENT_DECODING,0L);
  curl_easy_setopt(client.get(),CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(client.get(),CURLOPT_WRITEDATA,&transfer);
  curl_easy_setopt(client.get(),CURLOPT_NOPROGRESS,0L);curl_easy_setopt(client.get(),CURLOPT_XFERINFOFUNCTION,progress);
  curl_easy_setopt(client.get(),CURLOPT_XFERINFODATA,&transfer);
  require(curl_easy_perform(client.get())==CURLE_OK && !stop.stop_requested());
  long status{};curl_easy_getinfo(client.get(),CURLINFO_RESPONSE_CODE,&status);return {status,std::move(transfer.body)};
}
}
DiscordVoiceBlockOptions discord_voice_block_environment() {
  DiscordVoiceBlockOptions options;
  if(const auto *url=std::getenv("H2_ACCOUNT_URL"))options.endpoint=url;else options.endpoint=default_online_origin;
  if(const auto *local=std::getenv("H2_ACCOUNT_ALLOW_LOOPBACK_HTTP"))options.allow_loopback_http=std::string(local)=="1";
  return options;
}
struct DiscordVoiceBlocks::Impl {
  std::shared_ptr<DiscordLogin> login;
  DiscordVoiceBlockOptions options;
  Http http;
  std::mutex mutex;
  std::condition_variable_any wake;
  std::vector<std::string> desired;
  std::string subject;
  bool configured{},pending{},known{};
  std::uint64_t generation{};
  std::stop_source cancellation;
  std::jthread worker;
  Impl(std::shared_ptr<DiscordLogin> identity,DiscordVoiceBlockOptions o,Http h)
      :login(std::move(identity)),options(std::move(o)),http(h?std::move(h):transport) {
    try {options.endpoint=origin(options.endpoint,options.allow_loopback_http);
      require(login && options.retry_interval>=std::chrono::milliseconds(10));configured=true;}catch(...){return;}
    worker=std::jthread([this](std::stop_token stop){run(stop);});
  }
  ~Impl(){worker.request_stop();{std::lock_guard lock(mutex);cancellation.request_stop();}wake.notify_all();if(worker.joinable())worker.join();}
  void run(std::stop_token stop) {
    while(!stop.stop_requested()) {
      std::unique_lock lock(mutex);wake.wait(lock,stop,[&]{return pending;});if(stop.stop_requested())break;
      const auto version=generation;const auto owner=subject;const auto subjects=desired;pending=false;
      cancellation=std::stop_source{};const auto cancelled=cancellation.get_token();lock.unlock();
      bool success=false;
      const auto identity=login->snapshot().identity;const auto token=login->authorization();
      if(identity && identity->id==owner && token)try {
        const auto result=http({options.endpoint+"/v1/account/voice-blocks",*token,Json{{"blocked",subjects}}.dump()},cancelled);
        success=!cancelled.stop_requested() && result.status==200 && result.body.size()<=1024 &&
            Json::parse(result.body)==Json{{"state","updated"}};
      }catch(...){}
      lock.lock();
      if(version!=generation || cancelled.stop_requested())continue;
      if(!success) {
        // Changes wake this backoff and replace the old full snapshot. A
        // successful response is never required to discover a newer update.
        wake.wait_for(lock,stop,options.retry_interval,[&]{return version!=generation;});
        if(version==generation)pending=true;
      }
    }
  }
};
DiscordVoiceBlocks::DiscordVoiceBlocks(std::shared_ptr<DiscordLogin> login,DiscordVoiceBlockOptions options,Http http)
    :impl_(std::make_unique<Impl>(std::move(login),std::move(options),std::move(http))){}
DiscordVoiceBlocks::~DiscordVoiceBlocks()=default;
bool DiscordVoiceBlocks::update(std::vector<std::string> subjects) {
  if(subjects.size()>2000 || std::any_of(subjects.begin(),subjects.end(),[](const auto &s){
      return s.size()<17 || s.size()>20 || s.find_first_not_of("0123456789")!=std::string::npos;}))return false;
  const auto identity=impl_->login?impl_->login->snapshot().identity:std::nullopt;
  if(!identity)return false;
  std::sort(subjects.begin(),subjects.end());subjects.erase(std::unique(subjects.begin(),subjects.end()),subjects.end());
  std::lock_guard lock(impl_->mutex);if(!impl_->configured)return false;
  if(impl_->known && identity->id==impl_->subject && subjects==impl_->desired)return true;
  impl_->desired=std::move(subjects);impl_->subject=identity->id;impl_->known=true;impl_->pending=true;++impl_->generation;impl_->wake.notify_all();return true;
}
void DiscordVoiceBlocks::reset() {
  std::lock_guard lock(impl_->mutex);impl_->desired.clear();impl_->subject.clear();impl_->known=impl_->pending=false;
  ++impl_->generation;impl_->cancellation.request_stop();impl_->wake.notify_all();
}
} // namespace halo2
