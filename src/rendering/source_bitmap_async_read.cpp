#include "halo2/source_bitmap_async_read.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace halo2 {
SourceAsyncRequest source_bitmap_async_request(const SourceBitmapReadRequest &input,
    const SourceBitmapFileBank &files){
    if(!input.done)
        throw std::invalid_argument("source bitmap request requires actual done storage");
    const std::uint32_t tokens[]{files.current,files.mainmenu,files.shared,files.single_player_shared};
    SourceAsyncRequest request;
    request.task=source_async_read_task({tokens[input.location>>30],input.buffer,
        input.size,input.location&0x3fffffffu,input.bytes_output});
    request.priority=input.priority;request.category=input.category;
    request.callback=source_async_read_callback;request.done=input.done;return request;
}
struct SourceAsyncOwnedFiles::Impl {
    std::unordered_map<std::uint32_t,std::ifstream> files;
};
SourceAsyncOwnedFiles::SourceAsyncOwnedFiles(std::span<const SourceAsyncOwnedFile> inputs)
    :impl_(std::make_unique<Impl>()){
    for(const auto &input:inputs){
        if(impl_->files.contains(input.token))
            throw std::invalid_argument("source async owned file token is duplicated");
        std::ifstream file(input.path,std::ios::binary);
        if(!file)throw std::invalid_argument("source async owned read-only file cannot be opened");
        impl_->files.emplace(input.token,std::move(file));
    }
}
SourceAsyncOwnedFiles::~SourceAsyncOwnedFiles()=default;
SourceAsyncReadExecution SourceAsyncOwnedFiles::read(SourceAsyncTask &task,
    std::uint32_t limit,const SourceAsyncOwnedBuffer &buffer){
    SourceAsyncReadExecution out;out.step=source_async_read_begin(task,limit);
    const auto found=impl_->files.find(out.step.file);
    if(found==impl_->files.end())
        throw std::invalid_argument("source async read has no actual owned file binding");
    const auto processed=task.words[5];
    if(task.words[1]!=buffer.base_token ||
       std::uint64_t(buffer.base_token)+processed>std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("source async read address is not represented by its native buffer");
    if(processed>buffer.bytes.size() || out.step.size>buffer.bytes.size()-processed ||
       std::uint64_t(out.step.buffer)+out.step.size>0x100000000ull)
        throw std::invalid_argument("source async read range is not represented by its native buffer");
    if(std::uint64_t(out.step.size)>std::uint64_t(std::numeric_limits<std::streamsize>::max()))
        throw std::invalid_argument("source async read count is not representable by the native stream");
    auto &file=found->second;
    file.clear();file.seekg(std::streamoff(out.step.offset),std::ios::beg);
    const auto position=file.tellg();
    if(!file || position==std::streampos(-1))out.io.seek_low=source_async_none;
    else{
        out.io.seek_low=std::uint32_t(std::uint64_t(std::streamoff(position)));
        if(out.io.seek_low!=source_async_none){
            // A valid zero-count native read still executes. Its storage is
            // immaterial, and avoids passing a null span pointer to the stream.
            char zero_count_storage{};
            auto destination=out.step.size?
                reinterpret_cast<char*>(buffer.bytes.data()+processed):&zero_count_storage;
            file.read(destination,std::streamsize(out.step.size));
            out.io.bytes_read=std::uint32_t(file.gcount());
        }
    }
    out.completion=source_async_read_finish(task,out.step,out.io);return out;
}
} // namespace halo2
