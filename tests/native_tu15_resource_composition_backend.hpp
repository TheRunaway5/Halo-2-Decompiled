#pragma once
#include "halo2/source_animation_cache_io_tu15.hpp"
#include "halo2/source_bitmap_async_read.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include <map>

namespace halo2::resource_composition_fixture {
#include "resource_composition_registry.inc"
// Test-only backed synchronous I/O provider. Completion remains the shared
// production World policy; this is intentionally distinct from a pending
// SourceAnimationAsyncWorker and never manufactures its completion timeline.
class FileBackend {
    static constexpr std::uint32_t token=0xf4000001u,buffer=0x64000000u;
    SourceAsyncOwnedFiles files_;
    std::uint32_t chunk_;
public:
    ResourceReadOwner retained;
    std::uint64_t actual_file_calls{},actual_file_bytes{};
    FileBackend(const std::filesystem::path&path,std::uint32_t chunk)
        :files_(std::array{SourceAsyncOwnedFile{token,path}}),chunk_(chunk) {
        if(!chunk)throw std::invalid_argument("Composition file provider requires a positive chunk");
    }
    std::vector<std::byte> read(const SourceAnimationCacheReadTu15&request) {
        // The fixture binds the actual current-cache file (partition zero).
        if(request.encoded_offset>>30u)throw std::invalid_argument("Unbound composition cache partition");
        std::vector<std::byte>payload(request.bytes);
        auto task=source_async_read_task({token,buffer,request.bytes,request.encoded_offset,0});
        SourceAsyncReadExecution execution;
        do{execution=files_.read(task,chunk_,{buffer,payload});++actual_file_calls;actual_file_bytes+=execution.io.bytes_read;}
        while(execution.completion.callback_result==0);
        if(task.words[5]!=request.bytes)throw FormatError("Composition cache sectors were not fully backed by the actual file");
        return payload;
    }
    std::uint32_t retain(const SourceAnimationCacheReadTu15&request) {
        // Match the actual animation-provider's body_pose_animation=true
        // argument. This fixture does not own a live World publication epoch.
        if(retained.body_pose_publication_active)throw std::logic_error("Composition fixture cannot qualify an active World body-pose publication epoch");
        return retained.retain_resource_read({request.descriptor,request.destination,request.bytes},true);
    }
};
} // namespace halo2::resource_composition_fixture
