#pragma once
#include "rust/cxx.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace halo2::online_protocol_bridge {
// Storage only. Rust owns protocol validation, state, hashing and lifetimes.
// Unique ownership and pinned mutable access prevent native bank aliases.
class NativeSnapshotBuffer {
public:
    explicit NativeSnapshotBuffer(std::size_t size):bytes_(size){}
    explicit NativeSnapshotBuffer(std::vector<std::byte> bytes):bytes_(std::move(bytes)){}
    rust::Slice<const std::uint8_t> snapshot_buffer_bytes()const noexcept {
        return {reinterpret_cast<const std::uint8_t*>(bytes_.data()),bytes_.size()};
    }
    void snapshot_buffer_write(std::uint64_t offset,rust::Slice<const std::uint8_t> data){
        if(offset>bytes_.size()||data.size()>bytes_.size()-offset)throw std::out_of_range("Snapshot byte-buffer range");
        for(std::size_t i=0;i<data.size();++i)bytes_[offset+i]=std::byte(data[i]);
    }
    std::span<const std::byte> bytes()const noexcept{return bytes_;}
private:
    std::vector<std::byte> bytes_;
};
inline std::unique_ptr<NativeSnapshotBuffer> new_snapshot_buffer(std::uint64_t size){return std::make_unique<NativeSnapshotBuffer>(std::size_t(size));}
struct NativeSnapshotToken;
struct NativeSnapshotWitness;
class NativeSnapshotLeaseIo {
public:
    virtual ~NativeSnapshotLeaseIo()=default;
    // These callbacks only construct/retire actual native delivery storage.
    // They must not call the Rust snapshot owner or any native world executor.
    virtual NativeSnapshotWitness snapshot_witness()const noexcept=0;
    virtual void snapshot_issue(const NativeSnapshotToken &,std::unique_ptr<NativeSnapshotBuffer>)const noexcept=0;
    virtual void snapshot_destroy()const noexcept=0;
};
}
