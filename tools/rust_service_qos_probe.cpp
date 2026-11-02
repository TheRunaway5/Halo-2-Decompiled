// Real native QoS socket IO. Tokens are issued by the Rust HTTP fixture and
// passed only in private environment values; no provider or latency is forged.
#include "halo2/online_search_qos.hpp"
#include <curl/curl.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace { volatile std::sig_atomic_t stopped{}; void stop(int) {stopped=1;} }
int main() {try {
  const char *url=std::getenv("H2_TEST_QOS_URL"),*a=std::getenv("H2_TEST_QOS_TOKEN_A"),*b=std::getenv("H2_TEST_QOS_TOKEN_B");
  if(!url || !a || !b)throw std::runtime_error("Private QoS fixture inputs required");
  if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)throw std::runtime_error("Native HTTP initialization failed");
  std::signal(SIGINT,stop);
  {
    halo2::OnlineSearchQos first(url,a,true),second(url,b,true);
    std::cout<<"READY native QoS clients\n"<<std::flush;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(25);
    while(!stopped && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if(!stopped)throw std::runtime_error("Native QoS fixture deadline expired");
  }
  std::cout<<"PASS native QoS client shutdown\n";return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
