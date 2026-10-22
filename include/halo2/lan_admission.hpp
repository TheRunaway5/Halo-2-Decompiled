#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

namespace halo2 {
// Native raw-UDP admission only; this is not the retail System Link protocol.
// Cookies bind the exact request, source IP/port and a short time window. No
// player or pending-challenge state is allocated for an unverified endpoint.
class LanAdmission {
public:
  using Cookie=std::array<std::uint8_t,16>;
  using Endpoint=std::array<std::uint8_t,6>;
  static std::uint64_t milliseconds(){return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();}
  LanAdmission(){if(RAND_bytes(secret_.data(),secret_.size())!=1)throw std::runtime_error("Cannot initialize LAN admission");}
  Cookie issue(const Endpoint &source,std::span<const std::byte> request,std::uint64_t now)const{
    return mac(source,request,now/30000);
  }
  bool verify(const Endpoint &source,std::span<const std::byte> request,const Cookie &cookie,std::uint64_t now)const{
    const auto current=mac(source,request,now/30000);
    if(CRYPTO_memcmp(current.data(),cookie.data(),cookie.size())==0)return true;
    if(now<30000)return false;
    const auto previous=mac(source,request,now/30000-1);
    return CRYPTO_memcmp(previous.data(),cookie.data(),cookie.size())==0;
  }
  bool reserve(const Endpoint &source,std::size_t bytes,bool challenge,std::uint64_t now){
    std::lock_guard lock(mutex_);
    std::array<std::uint8_t,4> ip;std::copy_n(source.begin(),4,ip.begin());
    if(!sources_.contains(ip)&&sources_.size()==256){
      auto oldest=std::min_element(sources_.begin(),sources_.end(),[](const auto &a,const auto &b){return a.second.used<b.second.used;});
      sources_.erase(oldest);
    }
    auto &entry=sources_[ip];entry.used=now;
    const unsigned which=challenge?0:1;
    const double rate=challenge?4096:128*1024,burst=challenge?8192:256*1024;
    const double global_rate=challenge?64*1024:1024*1024,global_burst=challenge?128*1024:2*1024*1024;
    refill(entry.budgets[which],rate,burst,now);refill(global_[which],global_rate,global_burst,now);
    if(bytes>entry.budgets[which].credits||bytes>global_[which].credits)return false;
    entry.budgets[which].credits-=bytes;global_[which].credits-=bytes;return true;
  }
private:
  std::array<unsigned char,32> secret_{};
  Cookie mac(const Endpoint &source,std::span<const std::byte> request,std::uint64_t window)const{
    // HMAC's input is bounded by one native datagram.
    std::array<unsigned char,1420> input{};std::size_t at{};
    if(request.size()>1400)throw std::runtime_error("LAN admission request exceeds bounds");
    for(auto b:source)input[at++]=b;
    for(unsigned i=0;i<8;++i)input[at++]=static_cast<unsigned char>((window>>(i*8))&255);
    for(auto b:request)input[at++]=std::to_integer<unsigned char>(b);
    std::array<unsigned char,32> digest{};unsigned size{};
    if(!HMAC(EVP_sha256(),secret_.data(),secret_.size(),input.data(),at,digest.data(),&size)||size!=32)
      throw std::runtime_error("Cannot authenticate LAN admission");
    Cookie result;std::copy_n(digest.begin(),result.size(),result.begin());return result;
  }
  struct Budget {double credits{};std::uint64_t at{};bool initialized{};};
  static void refill(Budget &b,double rate,double burst,std::uint64_t now){
    if(!b.initialized){b={burst,now,true};return;}
    if(now>b.at){b.credits=std::min(burst,b.credits+double(now-b.at)*rate/1000);b.at=now;}
  }
  struct Source {std::array<Budget,2> budgets;std::uint64_t used{};};
  std::mutex mutex_;
  std::map<std::array<std::uint8_t,4>,Source> sources_;
  std::array<Budget,2> global_;
};
}
