#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace halo2 {
inline constexpr std::uint32_t source_async_none=0xffffffffu;
inline constexpr std::uint32_t source_async_read_callback=0x1a0e70u;
struct SourceAsyncTask {std::array<std::uint32_t,10> words{};};
struct SourceAsyncNode {
    std::int32_t priority{};
    std::uint32_t job=source_async_none;
    SourceAsyncTask task;
    std::uint32_t category{},callback{},done{},next=source_async_none;
};
static_assert(sizeof(SourceAsyncNode)==64);
struct SourceAsyncInsertState {
    std::uint32_t file{},offset{},valid_word{};
};
bool source_async_before(const SourceAsyncNode &incoming,const SourceAsyncNode &other,
    SourceAsyncInsertState &);
struct SourceAsyncQueue {
    std::array<SourceAsyncNode,150> nodes{};
    std::uint32_t free_head=source_async_none,work_head=source_async_none;
    std::uint32_t counter{},requests_added{};
};
// Complete120A90 title-bank initialization, preserving both incoming counters.
// This neither creates kernel objects nor starts a worker.
void source_async_initialize(SourceAsyncQueue &);
// These list operations require actual exclusive access from their owner.
// They do not return a fabricated acquisition status. An empty free list is
// an availability boundary; original1208B0 releases, yields and retries.
std::optional<unsigned> source_async_take_free(SourceAsyncQueue &);
void source_async_insert(SourceAsyncQueue &,unsigned node);
std::uint32_t source_async_publish(SourceAsyncQueue &,unsigned node);
bool source_async_reprioritize(SourceAsyncQueue &,std::uint32_t job,std::int32_t priority);
unsigned source_async_count(const SourceAsyncQueue &);
bool source_async_has_category(const SourceAsyncQueue &,std::uint32_t category);
void source_async_unlink(SourceAsyncQueue &,unsigned node);
void source_async_return_free(SourceAsyncQueue &,unsigned node);

struct SourceAsyncRequest {
    SourceAsyncTask task;
    std::int32_t priority{};
    std::uint32_t category{},callback{},done{};
};
//120BA0 /1A0F10 request prefix: increment requests_added and return the actual
// done token that must be cleared before free-node acquisition. No node, job,
// kernel result, semaphore notification or readiness is supplied here.
std::uint32_t source_async_begin_request(SourceAsyncQueue &,const SourceAsyncRequest &);
void source_async_bind_request(SourceAsyncQueue &,unsigned,const SourceAsyncRequest &);
struct SourceAsyncCallbackEffect {
    bool remove{},signal_again{};
    std::optional<std::uint32_t> set_done;
};
//120C40 completion protocol. Callback execution/result is a distinct service.
// Only actual result1 with nonNULLdone permits the byte write before unlink.
SourceAsyncCallbackEffect source_async_callback_effect(std::int32_t actual_result,
    std::uint32_t actual_done);

struct SourceAsyncReadInput {
    std::uint32_t file{},buffer{},size{},offset{},bytes_read{};
};
SourceAsyncTask source_async_read_task(const SourceAsyncReadInput &);
struct SourceAsyncReadStep {
    std::uint32_t file{},offset{},buffer{},size{};
};
//1A0E70 always calls SetFilePointer, even when the requested size is zero.
SourceAsyncReadStep source_async_read_begin(const SourceAsyncTask &,
    std::uint32_t actual_chunk_limit);
struct SourceAsyncReadResult {
    std::uint32_t seek_low{},bytes_read{};
};
struct SourceAsyncReadCompletion {
    std::int32_t callback_result{};
    std::optional<std::uint32_t> cumulative_write;
};
// Caller must supply an actual I/O outcome. Seek lowFFFFFFFF suppresses ReadFile
// and its bytes output; the ReadFile BOOL is not consumed by original1A0E70.
// This does not execute I/O or create an original completion timeline.
SourceAsyncReadCompletion source_async_read_finish(SourceAsyncTask &,
    const SourceAsyncReadStep &,const SourceAsyncReadResult &);
} // namespace halo2
