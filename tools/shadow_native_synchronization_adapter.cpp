// Reference CPU platform adapter: restore real quiescent native objects from
// their captured ownership. This does not prescribe a game/kernel wait result.
#include "halo2/native_synchronization.hpp"
#include <algorithm>
#include <bit>
#include <string>
#include <vector>
#include <memory>
using namespace halo2;
namespace {
thread_local std::string error;
struct Owner {
    NativeSynchronization synchronization;
    NativeSynchronizationServices services;
    std::vector<std::pair<std::uint32_t,NativeSynchronization::ObjectKind>> leases;
    std::vector<std::pair<std::uint32_t,NativeSynchronization::ObjectKind>> objects;
    explicit Owner(std::uint32_t first,std::uint64_t limit):synchronization({first,limit}){}
    ~Owner(){
        // Teardown only, after original execution has stopped. No cleanup
        // result is ever delivered back to original instructions.
        for(auto [handle,kind]:leases)
            if(kind==NativeSynchronization::ObjectKind::Mutex)services.release_mutex(handle);
            else services.release_semaphore(handle,1);
    }
    void released(std::uint32_t handle) {
        const auto at=std::find_if(leases.begin(),leases.end(),[&](const auto& v){return v.first==handle;});
        if(at!=leases.end())leases.erase(at);
    }
};
}
extern "C" {
const char* shadow_sync_error(){return error.c_str();}
void* shadow_sync_restore(std::uint32_t first,std::uint64_t next,std::uint64_t limit,
    const std::uint32_t* rows,std::uint32_t count) {
    error.clear();
    try {
        auto owner=std::make_unique<Owner>(first,limit);
        NativeSynchronization::Checkpoint checkpoint;checkpoint.first=first;checkpoint.next=next;checkpoint.limit=limit;
        for(std::uint32_t n=0;n<count;++n) {
            if(rows[n*4+1]>1)throw std::runtime_error("Unknown native synchronization kind");
            checkpoint.objects.push_back({rows[n*4],NativeSynchronization::ObjectKind(rows[n*4+1]),
                std::bit_cast<std::int32_t>(rows[n*4+2]),std::bit_cast<std::int32_t>(rows[n*4+3])});
        }
        auto prepared=owner->synchronization.prepare_restore(checkpoint);
        if(!owner->synchronization.commit_restore(std::move(prepared)))
            throw std::runtime_error("Native synchronization ownership restore did not commit");
        for(const auto& object:checkpoint.objects)owner->objects.emplace_back(object.handle,object.kind);
        owner->services=owner->synchronization.services();return owner.release();
    }catch(const std::exception& e){error=e.what();return nullptr;}
}
int shadow_sync_wait(void* token,std::uint32_t handle,std::uint32_t milliseconds,int* native_error) {
    auto& owner=*static_cast<Owner*>(token);
    const auto result=owner.services.wait(handle,milliseconds);*native_error=result.error.value();
    if(result.state==NativeSynchronizationWaitState::Acquired) {
        const auto at=std::find_if(owner.objects.begin(),owner.objects.end(),[&](const auto& v){return v.first==handle;});
        if(at==owner.objects.end())throw std::runtime_error("Acquired native handle absent from restored ownership");
        owner.leases.push_back(*at);
    }
    return int(result.state);
}
int shadow_sync_release_mutex(void* token,std::uint32_t handle) {
    auto& owner=*static_cast<Owner*>(token);const auto result=owner.services.release_mutex(handle);
    if(!result)owner.released(handle);return result.value();
}
int shadow_sync_release_semaphore(void* token,std::uint32_t handle,std::int32_t count,std::int32_t* previous) {
    auto& owner=*static_cast<Owner*>(token);const auto result=owner.services.release_semaphore(handle,count);
    if(!result.previous_count)return result.error?result.error.value():22;
    *previous=*result.previous_count;for(int n=0;n<count;++n)owner.released(handle);
    return 0;
}
void shadow_sync_destroy(void* token){delete static_cast<Owner*>(token);}
}
