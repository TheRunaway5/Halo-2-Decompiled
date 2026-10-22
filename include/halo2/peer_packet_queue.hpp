#pragma once
#include <array>
#include <cstddef>
#include <deque>
#include <span>
#include <vector>

namespace halo2 {
// Callers serialize access. Each authenticated connection owns its capacity;
// a busy sender cannot consume another connection's receive slots.
class PeerPacketQueue {
public:
  bool push(unsigned peer, std::span<const std::byte> bytes) {
    if(peer>=queues_.size() || bytes.empty() || bytes.size()>1404 || queues_[peer].size()>=512)return false;
    queues_[peer].emplace_back(bytes.begin(),bytes.end());return true;
  }
  bool receive(std::vector<std::byte> &bytes,unsigned &peer) {
    for(unsigned i=0;i<queues_.size();++i) {
      const auto slot=(next_+i)%queues_.size();auto &queue=queues_[slot];
      if(queue.empty())continue;
      bytes=std::move(queue.front());queue.pop_front();peer=slot;next_=(slot+1)%queues_.size();return true;
    }
    return false;
  }
private:
  std::array<std::deque<std::vector<std::byte>>,16> queues_;
  unsigned next_{};
};
}
