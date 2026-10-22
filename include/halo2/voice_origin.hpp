#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <openssl/evp.h>

namespace halo2 {
class VoiceOrigin {
public:
  using Key=std::array<unsigned char,32>;
  using Signature=std::array<std::byte,64>;
  explicit VoiceOrigin(std::string room):room_(std::move(room)){}
  bool configure(Key seed,std::vector<Key> keys,unsigned self){
    if(self>=keys.size()||keys.size()>16)return false;
    if(ready_)return seed==seed_&&keys==keys_;
    Pkey key(EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,nullptr,seed.data(),seed.size()),EVP_PKEY_free);
    Key public_key;std::size_t size=public_key.size();
    if(!key||EVP_PKEY_get_raw_public_key(key.get(),public_key.data(),&size)!=1||size!=32||public_key!=keys[self])return false;
    seed_=seed;keys_=std::move(keys);ready_=true;return true;
  }
  bool ready()const{return ready_;}
  std::optional<Signature> sign(std::span<const std::byte> body,std::uint32_t epoch)const{
    if(!ready_||body.size()<=22||body.size()>278)return {};
    Pkey key(EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,nullptr,seed_.data(),seed_.size()),EVP_PKEY_free);
    Context context(EVP_MD_CTX_new(),EVP_MD_CTX_free);auto input=message(body,epoch);Signature result;std::size_t size=result.size();
    if(!key||!context||EVP_DigestSignInit(context.get(),nullptr,nullptr,nullptr,key.get())!=1||
        EVP_DigestSign(context.get(),reinterpret_cast<unsigned char*>(result.data()),&size,input.data(),input.size())!=1||size!=64)return {};
    return result;
  }
  bool verify(std::span<const std::byte> body,const Signature &signature,unsigned owner,std::uint32_t epoch)const{
    if(!ready_||owner>=keys_.size()||body.size()<=22||body.size()>278)return false;
    Pkey key(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,nullptr,keys_[owner].data(),32),EVP_PKEY_free);
    Context context(EVP_MD_CTX_new(),EVP_MD_CTX_free);auto input=message(body,epoch);
    return key&&context&&EVP_DigestVerifyInit(context.get(),nullptr,nullptr,nullptr,key.get())==1&&
        EVP_DigestVerify(context.get(),reinterpret_cast<const unsigned char*>(signature.data()),64,input.data(),input.size())==1;
  }
private:
  using Pkey=std::unique_ptr<EVP_PKEY,decltype(&EVP_PKEY_free)>;
  using Context=std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)>;
  std::vector<unsigned char> message(std::span<const std::byte> body,std::uint32_t epoch)const{
    const char domain[]="H2VOICE1";std::vector<unsigned char> out(domain,domain+sizeof(domain));
    out.insert(out.end(),room_.begin(),room_.end());
    for(unsigned i=0;i<4;++i)out.push_back((epoch>>(8*i))&255);
    for(auto b:body.first(13))out.push_back(std::to_integer<unsigned char>(b));
    for(auto b:body.subspan(22))out.push_back(std::to_integer<unsigned char>(b));return out;
  }
  std::string room_;Key seed_{};std::vector<Key> keys_;bool ready_{};
};
}
