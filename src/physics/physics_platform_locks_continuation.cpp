#include "halo2/physics_platform_locks.hpp"
#include <algorithm>
#include <array>

namespace halo2 {
namespace {
void require(bool value,const char *why){if(!value)throw std::invalid_argument(why);}
template<class Read>void require_released(std::uint32_t p,Read &&read) {
    require(std::uint64_t(p)+28<=0x100000000ull,"Critical-section checkpoint wraps source addresses");
    std::array<std::byte,28> bytes;read(p,std::span(bytes));
    const auto word=[&](unsigned offset) {
        std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[offset+i])<<(8*i);
        return value;
    };
    require(word(16)==0xffffffffu && word(20)==0 && word(24)==0,
            "Critical-section continuation requires no owner, recursion or waiters");
}
void saved_read(const SourceRuntimeMemory::Checkpoint &bank,std::uint32_t p,std::span<std::byte> output) {
    require(std::uint64_t(p)+output.size()<=0x100000000ull,"Critical-section checkpoint wraps source extent");
    for(std::size_t i=0;i<output.size();++i) {
        const auto address=std::uint32_t(std::uint64_t(p)+i);
        auto at=std::upper_bound(bank.regions.begin(),bank.regions.end(),address,
            [](auto address,const auto &region){return address<region.address;});
        require(at!=bank.regions.begin(),"Critical-section checkpoint lacks source backing");--at;
        const auto offset=std::uint64_t(address)-at->address;
        require(offset<at->bytes.size() && (!at->page_bytes || at->committed[offset/at->page_bytes]),
                "Critical-section checkpoint extent is unavailable");
        require(at->access==SourceRuntimeMemory::Access::ReadWrite,"Critical-section checkpoint is read-only");
        output[i]=at->bytes[offset];
    }
}
}
PhysicsPlatformLocks::OwnershipCheckpoint PhysicsPlatformLocks::ownership_checkpoint(const SourcePhysicsContactMemory &memory) const {
    std::unique_lock gate(call_gate_,std::try_to_lock);
    require(gate.owns_lock() && !active_calls_ && !staging_,"Critical-section checkpoint has active native calls/restore");
    std::lock_guard registry(mutex_);OwnershipCheckpoint saved;
    const auto found=known_.find(&memory);
    if(found!=known_.end())for(auto address:found->second) {
        require_released(address,[&](auto p,auto bytes){memory.read(p,bytes);});
        saved.critical_sections.push_back(address);
    }
    return saved;
}
struct PhysicsPlatformLocks::PreparedOwnership::Impl {
    PhysicsPlatformLocks &owner;
    SourceRuntimeMemory &bank;
    std::unique_lock<std::recursive_mutex> gate;
    std::unique_lock<std::mutex> registry;
    std::map<const SourcePhysicsContactMemory *,std::set<std::uint32_t>> known;
    bool marked{};
    Impl(PhysicsPlatformLocks &owner,SourceRuntimeMemory &bank,const OwnershipCheckpoint &saved)
        :owner(owner),bank(bank),gate(owner.call_gate_,std::try_to_lock),registry(owner.mutex_,std::defer_lock) {
        require(gate.owns_lock() && !owner.active_calls_ && !owner.staging_,"Critical-section import has active calls/restore");
        registry.lock();
        const auto current=owner.known_.find(&bank);
        if(current!=owner.known_.end())for(auto address:current->second)
            require_released(address,[&](auto p,auto bytes){bank.read(p,bytes);});
        known=owner.known_;
        known[&bank]={saved.critical_sections.begin(),saved.critical_sections.end()};
        owner.staging_=true;marked=true;
    }
    ~Impl(){if(marked)owner.staging_=false;}
};
PhysicsPlatformLocks::PreparedOwnership::PreparedOwnership()=default;
PhysicsPlatformLocks::PreparedOwnership::~PreparedOwnership()=default;
PhysicsPlatformLocks::PreparedOwnership::PreparedOwnership(PreparedOwnership &&) noexcept=default;
PhysicsPlatformLocks::PreparedOwnership &PhysicsPlatformLocks::PreparedOwnership::operator=(PreparedOwnership &&) noexcept=default;
bool PhysicsPlatformLocks::PreparedOwnership::for_owner(const PhysicsPlatformLocks &owner,const SourceRuntimeMemory &bank) const noexcept {
    return impl_ && &impl_->owner==&owner && &impl_->bank==&bank;
}
PhysicsPlatformLocks::PreparedOwnership PhysicsPlatformLocks::prepare_ownership(SourceRuntimeMemory &bank,
    const OwnershipCheckpoint &saved,const SourceRuntimeMemory::Checkpoint &incoming) {
    SourceRuntimeMemory::validate_checkpoint(incoming);
    require(saved.version==1 && std::is_sorted(saved.critical_sections.begin(),saved.critical_sections.end()) &&
        std::adjacent_find(saved.critical_sections.begin(),saved.critical_sections.end())==saved.critical_sections.end(),
        "Invalid critical-section checkpoint registry");
    for(auto address:saved.critical_sections)
        require_released(address,[&](auto p,auto bytes){saved_read(incoming,p,bytes);});
    PreparedOwnership prepared;prepared.impl_=std::make_unique<PreparedOwnership::Impl>(*this,bank,saved);return prepared;
}
bool PhysicsPlatformLocks::commit_ownership(PreparedOwnership &&prepared) noexcept {
    if(!prepared.impl_ || &prepared.impl_->owner!=this)return false;
    known_.swap(prepared.impl_->known);prepared.impl_.reset();return true;
}
} // namespace halo2
