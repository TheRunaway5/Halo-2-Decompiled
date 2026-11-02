#pragma once
// CURL, URI, clock and cancellable scheduler IO only. Rust owns protocol policy.
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include <curl/curl.h>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>
namespace halo2::native_matchmaking_io {
namespace bridge=online_protocol_bridge;
inline void require(bool ok){if(!ok)throw std::runtime_error("Native match socket IO unavailable");}
inline bridge::MatchmakingQosConfig origin(const std::string &value,const std::string &bearer,bool loopback){
  bridge::MatchmakingQosConfig c;c.input_valid=true;c.origin=value;c.bearer=bearer;c.loopback=loopback;
  static const auto initialized=curl_global_init(CURL_GLOBAL_DEFAULT);
  if(initialized!=CURLE_OK)return c;
  std::unique_ptr<CURLU,decltype(&curl_url_cleanup)> url(curl_url(),curl_url_cleanup);
  if(!url||curl_url_set(url.get(),CURLUPART_URL,value.c_str(),0)!=CURLUE_OK)return c;
  const auto part=[&](CURLUPart key){char *raw=nullptr;const auto code=curl_url_get(url.get(),key,&raw,0);
    std::unique_ptr<char,decltype(&curl_free)> owner(raw,curl_free);return code==CURLUE_OK&&raw?std::string(raw):std::string{};};
  c.url_parsed=true;c.url_normalized=part(CURLUPART_URL);c.url_scheme=part(CURLUPART_SCHEME);c.url_host=part(CURLUPART_HOST);
  c.url_user=part(CURLUPART_USER);c.url_password=part(CURLUPART_PASSWORD);c.url_options=part(CURLUPART_OPTIONS);c.url_path=part(CURLUPART_PATH);return c;
}
class Socket {
  std::stop_token stop;
  mutable std::mutex mutex;
  mutable std::condition_variable_any wake;
  mutable std::unique_ptr<curl_slist,decltype(&curl_slist_free_all)> headers{nullptr,curl_slist_free_all};
  mutable std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> client{nullptr,curl_easy_cleanup};
  static int progress(void *p,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return static_cast<const std::stop_token*>(p)->stop_requested()?1:0;}
public:
  explicit Socket(std::stop_token token):stop(token){}
  std::uint64_t now_ms()const {return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
  bool cancelled()const{return stop.stop_requested();}
  void wait(std::uint32_t ms)const {std::unique_lock lock(mutex);wake.wait_for(lock,stop,std::chrono::milliseconds(ms),[]{return false;});}
  void open(const bridge::MatchmakingQosOpen &o)const {
    close();client.reset(curl_easy_init());require(bool(client));const auto authorization="Authorization: Bearer "+std::string(o.bearer);
    headers.reset(curl_slist_append(nullptr,authorization.c_str()));require(bool(headers));const std::string url(o.url);
    curl_easy_setopt(client.get(),CURLOPT_URL,url.c_str());curl_easy_setopt(client.get(),CURLOPT_HTTPHEADER,headers.get());
    curl_easy_setopt(client.get(),CURLOPT_CONNECT_ONLY,2L);curl_easy_setopt(client.get(),CURLOPT_PROTOCOLS_STR,"wss,ws");
    curl_easy_setopt(client.get(),CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(client.get(),CURLOPT_PROXY,"");curl_easy_setopt(client.get(),CURLOPT_NOSIGNAL,1L);
    curl_easy_setopt(client.get(),CURLOPT_TIMEOUT_MS,long(o.timeout_ms));curl_easy_setopt(client.get(),CURLOPT_CONNECTTIMEOUT_MS,long(o.connect_ms));
    curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(client.get(),CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(client.get(),CURLOPT_NOPROGRESS,0L);curl_easy_setopt(client.get(),CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(client.get(),CURLOPT_XFERINFODATA,&stop);
#ifdef CURLWS_NOAUTOPONG
    curl_easy_setopt(client.get(),CURLOPT_WS_OPTIONS,CURLWS_NOAUTOPONG);
#endif
    require(curl_easy_perform(client.get())==CURLE_OK);
  }
  bridge::MatchmakingQosFrame receive()const {
    require(bool(client));char bytes[1401];std::size_t count{};const curl_ws_frame *meta{};const auto result=curl_ws_recv(client.get(),bytes,sizeof(bytes),&count,&meta);
    bridge::MatchmakingQosFrame out;if(result==CURLE_AGAIN){out.again=true;return out;}require(result==CURLE_OK&&meta);
    out.text=meta->flags&CURLWS_TEXT;out.binary=meta->flags&CURLWS_BINARY;out.ping=meta->flags&CURLWS_PING;out.pong=meta->flags&CURLWS_PONG;out.close=meta->flags&CURLWS_CLOSE;
    out.continued=meta->flags&CURLWS_CONT;out.bytes_left=meta->bytesleft;for(std::size_t i=0;i<count;++i)out.data.push_back(static_cast<std::uint8_t>(bytes[i]));return out;
  }
  bridge::MatchmakingQosSent send(bridge::NativeSessionSendKind kind,rust::Slice<const std::uint8_t> bytes)const {
    require(bool(client));unsigned flags{};switch(kind){
      case bridge::NativeSessionSendKind::Text:flags=CURLWS_TEXT;break;case bridge::NativeSessionSendKind::Binary:flags=CURLWS_BINARY;break;
      case bridge::NativeSessionSendKind::Pong:flags=CURLWS_PONG;break;case bridge::NativeSessionSendKind::Close:flags=CURLWS_CLOSE;break;
      default:throw std::runtime_error("Unknown native socket send kind");}
    std::size_t count{};const auto result=curl_ws_send(client.get(),bytes.data(),bytes.size(),&count,0,flags);require(result==CURLE_OK||result==CURLE_AGAIN);
    bridge::MatchmakingQosSent out;out.again=result==CURLE_AGAIN;out.count=count;return out;
  }
  void close()const{client.reset();headers.reset();}
};
}
