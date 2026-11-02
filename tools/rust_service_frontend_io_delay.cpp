// Private native test IO only. The real Rust service response is unchanged.
// Retain a pending native party operation long enough to exercise its UI guard.
#include <curl/curl.h>
#include <dlfcn.h>
#include <chrono>
#include <string_view>
#include <thread>

extern "C" CURLcode curl_easy_perform(CURL *handle) {
  using Perform=CURLcode(*)(CURL*);
  using Info=CURLcode(*)(CURL*,CURLINFO,...);
  static const auto perform=reinterpret_cast<Perform>(dlsym(RTLD_NEXT,"curl_easy_perform"));
  static const auto info=reinterpret_cast<Info>(dlsym(RTLD_NEXT,"curl_easy_getinfo"));
  if(!perform||!info)return CURLE_FAILED_INIT;
  const auto result=perform(handle);char *url=nullptr;
  if(info(handle,CURLINFO_EFFECTIVE_URL,&url)==CURLE_OK && url && std::string_view(url).ends_with("/v1/party/commands"))
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  return result;
}
