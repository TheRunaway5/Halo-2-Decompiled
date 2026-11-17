#pragma once
#include "halo2/source_runtime_undo_journal.hpp"
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace halo2 {
// Parity oracle only; never a production route. The native candidate runs
// inside an undo journal, then the translated original runs for real. Every
// byte either of them stored outside the dead emulated call stack is compared
// by its final value: a byte only one side stored must still hold its value
// from before both runs on the other side. Later code observes these bytes
// and the returned arithmetic state, not the order of the stores.
struct SourceNativeStoreOracleTu15 {
    static constexpr std::uint32_t stack_first=0x600c0000u,stack_end=0x60100000u;
    struct Mismatch {std::uint32_t address;std::uint8_t native,original;};
    std::uint64_t native_bytes{},original_bytes{},mismatches{},topology_changes{};
    // A candidate whose services commit or mount backing (for example a lazy
    // lighting cache) cannot be rolled back; such a change is kept and
    // counted. Otherwise the oracle refuses it before the change happens.
    bool keep_topology_changes{};
    std::vector<Mismatch> first;
    class Stores {
        SourceRuntimeMemory& bank_;
    public:
        std::unordered_map<std::uint32_t,std::byte> before;
        explicit Stores(SourceRuntimeMemory& bank):bank_(bank) {
            source_runtime_register_write_observer(bank_,this,[this](std::uint32_t at,std::size_t size) {
                if(at>=stack_first && std::uint64_t(at)+size<=stack_end)return;
                const auto bytes=bank_.view(at,size);
                for(std::size_t n=0;n<size;++n)before.try_emplace(at+std::uint32_t(n),bytes[n]);
            });
        }
        ~Stores(){source_runtime_unregister_write_observer(bank_,this);}
        Stores(const Stores&)=delete;Stores& operator=(const Stores&)=delete;
    };
    template<class Native,class Original>
    void run(SourceRuntimeMemory& memory,Native&& native,Original&& original) {
        std::vector<std::pair<std::uint32_t,std::byte>> native_final;
        std::unordered_map<std::uint32_t,std::byte> native_before;
        {
            SourceRuntimeUndoJournal undo(memory,SourceRuntimeUndoJournal::Storage::packed,[this](auto,auto) {
                if(!keep_topology_changes)throw std::runtime_error("Native store oracle cannot change World topology");
                ++topology_changes;
            });
            {
                Stores stores(memory);
                native();
                native_before=std::move(stores.before);
            }
            native_final.reserve(native_before.size());
            for(const auto& [at,value]:native_before)native_final.emplace_back(at,memory.view(at,1)[0]);
        }
        std::unordered_map<std::uint32_t,std::byte> original_before;
        {
            Stores stores(memory);
            original();
            original_before=std::move(stores.before);
        }
        native_bytes+=native_final.size();original_bytes+=original_before.size();
        const auto mismatch=[&](std::uint32_t at,std::byte native_value,std::byte original_value) {
            if(native_value==original_value)return;
            ++mismatches;
            if(first.size()<32)first.push_back({at,std::to_integer<std::uint8_t>(native_value),std::to_integer<std::uint8_t>(original_value)});
        };
        for(const auto& [at,value]:native_final)mismatch(at,value,memory.view(at,1)[0]);
        for(const auto& [at,value]:original_before)
            if(!native_before.contains(at))mismatch(at,value,memory.view(at,1)[0]);
    }
};
}
