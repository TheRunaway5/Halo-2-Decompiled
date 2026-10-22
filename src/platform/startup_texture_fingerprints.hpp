#pragma once
#include <cstddef>
#include <cstdint>
#include <future>
#include <functional>
#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

namespace halo2::detail {
inline std::uint64_t texture_bytes_fnv1a(std::span<const std::byte> bytes) noexcept {
  std::uint64_t hash=14695981039346656037ull;
  for(const auto byte:bytes){hash^=std::to_integer<unsigned char>(byte);hash*=1099511628211ull;}
  return hash;
}
// All supplied immutable byte owners must outlive this scoped bank. One worker
// computes the original complete FNV1a identities while the renderer creates
// GPU allocations and records uploads. Destruction joins before owners expire.
class StartupTextureFingerprints {
  using Key=std::pair<std::uintptr_t,std::size_t>;
  struct Entry {
    std::span<const std::byte> bytes;
    std::promise<std::uint64_t> promise;
    std::shared_future<std::uint64_t> result=promise.get_future().share();
    explicit Entry(std::span<const std::byte> value):bytes(value){}
  };
  std::map<Key,Entry*> lookup_;
  std::vector<std::unique_ptr<Entry>> entries_;
  std::thread worker_;
  static Key key(std::span<const std::byte> bytes){return {reinterpret_cast<std::uintptr_t>(bytes.data()),bytes.size()};}
  static std::size_t bucket(std::span<const std::byte> bytes) {
    // A bucket only; compare every byte before sharing a computed identity.
    std::array<std::byte,256> samples;
    if(bytes.size()>samples.size()) {
      const auto extent=bytes.size()-16;
      for(unsigned i=0;i<16;++i)std::memcpy(samples.data()+i*16,bytes.data()+(extent/15)*i+(extent%15)*i/15,16);
      bytes=samples;
    }
    return std::hash<std::string_view>{}(bytes.empty()?std::string_view{}:
        std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
  }
public:
  explicit StartupTextureFingerprints(std::span<const std::span<const std::byte>> inputs) {
    std::map<std::pair<std::size_t,std::size_t>,std::vector<Entry*>> equal_bytes;
    for(const auto bytes:inputs)if(!lookup_.contains(key(bytes))) {
      auto& candidates=equal_bytes[{bucket(bytes),bytes.size()}];
      const auto same=std::find_if(candidates.begin(),candidates.end(),[&](const auto* entry) {
        return bytes.empty() || std::memcmp(bytes.data(),entry->bytes.data(),bytes.size())==0;
      });
      if(same!=candidates.end()){lookup_.emplace(key(bytes),*same);continue;}
      auto entry=std::make_unique<Entry>(bytes);lookup_.emplace(key(bytes),entry.get());entries_.push_back(std::move(entry));
      candidates.push_back(entries_.back().get());
    }
    worker_=std::thread([this] {
      for(const auto& entry:entries_)entry->promise.set_value(texture_bytes_fnv1a(entry->bytes));
    });
  }
  StartupTextureFingerprints(const StartupTextureFingerprints&)=delete;
  StartupTextureFingerprints& operator=(const StartupTextureFingerprints&)=delete;
  ~StartupTextureFingerprints(){if(worker_.joinable())worker_.join();}
  std::uint64_t get(std::span<const std::byte> bytes) const {
    const auto it=lookup_.find(key(bytes));
    return it==lookup_.end()?texture_bytes_fnv1a(bytes):it->second->result.get();
  }
};
}
