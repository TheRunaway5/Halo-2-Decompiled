#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"

namespace halo2 {
enum class NativeTu15ResourcePagePhase {bytes,cache,policy,page_result,inactive_pool,specific,rows,pending,job,resource,page_handle,finished};
struct NativeTu15ResourcePage {
    std::uint32_t resource{},mode{},bytes{},cache{},policy{},page_handle{},inactive_pool{},specific_datum{},rows{},record{},pending_word{};
    bool allocated{};
};
struct NativeTu15ResourcePageResume {
    std::uint32_t value{},pool{},resource{},page_handle{},pending_word{};
};
// Allocate real cache pages and their matching inactive-datum record. Child
// owners determine page/datum outcomes and preserve their actual continuation.
// All publication addresses use the authored wrapping32-bit arithmetic.
template<class Memory,class Owners,class Progress>NativeTu15ResourcePage
native_tu15_allocate_resource_page(const Memory&memory,std::uint32_t resource,
    std::uint32_t mode,Owners&owners,Progress progress) {
    using P=NativeTu15ResourcePagePhase;NativeTu15ResourcePage data;data.resource=resource;data.mode=mode;
    const auto read=[&](P phase,std::uint32_t address){progress(phase,data);return native_node_resource_value(memory,address);};
    data.bytes=read(P::bytes,data.resource+4u);data.cache=read(P::cache,0x4f0424u);
    data.policy=read(P::policy,0x474d78u+data.mode*12u);
    const auto page=owners.page(data);data.page_handle=page.value;data.resource=page.resource;data.pending_word=page.pending_word;
    progress(P::page_result,data);
    if(data.page_handle==0xffffffffu){progress(P::finished,data);return data;}
    data.allocated=true;data.inactive_pool=read(P::inactive_pool,0x4f0418u);
    progress(P::specific,data);const auto specific=owners.specific(data);
    data.specific_datum=specific.value;data.inactive_pool=specific.pool;data.resource=specific.resource;
    data.page_handle=specific.page_handle;data.pending_word=specific.pending_word;
    data.rows=read(P::rows,data.inactive_pool+0x44u);data.record=data.rows+(data.specific_datum&0xffffu)*16u;
    progress(P::pending,data);owners.store(data.record+4u,data.pending_word&255u,1,0x12609eu);
    progress(P::job,data);owners.store(data.record+8u,0xffffffffu,4,0x1260a1u);
    progress(P::resource,data);owners.store(data.record+12u,data.resource,4,0x1260a8u);
    progress(P::page_handle,data);owners.store(data.resource+12u,data.page_handle,4,0x1260abu);
    progress(P::finished,data);return data;
}
} // namespace halo2
