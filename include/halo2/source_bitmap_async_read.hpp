#pragma once
#include "halo2/source_async_jobs.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace halo2 {
struct SourceBitmapFileBank {
    std::uint32_t current{},mainmenu{},shared{},single_player_shared{};
};
struct SourceBitmapReadRequest {
    std::uint32_t location{},size{},buffer{},bytes_output{},done{},category{};
    std::int32_t priority{};
};
// Complete213760/1A0F10 request construction before free-node acquisition.
// These are actual caller file tokens, not generated Xbox/native handles.
SourceAsyncRequest source_bitmap_async_request(const SourceBitmapReadRequest &,
    const SourceBitmapFileBank &);

struct SourceAsyncOwnedFile {
    std::uint32_t token{};
    std::filesystem::path path;
};
struct SourceAsyncOwnedBuffer {
    std::uint32_t base_token{};
    std::span<std::byte> bytes;
};
struct SourceAsyncReadExecution {
    SourceAsyncReadStep step;
    SourceAsyncReadResult io;
    SourceAsyncReadCompletion completion;
};
// Native-owned read-only streams, with explicit token/filename bindings.
// Actual seek/read outcomes feed1A0E70; no original kernel return or timeline
// is reconstructed. This does not allocate images, write done, or run a worker.
// The source worker executes callbacks serially; caller owns that ordering.
class SourceAsyncOwnedFiles {
public:
    explicit SourceAsyncOwnedFiles(std::span<const SourceAsyncOwnedFile>);
    ~SourceAsyncOwnedFiles();
    SourceAsyncOwnedFiles(const SourceAsyncOwnedFiles &)=delete;
    SourceAsyncOwnedFiles &operator=(const SourceAsyncOwnedFiles &)=delete;
    SourceAsyncReadExecution read(SourceAsyncTask &,std::uint32_t actual_chunk_limit,
        const SourceAsyncOwnedBuffer &);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
