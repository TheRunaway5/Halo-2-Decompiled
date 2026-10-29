#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <exception>
#include <utility>
#include <vector>

namespace halo2 {
// A temporary source call may publish scratch bytes but must restore every
// store, in reverse order, before returning. Keep each store's exact before
// bytes: overlapping writes and other active observers see the same rollback
// as the original per-store vector journal. No topology change is permitted.
class SourceRuntimeUndoJournal final {
public:
    enum class Storage { packed, store_vectors };
    explicit SourceRuntimeUndoJournal(SourceRuntimeMemory& bank,Storage storage,
        SourceRuntimeWriteGuard topology_guard):
        bank_(bank),storage_(storage) {
        source_runtime_register_topology_guard(bank_,this,std::move(topology_guard));
        try {
            SourceRuntimeWriteGuard observe;
            if(storage_==Storage::store_vectors) {
                observe=[this](auto at,auto size) {
                    const auto original=bank_.view(at,size);
                    store_vectors_.emplace_back(at,std::vector<std::byte>(original.begin(),original.end()));
                };
            } else {
                observe=[this](auto at,auto size) {
                    const auto original=bank_.view(at,size);const auto first=bytes_.size();
                    // Grow only these two arenas instead of allocating a vector
                    // for every original scalar/SIMD store. A rejected allocation
                    // leaves all prior undo records usable before the store runs.
                    bytes_.insert(bytes_.end(),original.begin(),original.end());
                    records_.push_back({at,first,size});
                };
            }
            source_runtime_register_write_observer(bank_,this,std::move(observe));
        } catch(...) {
            source_runtime_unregister_topology_guard(bank_,this);throw;
        }
    }
    ~SourceRuntimeUndoJournal() {
        source_runtime_unregister_write_observer(bank_,this);
        try {
            if(storage_==Storage::store_vectors) {
                for(auto at=store_vectors_.rbegin();at!=store_vectors_.rend();++at)
                    bank_.write(at->first,at->second);
            } else {
                for(auto at=records_.rbegin();at!=records_.rend();++at)
                    bank_.write(at->address,std::span<const std::byte>(bytes_).subspan(at->first,at->size));
            }
        } catch(...) {std::terminate();}
        source_runtime_unregister_topology_guard(bank_,this);
    }
    SourceRuntimeUndoJournal(const SourceRuntimeUndoJournal&)=delete;
    SourceRuntimeUndoJournal& operator=(const SourceRuntimeUndoJournal&)=delete;
private:
    struct Record {std::uint32_t address;std::size_t first,size;};
    SourceRuntimeMemory& bank_;
    Storage storage_;
    std::vector<Record> records_;
    std::vector<std::byte> bytes_;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> store_vectors_;
};
} // namespace halo2
